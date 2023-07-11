#include <fat32.h>
#include "../../lib/printk.h"
#include "../../lib/lib.h"
#include <buf.h>
#include <memory.h>
#include <VFS.h>
#include <error.h>
#include <process.h>
#include <sysflags.h>

//#define QEMU

#define CACHE_SIZE (BSIZE) 
unsigned long cache[CACHE_SIZE]; // 缓存数组
int cache_index = 0; // 缓存索引
int cache_valid = 0; // 缓存有效标志

struct super_block * root_sb = NULL;
struct Disk_Partition_Table mbr;
struct FAT32_BootSector fat32_boot;
struct FAT32_FSInfo fsinfo;

unsigned long FirstDataSector = 0;          //数据区起始扇区号
unsigned long BytesPerClus = 0;             //每簇字节数
unsigned long FirstFAT1Sector = 0;          //fat1起始扇区号
unsigned long FirstFAT2Sector = 0;          //fat2起始扇区号

//由fat表项编号来读取fat表项值
unsigned int read_FAT_Entry(struct FAT32_sb_info * fsbi, unsigned int fat_entry)
{
	unsigned int buffer[128];  //128 * 4 = 512
    //512/4 = 128个fat项,得到fat表项所在扇区
	Buf *buf = bread(1, fsbi->FAT1_firstsector + (fat_entry >> 7));
	memcpy(buffer, buf->data, BSIZE);
	//& 0x7f相当于对128取余，得到fat表项在本扇区内偏移量
	//printk("read_FAT_Entry fat_entry:%#018lx,%#010x\n",fat_entry,buffer[fat_entry & 0x7f]);
    brelease(buf);
	return buffer[fat_entry & 0x7f] & 0x0fffffff;  //FAT表项的值只占用低28位，高4位是保留位，不用于存储数据
}

//used for debug
unsigned int read_FAT_Entry_test(unsigned int fat_entry)
{
    //512/4 = 128个fat项,得到fat表项所在扇区
	Buf *buf = bread(1, FirstFAT1Sector + (fat_entry >> 7));
	printk("read_FAT_Entry fat_entry:%#018lx,%#010x\n",fat_entry,buf->data[fat_entry & 0x7f]);
    unsigned int result = (unsigned int)buf->data[fat_entry & 0x7f] & 0x0fffffff; //& 0x7f相当于对128取余，得到fat表项在本扇区内偏移量
    //FAT表项的值只占用低28位，高4位是保留位，不用于存储数据
    brelease(buf);
	return result;
}

//由fat表项编号来写入fat表项值
unsigned long write_FAT_Entry(struct FAT32_sb_info * fsbi,unsigned int fat_entry,unsigned int value)
{
	int i = 0;
	unsigned int buf[128];
	Buf *buffer = bread(1, fsbi->FAT1_firstsector + (fat_entry >> 7));
	if(buf == NULL)
	{
		printk("read FAT failed!\n");
		return 1;
	}
	memcpy(buf, buffer->data, BSIZE);
	buf[fat_entry & 0x7f] = (buf[fat_entry & 0x7f] & 0xf0000000) | (value & 0x0fffffff);
	memcpy(buffer->data, buf, BSIZE);
	for(i = 0;i < fsbi->NumFATs;i++)
	{
		buffer->blockno = fsbi->FAT1_firstsector + fsbi->sector_per_FAT * i + (fat_entry >> 7);
		bwrite(buffer);
	}
	brelease(buffer);
    return 0;	
}

//used for debug
unsigned long write_FAT_Entry_test(unsigned int fat_entry,unsigned int value)
{
    int pos = fat_entry >> 7;
	Buf *buf = bread(1, FirstFAT1Sector + pos);
    //由于data是unsigned char类型，写入要将unsigned int处理成char
    unsigned int res = (((unsigned int)buf->data[fat_entry & 0x7f] & 0xf0000000) | (value & 0x0fffffff));
	buf->data[pos] = (unsigned char)(res & 0x000000ff);
    buf->data[pos + 1] = (unsigned char)((res & 0x0000ff00) >> 8);
    buf->data[pos + 2] = (unsigned char)((res & 0x00ff0000) >> 16);
    buf->data[pos + 3] = (unsigned char)((res & 0xff000000) >> 24);
	buf->blockno = FirstFAT1Sector + (fat_entry >> 7);
    bwrite(buf);
    buf->blockno = FirstFAT2Sector + (fat_entry >> 7);
    bwrite(buf);
    brelease(buf);
    return 1;	
}

/*
如果一个簇有多个扇区，通过该函数读取簇，返回一个存放数据的缓冲区,失败返回NULL
数据量要小于一页
block: 起始扇区
count: 要读取的扇区数量
size:  传出参数，返回缓冲区的大小
*/
unsigned char* read_more_sector(unsigned int block, unsigned int count, unsigned int *size)
{
	Buf *buffer = NULL;
	unsigned char * buf = alloc_pgtable();
	unsigned int count_sector = 0;
	unsigned int buflen = 0;
	//read enough sector,save in buf
	for(count_sector = 0; count_sector < count; count_sector++)
	{
		buffer = bread(1, block + count_sector);
		if(buffer == NULL)
		{
			page_free_addr(buf);
			return NULL;
		}
		else
		{
			memcpy(buf + buflen, buffer->data, BSIZE);
			buflen += BSIZE;
			brelease(buffer);
		}
	}
	*size = buflen;
	return buf;
}

