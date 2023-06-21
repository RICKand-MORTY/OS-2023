#include <virtio.h>
#include <memory.h>
#include <io.h>
#include "../../lib/lib.h"

#define VIRTIO_BLK_REQ_HEADER_SIZE      16       //desc1 header
#define VIRTIO_BLK_REQ_MIDDLE_SIZE      512      //desc2 data
#define VIRTIO_BLK_REQ_FOOTER_SIZE      1        //desc3 status
struct vring g_vring;

//reference from xv6
static struct disk {
	char pages[3*PAGE_SIZE];    //desc,avail,used save in

    // a set (not a ring) of DMA descriptors, with which the
    // driver tells the device where to read and write individual
    // disk operations. there are NUM descriptors.
    // most commands consist of a "chain" (a linked list) of a couple of
    // these descriptors.
    struct vring_desc *desc;

    // a ring in which the driver writes descriptor numbers
    // that the driver would like the device to process.  it only
    // includes the head descriptor of each chain. the ring has
    // NUM elements.
    struct vring_avail *avail;

    // a ring in which the device writes descriptor numbers that
    // the device has finished processing (just the head of each chain).
    // there are NUM used ring entries.
    struct vring_used *used;

    // our own book-keeping.
    char free[QUEUE_SIZE];  // is a descriptor free?
    __virtio16 used_idx; //表示驱动已经查看过的已用环中的元素数量，用于判断是否有新的完成通知

    //表示每个请求链表的相关信息，包括请求对应的缓冲区指针和状态标志
    struct {
        struct buf *b;
        char status;
    } info[QUEUE_SIZE];

    //表示每个请求链表对应的磁盘命令头部，与描述符一一对应，方便驱动构造和解析请求
    struct virtio_blk_req ops[QUEUE_SIZE];
    
    spinlock vdisk_lock;
  
} disk;

//find a free desc
static int alloc_desc()
{
  for(int i = 0; i < QUEUE_SIZE; i++){
    if(disk.free[i]){
      disk.free[i] = 0;
      return i;
    }
  }
  return -1;
}

//free a desc
static void
free_desc(int i)
{
  disk.desc[i].addr = 0;
  disk.desc[i].len = 0;
  disk.desc[i].flags = 0;
  disk.desc[i].next = 0;
  disk.free[i] = 1;
}

// free a chain of descriptors.
static void free_chain(int i)
{
  while(1){
    int flag = disk.desc[i].flags;
    int nxt = disk.desc[i].next;
    free_desc(i);
    if(flag & VIRTQ_DESC_F_NEXT)
      i = nxt;
    else
      break;
  }
}

static int alloc3_desc(int *idx)
{
  for(int i = 0; i < 3; i++){
    idx[i] = alloc_desc();
    if(idx[i] < 0){
      for(int j = 0; j < i; j++)
        free_desc(idx[j]);
      return -1;
    }
  }
  return 0;
}

