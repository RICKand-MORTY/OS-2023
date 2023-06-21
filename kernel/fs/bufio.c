#include<buf.h>
#include"../../lib/printk.h"
#include"../../lib/lib.h"
#include<process.h>
#include<virtio.h>

struct {
  spinlock lock;
  Buf buf[NUMBUF];

  // Linked list of all buffers, through prev/next.
  // Sorted by how recently the buffer was used.
  // head.next is most recent, head.prev is least.
  LinkList head;
} bcache;

void binit(void)
{
    memset(&bcache, 0, sizeof(bcache));
    spin_init(&bcache.lock);
    list_init(&bcache.head);
    for(int i = 0; i < NUMBUF; i++)
    {
        list_add_tail(&bcache.buf[i].list, &bcache.head);
        init_sleeplock(&bcache.buf[i].splock, "bache");
    }
}


static Buf* bget(unsigned int dev, unsigned int blockno)
{
    Buf* b;
    PLinkList p = &bcache.head;
    p = p->next;
    spin_lock(&bcache.lock);
    //find if the block is cached
    for( ; p != &bcache.head; p = p->next)
    {
        PLinkList tmp = p;
        b = container_of(tmp, Buf, list);
        if(b->dev == dev && b->blockno == blockno)
        {
            b->refcnt++;
            spin_unlock(&bcache.lock);
            acquire_sleeplock(&b->splock);
            return b;
        }
    }

    //if not cached
    p = bcache.head.prev;
    for( ; p != &bcache.head; p = p->prev)
    {
        PLinkList tmp = p;
        b = container_of(tmp, Buf, list);
        if(b->refcnt == 0)
        {
            b->dev = dev;
            b->blockno = blockno;
            b->valid = 0;
            b->refcnt = 1;
            spin_unlock(&bcache.lock);
            acquire_sleeplock(&b->splock);
            return b;
        }
    }
    printk("error: No Buf!\n");
    spin_unlock(&bcache.lock);
}

// Return a locked buf with the contents of the indicated block.
Buf* bread(unsigned int dev, unsigned int blockno)
{
    Buf* b = bget(dev, blockno);
    if(!b->valid)
    {
        virtio_disk_rw(b, 0);
        b->valid = 1;
    }
    return b;
}

// Write b's contents to disk.  Must be locked.
void bwrite(Buf *b)
{
  if(!holding_sleeplock(&b->splock))
  {
    printk("No locked!\n");
  }
  virtio_disk_rw(b, 1);
}

// Release a locked buffer.
// Move to the head of the least-recently-used list.
void brelease(Buf* b)
{
    if(!holding_sleeplock(&b->splock))
    {
        printk("No locked!\n");
    }
    release_sleeplock(&b->splock);
    spin_lock(&bcache.lock);
    b->refcnt--;
    if(!b->refcnt)
    {
        b->list.next->prev = b->list.prev;
        b->list.prev->next = b->list.next;
        b->list.next = bcache.head.next;
        b->list.prev = &bcache.head;
        bcache.head.next->prev = &b->list;
        bcache.head.next = &b->list;
    }
    spin_unlock(&bcache.lock);
}

void b_add_ref(Buf *b) 
{
  spin_lock(&bcache.lock);
  b->refcnt++;
  spin_unlock(&bcache.lock);
}

void b_de_ref(Buf *b) 
{
  spin_lock(&bcache.lock);
  b->refcnt--;
  spin_unlock(&bcache.lock);
}