/*
如果一个簇有多个扇区，通过该函数写入多个扇区，返回成功写入的字节数
block: 起始扇区
count: 扇区数量
buf:   存放数据的缓冲区
失败返回0
*/
unsigned int write_more_sector(unsigned char* buf, unsigned int block, unsigned int count)
{
	Buf *buffer = NULL;
	int i = 0;
	unsigned int count_sector = 0;
	unsigned int buflen = 0;
	if(buf == NULL)
	{
		printk("buf is NULL!!!\n");
		return 0;
	}
	for(; count_sector < count; count_sector++)
	{
		buffer = bget(1, block + count_sector);
		if(buffer == NULL)
		{
			return 0;
		}
		memcpy(buffer->data, buf + buflen, BSIZE);
		buflen += BSIZE;
		bwrite(buffer);
		brelease(buffer);
	}
	return buflen;
}

long FAT32_open(struct index_node * inode,struct file * filp)
{
	return 1;
}


long FAT32_close(struct index_node * inode,struct file * filp)
{
	return 1;
}


long FAT32_read(struct file * filp,char * buf,unsigned long count,long * position)
{
	struct FAT32_inode_info * finode = filp->dentry->dir_inode->private_index_info;
	struct FAT32_sb_info * fsbi = filp->dentry->dir_inode->sb->private_sb_info;

	unsigned int read_size = 0;
	unsigned long cluster = finode->first_cluster;
	unsigned long sector = 0;
	int i,length = 0;
	long retval = 0;
	int index = *position / fsbi->bytes_per_cluster;	//位置所在簇
	long offset = *position % fsbi->bytes_per_cluster;	//位置所在簇内偏移
	unsigned char * buffer = NULL;

	if(!cluster)
		return -EFAULT;
	/*for(i = 0;i < index;i++)	//取得对应簇号
	{
		if(cache_valid && i > cache_index && cache[i%BSIZE] ) // 如果缓存有效且索引在缓存范围内，直接从缓存中获取簇号
			cluster = cache[i%BSIZE];
		else // 否则，调用read_FAT_Entry函数，并将结果存入缓存中
		{
			cluster = read_FAT_Entry(fsbi,cluster);
			if(cache_index < CACHE_SIZE) // 如果缓存未满，将簇号添加到缓存末尾
			cache[cache_index++] = cluster;
			else // 如果缓存已满，将整个缓存向前移动一位，丢弃最旧的簇号，将新的簇号添加到缓存末尾
			{
			memmove(cache, cache + 1, (CACHE_SIZE - 1) * sizeof(unsigned long));
			cache[CACHE_SIZE - 1] = cluster;
			}
			cache_valid = 1; // 设置缓存有效标志
 		}
	}*/
	for(i = 0;i < index;i++)
	cluster = read_FAT_Entry(fsbi,cluster);

	//index更新为指向读取结束的位置
	if(*position + count > filp->dentry->dir_inode->file_size)
		index = count = filp->dentry->dir_inode->file_size - *position;
	else
		index = count;

	printk("FAT32_read first_cluster:%d,size:%d,preempt_count:%d\n",finode->first_cluster,filp->dentry->dir_inode->file_size,get_current_task()->count);

	do
	{
		sector = fsbi->Data_firstsector + (cluster - 2) * fsbi->sector_per_cluster;
		buffer = read_more_sector(sector, fsbi->sector_per_cluster, &read_size);
		if(!buffer)
		{
			printk("FAT32 FS(read) read disk ERROR!!!!!!!!!!\n");
			retval = -EIO;
			break;
		}
		//长度最多只有fsbi->bytes_per_cluster - offset
		length = index <= fsbi->bytes_per_cluster - offset ? index : fsbi->bytes_per_cluster - offset;
		memcpy(buf, buffer + offset, length);
		index -= length;
		buf += length;
		offset -= offset;
		*position += length;
		//index不为0说明跨越了簇号
	}while(index && (cluster = read_FAT_Entry(fsbi,cluster)));

	page_free_addr(buffer);
	if(!index)
		retval = count;
	return retval;
}

//获取一个空闲的簇，返回其在fat中的表项
unsigned long FAT32_find_available_cluster(struct FAT32_sb_info * fsbi)
{
	int i = 0, j = 0;
	unsigned int *buf;
	int size = 0;
	for(int i = 0; i < fsbi->sector_per_FAT; i++)
	{
		buf = (unsigned int *)read_more_sector(fsbi->FAT1_firstsector + i, 1, &size);
		for(j = 0;j < 128;j++)
		{
			if((buf[j] & 0x0fffffff) == 0)
				return (i << 7) + j;
		}
	}
}

