#include <virtio.h>
#include <memory.h>
#include <io.h>

#define VIRTIO_BLK_REQ_HEADER_SIZE      16       //desc1 header
#define VIRTIO_BLK_REQ_MIDDLE_SIZE      512      //desc2 data
#define VIRTIO_BLK_REQ_FOOTER_SIZE      1        //desc3 status
struct vring g_vring;

// a function to allocate memory for the vring
void *vring_alloc(uint32_t size) {
    // align the size to 4096 bytes
    size = (size + 4095) & ~4095;
    // allocate memory using allo_page() function
    void *ptr = alloc_pgtable(); // for example
    if (!ptr) {
        printk("Memory allocation failed\n");
        return NULL;
    }
    // zero out the memory
    memset(ptr, 0, size);
    return ptr;
}

// a function to calculate the size of vring memory
uint32_t vring_size(unsigned int num, unsigned long align) {
    uint32_t size = num * sizeof(struct vring_desc);
    size += sizeof(__virtio16) * (3 + num);
    size = (size + align - 1) & ~(align - 1);
    size += sizeof(__virtio16) * 3 + sizeof(struct vring_used_elem) * num;
    return size;
}


// a function to initialize the vring
void vring_init(struct vring *vr, unsigned int num, void *p) {
    vr->num = num;
    vr->desc = p;
    vr->avail = p + num * sizeof(struct vring_desc);
    vr->used = (void *)(((unsigned long)&vr->avail->ring[num] + sizeof(__virtio16) + 4095) & ~4095);
}

// a function to perform a read or write operation on the virtio device
int virtio_rw(uint64_t sector, uint8_t *buffer, int write) {
    // select the queue 0
    g_regs->QueueSel = 0;
    // get the queue size
    unsigned int qsize = g_regs->QueueNum;
    if (qsize == 0) {
        printk("Queue size is zero\n");
        return -1;
    }
    if (qsize > QUEUE_SIZE) {
        qsize = QUEUE_SIZE; // we only use QUEUE_SIZE descriptors
    }
    
	// allocate memory for the request structure and the vring
	struct virtio_blk_req *req = vring_alloc(sizeof(struct virtio_blk_req));
	struct vring vr;
	vring_init(&vr, qsize, vring_alloc(vring_size(qsize, g_regs->QueueAlign)));
	
	// set up the request header
	req->type = write ? VIRTIO_BLK_T_OUT : VIRTIO_BLK_T_IN;
	req->reverse = 0;
	req->sector = sector;

	// set up the descriptor table
	req->desc[0].addr = (uint64_t)&req->type; // header address
	req->desc[0].len = sizeof(req->type) + sizeof(req->reverse) + sizeof(req->sector); // header length
	req->desc[0].flags = VIRTQ_DESC_F_NEXT; // next descriptor follows
	req->desc[0].next = 1; // next descriptor index
	
	req->desc[1].addr = (uint64_t)buffer; // data buffer address
	req->desc[1].len = 512; // data buffer length
	req->desc[1].flags = VIRTQ_DESC_F_NEXT | (write ? VIRTQ_DESC_F_WRITE : 0); // next descriptor follows and write flag depends on operation type
	req->desc[1].next = 2; // next descriptor index
	
	req->desc[2].addr = (uint64_t)&req->status; // status address
	req->desc[2].len = sizeof(req->status); // status length
	req->desc[2].flags = VIRTQ_DESC_F_WRITE; // write flag set
	req->desc[2].next = 0; // no next descriptor
	
	// set up the avail ring
	vr.avail->flags = 0; // no flags set
	vr.avail->idx = 1; // one available descriptor chain
	vr.avail->ring[0] = 0; // index of the first descriptor
	
	// tell the device the queue address by writing the physical page number to QueuePFN register
	g_regs->QueuePFN = (uint32_t)((unsigned long)vr.desc >> g_regs->GuestPageSize);
	
	// notify the device by writing the queue index to QueueNotify register
	g_regs->QueueNotify = 0;
	
	// wait for the device to process the request by checking the InterruptStatus register and the used index

    // add a timeout mechanism to avoid dead loop
    int timeout = 0;    // set timeout to 10 seconds

	while (!(g_regs->InterruptStatus & 1) && vr.used->idx == 0) {
        if (timeout == 10000) {
            printk("Request timed out\n");
            return -1;
        }
        timeout++;
    }
	
	// acknowledge the interrupt by writing 1 to InterruptACK register
	g_regs->InterruptACK = g_regs->InterruptStatus;
	
	// check the status of the request by reading the status byte
	if (req->status != 0) {
		printk("Request failed with status %d\n", req->status);
		return -1;
	}
    
    // check the used ring for errors or unexpected lengths
    struct vring_used_elem *used_elem = &vr.used->ring[vr.used->idx % qsize];
    if (used_elem->id != 0) {
        printk("Unexpected used element id %d\n", used_elem->id);
        return -1;
    }
    if (used_elem->len != req->desc[0].len + req->desc[1].len + req->desc[2].len) {
        printk("Unexpected used element length %d\n", used_elem->len);
        return -1;
    }
    if (vr.used->flags & VIRTQ_USED_F_NO_NOTIFY) {
        printk("Device requests no interrupts\n");
        return -1;
    }
    if (vr.used->flags & VIRTQ_USED_F_INDIRECT) {
        printk("Device uses indirect descriptors\n");
        return -1;
    }
	
	// free the memory allocated for the request and the vring
	//free(req);
	//free(vr.desc);
	printk("request sent!\n");
	return 0; // success
}



