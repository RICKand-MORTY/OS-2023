#ifndef _FS_H
#define _FS_H


typedef struct _superblock {
  unsigned int magic;        // Must be FSMAGIC
  unsigned int size;         // Size of file system image (blocks)
  unsigned int nblocks;      // Number of data blocks
  unsigned int ninodes;      // Number of inodes.
  unsigned int nlog;         // Number of log blocks
  unsigned int logstart;     // Block number of first log block
  unsigned int inodestart;   // Block number of first inode block
  unsigned int bmapstart;    // Block number of first free map block
}superblock,*superblock_p;


#endif