long FAT32_write(struct file * filp,char * buf,unsigned long count,long * position)
{
	struct FAT32_inode_info * finode = filp->dentry->dir_inode->private_index_info;
	struct FAT32_sb_info * fsbi = filp->dentry->dir_inode->sb->private_sb_info;

	unsigned long cluster = finode->first_cluster;
	unsigned long next_cluster = 0;
	unsigned long sector = 0;
	int i,length = 0;
	long retval = 0;
	long flags = 0;
	int index = *position / fsbi->bytes_per_cluster;
	long offset = *position % fsbi->bytes_per_cluster;
	unsigned char * buffer = NULL;
	unsigned int bufsize = 0;

	if(!cluster)
	{
		cluster = FAT32_find_available_cluster(fsbi);
		flags = 1;
	}
	else
		for(i = 0;i < index;i++)
			cluster = read_FAT_Entry(fsbi,cluster);

	if(!cluster)
	{
		return -ENOSPC;
	}

	if(flags)
	{
		//创建文件
		finode->first_cluster = cluster;
		filp->dentry->dir_inode->sb->sb_ops->write_inode(filp->dentry->dir_inode);
		write_FAT_Entry(fsbi,cluster,0x0ffffff8);
	}

	index = count;

	do
	{
		if(!flags)
		{
			sector = fsbi->Data_firstsector + (cluster - 2) * fsbi->sector_per_cluster;
			buffer = read_more_sector(sector, fsbi->sector_per_cluster, &bufsize);
			if(!buffer)
			{
				retval = -EIO;
				break;
			}
		}

		length = index <= fsbi->bytes_per_cluster - offset ? index : fsbi->bytes_per_cluster - offset;
		memcpy(buffer + offset, buf, length);
		if(!write_more_sector(buffer, sector, fsbi->sector_per_cluster))
		{
			printk("FAT32 FS(write) write disk ERROR!!!!!!!!!!\n");
			retval = -EIO;
			break;
		}

		index -= length;
		buf += length;
		offset -= offset;
		*position += length;

		if(index)
			next_cluster = read_FAT_Entry(fsbi,cluster);
		else
			break;

		if(next_cluster >= 0x0ffffff8)
		{
			next_cluster = FAT32_find_available_cluster(fsbi);
			if(!next_cluster)
			{
				page_free_addr(buffer);
				return -ENOSPC;
			}			
				
			write_FAT_Entry(fsbi,cluster,next_cluster);
			write_FAT_Entry(fsbi,next_cluster,0x0ffffff8);
			cluster = next_cluster;
			flags = 1;
		}

	}while(index);

	if(*position > filp->dentry->dir_inode->file_size)
	{
		filp->dentry->dir_inode->file_size = *position;
		filp->dentry->dir_inode->sb->sb_ops->write_inode(filp->dentry->dir_inode);
	}

	page_free_addr(buffer);
	if(!index)
		retval = count;
	return retval;
}


long FAT32_lseek(struct file * filp,long offset,long origin)
{
	struct index_node *inode = filp->dentry->dir_inode;
	long pos = 0;
	switch (origin)
	{
	case SEEK_SET:
		pos = offset;
		break;
	case SEEK_CUR:
		pos = filp->position + offset;
		break;
	case SEEK_END:
		pos = filp->dentry->dir_inode->file_size + offset;
		break;
	default:
		return -EINVAL;
		break;
	}
	if(pos < 0 || pos > filp->dentry->dir_inode->file_size)
	{
		return -EOVERFLOW;
	}
	filp->position = pos;
	printk("FAT32 lseek alert position:%d\n", filp->position);
	return pos;
}


long FAT32_ioctl(struct index_node * inode,struct file * filp,unsigned long cmd,unsigned long arg)
{}


struct file_operations FAT32_file_ops = 
{
	.open = FAT32_open,
	.close = FAT32_close,
	.read = FAT32_read,
	.write = FAT32_write,
	.lseek = FAT32_lseek,
	.ioctl = FAT32_ioctl,
};

/*
类似lookup_test
从父目录中搜索子目录项,若搜索成功,会对子目录项进行初始化,若失败,返回NULL
parent_inode:父目录的index_node结构
dest_dentry:要搜索的子目录项
*/
struct dir_entry * FAT32_lookup(struct index_node * parent_inode,struct dir_entry * dest_dentry)
{
	struct FAT32_inode_info * finode = parent_inode->private_index_info;
	struct FAT32_sb_info * fsbi = parent_inode->sb->private_sb_info;
	unsigned int buflen = 0;
	unsigned int cluster = 0;
	unsigned long sector = 0;
	Buf *buffer = NULL;
	int i = 0,j = 0,x = 0;
	struct FAT32_Directory * tmpdentry = NULL;
	struct FAT32_LongDirectory * tmpldentry = NULL;
	struct index_node * p = NULL;
	unsigned char * buf = NULL;		//use size:fsbi->bytes_per_cluster
	
	cluster = finode->first_cluster;

next_cluster:
	sector = fsbi->Data_firstsector + (cluster - 2) * fsbi->sector_per_cluster;
	printk("lookup cluster:%#010x,sector:%#018lx\n",cluster,sector);
	
	buf = read_more_sector(sector, fsbi->sector_per_cluster, &buflen);
	if(buf == NULL)
	{
		printk("FAT32 FS(lookup) read disk ERROR!!!!!!!!!!\n");
		return NULL;
	}

	tmpdentry = (struct FAT32_Directory *)buf;

