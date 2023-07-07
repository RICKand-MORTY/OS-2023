#ifndef _ELF_H
#define _ELF_H


#define EI_NIDENT	16
#define	ELFMAG		"\177ELF"

/*e_type*/
#define ET_NONE              0           //No file type
#define ET_REL               1           //Relocatable file
#define ET_EXEC              2           //Executable file
#define ET_DYN               3           //Shared object file
#define ET_CORE              4           //Core file
#define ET_LOPROC            0xff00      //Processor-specific
#define ET_HIPROC            0xffff      //Processor-specific

/*sh_type*/
#define SHT_NULL            0
#define SHT_PROGBITS        1            //程序,数据
#define SHT_SYMTAB          2            //符号表
#define SHT_STRTAB          3            //字符串表
#define SHT_RELA            4            //重定位表
#define SHT_HASH            5
#define SHT_DYNAMIC         6            //动态链接
#define SHT_NOTE            7
#define SHT_NOBITS          8
#define SHT_REL             9
#define SHT_SHLIB           10
#define SHT_DYNSYM          11
#define SHT_LOPROC          0x70000000
#define SHT_HIPROC          0x7fffffff
#define SHT_LOUSER          0x80000000
#define SHT_HIUSER          0xffffffff

/* 64-bit ELF base types. */
typedef unsigned long long Elf64_Addr;
typedef unsigned short Elf64_Half;
typedef signed short Elf64_SHalf;
typedef unsigned long long Elf64_Off;
typedef signed int Elf64_Sword;
typedef unsigned int Elf64_Word;
typedef unsigned long long Elf64_Xword;
typedef signed long long Elf64_Sxword;

typedef struct elf64_hdr 
{
    unsigned char	e_ident[EI_NIDENT];	/* ELF "magic number" */
    Elf64_Half e_type;
    Elf64_Half e_machine;
    Elf64_Word e_version;
    Elf64_Addr e_entry;		/* Entry point virtual address */
    Elf64_Off e_phoff;		/* Program header table file offset */
    Elf64_Off e_shoff;		/* Section header table file offset */
    Elf64_Word e_flags;
    Elf64_Half e_ehsize;
    Elf64_Half e_phentsize;
    Elf64_Half e_phnum;
    Elf64_Half e_shentsize;
    Elf64_Half e_shnum;
    Elf64_Half e_shstrndx;
} Elf64_Ehdr;

/*segment head*/
typedef struct elf64_phdr {
  Elf64_Word p_type;
  Elf64_Word p_flags;
  Elf64_Off p_offset;		/* Segment file offset */
  Elf64_Addr p_vaddr;		/* Segment virtual address */
  Elf64_Addr p_paddr;		/* Segment physical address */
  Elf64_Xword p_filesz;		/* Segment size in file */
  Elf64_Xword p_memsz;		/* Segment size in memory */
  Elf64_Xword p_align;		/* Segment alignment, file & memory */
} Elf64_Phdr;

/*section head*/
typedef struct elf64_shdr {
  Elf64_Word sh_name;		/* Section name, index in string tbl */
  Elf64_Word sh_type;		/* Type of section */
  Elf64_Xword sh_flags;		/* Miscellaneous section attributes */
  Elf64_Addr sh_addr;		/* Section virtual addr at execution */
  Elf64_Off sh_offset;		/* Section file offset */
  Elf64_Xword sh_size;		/* Size of section in bytes */
  Elf64_Word sh_link;		/* Index of another section */
  Elf64_Word sh_info;		/* Additional section information */
  Elf64_Xword sh_addralign;	/* Section alignment */
  Elf64_Xword sh_entsize;	/* Entry size if section holds table */
} Elf64_Shdr;

/**
 * Exported symbol struct
 */
typedef struct {
  const char *name; /*!< Name of symbol */
  void *ptr; /*!< Pointer of symbol in memory */
} ELFSymbol_t;

/**
 * Environment for execution
 */
typedef struct ELFEnv {
  const ELFSymbol_t *exported; /*!< Pointer to exported symbols array */
  unsigned int exported_size; /*!< Elements on exported symbol array */
} ELFEnv_t;


typedef struct loader_env {
  int fd;
  const struct ELFEnv * env;
} loader_env_t;

typedef struct {
  void *data;
  int secIdx;                   //section index
  unsigned long relSecIdx;      //index in relocation
} ELFSection_t;

//exe info
typedef struct ELFExec {

  loader_env_t user_data;

  unsigned long sections;
  unsigned long sectionTable;
  unsigned long sectionTableStrings;

  unsigned long symbolCount;
  unsigned long symbolTable;
  unsigned long symbolTableStrings;
  unsigned long entry;

  ELFSection_t text;
  ELFSection_t rodata;
  ELFSection_t data;
  ELFSection_t bss;
  ELFSection_t init_array;
  ELFSection_t fini_array;
  ELFSection_t sdram_rodata;
  ELFSection_t sdram_data;
  ELFSection_t sdram_bss;

  unsigned int fini_array_size;

} ELFExec_t;

//use for searching each section
typedef enum {
  FoundERROR = 0,
  FoundSymTab = (1 << 0),
  FoundStrTab = (1 << 2),
  FoundText = (1 << 3),
  FoundRodata = (1 << 4),
  FoundData = (1 << 5),
  FoundBss = (1 << 6),
  FoundRelText = (1 << 7),
  FoundRelRodata = (1 << 8),
  FoundRelData = (1 << 9),
  FoundRelBss = (1 << 10),
  FoundInitArray = (1 << 11),
  FoundRelInitArray = (1 << 12),
  FoundFiniArray = (1 << 13),
  FoundRelFiniArray = (1 << 14),
  FoundSDRamRodata = (1 << 15),
  FoundSDRamData = (1 << 16),
  FoundSDRamBss = (1 << 17),
  FoundRelSDRamRodata = (1 << 18),
  FoundRelSDRamData = (1 << 19),
  FoundRelSDRamBss = (1 << 20),
  FoundValid = FoundSymTab | FoundStrTab,
  FoundExec = FoundValid | FoundText,
  FoundAll = FoundSymTab | FoundStrTab | FoundText | FoundRodata | FoundData
      | FoundBss | FoundRelText | FoundRelRodata | FoundRelData | FoundRelBss
      | FoundInitArray | FoundRelInitArray
      | FoundFiniArray | FoundRelFiniArray
} FindFlags_t;



#endif