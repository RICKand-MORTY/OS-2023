#include<log.h>
#include<buf.h>
#include<process.h>

struct log log;

static void recover_from_log(void);

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

void begin_add_log(void)
{
  spin_lock(&log.snlock);
  while (1)
  {
    if(log.committing || log.lh.n + (log.outstanding+1)* LOGSIZE > LOGSIZE)
    {
      sleep(get_current_task()->pid);
    }
    else
    {
      log.outstanding += 1;
      spin_unlock(&log.snlock);
      break;
    }
  }
}