	for(i = 0;i < fsbi->bytes_per_cluster;i+= 32,tmpdentry++)
	{
		if(tmpdentry->DIR_Attr == ATTR_LONG_NAME)
			continue;
		if(tmpdentry->DIR_Name[0] == 0xe5 || tmpdentry->DIR_Name[0] == 0x00 || tmpdentry->DIR_Name[0] == 0x05)
			continue;

		tmpldentry = (struct FAT32_LongDirectory *)tmpdentry-1;
		j = 0;

		//long file/dir name compare
		while(tmpldentry->LDIR_Attr == ATTR_LONG_NAME && tmpldentry->LDIR_Ord != 0xe5)
		{
			for(x=0;x<5;x++)
			{
				if(j>dest_dentry->name_length && tmpldentry->LDIR_Name1[x] == 0xffff)
					continue;
				else if(j>dest_dentry->name_length || tmpldentry->LDIR_Name1[x] != (unsigned short)(dest_dentry->name[j++]))
					goto continue_cmp_fail;
			}
			for(x=0;x<6;x++)
			{
				if(j>dest_dentry->name_length && tmpldentry->LDIR_Name2[x] == 0xffff)
					continue;
				else if(j>dest_dentry->name_length || tmpldentry->LDIR_Name2[x] != (unsigned short)(dest_dentry->name[j++]))
					goto continue_cmp_fail;
			}
			for(x=0;x<2;x++)
			{
				if(j>dest_dentry->name_length && tmpldentry->LDIR_Name3[x] == 0xffff)
					continue;
				else if(j>dest_dentry->name_length || tmpldentry->LDIR_Name3[x] != (unsigned short)(dest_dentry->name[j++]))
					goto continue_cmp_fail;
			}

			if(j >= dest_dentry->name_length)
			{
				goto find_lookup_success;
			}

			tmpldentry --;
		}

		//short file/dir base name compare
		j = 0;
		for(x=0;x<8;x++)
		{
			switch(tmpdentry->DIR_Name[x])
			{
				case ' ':
					if(!(tmpdentry->DIR_Attr & ATTR_DIRECTORY))
					{
						if(dest_dentry->name[j]=='.')
							continue;
						else if(tmpdentry->DIR_Name[x] == dest_dentry->name[j])
						{
							j++;
							break;
						}
						else
							goto continue_cmp_fail;
					}
					else
					{
						if(j < dest_dentry->name_length && tmpdentry->DIR_Name[x] == dest_dentry->name[j])
						{
							j++;
							break;
						}
						else if(j == dest_dentry->name_length)
							continue;
						else
							goto continue_cmp_fail;
					}

				case 'A' ... 'Z':
				case 'a' ... 'z':
					if(tmpdentry->DIR_NTRes & LOWERCASE_BASE)
						if(j < dest_dentry->name_length && tmpdentry->DIR_Name[x] + 32 == dest_dentry->name[j])
						{
							j++;
							break;
						}
						else
							goto continue_cmp_fail;
					else
					{
						if(j < dest_dentry->name_length && tmpdentry->DIR_Name[x] == dest_dentry->name[j])
						{
							j++;
							break;
						}
						else
							goto continue_cmp_fail;
					}

				case '0' ... '9':
					if(j < dest_dentry->name_length && tmpdentry->DIR_Name[x] == dest_dentry->name[j])
					{
						j++;
						break;
					}
					else
						goto continue_cmp_fail;

				default :
					j++;
					break;
			}
		}
		//short file ext name compare
		if(!(tmpdentry->DIR_Attr & ATTR_DIRECTORY))
		{
			j++;
			for(x=8;x<11;x++)
			{
				switch(tmpdentry->DIR_Name[x])
				{
					case 'A' ... 'Z':
					case 'a' ... 'z':
						if(tmpdentry->DIR_NTRes & LOWERCASE_EXT)
							if(tmpdentry->DIR_Name[x] + 32 == dest_dentry->name[j])
							{
								j++;
								break;
							}
							else
								goto continue_cmp_fail;
						else
						{
							if(tmpdentry->DIR_Name[x] == dest_dentry->name[j])
							{
								j++;
								break;
							}
							else
								goto continue_cmp_fail;
						}

					case '0' ... '9':
						if(tmpdentry->DIR_Name[x] == dest_dentry->name[j])
						{
							j++;
							break;
						}
						else
							goto continue_cmp_fail;

					case ' ':
						if(tmpdentry->DIR_Name[x] == dest_dentry->name[j])
						{
							j++;
							break;
						}
						else
							goto continue_cmp_fail;

					default :
						goto continue_cmp_fail;
				}
			}
		}
		goto find_lookup_success;

continue_cmp_fail:;
	}
	
	cluster = read_FAT_Entry(fsbi,cluster);
	if(cluster < 0x0ffffff7)
		goto next_cluster;

	page_free_addr(buf);
	return NULL;

find_lookup_success:
	p = (struct index_node *)alloc_pgtable();	//size:sizeof(struct index_node)
	memset(p,0,sizeof(struct index_node));
	p->file_size = tmpdentry->DIR_FileSize;
	p->blocks = (p->file_size + fsbi->bytes_per_cluster - 1)/fsbi->bytes_per_sector;
	p->attribute = (tmpdentry->DIR_Attr & ATTR_DIRECTORY) ? FS_ATTR_DIR : FS_ATTR_FILE;
	p->sb = parent_inode->sb;
	p->f_ops = &FAT32_file_ops;
	p->inode_ops = &FAT32_inode_ops;