void virtio_disk_rw(Buf *b, int write)
{
    uint64_t sector = b->blockno * (BSIZE / 512);
    spin_lock(&disk.vdisk_lock);
    
    // allocate the three descriptors.
    int idx[3];
    while(1)
    {
        if(alloc3_desc(idx) == 0) {
        break;
        }
    }

    struct virtio_blk_req *buf0 = &disk.ops[idx[0]];

    if(write)
        buf0->type = VIRTIO_BLK_T_OUT; // write the disk
    else
        buf0->type = VIRTIO_BLK_T_IN; // read the disk
    buf0->reverse = 0;
    buf0->sector = sector;

    /*
    构造包含三个描述符的链
    第一个是sizeof(struct virtio_blk_req)个字节长，包含type, reserved, 和sector。该描述符是只读的。
    第二个是BSIZE字节长，指向一个完全不同的内存地址（data传递给函数）,用于存放数据。该描述符是设备可写的。
    第三个是1字节长，指向结构的末尾virtio_blk_req ，设备可以在此处写入状态信息。
    */

    //idx0
    create_pgd_mapping(_pgd_page_begin,(u64)buf0,(u64)buf0,sizeof(struct virtio_blk_req), PAGE_KERNEL_READ_EXEC, alloc_pgtable, 0 );
    disk.desc[idx[0]].addr = (__virtio64)buf0;
    disk.desc[idx[0]].len = sizeof(struct virtio_blk_req);
    disk.desc[idx[0]].flags = VIRTQ_DESC_F_NEXT;
    disk.desc[idx[0]].next = idx[1];
    
    //idx1
    disk.desc[idx[1]].addr = (__virtio64) b->data;
    disk.desc[idx[1]].len = BSIZE;
    if(write)
        disk.desc[idx[1]].flags = 0; // device reads b->data
    else
        disk.desc[idx[1]].flags = VIRTQ_DESC_F_WRITE; // device writes b->data
    disk.desc[idx[1]].flags |= VIRTQ_DESC_F_NEXT;
    disk.desc[idx[1]].next = idx[2];

    //idx2
    disk.info[idx[0]].status = 0; // device writes 0 on success
    disk.desc[idx[2]].addr = (__virtio64) &disk.info[idx[0]].status;
    disk.desc[idx[2]].len = 1;
    disk.desc[idx[2]].flags = VIRTQ_DESC_F_WRITE; // device writes the status
    disk.desc[idx[2]].next = 0;

    b->disk = 1;
    disk.info[idx[0]].b = b;    //specify the buf
    
    disk.avail->ring[disk.avail->idx % QUEUE_SIZE] = idx[0];    //use % because is a circular
    __sync_synchronize();

    // tell the device another avail ring entry is available.
    disk.avail->idx += 1;
    __sync_synchronize();

    writeword(0, &g_regs->QueueNotify);
    __sync_synchronize();

    while(b->disk == 1)
    {
        printk("waiting!\n");
        __sync_synchronize(); // ensure memory barrier before checking b->disk
    }


    disk.info[idx[0]].b = 0;
    free_chain(idx[0]);

}

void virtio_disk_intr()
{
    //ack interrupt
    writeword(readword(&g_regs->InterruptStatus) & 0x3, &g_regs->InterruptACK);
    __sync_synchronize();

    // the device increments disk.used->idx when it
    // adds an entry to the used ring.
    while(disk.used_idx != disk.used->idx)
    {
        __sync_synchronize();
        int id = disk.used->ring[disk.used_idx % QUEUE_SIZE].id;

        if(disk.info[id].status != 0)
        printk("virtio_disk_intr status");

        struct buf *b = disk.info[id].b;
        b->disk = 0;   // disk is done with buf
        disk.used_idx += 1;
    }
}



