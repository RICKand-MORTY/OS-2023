#ifndef _FAT32_H
#define _FAT32_H


/*fat32引导扇区*/
struct FAT32_BootSector
{
	unsigned char BS_jmpBoot[3];
	unsigned char BS_OEMName[8];
	unsigned short BPB_BytesPerSec;
	unsigned char BPB_SecPerClus;
	unsigned short BPB_RsvdSecCnt;
	unsigned char BPB_NumFATs;
	unsigned short BPB_RootEntCnt;
	unsigned short BPB_TotSec16;
	unsigned char BPB_Media;
	unsigned short BPB_FATSz16;
	unsigned short BPB_SecPerTrk;
	unsigned short BPB_NumHeads;
	unsigned int BPB_HiddSec;
	unsigned int BPB_TotSec32;
	
	unsigned int BPB_FATSz32;
	unsigned short BPB_ExtFlags;
	unsigned short BPB_FSVer;
	unsigned int BPB_RootClus;
	unsigned short BPB_FSInfo;
	unsigned short BPB_BkBootSec;
	unsigned char BPB_Reserved[12];

	unsigned char BS_DrvNum;
	unsigned char BS_Reserved1;
	unsigned char BS_BootSig;
	unsigned int BS_VolID;
	unsigned char BS_VolLab[11];
	unsigned char BS_FilSysType[8];

	unsigned char BootCode[420];

	unsigned short BS_TrailSig;
}__attribute__((packed));


struct FAT32_FSInfo
{
	unsigned int FSI_LeadSig;
	unsigned char FSI_Reserved1[480];
	unsigned int FSI_StrucSig;
	unsigned int FSI_Free_Count;
	unsigned int FSI_Nxt_Free;
	unsigned char FSI_Reserved2[12];
	unsigned int FSI_TrailSig;
}__attribute__((packed));

#define	ATTR_READ_ONLY	(1 << 0)
#define ATTR_HIDDEN	(1 << 1)
#define ATTR_SYSTEM	(1 << 2)
#define ATTR_VOLUME_ID	(1 << 3)
#define ATTR_DIRECTORY	(1 << 4)
#define ATTR_ARCHIVE	(1 << 5)
#define ATTR_LONG_NAME	(ATTR_READ_ONLY | ATTR_HIDDEN | ATTR_SYSTEM | ATTR_VOLUME_ID)

/*短目录项*/
struct FAT32_Directory
{
	unsigned char DIR_Name[11];
	unsigned char DIR_Attr;
	unsigned char DIR_NTRes;
	unsigned char DIR_CrtTimeTenth;
	unsigned short DIR_CrtTime;	
	unsigned short DIR_CrtDate;
	unsigned short DIR_LastAccDate;
	unsigned short DIR_FstClusHI;
	unsigned short DIR_WrtTime;
	unsigned short DIR_WrtDate;
	unsigned short DIR_FstClusLO;
	unsigned int DIR_FileSize;
}__attribute__((packed));

#define LOWERCASE_BASE (8)
#define LOWERCASE_EXT (16)
struct FAT32_LongDirectory
{
	unsigned char LDIR_Ord;
	unsigned short LDIR_Name1[5];
	unsigned char LDIR_Attr;
	unsigned char LDIR_Type;
	unsigned char LDIR_Chksum;
	unsigned short LDIR_Name2[6];
	unsigned short LDIR_FstClusLO;
	unsigned short LDIR_Name3[2];
}__attribute__((packed));

struct FAT32_sb_info
{
	unsigned long start_sector;
	unsigned long sector_count;

	long sector_per_cluster;
	long bytes_per_cluster;
	long bytes_per_sector;

	unsigned long Data_firstsector;
	unsigned long FAT1_firstsector;
	unsigned long sector_per_FAT;
	unsigned long NumFATs;

	unsigned long fsinfo_sector_infat;
	unsigned long bootsector_bk_infat;
	
	struct FAT32_FSInfo * fat_fsinfo;
};

struct FAT32_inode_info
{
	unsigned long first_cluster;
	unsigned long dentry_location;	//dentry struct in cluster(0 is root,1 is invalid)
	unsigned long dentry_position;	//dentry struct offset in cluster

	unsigned short create_date;
	unsigned short create_time;

	unsigned short write_date;
	unsigned short write_time;
};

extern struct index_node_operations FAT32_inode_ops;
extern struct file_operations FAT32_file_ops;
extern struct dir_entry_operations FAT32_dentry_ops;
extern struct super_block_operations FAT32_sb_ops;

void FAT32_init();
struct dir_entry * path_walk(char * name,unsigned long flags);
unsigned char* read_more_sector(unsigned int block, unsigned int count, unsigned int *size);
#endif