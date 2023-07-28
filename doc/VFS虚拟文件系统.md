## VFS虚拟文件系统

我们的文件系统部分主要参考了《一个64位操作系统的设计与实现》一书，该设计方式借鉴了Linux的虚拟文件系统(VFS),VFS是对文件系统的抽象概括，其好处在于可以让应用程序无需关注不同文件系统的细节，只需使用其提供的接口即可，目前OS仅支持FAT32，操作方法并未全部实现

### 相关结构

super_block结构用于记录文件系统引导扇区的信息和其他操作系统分配的资源信息

```c
/*VFS.h*/
struct super_block
{
	struct dir_entry * root;				//根目录目录项

	struct super_block_operations * sb_ops;	//与superblock有关的操作集合

	void * private_sb_info;					
};
```

其中`private_sb_info`成员用于存储文件系统信息,在使用时指向如下结构体：

```c
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

```

`FAT32_sb_info`结构体保存着与FAT32相关的信息

`index_node`结构体用于记录文件的相关信息，`f_ops`和`inode_ops`分别是`file`和`inode`相关的操作方法集合

```C
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
```

`private_index_info`存储着文件相关信息,使用时指向如下结构体

```C
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
```

`dir_entry`是与目录和文件层级相关的结构,由`child_node`和`subdirs_list`两个成员描述目录项之间的层级关系，`dir_ops`是与目录项相关的方法集合

```c
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

```

file结构体与文件IO相关，其中position成员表示文件指针，mode表示文件操作模式

```c
struct file
{
	long position;
	unsigned long mode;

	struct dir_entry * dentry;

	struct file_operations * f_ops;

	void * private_data;
};
```

### 相关方法

与super_block相关：

```C
struct super_block_operations
{
	void(*write_superblock)(struct super_block * sb);	//未实现
	void(*put_superblock)(struct super_block * sb);

	void(*write_inode)(struct index_node * inode);
};
```

`write_node`用于将发生修改的`inode`写回，put方法用于释放超级块

与`inode`相关：

```C
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
```

其中lookup方法已经实现，用于从父目录中搜索子目录项,若搜索成功,会对子目录项进行初始化,若失败,返回NULL

与目录项相关的方法：

```c
struct dir_entry_operations
{
	long (*compare)(struct dir_entry * parent_dentry,char * source_filename,char * destination_filename);
	long (*hash)(struct dir_entry * dentry,char * filename);
	long (*release)(struct dir_entry * dentry);
	long (*iput)(struct dir_entry * dentry,struct index_node * inode);
};
```

目前目录项的方法都还未实现

与文件相关的方法：

```c
struct file_operations
{
	long (*open)(struct index_node * inode,struct file * filp);
	long (*close)(struct index_node * inode,struct file * filp);
	long (*read)(struct file * filp,char * buf,unsigned long count,long * position);
	long (*write)(struct file * filp,char * buf,unsigned long count,long * position);
	long (*lseek)(struct file * filp,long offset,long origin);
	long (*ioctl)(struct index_node * inode,struct file * filp,unsigned long cmd,unsigned long arg);
	long (*readdir)(struct file * filp,void * dirent,filldir_t filler);
};

```

open和close作为系统调用实现，read方法用于从文件中读取指定长度数据并放入缓冲区,write用于写入指定长度数据到文件中,`lseek`用于设置文件指针，即file结构体中的position成员的值，`readdir`方法功能类似lookup方法，用于从指定目录中找到有效目录项，`ioctl`方法暂时还未实现



### 文件系统初始化

以下结构体用于表示文件系统类型，每种文件系统有自己的`read_superblock`方法解析自己的超级块,各个文件系统以链表的形式连接

```c
struct file_system_type
{
	char * name;
	int fs_flags;
	//用于解析引导扇区
	struct super_block * (*read_superblock)(struct Disk_Partition_Table_Entry * DPTE,void * buf);
	struct file_system_type * next;
};

```

MBR结构如下，初始化文件系统时将会解析DPTE表项：

```c
/*MBR*/
struct Disk_Partition_Table
{
	unsigned char BS_Reserved[446];             //used for bootloader
	struct Disk_Partition_Table_Entry DPTE[4];  
	unsigned short BS_TrailSig;
}__attribute__((packed));

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
```

在`VFS.c`文件中实现了两个函数:

* `mount_fs`:该函数是文件系统的挂载函数，将会搜索目标文件系统名并解析引导扇区
* register_filesystem:该函数用于注册文件系统，将文件系统插入链表中

FAT32初始化流程(`fat32.c/FAT32_init`)：

先调用register_filesystem函数注册FAT32文件系统，然后根据MBR中各个分区表中的逻辑起始扇区号，将引导扇区数据读入内存，并调用mount_fs函数解析引导扇区，在mount_fs函数中会调用`file_system_type`的`read_superblock`方法来解析引导扇区，FAT32的`read_superblock`方法是位于fat32.c中的fat32_read_superblock函数，该函数将会根据传入的引导扇区的数据建立superblock结构