static int virtio_blk_init_legacy(virtio_regs_legacy *regs, uint32_t intid)
{

    /*4.Read device feature bits, and write the subset of feature bits understood by the OS and driver to the
    device. During this step the driver MAY read (but MUST NOT write) the device-specific configuration
    fields to check that it can support the device before accepting it.*/
    uint64_t features = readword(&g_regs->HostFeatures);
    features &= ~(1 << VIRTIO_BLK_F_RO);
    features &= ~(1 << VIRTIO_BLK_F_SCSI);
    features &= ~(1 << VIRTIO_BLK_F_CONFIG_WCE);
    features &= ~(1 << VIRTIO_BLK_F_MQ);
    features &= ~(1 << VIRTIO_F_ANY_LAYOUT);
    features &= ~(1 << VIRTIO_RING_F_EVENT_IDX);
    features &= ~(1 << VIRTIO_RING_F_INDIRECT_DESC);
    writeword(features, &g_regs->GuestFeatures);
    refresh_cache();

    writeword(readword(&regs->Status)|VIRTIO_STATUS_FEATURES_OK , &regs->Status);
    refresh_cache();

    //Set the DRIVER_OK status bit. At this point the device is “live”.
    writeword((readword(&regs->Status) | VIRTIO_STATUS_DRIVER_OK), &regs->Status);
    refresh_cache();
    
    writeword(PAGE_SIZE, &regs->GuestPageSize);
    //printk("GuestPageSize = 0x%x\n", ((unsigned long)&regs->GuestPageSize - (unsigned long)regs));
    refresh_cache();

    /*
    Perform device-specific setup, including discovery of virtqueues for the device, optional per-bus setup,
    reading and possibly writing the device’s virtio configuration space, and population of virtqueues.
    */
    //configure the queue:
    //a. Select the queue writing its index (first queue is 0) to QueueSel(init queue)
    writeword(0, &regs->QueueSel);
    //printk("regs->QueueSel = 0x%x\n", ((unsigned long)&regs->QueueSel - (unsigned long)regs));
    refresh_cache();

    //b. Check if the queue is not already in use: read QueuePFN, expecting a returned value of zero (0x0).
    if(readword(&regs->QueuePFN))
    {
        printk("the queue is already in use\n");
    }
    refresh_cache();
    /*
    c. Read maximum queue size (number of elements) from QueueNumMax. If the returned value is zero
    (0x0) the queue is not available.
    */
    uint32_t size = readword(&regs->QueueNumMax);
    if(size == 0)
    {
        printk("device queue not available!\n");
        return -1;
    }
    else if (size < QUEUE_SIZE)
    {
        printk("device queue too short!\n");
        return -1;
    }

    writeword(QUEUE_SIZE, &regs->QueueNum);
    refresh_cache();

    memset(disk.pages, 0, sizeof(disk.pages));
    writeword(((u64)disk.pages) >> PAGE_SHIFT, &regs->QueuePFN);
    refresh_cache();

    /*
     Allocate and zero the queue pages in contiguous virtual memory,
    aligning the Used Ring to an optimal boundary (usually page size). 
    The driver should choose a queue size smaller than or equal to
    */
    disk.desc = (struct vring_desc*)((unsigned long)disk.pages);
    disk.avail = (struct vring_avail*)(((char*)disk.desc) + QUEUE_SIZE*sizeof(struct vring_desc));
    disk.used = (struct vring_used*)PAGE_ALIGN_UP((unsigned long)(disk.pages + PAGE_SIZE));
    
    for(int i = 0; i < QUEUE_SIZE; i++)
    {
        disk.free[i] = 1;
    }

    printk("virtio device init finish!\n");
    g_regs = regs;
    return 0;
}

//init blk device(nonlegacy device)
static int virtio_blk_init_nonlegacy(virtio_regs *regs, uint32_t intid)
{
    /*4.Read device feature bits, and write the subset of feature bits understood by the OS and driver to the
    device. During this step the driver MAY read (but MUST NOT write) the device-specific configuration
    fields to check that it can support the device before accepting it.*/
    uint32_t features = readword(&regs->DeviceFeatures);
    features &= ~(1 << VIRTIO_BLK_F_RO);
    features &= ~(1 << VIRTIO_BLK_F_SCSI);
    features &= ~(1 << VIRTIO_BLK_F_CONFIG_WCE);
    features &= ~(1 << VIRTIO_BLK_F_MQ);
    features &= ~(1 << VIRTIO_F_ANY_LAYOUT);
    features &= ~(1 << VIRTIO_RING_F_EVENT_IDX);
    features &= ~(1 << VIRTIO_RING_F_INDIRECT_DESC);
    writeword(features, &regs->DriverFeatures);

    /*5. Set the FEATURES_OK status bit. The driver MUST NOT accept new feature bits after this step.*/
    writeword(readword(&regs->Status) | VIRTIO_STATUS_FEATURES_OK, &regs->Status);
    refresh_cache();

    /*6.Re-read device status to ensure the FEATURES_OK bit is still set: otherwise, the device does not
    support our subset of features and the device is unusable.*/

    if(!(readword(&regs->Status) & VIRTIO_STATUS_FEATURES_OK))
    {
        printk("the device does not support our subset of features and the device is unusable!\n");
        return -1;
    }

    /*7. Perform device-specific setup, including discovery of virtqueues for the device, optional per-bus setup,
    reading and possibly writing the device’s virtio configuration space, and population of virtqueues.*/
    //init queue
    writeword(0, &regs->QueueSel);
    refresh_cache();
    //check queue_ready
    if(readword(&regs->QueueReady))
    {
        printk("device not ready!\n");
        return -1;
    }
    // check maximum queue size.
    uint32_t size = readword(&regs->QueueNumMax);
    if(size == 0)
    {
        printk("device queue not available!\n");
        return -1;
    }
    else if (size < QUEUE_SIZE)
    {
        printk("device queue too short!\n");
        return -1;
    }
    //alloc memory for g_vring
    g_vring.avail = alloc_pgtable();
    g_vring.desc = alloc_pgtable();
    g_vring.used = alloc_pgtable();
    if(g_vring.avail == 1 || g_vring.desc == 1 || g_vring.used == 1)
    {
        printk("not enough for g_vring!\n");
        return -1;
    }
    g_vring.num = QUEUE_SIZE;
    //set queue size
    writeword(QUEUE_SIZE, &regs->QueueNum);
    refresh_cache();
    // write physical addresses.
    writeword((uint64_t)g_vring.desc, &regs->QueueDescLow);
    refresh_cache();
    writeword((uint64_t)g_vring.desc >> 32, &regs->QueueDescHigh);
    refresh_cache();
    writeword((uint64_t)g_vring.avail, &regs->QueueAvailLow);
    refresh_cache();
    writeword((uint64_t)g_vring.avail >> 32, &regs->QueueAvailHigh);
    refresh_cache();
    writeword((uint64_t)g_vring.used, &regs->QueueUsedLow);
    refresh_cache();
    writeword((uint64_t)g_vring.used >> 32, &regs->QueueUsedHigh);
    refresh_cache();
    // queue is ready
    writeword(0x01, &regs->QueueReady);
    refresh_cache();
    
    //8. Set the DRIVER_OK status bit. At this point the device is “live”.
    writeword(readword(&regs->Status) | VIRTIO_STATUS_DRIVER_OK, &regs->Status);
    refresh_cache();
    printk("device ready!\n");
    g_regs = regs;
    return 0;
}


