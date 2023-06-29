#include <fat32.h>
#include "../../lib/printk.h"
#include "../../lib/lib.h"
#include <buf.h>

//#define MBR
#define NULL 0

struct Disk_Partition_Table mbr;
struct FAT32_BootSector fat32_boot;
struct FAT32_FSInfo fsinfo;

unsigned long FirstDataSector = 0;          //数据区起始扇区号
unsigned long BytesPerClus = 0;             //每簇字节数
unsigned long FirstFAT1Sector = 0;          //fat1起始扇区号
unsigned long FirstFAT2Sector = 0;          //fat2起始扇区号

void fat32_init()
{
    Buf *buf = NULL;
    memset(&mbr, 0, sizeof(mbr));
    memset(&fat32_boot, 0, sizeof(fat32_boot));
    memset(&fsinfo, 0, sizeof(fsinfo));

    #ifdef MBR
    //read MBR
    buf = bread(1, 0);      
    mbr = *(struct Disk_Partition_Table*)buf->data;
    printk("\npartition 1: start_LBA=%#018lx\ttype=%#018lx\n", mbr.DPTE[0].start_LBA, mbr.DPTE[0].type);
    brelease(buf);
    #endif

    //read fat32 boot sector
    buf = bread(1, mbr.DPTE[0].start_LBA);
    fat32_boot = *(struct FAT32_BootSector*)buf->data;
    printk("\nFAT32 Boot Sector:\nBPB_FSInfo:%#018lx\n\tBPB_BkBootSec:%#018lx\n\tBPB_TotSec32:%#018lx\n\tBPB_SecPerClus:%#018lx",fat32_boot.BPB_FSInfo,fat32_boot.BPB_BkBootSec,fat32_boot.BPB_TotSec32,fat32_boot.BPB_SecPerClus);
    brelease(buf);

    //read fs_info
    buf = bread(1, mbr.DPTE[0].start_LBA + fat32_boot.BPB_FSInfo);
    fsinfo = *(struct FAT32_FSInfo *)(buf->data);
    printk("FAT32 FSInfo:\n\tFSI_LeadSig:%#018lx\n\tFSI_StrucSig:%#018lx\n\tFSI_Free_Count:%#018lx\n",fsinfo.FSI_LeadSig,fsinfo.FSI_StrucSig,fsinfo.FSI_Free_Count);

    FirstDataSector = mbr.DPTE[0].start_LBA + fat32_boot.BPB_RsvdSecCnt + fat32_boot.BPB_FATSz32 * fat32_boot.BPB_NumFATs;
	FirstFAT1Sector = mbr.DPTE[0].start_LBA + fat32_boot.BPB_RsvdSecCnt;
	FirstFAT2Sector = FirstFAT1Sector + fat32_boot.BPB_FATSz32;
	BytesPerClus = fat32_boot.BPB_SecPerClus * fat32_boot.BPB_BytesPerSec;

    printk("FirstDataSector = %d\tFirstFAT1Sector = %d\nFirstFAT2Sector = %d\tBytesPerClus = %d\n",FirstDataSector, FirstFAT1Sector, FirstFAT2Sector, BytesPerClus);
}