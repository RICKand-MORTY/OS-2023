#include <elf.h>
#include "../../lib/printk.h"
#include "../../lib/lib.h"
#include <memory.h>
#include <process.h>
#include <fat32.h>
#include <VFS.h>
#include <sysflags.h>
#include <error.h>

#define LOADER_SEEK_SET(filp, off)   filp->f_ops->lseek(filp, off, SEEK_SET)
#define LOADER_SEEK_CUR(filp)        filp->f_ops->lseek(filp, 0, SEEK_CUR)

//初始化elf文件，读出字符串所在段,成功返回0
static int initElf(ELFExec_t *e)
{
  Elf64_Ehdr h;
  Elf64_Shdr sh;
  struct file * filp = NULL;
  unsigned char *p = ELFMAG;
  unsigned char *q = NULL;
  int i = 0;

  filp = get_current_task()->file_struct[e->user_data.fd];
  filp->f_ops->lseek(filp, 0, SEEK_SET);    //文件指针回到起点
  //读取elf文件头
  if(filp->f_ops->read(filp, (char *)&h, sizeof(h), &filp->position) < 0)
  {
    printk("read error!\n");
    return -EIO;
  }
  q = (char*)h.e_ident;
  for(i = 0; i < strlen(ELFMAG); i++)
  {
    if(*p != *q)
    {
      printk("ELF format error!\n");
      return -ENOEXEC;
    }
    q++;
    p++;
  }
  //将文件指针移动到.shstrtab位置
  LOADER_SEEK_SET(filp, h.e_shoff + h.e_shstrndx * sizeof(sh));
  //把.shstrtab读出到sh
  if(filp->f_ops->read(filp, (char *)&sh, sizeof(sh), &filp->position) < 0)
  {
    printk("read error!\n");
    return -EIO;
  }
  e->entry = h.e_entry;         //入口点
  e->sections = h.e_shnum;      //段描述符数量
  e->sectionTable = h.e_shoff;  //段表偏移
  e->sectionTableStrings = sh.sh_offset;  //.shstrtab相对于文件的偏移
  printk("e->sectionTable:%ld e->sectionTableStrings:%ld\n", e->sectionTable, e->sectionTableStrings);
  return 0;
}

int load_elf(char *path, loader_env_t user_data, ELFExec_t **exec_ptr,char* argv, char* envp, int fd)
{
  ELFExec_t *exec = (ELFExec_t *)alloc_pgtable();
  int i = 0;

  if (exec == 1) {
    printk("allocation failed\n\n");
    return -1;
  }
  exec->user_data = user_data;
  if(argv)
  {
    for(i = 0; argv[i] != NULL;i++)
    {
      if(exec->argv_size >= MAXARGSIZE)
      {
        printk("arg max!\n");
        break;
      }
      exec->argv[i] = argv[i];
      exec->argv_size++;
      
    }
  }
  if(envp)
  {
    for(i = 0; envp[i] != NULL;i++)
    {
      if(exec->envp_size >= MAXARGSIZE)
      {
        printk("arg max!\n");
        break;
      }
      exec->envp[i] = envp[i];
      exec->envp_size++;
    }
  }
  exec->user_data.fd = fd;
  if (initElf(exec) != 0) {
    printk("Invalid elf %s\n", path);
    return -1;
  }

}