//initial device
static int virtio_dev_init(uint64_t virt, uint64_t intid)
{
    virtio_regs_legacy *regs = (virtio_regs*)virt;
    if (readword(&regs->MagicValue) != VIRTIO_MAGIC) {
		printk("error: virtio at 0x%x had wrong magic value 0x%x,expected 0x%x\n",virt, regs->MagicValue, VIRTIO_MAGIC);
		return -1;
	}
	if (readword(&regs->Version) != 1) {
		printk("error: virtio at 0x%x had wrong version 0x%x, expected 0x%x\n",virt, regs->Version, VIRTIO_VERSION_LEGACY);
		return -1;
	}

	if (readword(&regs->DeviceID) == 0) {
		printk("this is a invalid device\n");
		return -1;
	}

    if(readword(&regs->VendorID) != VIRTIO_VENDORID)
    {
        printk("vendorid wrong!\n");
        return -1;
    }

    //1.reset device
    writeword(0, &regs->Status);
    refresh_cache();
    //2.Set the ACKNOWLEDGE status bit: the guest OS has noticed the device.
    writeword(readword(&regs->Status) | VIRTIO_STATUS_ACKNOWLEDGE, &regs->Status);
    refresh_cache();
    //3.Set the DRIVER status bit: the guest OS knows how to drive the device
    writeword(readword(&regs->Status) | VIRTIO_STATUS_DRIVER, &regs->Status);
    refresh_cache();

    switch (readword(&regs->DeviceID))
    {
    case VIRTIO_DEV_BLK:
        g_regs = regs;
        virtio_blk_init_legacy(regs, intid);
        break;
    default:
        printk("not support this device! DeviceID = %d",readword(&regs->DeviceID));
        break;
    }
}


void virtio_init()
{
    /*uint64_t virt_page = alloc_pgtable();
    for(int i=0;i<11;i++)
    {
        alloc_pgtable();
    }
    //MAP VIRTIO
    //create_pgd_mapping((pgd_page*)virt_page,0x10001000,virt_page,(PAGE_SIZE*12), PAGE_KERNEL, alloc_pgtable, 0); 
    //refresh_cache();
    printk("0x%x",(*(volatile unsigned int*)virt_page));*/
    for (int i = 0; i < 1; i++)
    	virtio_dev_init(0x10001000 + VIRTIO_REGS_SIZE * i, 32 + 0x10 + i);
}