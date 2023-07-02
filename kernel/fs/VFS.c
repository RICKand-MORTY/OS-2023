#include <VFS.h>
#include "../../lib/lib.h"


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