	p->private_index_info = (struct FAT32_inode_info *)alloc_pgtable();	//size:sizeof(struct FAT32_inode_info)
	memset(p->private_index_info,0,sizeof(struct FAT32_inode_info));
	finode = p->private_index_info;

	finode->first_cluster = (tmpdentry->DIR_FstClusHI<< 16 | tmpdentry->DIR_FstClusLO) & 0x0fffffff;
	finode->dentry_location = cluster;
	finode->dentry_position = tmpdentry - (struct FAT32_Directory *)buf;
	finode->create_date = tmpdentry->DIR_CrtTime;
	finode->create_time = tmpdentry->DIR_CrtDate;
	finode->write_date = tmpdentry->DIR_WrtTime;
	finode->write_time = tmpdentry->DIR_WrtDate;

	dest_dentry->dir_inode = p;
	page_free_addr(buf);
	return dest_dentry;	
}

long FAT32_create(struct index_node * inode,struct dir_entry * dentry,int mode)
{}

long FAT32_mkdir(struct index_node * inode,struct dir_entry * dentry,int mode)
{}


long FAT32_rmdir(struct index_node * inode,struct dir_entry * dentry)
{}

long FAT32_rename(struct index_node * old_inode,struct dir_entry * old_dentry,struct index_node * new_inode,struct dir_entry * new_dentry)
{}

long FAT32_getattr(struct dir_entry * dentry,unsigned long * attr)
{}

long FAT32_setattr(struct dir_entry * dentry,unsigned long * attr)
{}

struct index_node_operations FAT32_inode_ops = 
{
	.create = FAT32_create,
	.lookup = FAT32_lookup,
	.mkdir = FAT32_mkdir,
	.rmdir = FAT32_rmdir,
	.rename = FAT32_rename,
	.getattr = FAT32_getattr,
	.setattr = FAT32_setattr,
};



/*
used for debug
从指定目录搜索与目标名name匹配的目录项，返回一个短目录项
name: 目标名
namelen: 目标名长度
dentry: 指定目录项
flags: 标记(暂时不用)
*/
struct FAT32_Directory * lookup_test(char * name,int namelen,struct FAT32_Directory *dentry,int flags)
{
	unsigned int cluster = 0;
	unsigned long sector = 0;
	int i = 0,j = 0,x = 0;
	struct FAT32_Directory *tmpdentry = NULL;
	struct FAT32_LongDirectory *tmpldentry = NULL;
	struct FAT32_Directory *p = NULL;
    Buf *buf = NULL;

	//获取根目录起始簇号
	cluster = (dentry->DIR_FstClusHI << 16 | dentry->DIR_FstClusLO) & 0x0fffffff;
next_cluster:
    //由簇号获得扇区号
	sector = FirstDataSector + (cluster - 2) * fat32_boot.BPB_SecPerClus;
	printk("lookup cluster:%#010x,sector:%#018lx\n",cluster,sector);
    buf = bread(1, sector);
	if(!buf)
	{
		printk("FAT32 FS(lookup) read disk ERROR!!!!!!!!!!\n");
		return NULL;
	}

