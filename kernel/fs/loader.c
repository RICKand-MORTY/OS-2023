#include <elf.h>
#include "../../lib/printk.h"
#include "../../lib/lib.h"
#include <memory.h>
#include <process.h>
#include <fat32.h>
#include <VFS.h>
#include <sysflags.h>
#include <error.h>
#include "../../usr/user_syscall.h"

#define LOADER_SEEK_SET(filp, off)   filp->f_ops->lseek(filp, off, SEEK_SET)
#define LOADER_SEEK_CUR(filp)        filp->f_ops->lseek(filp, 0, SEEK_CUR)
#define IS_FLAGS_SET(v, m)           ((v&m) == m)
#define LOADER_MAX_SYM_LENGTH        33

typedef void (entry_t)(void);

/*
读取段表头存入h中，失败返回-1，成功返回0
n表示段表索引
*/
static int readSecHeader(ELFExec_t *e, int n, Elf64_Shdr *h, struct file *filp)
{
  long offset = e->sectionTable + n * sizeof(Elf64_Shdr);
  if(LOADER_SEEK_SET(filp, offset) < 0)
  {
    //移动文件指针到段表头处
    return -1;
  }
  if(filp->f_ops->read(filp, (char *)h, sizeof(Elf64_Shdr), &filp->position) < 0)
  {
    return -1;
  }
  return 0;
}


//读取段表名，存入buf中,失败返回-1,成功返回0
static int readSectionName(ELFExec_t *e, long off, char *buf, unsigned long max, struct file *filp)
{
  int ret = -1;
  long offset = e->sectionTableStrings + off;
  long old = LOADER_SEEK_CUR(filp);
  if(LOADER_SEEK_SET(filp, offset) < 0)
  {
    return -1;
  }
  else
  {
    if(filp->f_ops->read(filp, buf, max, &filp->position) < 0)
    {
      return -1;
    }
    else
    {
      ret = 0;
    }
  }
  LOADER_SEEK_SET(filp, old);   //文件指针归位
  return 0;
}

//读取符号名,存入buf,成功返回0，失败返回-1
static int readSymbolName(ELFExec_t *e, long off, char *buf, unsigned long max, struct file *filp)
{
  int ret = -1;
  long offset = e->symbolTableStrings + off;
  long old = LOADER_SEEK_CUR(filp);
  if(LOADER_SEEK_SET(filp, off) < 0)
  {
    return -1;
  }
  else
  {
    if(filp->f_ops->read(filp, buf, max, &filp->position) < 0)
    {
      return -1;
    }
    else
    {
      ret = 0;
    }
  }
  LOADER_SEEK_SET(filp, old); 
  return 0;
}

//将section内容从硬盘中读出到s中,成功返回0,失败返回-1
static int loadSecData(ELFExec_t *e, ELFSection_t *s, Elf64_Shdr *sh, struct file * filp)
{
  unsigned long needpage = 0;
  if (!sh->sh_size) {
    printk(" No data for section!\n");
    return 0;
  }
  if(!s)
  {
    printk("s is NULL!\n");
    return -1;
  }
  needpage = ((sh->sh_size + PAGE_SIZE - 1) & ~(PAGE_SIZE -1)) / PAGE_SIZE;
  s->data = (void *)more_page_alloc(needpage);
  if(s->data == 1)
  {
    printk("alloc page for section fail!\n");
    return -1;
  }
  memset(s->data, 0, needpage * PAGE_SIZE);
  if (sh->sh_type != SHT_NOBITS)
  {
    if(LOADER_SEEK_SET(filp, sh->sh_offset) < 0)
    {
      if(s->data)
      {
        more_page_free(s->data, needpage);
      }
      return -1;
    }
    if(filp->f_ops->read(filp, s->data, sh->sh_size, &filp->position) < 0)
    {
      if(s->data)
      {
        more_page_free(s->data, needpage);
      }
      return -1;
    }
  }
  s->sec_size = sh->sh_size;
  return 0;
}

