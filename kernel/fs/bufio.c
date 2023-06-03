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
    spin_init(&bcache.lock);
    list_init(&bcache.head);
    memset(&bcache, 0, sizeof(bcache));
    for(int i = 0; i < NUMBUF; i++)
    {
        list_add(&bcache.head, &bcache.buf[i].list, ADD_TAIL);
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
        b = container_of(p, Buf, list);
        if(b->dev == dev && b->blockno == blockno)
        {
            b->refcnt++;
            spin_unlock(&bcache.lock);
            acquire_sleeplock(&b->splock);
            return b;
        }
    }

    //if not cached
    p = &bcache.head.prev;
    for( ; p != &bcache.head; p = p->prev)
    {
        b = container_of(p, Buf, list);
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
}

// Return a locked buf with the contents of the indicated block.
Buf* bread(unsigned int dev, unsigned int blockno)
{
    Buf* b = bget(dev, blockno);
    if(!b->valid)
    {
        virtio_rw(blockno / 512, b, VIRTIO_BLK_T_IN);   //need rewrite
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
  virtio_rw(0, b, VIRTIO_BLK_T_OUT);    //need rewrite
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
        list_delete_node(&b->list.prev, &b->list.next);
        list_add(&bcache.head, &b->list, ADD_TAIL);
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