	tmpdentry = (struct FAT32_Directory *)buf->data;

    
	for(i = 0;i < BytesPerClus;i+= 32,tmpdentry++)
	{
        //匹配长目录项
		if(tmpdentry->DIR_Attr == ATTR_LONG_NAME)
			continue;
		if(tmpdentry->DIR_Name[0] == 0xe5 || tmpdentry->DIR_Name[0] == 0x00 || tmpdentry->DIR_Name[0] == 0x05)
			continue;

		tmpldentry = (struct FAT32_LongDirectory *)tmpdentry-1;
		j = 0;

		//long file/dir name compare
		while(tmpldentry->LDIR_Attr == ATTR_LONG_NAME && tmpldentry->LDIR_Ord != 0xe5)
		{
			for(x=0;x<5;x++)
			{
				if(j>namelen && tmpldentry->LDIR_Name1[x] == 0xffff)
					continue;
				else if(j>namelen || tmpldentry->LDIR_Name1[x] != (unsigned short)(name[j++]))
					goto continue_cmp_fail;
			}
			for(x=0;x<6;x++)
			{
				if(j>namelen && tmpldentry->LDIR_Name2[x] == 0xffff)
					continue;
				else if(j>namelen || tmpldentry->LDIR_Name2[x] != (unsigned short)(name[j++]))
					goto continue_cmp_fail;
			}
			for(x=0;x<2;x++)
			{
				if(j>namelen && tmpldentry->LDIR_Name3[x] == 0xffff)
					continue;
				else if(j>namelen || tmpldentry->LDIR_Name3[x] != (unsigned short)(name[j++]))
					goto continue_cmp_fail;
			}

			if(j>=namelen)
			{
				p = alloc_pgtable();
                if(p == NULL)
                {
                    printk("ALLOC FAIL!!!\n");
                    return NULL;
                }
				*p = *tmpdentry;
				brelease(buf);
				return p;
			}

			tmpldentry --;
		}

		//short file/dir base name compare
		j = 0;
		for(x=0;x<8;x++)
		{
			switch(tmpdentry->DIR_Name[x])
			{
				case ' ':
					if(!(tmpdentry->DIR_Attr & ATTR_DIRECTORY))
					{
						if(name[j]=='.')	//空格前有.代表空格在扩展名
							continue;
						else if(tmpdentry->DIR_Name[x] == name[j])
						{
							//name可能有空格
							j++;
							break;
						}
						else
							goto continue_cmp_fail;
					}
					else
					{
						if(j < namelen && tmpdentry->DIR_Name[x] == name[j])
						{
							j++;
							break;
						}
						else if(j == namelen)
							continue;
						else
							goto continue_cmp_fail;
					}

				case 'A' ... 'Z':
				case 'a' ... 'z':
					if(tmpdentry->DIR_NTRes & LOWERCASE_BASE)
						if(j < namelen && tmpdentry->DIR_Name[x] + 32 == name[j])	//+32转小写
						{			
							j++;
							break;
						}
						else
							goto continue_cmp_fail;
					else
					{
						if(j < namelen && tmpdentry->DIR_Name[x] == name[j])
						{
							j++;
							break;
						}
						else
							goto continue_cmp_fail;
					}

				case '0' ... '9':
					if(j < namelen && tmpdentry->DIR_Name[x] == name[j])
					{
						j++;
						break;
					}
					else
						goto continue_cmp_fail;

				default :
					j++;
					break;
			}
		}
		//short file ext name compare
		if(!(tmpdentry->DIR_Attr & ATTR_DIRECTORY))
		{
			j++;
			for(x=8;x<11;x++)
			{
				switch(tmpdentry->DIR_Name[x])
				{
					case 'A' ... 'Z':
					case 'a' ... 'z':
						if(tmpdentry->DIR_NTRes & LOWERCASE_EXT)
							if(tmpdentry->DIR_Name[x] + 32 == name[j])	//+32转小写
							{
								j++;
								break;
							}
							else
								goto continue_cmp_fail;
						else
						{
							if(tmpdentry->DIR_Name[x] == name[j])
							{
								j++;
								break;
							}
							else
								goto continue_cmp_fail;
						}

					case '0' ... '9':
						if(tmpdentry->DIR_Name[x] == name[j])
						{
							j++;
							break;
						}
						else
							goto continue_cmp_fail;

					case ' ':
						if(tmpdentry->DIR_Name[x] == name[j])
						{
							j++;
							break;
						}
						else
							goto continue_cmp_fail;

					default :
						goto continue_cmp_fail;
				}
			}
		}
		p = alloc_pgtable();
		*p = *tmpdentry;
		brelease(buf);
		return p;

continue_cmp_fail:;
	}
	//失败则搜索下一个簇号
	cluster = read_FAT_Entry_test(cluster);
	if(cluster < 0x0ffffff7)	//0x0ffffff7是坏簇,xFFFFFF8H-XFFFFFFF为文件最后一个簇
		goto next_cluster;

	brelease(buf);
	return NULL;
}


//搜索指定目录下文件
//name:文件目录   flag:标识符
//flag=1返回父目录项 flag=0返回目标目录项
struct dir_entry * path_walk(char * name,unsigned long flags)
{
	char * tmpname = NULL;
	int tmpnamelen = 0;
	struct dir_entry * parent = root_sb->root;
	struct dir_entry * path = NULL;

	while(*name == '/')
		name++;

	if(!*name)
	{
		return parent;
	}

	for(;;)
	{
		tmpname = name;
		while(*name && (*name != '/'))
			name++;
		tmpnamelen = name - tmpname;

		path = (struct dir_entry *)alloc_pgtable();
		if(path == 1)
		{
			return NULL;
		}

		path->name = (char *)alloc_pgtable();		//length = tmpnamelen + 1
		memcpy(path->name,tmpname,tmpnamelen);
		path->name_length = tmpnamelen;

		if(parent->dir_inode->inode_ops->lookup(parent->dir_inode,path) == NULL)
		{
			printk("can not find file or dir:%s\n",path->name);
			page_free_addr(path->name);
			page_free_addr(path);
			return NULL;
		}

		list_init(&path->child_node);
		list_init(&path->subdirs_list);
		path->parent = parent;
		list_add_tail(&path->child_node,&parent->subdirs_list);

		if(!*name)
			goto last_component;
		while(*name == '/')
			name++;
		if(!*name)
			goto last_slash;

		parent = path;
	}

last_slash:
last_component:

	if(flags & 1)
	{
		return parent;
	}

	return path;
}

struct FAT32_Directory * path_walk_test(char * name,unsigned long flags)
{
    char * tmpname = NULL;
	int tmpnamelen = 0;
	struct FAT32_Directory *parent = NULL;
	struct FAT32_Directory *path = NULL;
	char * dentryname = NULL;

	while(*name == '/')
		name++;

	if(!*name)
		return NULL;

	parent = (struct FAT32_Directory *)page_alloc();
	dentryname = page_alloc();
	memset(parent,0,sizeof(struct FAT32_Directory));
	memset(dentryname,0,PAGE_SIZE);
	parent->DIR_FstClusLO = fat32_boot.BPB_RootClus & 0xffff;
	parent->DIR_FstClusHI = (fat32_boot.BPB_RootClus >> 16) & 0x0fff;

