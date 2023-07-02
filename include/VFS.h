#ifndef _VFS_H  
#define _VFS_H


#include "LinkList.h"

struct super_block
{
	struct dir_entry * root;				//根目录目录项

	struct super_block_operations * sb_ops;	//与superblock有关的操作集合

	void * private_sb_info;					
};

struct index_node
{
	unsigned long file_size;
	unsigned long blocks;
	unsigned long attribute;					//保存目录项属性

	struct super_block * sb;

	struct file_operations * f_ops;
	struct index_node_operations * inode_ops;

	void * private_index_info;
};

#define FS_ATTR_FILE	(1UL << 0)
#define FS_ATTR_DIR		(1UL << 1)

struct dir_entry
{
	char * name;
	int name_length;
	LinkList child_node;
	LinkList subdirs_list;

	struct index_node * dir_inode;
	struct dir_entry * parent;

	struct dir_entry_operations * dir_ops;
};

struct file
{
	long position;
	unsigned long mode;

	struct dir_entry * dentry;

	struct file_operations * f_ops;

	void * private_data;
};

struct super_block_operations
{
	void(*write_superblock)(struct super_block * sb);
	void(*put_superblock)(struct super_block * sb);

	void(*write_inode)(struct index_node * inode);
};

struct index_node_operations
{
	long (*create)(struct index_node * inode,struct dir_entry * dentry,int mode);
	struct dir_entry* (*lookup)(struct index_node * parent_inode,struct dir_entry * dest_dentry);
	long (*mkdir)(struct index_node * inode,struct dir_entry * dentry,int mode);
	long (*rmdir)(struct index_node * inode,struct dir_entry * dentry);
	long (*rename)(struct index_node * old_inode,struct dir_entry * old_dentry,struct index_node * new_inode,struct dir_entry * new_dentry);
	long (*getattr)(struct dir_entry * dentry,unsigned long * attr);
	long (*setattr)(struct dir_entry * dentry,unsigned long * attr);
};

struct dir_entry_operations
{
	long (*compare)(struct dir_entry * parent_dentry,char * source_filename,char * destination_filename);
	long (*hash)(struct dir_entry * dentry,char * filename);
	long (*release)(struct dir_entry * dentry);
	long (*iput)(struct dir_entry * dentry,struct index_node * inode);
};

struct file_operations
{
	long (*open)(struct index_node * inode,struct file * filp);
	long (*close)(struct index_node * inode,struct file * filp);
	long (*read)(struct file * filp,char * buf,unsigned long count,long * position);
	long (*write)(struct file * filp,char * buf,unsigned long count,long * position);
	long (*lseek)(struct file * filp,long offset,long origin);
	long (*ioctl)(struct index_node * inode,struct file * filp,unsigned long cmd,unsigned long arg);
};

struct file_system_type
{
	char * name;
	int fs_flags;
	//用于解析引导扇区
	struct super_block * (*read_superblock)(struct Disk_Partition_Table_Entry * DPTE,void * buf);
	struct file_system_type * next;
};

/*MBR内分区表*/
struct Disk_Partition_Table_Entry
{
	unsigned char flags;				//引导标志
	unsigned char start_head;			//磁头号、扇区号、柱面号(sd卡不用管)
	unsigned short  start_sector	:6,	//0~5
			start_cylinder	:10;		//6~15
	unsigned char type;					//分区类型符(83H为Linux分区)
	unsigned char end_head;				//结束磁头号、扇区号、柱面号
	unsigned short  end_sector	:6,		//0~5
			end_cylinder	:10;		//6~15
	unsigned int start_LBA;				//逻辑起始扇区号
	unsigned int sectors_limit;			//本分区的总扇区数
	
}__attribute__((packed));

/*MBR*/
struct Disk_Partition_Table
{
	unsigned char BS_Reserved[446];             //used for bootloader
	struct Disk_Partition_Table_Entry DPTE[4];  
	unsigned short BS_TrailSig;
}__attribute__((packed));

struct super_block* mount_fs(char * name,struct Disk_Partition_Table_Entry * DPTE,void * buf);
unsigned long register_filesystem(struct file_system_type * fs);
#endif