int virtio_blk_rw(uint32_t type, uint32_t sector, uint8_t *data)
{
    uint32_t mode = 0;
    req.reverse = 0;
    req.type = type;
    req.sector = sector;
    if(type == VIRTIO_BLK_T_IN)
    {
        mode = VIRTQ_DESC_F_WRITE;
    }
    req.desc[0].addr = alloc_pgtable();
    req.desc[0].len = VIRTIO_BLK_REQ_HEADER_SIZE;
    req.desc[0].flags = VIRTQ_DESC_F_NEXT;
    req.desc[1].addr = data;
    req.desc[1].flags = mode | VIRTQ_DESC_F_NEXT;
    req.desc[1].len = VIRTIO_BLK_REQ_MIDDLE_SIZE;
    req.desc[2].addr = alloc_pgtable();
    req.desc[2].len = VIRTIO_BLK_REQ_FOOTER_SIZE;
    req.desc[2].flags = VIRTQ_DESC_F_WRITE;

    req.desc[0].next = 1;
    req.desc[1].next = 2;

    req.avail.ring[req.avail.idx] = 0;
    refresh_cache();
    req.avail.idx += 1;
    refresh_cache();
    writeword(0, &g_regs->QueueNotify);
    refresh_cache();
    printk("request sent!\n");
    return 0;
}


static int virtio_blk_init_legacy(virtio_regs_legacy *regs, uint32_t intid)
{

    /*4.Read device feature bits, and write the subset of feature bits understood by the OS and driver to the
    device. During this step the driver MAY read (but MUST NOT write) the device-specific configuration
    fields to check that it can support the device before accepting it.*/
    refresh_cache();
    writeword(0, &regs->HostFeaturesSel);
    refresh_cache();
    uint32_t features = readword(&regs->HostFeatures);
    refresh_cache();
    features &= ~(1 << VIRTIO_BLK_F_RO);
    features &= ~(1 << VIRTIO_BLK_F_SCSI);
    features &= ~(1 << VIRTIO_BLK_F_CONFIG_WCE);
    features &= ~(1 << VIRTIO_BLK_F_MQ);
    features &= ~(1 << VIRTIO_F_ANY_LAYOUT);
    features &= ~(1 << VIRTIO_RING_F_EVENT_IDX);
    features &= ~(1 << VIRTIO_RING_F_INDIRECT_DESC);
    refresh_cache();
    writeword(0, &regs->GuestFeatures);
    refresh_cache();
    writeword(features, &regs->GuestFeatures);
    refresh_cache();
    


    writeword(readword(&regs->Status)|VIRTIO_STATUS_FEATURES_OK , &regs->Status);
    refresh_cache();
    /*
    7. Perform device-specific setup, including discovery of virtqueues for the device, optional per-bus setup,
    reading and possibly writing the device’s virtio configuration space, and population of virtqueues.
    */
    //configure the queue:
    //a. Select the queue writing its index (first queue is 0) to QueueSel(init queue)
    refresh_cache();
    writeword(0, &regs->QueueSel);
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
   refresh_cache();
    uint32_t size = readword(&regs->QueueNumMax);
    refresh_cache();
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

    /*if((readword(&regs->HostFeatures) & VIRTIO_F_VERSION_1) == 0)
    {
        printk("Device does not support virtio 1.0 %x\n", regs->HostFeatures);
		return -1;
    }*/
    /*
    d. Allocate and zero the queue pages in contiguous virtual memory,
    aligning the Used Ring to an optimal boundary (usually page size). 
    The driver should choose a queue size smaller than or equal to
    */
    g_vring.avail = alloc_pgtable();
    g_vring.desc = alloc_pgtable();
    g_vring.used = PAGE_ALIGN_UP(alloc_pgtable());
    if(g_vring.avail == 1 || g_vring.desc == 1 || g_vring.used == 1)
    {
        printk("not enough for g_vring!\n");
        return -1;
    }
    g_vring.num = QUEUE_SIZE;
    refresh_cache();
    //e. Notify the device about the queue size by writing the size to QueueNum.
    writeword(QUEUE_SIZE, &regs->QueueNum);
    refresh_cache();
    printk("regs->QueueNum = 0x%x",(unsigned long)(&regs->QueueNum)-(unsigned long)(regs));
    //f. Notify the device about the used alignment by writing its value in bytes to QueueAlign.
    writeword(PAGE_SIZE, &regs->QueueAlign);
    refresh_cache();
    //g. Write the physical number of the first page of the queue to the QueuePFN register
    writeword(g_vring.desc, &regs->QueuePFN);
    refresh_cache();

    //8. Set the DRIVER_OK status bit. At this point the device is “live”.
    writeword((readword(&regs->Status) | VIRTIO_STATUS_DRIVER_OK), &regs->Status);
    refresh_cache();
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
	if (readword(&regs->Version) != 2) {
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
    for (int i = 0; i < 8; i++)
    	virtio_dev_init(0x10001000 + VIRTIO_REGS_SIZE * i, 32 + 0x10 + i);
}