	while(1)
	{
		tmpname = name;
		while(*name && (*name != '/'))
			name++;
		tmpnamelen = name - tmpname;
		memcpy(dentryname, tmpname, tmpnamelen);
		dentryname[tmpnamelen] = '\0';

		path = lookup_test(dentryname,tmpnamelen,parent,flags);
		if(path == NULL)
		{
			printk("can not find file or dir:%s\n",dentryname);
            page_free_addr(dentryname);
			page_free_addr(parent);
			return NULL;
		}

		if(!*name)
			goto last_component;
		while(*name == '/')
			name++;
		if(!*name)
			goto last_slash;

		*parent = *path;
		page_free_addr(path);
	}

last_slash:
last_component:
	if(flags & 1)
	{
		page_free_addr(dentryname);
		page_free_addr(path);
		return parent;
	}

	page_free_addr(dentryname);
	page_free_addr(parent);
	return path;

}

//建立superblock结构，buf是引导扇区数据
struct super_block * fat32_read_superblock(struct Disk_Partition_Table_Entry * DPTE,void * buf)
{
	struct super_block * sbp = NULL;
	struct FAT32_inode_info * finode = NULL;
	struct FAT32_BootSector * fbs = NULL;
	struct FAT32_sb_info * fsbi = NULL;
	Buf *buffer = NULL;

	////super block
	sbp = (struct super_block *)alloc_pgtable();
	if(sbp == 1)
	{
		return NULL;
	}

	sbp->sb_ops = &FAT32_sb_ops;
	sbp->private_sb_info = (struct FAT32_sb_info *)alloc_pgtable();
	if(sbp->private_sb_info == 1)
	{
		page_free_addr(sbp);
		return NULL;
	}

	////fat32 boot sector
	fbs = (struct FAT32_BootSector *)buf;
	fsbi = sbp->private_sb_info;	
	fsbi->start_sector = DPTE->start_LBA;
	fsbi->sector_count = DPTE->sectors_limit;
	fsbi->sector_per_cluster = fbs->BPB_SecPerClus;
	fsbi->bytes_per_cluster = fbs->BPB_SecPerClus * fbs->BPB_BytesPerSec;
	fsbi->bytes_per_sector = fbs->BPB_BytesPerSec;
	fsbi->Data_firstsector = DPTE->start_LBA + fbs->BPB_RsvdSecCnt + fbs->BPB_FATSz32 * fbs->BPB_NumFATs;
	fsbi->FAT1_firstsector = DPTE->start_LBA + fbs->BPB_RsvdSecCnt;
	fsbi->sector_per_FAT = fbs->BPB_FATSz32;
	fsbi->NumFATs = fbs->BPB_NumFATs;
	fsbi->fsinfo_sector_infat = fbs->BPB_FSInfo;
	fsbi->bootsector_bk_infat = fbs->BPB_BkBootSec;			//引导扇区备份
	
	printk("FAT32 Boot Sector\n\tBPB_FSInfo:%#018lx\n\tBPB_BkBootSec:%#018lx\n\tBPB_TotSec32:%#018lx\n",fbs->BPB_FSInfo,fbs->BPB_BkBootSec,fbs->BPB_TotSec32);
	
	////fat32 fsinfo sector
	
	fsbi->fat_fsinfo = (struct FAT32_FSInfo *)alloc_pgtable();
	if(fsbi->fat_fsinfo == 1)
	{
		page_free_addr(sbp);
		page_free_addr(sbp->private_sb_info);
		return NULL;
	}
	buffer = bread(1, DPTE->start_LBA + fbs->BPB_FSInfo);
	if(buffer == NULL)
	{
		printk("buffer read error!\n");
		page_free_addr(sbp);
		page_free_addr(sbp->private_sb_info);
		return NULL;
	}
	memcpy(fsbi->fat_fsinfo, buffer->data, BSIZE);
	brelease(buffer);

	printk("FAT32 FSInfo\n\tFSI_LeadSig:%#018lx\n\tFSI_StrucSig:%#018lx\n\tFSI_Free_Count:%#018lx\n",fsbi->fat_fsinfo->FSI_LeadSig,fsbi->fat_fsinfo->FSI_StrucSig,fsbi->fat_fsinfo->FSI_Free_Count);
	
	////directory entry
	sbp->root = (struct dir_entry *)alloc_pgtable();

	list_init(&sbp->root->child_node);
	list_init(&sbp->root->subdirs_list);
	sbp->root->parent = sbp->root;
	sbp->root->dir_ops = &FAT32_dentry_ops;
	sbp->root->name = (char *)alloc_pgtable();
	sbp->root->name[0] = '/'; 
	sbp->root->name_length = 1;

	////index node
	sbp->root->dir_inode = (struct index_node *)((char *)sbp->root->name + 8);		//continue use the page space
	sbp->root->dir_inode->inode_ops = &FAT32_inode_ops;
	sbp->root->dir_inode->f_ops = &FAT32_file_ops;
	sbp->root->dir_inode->file_size = 0;
	sbp->root->dir_inode->blocks = (sbp->root->dir_inode->file_size + fsbi->bytes_per_cluster - 1)/fsbi->bytes_per_sector;
	sbp->root->dir_inode->attribute = FS_ATTR_DIR;
	sbp->root->dir_inode->sb = sbp;