//填充段表对应在e中的信息，并读取部分段
static int placeInfo(ELFExec_t *e, Elf64_Shdr *sh, const char *name, int n, struct file * filp) {
  if (!strcmp(name, ".symtab")) {
    e->symbolTable = sh->sh_offset;
    e->symbolCount = sh->sh_size / sizeof(Elf64_Sym);
    return FoundSymTab;
  } else if (!strcmp(name, ".strtab")) {
    e->symbolTableStrings = sh->sh_offset;
    return FoundStrTab;
  } else if (!strcmp(name, ".text")) {
    if (loadSecData(e, &e->text, sh, filp) == -1)
      return FoundERROR;
    e->text.secIdx = n;
    e->entry = (char *)e->entry - sh->sh_addr;
    e->text.sec_size = sh->sh_size;
    return FoundText;
  } else if (!strcmp(name, ".rodata")) {
    if (loadSecData(e, &e->rodata, sh, filp) == -1)
      return FoundERROR;
    e->rodata.secIdx = n;
    return FoundRodata;
  } else if (!strcmp(name, ".data")) {
    if (loadSecData(e, &e->data, sh, filp) == -1)
      return FoundERROR;
    e->data.secIdx = n;
    return FoundData;
  } else if (!strcmp(name, ".bss")) {
    if (loadSecData(e, &e->bss, sh, filp) == -1)
      return FoundERROR;
    e->bss.secIdx = n;
    return FoundBss;
  } else if (!strcmp(name, ".sdram_rodata")) {
    if (loadSecData(e, &e->sdram_rodata, sh, filp) == -1)
      return FoundERROR;
    e->sdram_rodata.secIdx = n;
    return FoundSDRamRodata;
  } else if (!strcmp(name, ".sdram_data")) {
    if (loadSecData(e, &e->sdram_data, sh, filp) == -1)
      return FoundERROR;
    e->sdram_data.secIdx = n;
    return FoundSDRamData;
  } else if (!strcmp(name, ".sdram_bss")) {
    if (loadSecData(e, &e->sdram_bss, sh, filp) == -1)
      return FoundERROR;
    e->sdram_bss.secIdx = n;
    return FoundSDRamBss;
  } else if (!strcmp(name, ".init_array")) {
    if (loadSecData(e, &e->init_array, sh, filp) == -1)
      return FoundERROR;
    e->init_array.secIdx = n;
    return FoundInitArray;
  } else if (!strcmp(name, ".fini_array")) {
    if (loadSecData(e, &e->fini_array, sh, filp) == -1)
      return FoundERROR;
    e->fini_array.secIdx = n;
    e->fini_array_size = sh->sh_size;
    return FoundFiniArray;
  } else if (!strcmp(name, ".rel.text")) {
    e->text.relSecIdx = n;
    return FoundRelText;
  } else if (!strcmp(name, ".rel.rodata")) {
    e->rodata.relSecIdx = n;
    return FoundRelRodata;
  } else if (!strcmp(name, ".rel.data")) {
    e->data.relSecIdx = n;
    return FoundRelData;
  } else if (!strcmp(name, ".rel.sdram_rodata")) {
    e->sdram_rodata.relSecIdx = n;
    return FoundRelSDRamRodata;
  } else if (!strcmp(name, ".rel.sdram_data")) {
    e->sdram_data.relSecIdx = n;
    return FoundRelSDRamData;
  } else if (!strcmp(name, ".rel.init_array")) {
    e->init_array.relSecIdx = n;
    return FoundRelInitArray;
  } else if (!strcmp(name, ".rel.fini_array")) {
    e->fini_array.relSecIdx = n;
    return FoundRelFiniArray;
  } else if (!strcmp(name, ".sdata")){
    if (loadSecData(e, &e->sdata, sh, filp) == -1)
    {
      e->sdata.secIdx = n;
      e->sdata.sec_size = sh->sh_size;
      return FoundSdata;
    }
  }
  return 0;
}


//加载所有段的内容，存入e中，返回已经加载的段的标记
static int loadSymbols(ELFExec_t *e, struct file *filp)
{
  int n;
  int founded = 0;
  printk("load symbols...\n");
  for(n = 0; n < e->sections; n++)
  {
    Elf64_Shdr sh;
    char name[LOADER_MAX_SYM_LENGTH] = "<unamed>";
    if (readSecHeader(e, n, &sh, filp) != 0) {
      printk("Error reading section\n");
      return -1;
    }
    if (sh.sh_name)//sh_name是.shstrtab段中的偏移.
      readSectionName(e, sh.sh_name, name, sizeof(name), filp);
    printk("Examining section %d %s\n", n, name);
    founded |= placeInfo(e, &sh, name, n, filp);
    if (IS_FLAGS_SET(founded, FoundAll))
      return FoundAll;
  }
  printk("load finish!\n");
  return founded;
}

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

static void freeSection(ELFSection_t *s) 
{
  int needpage = 0;
  if (s->data)
  {
    needpage = ((s->sec_size + PAGE_SIZE - 1) & ~(PAGE_SIZE -1)) / PAGE_SIZE;
    more_page_free(s->data, needpage);
  }
}

static void freeElf(ELFExec_t *e) {
  freeSection(&e->text);
  freeSection(&e->rodata);
  freeSection(&e->data);
  freeSection(&e->bss);
  freeSection(&e->sdram_rodata);
  freeSection(&e->sdram_data);
  freeSection(&e->sdram_bss);
  freeSection(&e->init_array);
  freeSection(&e->fini_array);
}

