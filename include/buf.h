#ifndef _BUF_H
#define _BUF_H


#include "LinkList.h"
#include "sleeplock.h"

#define BSIZE   (512)
#define NUMBUF  (30)

//reference from xv6
typedef struct buf {
  int valid;   // has data been read from disk?
  int disk;    // does disk "own" buf?
  unsigned int dev;
  unsigned int blockno;
  sleeplock splock;
  unsigned int refcnt;      //reference count
  LinkList list;
  unsigned char data[BSIZE];
}Buf;

void binit(void);
Buf* bread(unsigned int dev, unsigned int blockno);
void bwrite(Buf *b);
void brelease(Buf* b);
void b_add_ref(Buf *b);
void b_de_ref(Buf *b); 
#endif