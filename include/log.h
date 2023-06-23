#ifndef _LOG_H
#define _LOG_H

#include<fs.h>
#include "spinlock.h"

#define LOGSIZE     30      //log maximum size of blocks
#define BSIZE       1024    //block size
#define MAXBLOCKS   10      //maximum blocks of log in an operation(syscall)


struct logheader {
  int n;  //日志块的数量
  int block[LOGSIZE]; //扇区号表明该日志块，应该写入的位置。
};

struct log {
  spinlock snlock;
  int start;  //日志区域在磁盘上的起始块号
  int size; // 日志区域在磁盘上的总块数(容量)
  // 当前有多少个文件系统操作正在进行，用于判断是否需要提交或安装日志
  int outstanding; // how many FS sys calls are executing.
  // 当前是否有其他进程正在提交日志，如果有，就需要等待它完成
  int committing;  // in commit(), please wait.
  int dev;//// 日志所在的设备号
  struct logheader lh;
};

void begin_add_log(void);
int initlog(int dev, superblock_p sb);
#endif