//执行.init_array的每一个初始化函数
static void do_init(ELFExec_t *e, struct file * filp)
{
  Elf64_Shdr sh;
  if(e->init_array.data)
  {
    printk("Processing section .init_array.\n");
    if(readSecHeader(e, e->init_array.relSecIdx, &sh, filp) != 0)
    {
      printk("Error reading section header");
      return;
    }
    entry_t **entry = (entry_t**) (e->init_array.data);
    int i;
    int n = sh.sh_size >> 3;    //64位一项8字节
    for(i=0;i<n;i++) 
    {
      printk("Processing .init_array[%d] : %08x->%08x\n", i, (int)entry, (int)*entry);
      (*entry)();
      entry++;
    }
    printk("Processing section .init_array finish!\n");
  }
  else
  {
    printk("No section: .init_array.\n");
  }
}



//执行fini_array中的函数
static void do_fini(ELFExec_t *e, struct file * filp)
{
  if (e->fini_array.data) {
    entry_t **entry = (entry_t**) (e->fini_array.data);
    int i;
    int n = e->fini_array_size >> 2;
    for(i=0;i<n;i++) {
      printk("Processing .fini_array[%d] : %08x->%08x\n", i, (int)entry, (int)*entry);
      (*entry)();
      entry++;
    }
  } 
  else
  {
    printk("No .fini_array");
  }
}

//卸载elf
int unload_elf(ELFExec_t *exec, int exec_page_size, struct file * filp) 
{
  do_fini(exec, filp);
  freeElf(exec);
  more_page_free(exec, exec_page_size);
  return 0;
}
//执行elf文件,成功返回0,失败返回-1
int jumpTo(ELFExec_t *e) 
{
  void * stack = NULL;
  if (e->entry != 1) {
    entry_t *entry = (entry_t*) (e->text.data + e->entry);
    char * addr = (char *)(e->text.data + e->entry);
    print("elf running now!\n\n");
    /*for(int i=0;i<e->text.sec_size;i++)
    {
      
      if(i % 16 == 0)
      {
        print("\n");
      }
      print("%02x ",*addr);
      addr++;
    }*/
    //entry();
    stack = malloc(2); //use for stack
    memset(stack, 0, PAGE_SIZE * 2); // clear both pages
    if(stack != (void *)1) // check if allocation succeeded
    {
      register unsigned long saved = 0;
      register unsigned long ra = 0;
      void * tos = (void *)(((unsigned long)stack + PAGE_SIZE * 2) & (long)(-16));
      //tos = (void *)((unsigned long)tos & -16); // align tos to 16 bytes
      /* s->saved */
		__asm__ volatile("MV %0, sp\n\t" : : "r"(saved));
		/* tos->MSP */
		__asm__ volatile("MV sp, %0\n\t" : : "r"(tos));
		/* push saved */
		__asm__ volatile("ADDI sp, sp, -8\n\t"
				 "SD %0, 0(sp)\n\t" : : "r"(saved));

    entry();

		/* pop saved */
		__asm__ volatile("LD %0, 0(sp)\n\t"
				 "ADDI sp, sp, 8\n\t" : : "r"(saved));
		/* saved->sp */
		__asm__ volatile("MV sp, %0\n\t" : : "r"(saved));

      free(stack, 2);
      print("elf exec finish!\n");
      return 0;
      
    }
    else
    {
      return -1; // allocation failed
    }
  } 
  else
  {
    print("No entry defined.\n");
    return -1;
  }
}



int load_elf(char *path, loader_env_t user_data, ELFExec_t *exec, char* argv, char* envp, int fd)
{
  struct file * filp = NULL;
  int i = 0;
  int filesize = exec->file_size;
  int needpage = ((filesize + PAGE_SIZE - 1) & ~(PAGE_SIZE -1)) / PAGE_SIZE;
  needpage = 1;     //debug
  user_data.fd = fd;
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
  if (initElf(exec) != 0) 
  {
    printk("Invalid elf %s\n", path);
    return -1;
  }
  filp = get_current_task()->file_struct[fd];
  /*if (!IS_FLAGS_SET(loadSymbols(exec, filp), FoundValid)) 
  {
    //至少要有符号表和字符串表
    more_page_free(exec, needpage);
    return -2;
  }*/
  /*if (relocateSections(exec) != 0) {
    more_page_free(exec, needpage);
    return -3;
  }*/
  loadSymbols(exec, filp);
  do_init(exec, filp);
  //jumpTo(exec);
  //unload_elf(exec, needpage, filp);
  //more_page_free(exec, needpage);
  return 0;
}