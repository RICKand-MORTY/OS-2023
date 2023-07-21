#include <VFS.h>
#include "../../lib/lib.h"
#include "../../usr/dirent.h"
#include <memory.h>

#define NULL (void *)0

struct file_system_type filesystem = {"filesystem",0};

//挂载函数搜索目标文件系统名并解析引导扇区
//name为文件系统名，DPTE为MBR内分区表，buf为引导扇区的数据
struct super_block* mount_fs(char * name,struct Disk_Partition_Table_Entry * DPTE,void * buf)
{
    struct file_system_type *p = NULL;
    for(p = &filesystem; p; p = p->next)
    {
        if(!strcmp(p->name, name))
        {
            return p->read_superblock(DPTE, buf);
        }
    }
    return 0;
}

//注册函数用于将文件系统插入到filesystem链表上
unsigned long register_filesystem(struct file_system_type * fs)
{
	struct file_system_type * p = NULL;

	for(p = &filesystem;p;p = p->next)
		if(!strcmp(fs->name,p->name))
			return 0;

	fs->next = filesystem.next;
	filesystem.next = fs;

	return 1;
}

/*填充dirent结构，返回dirent长度*/
int fill_dentry(void *buf,char *name, long namelen,long type,long offset)
{
    struct dirent * dent = (struct dirent *)buf;
    memcpy(dent->d_name, name , namelen);
    dent->d_namelen = namelen;
	dent->d_type = type;
	dent->d_offset = offset;
    more_page_free(name , 1);
	return sizeof(struct dirent) + namelen;
}