	////fat32 root inode
	sbp->root->dir_inode->private_index_info = (struct FAT32_inode_info *)alloc_pgtable();
	//memset(sbp->root->dir_inode->private_index_info,0,sizeof(struct FAT32_inode_info));
	finode = (struct FAT32_inode_info *)sbp->root->dir_inode->private_index_info;
	finode->first_cluster = fbs->BPB_RootClus;
	finode->dentry_location = 0;
	finode->dentry_position = 0; 
	finode->create_date = 0;
	finode->create_time = 0;
	finode->write_date = 0;
	finode->write_time = 0;

	return sbp;
}

void fat32_write_superblock(struct super_block * sb)
{

}

//释放superblock
void fat32_put_superblock(struct super_block * sb)
{
	page_free_addr(sb->private_sb_info);
	page_free_addr(sb->root->dir_inode->private_index_info);
	page_free_addr(sb->root->name);
	page_free_addr(sb->root->dir_inode);
	page_free_addr(sb->root);
	page_free_addr(sb);
}

//写回修改后的inode
void fat32_write_inode(struct index_node * inode)
{
	struct FAT32_Directory * fdentry = NULL;
	struct FAT32_Directory * buf = NULL;
	struct FAT32_inode_info * finode = inode->private_index_info;
	struct FAT32_sb_info * fsbi = inode->sb->private_sb_info;
	unsigned int buflen = 0;
	unsigned long sector = 0;

	if(finode->dentry_location == 0)
	{
		//is root
		printk("FS ERROR:write root inode!\n");	
		return ;
	}

	sector = fsbi->Data_firstsector + (finode->dentry_location - 2) * fsbi->sector_per_cluster;
	buf = (struct FAT32_Directory *)read_more_sector(sector, fsbi->sector_per_cluster, &buflen);
	fdentry = buf+finode->dentry_position;

	//update dentry info
	fdentry->DIR_FileSize = inode->file_size;
	fdentry->DIR_FstClusLO = finode->first_cluster & 0xffff;
	fdentry->DIR_FstClusHI = (fdentry->DIR_FstClusHI & 0xf000) | (finode->first_cluster >> 16);

	write_more_sector((unsigned char *)buf, sector, fsbi->sector_per_cluster);
	page_free_addr(buf);
}

struct super_block_operations FAT32_sb_ops = 
{
	.write_superblock = fat32_write_superblock,
	.put_superblock = fat32_put_superblock,

	.write_inode = fat32_write_inode,
};

//// these operation need cache and list
long FAT32_compare(struct dir_entry * parent_dentry,char * source_filename,char * destination_filename){}
long FAT32_hash(struct dir_entry * dentry,char * filename){}
long FAT32_release(struct dir_entry * dentry){}
long FAT32_iput(struct dir_entry * dentry,struct index_node * inode){}


struct dir_entry_operations FAT32_dentry_ops = 
{
	.compare = FAT32_compare,
	.hash = FAT32_hash,
	.release = FAT32_release,
	.iput = FAT32_iput,
};


struct file_system_type FAT32_fs_type=
{
	.name = "FAT32",
	.fs_flags = 0,
	.read_superblock = fat32_read_superblock,
	.next = NULL,
};

void FAT32_init()
{
	int i;
	Buf *buf = NULL;
	struct dir_entry * dentry = NULL;
	struct Disk_Partition_Table DPT = {0};
	memset(&DPT, 0, sizeof(DPT));
	memset(cache, 0, sizeof(unsigned long) * BSIZE);
	register_filesystem(&FAT32_fs_type);

	#ifdef QEMU
	buf = bread(1, 0);
	DPT = *(struct Disk_Partition_Table *)buf->data;
	printk("DPTE[0] start_LBA:%#018lx\ttype:%#018lx\n",DPT.DPTE[0].start_LBA,DPT.DPTE[0].type);
	brelease(buf);
	#endif
	
	buf = bread(1, DPT.DPTE[0].start_LBA);
	root_sb = mount_fs("FAT32",&DPT.DPTE[0],(void *)buf->data);

	/*dentry = path_walk("/aabbcc",0);
	if(dentry != NULL)
		printk("\nFind /busybox\nDIR_FirstCluster:%#018lx\tDIR_FileSize:%#018lx\n",((struct FAT32_inode_info *)(dentry->dir_inode->private_index_info))->first_cluster,dentry->dir_inode->file_size);
	else
		printk("Can`t find file\n");*/
}

//used for debug
void fat32_init_test()
{
    Buf *buf = NULL;
    memset(&mbr, 0, sizeof(mbr));
    memset(&fat32_boot, 0, sizeof(fat32_boot));
    memset(&fsinfo, 0, sizeof(fsinfo));

    #ifdef QEMU
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

    printk("FirstDataSector = 0x%x\tFirstFAT1Sector = 0x%x\nFirstFAT2Sector = 0x%x\tBytesPerClus = 0x%x\n",FirstDataSector, FirstFAT1Sector, FirstFAT2Sector, BytesPerClus);
    
    struct FAT32_Directory * dentry = NULL;
    dentry = path_walk("/busybox",0);
	if(dentry != NULL)
		printk("\nFind /busybox\nDIR_FstClusHI:%#018lx\tDIR_FstClusLO:%#018lx\tDIR_FileSize:%#018lx\n",dentry->DIR_FstClusHI,dentry->DIR_FstClusLO,dentry->DIR_FileSize);
	else
		printk("Can`t find file\n");
}
