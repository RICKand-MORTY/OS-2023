#include <fat32.h>
#include "../../lib/printk.h"
#include "../../lib/lib.h"
#include <buf.h>
#include <memory.h>

//#define MBR

struct Disk_Partition_Table mbr;
struct FAT32_BootSector fat32_boot;
struct FAT32_FSInfo fsinfo;

unsigned long FirstDataSector = 0;          //数据区起始扇区号
unsigned long BytesPerClus = 0;             //每簇字节数
unsigned long FirstFAT1Sector = 0;          //fat1起始扇区号
unsigned long FirstFAT2Sector = 0;          //fat2起始扇区号

//由fat表项编号来读取fat表项值
unsigned int read_FAT_Entry(unsigned int fat_entry)
{
    //512/4 = 128个fat项,得到fat表项所在扇区
	Buf *buf = bread(1, FirstFAT1Sector + (fat_entry >> 7));
	printk("DISK1_FAT32_read_FAT_Entry fat_entry:%#018lx,%#010x\n",fat_entry,buf[fat_entry & 0x7f]);
    unsigned int result = (unsigned int)buf->data[fat_entry & 0x7f] & 0x0fffffff; //& 0x7f相当于对128取余，得到fat表项在本扇区内偏移量
    //FAT表项的值只占用低28位，高4位是保留位，不用于存储数据
    brelease(buf);
	return result;
}

//由fat表项编号来写入fat表项值
unsigned long write_FAT_Entry(unsigned int fat_entry,unsigned int value)
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
从指定目录搜索与目标名name匹配的目录项，返回一个短目录项
name: 目标名
namelen: 目标名长度
dentry: 指定目录项
flags: 标记(暂时不用)
*/
struct FAT32_Directory * lookup(char * name,int namelen,struct FAT32_Directory *dentry,int flags)
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
	cluster = read_FAT_Entry(cluster);
	if(cluster < 0x0ffffff7)	//0x0ffffff7是坏簇,xFFFFFF8H-XFFFFFFF为文件最后一个簇
		goto next_cluster;

	brelease(buf);
	return NULL;
}


//搜索指定目录下文件
//name:文件目录   flag:标识符
//flag=1返回父目录项 flag=0返回目标目录项
struct FAT32_Directory * path_walk(char * name,unsigned long flags)
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

		path = lookup(dentryname,tmpnamelen,parent,flags);
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

void list_root_file(struct FAT32_Directory *dentry)
{
    
}

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

    printk("FirstDataSector = 0x%x\tFirstFAT1Sector = 0x%x\nFirstFAT2Sector = 0x%x\tBytesPerClus = 0x%x\n",FirstDataSector, FirstFAT1Sector, FirstFAT2Sector, BytesPerClus);
    
    struct FAT32_Directory * dentry = NULL;
    dentry = path_walk("/busybox",0);
	if(dentry != NULL)
		printk("\nFind /busybox\nDIR_FstClusHI:%#018lx\tDIR_FstClusLO:%#018lx\tDIR_FileSize:%#018lx\n",dentry->DIR_FstClusHI,dentry->DIR_FstClusLO,dentry->DIR_FileSize);
	else
		printk("Can`t find file\n");
}
