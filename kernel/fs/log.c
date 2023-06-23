#include<log.h>
#include<buf.h>
#include<process.h>
#include "../../lib/lib.h"

PCB *sleep_pro;
struct log log;

static void recover_from_log(void);
static void commit();

int initlog(int dev, superblock_p sb)
{
    if (sizeof(struct logheader) >= BSIZE)
    return -1;

    spin_init(&log.snlock);
    log.start = sb->logstart;
    log.size = sb->nlog;
    log.dev = dev;
    recover_from_log();
    return 0;
}

// Copy committed blocks from log to their home location
static void install_trans(int recovering)
{
    for(int i = 0; i < log.lh.n; i++)
    {
        Buf *lbuf = bread(log.dev, log.start+i+1);
        Buf *dbuf = bread(log.dev, log.lh.block[i]);
        memmove(dbuf->data, lbuf->data, BSIZE);  // lbuf-data to dbuf->data
        bwrite(dbuf);  // write dst to disk
        if(recovering == 0)//如果不是在恢复模式下，就取消缓存块dbuf的锁定状态，使其可以被其他进程使用
            b_de_ref(dbuf);
        brelease(lbuf);
        brelease(dbuf);
    }
}

//      | log header | log block 1 | log block 2 | … | log block n |
// Read the log header from disk into the in-memory log header
static void read_head(void)
{
  Buf *buf = bread(log.dev, log.start);
  struct logheader *lh = (struct logheader *) (buf->data);
  int i;
  log.lh.n = lh->n;
  for (i = 0; i < log.lh.n; i++) {
    log.lh.block[i] = lh->block[i];
  }
  brelease(buf);
}

static void write_head(void)
{
  /*header是需要写入到块中的，但是开始先要用bread从设备块中读出来，
  是因为bread函数不仅会从磁盘上读取块的数据，
  还会将这个块锁定在缓冲区中，防止其他进程修改它。这样，
  当函数要更新header时，就可以保证header不会被其他进程干扰。
  另外，bread函数还会检查缓冲区中是否已经有了这个块的数据，
  如果有，就不需要再从磁盘上读取，
  直接返回缓冲区中的数据。这样可以提高效率。*/
  Buf *buf = bread(log.dev, log.start);
  struct logheader *hb = (struct logheader *) (buf->data);
  int i;
  hb->n = log.lh.n;
  for (i = 0; i < log.lh.n; i++) {
    hb->block[i] = log.lh.block[i];
  }
  bwrite(buf);
  brelease(buf);
}

//从日志中恢复文件系统的状态
static void recover_from_log(void)
{
  read_head();
  install_trans(1); //recovery mode
  log.lh.n = 0;
  write_head(); 
}

void begin_log(void)
{
  spin_lock(&log.snlock);
  while (1)
  {
    if(log.committing || log.lh.n + (log.outstanding+1)* MAXBLOCKS > LOGSIZE)
    {
      sleep_pro = sleep(get_current_task()->pid);
    }
    else
    {
      log.outstanding += 1;
      spin_unlock(&log.snlock);
      break;
    }
  }
}

void end_log(void)
{
  int do_commit = 0;
  spin_lock(&log.snlock);
  log.outstanding -= 1;
  if(log.committing)
    printk("log.committing\n");
  if(log.outstanding == 0){
    //没有其他操作正在执行了
    do_commit = 1;
    log.committing = 1;
  } 
  else 
  {
    //还有其他操作正在执行
    if(sleep_pro->pid != 0)
    {
      wakeup(sleep_pro->pid);
    }
  }
  spin_unlock(&log.snlock);

  if(do_commit){
    // call commit w/o holding locks, since not allowed
    // to sleep with locks.
    commit(); //前面要先释放锁，后面再加上锁，因为睡眠不能持有锁，否则可能会死锁
    spin_lock(&log.snlock);
    log.committing = 0;
    if(sleep_pro->pid != 0)
    {
      wakeup(sleep_pro->pid);
    }
    spin_unlock(&log.snlock);
  }
}

static void write_log(void)
{
  int tail;

  for (tail = 0; tail < log.lh.n; tail++) {
    struct buf *to = bread(log.dev, log.start+tail+1); // log block
    struct buf *from = bread(log.dev, log.lh.block[tail]); // cache block
    memmove(to->data, from->data, BSIZE);
    bwrite(to);  // write the log
    brelease(from);
    brelease(to);
  }
}

static void commit()
{
  if (log.lh.n > 0) {
    write_log();     // Write modified blocks from cache to log
    write_head();    // Write header to disk -- the real commit
    install_trans(0); // Now install writes to home locations
    //install_trans是把write_log写入的数据又写回文件系统中。这样做的目的是为了保证文件系统的一致性，
    //即使在出现故障时也能恢复文件系统的状态。
    log.lh.n = 0;
    write_head(); 
  }
}

void log_write(struct buf *b)
{
  int i=0;
  spin_lock(&log.snlock);
  if (log.lh.n >= LOGSIZE || log.lh.n >= log.size - 1)
    printk("too big a transaction");
  if (log.outstanding < 1)
  //没有操作正在执行
    printk("log_write outside of trans");

  for (i = 0; i < log.lh.n; i++) {
    if (log.lh.block[i] == b->blockno)   // log absorption
      break;
  }
  log.lh.block[i] = b->blockno;
  if (i == log.lh.n) {  // Add new block to log?
    b_add_ref(b);
    log.lh.n++;
  }
  spin_unlock(&log.snlock);
}