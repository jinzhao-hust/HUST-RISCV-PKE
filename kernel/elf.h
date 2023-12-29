#ifndef _ELF_H_
#define _ELF_H_

#include "util/types.h"
#include "process.h"

#define MAX_CMDLINE_ARGS 64

// elf header structure
typedef struct elf_header_t {
  uint32 magic;
  uint8 elf[12];
  uint16 type;      /* Object file type */
  uint16 machine;   /* Architecture */
  uint32 version;   /* Object file version */
  uint64 entry;     /* Entry point virtual address */
  uint64 phoff;     /* Program header table file offset */
  uint64 shoff;     /* Section header table file offset */
  uint32 flags;     /* Processor-specific flags */
  uint16 ehsize;    /* ELF header size in bytes */
  uint16 phentsize; /* Program header table entry size */
  uint16 phnum;     /* Program header table entry count */
  uint16 shentsize; /* Section header table entry size */
  uint16 shnum;     /* Section header table entry count */
  uint16 shstrndx;  /* Section header string table index */
} elf_header;

// Program segment header.
typedef struct elf_prog_header_t {
  uint32 type;   /* Segment type */
  uint32 flags;  /* Segment flags */
  uint64 off;    /* Segment file offset */
  uint64 vaddr;  /* Segment virtual address */
  uint64 paddr;  /* Segment physical address */
  uint64 filesz; /* Segment size in file */
  uint64 memsz;  /* Segment size in memory */
  uint64 align;  /* Segment alignment */
} elf_prog_header;

// section header       added for @lab1_challenge1 
typedef struct elf_sect_header_t
{
  uint32	sh_name;		/* 4B section名称相对于字符串表的位置偏移 */
  uint32	sh_type;		/* 4B section的类型 */
  uint64	sh_flags;		/* 8B section的标志 */
  uint64	sh_addr;		/* 8B section的第一个字节应处的位置 */
  uint64	sh_offset;		/* 8B section的第一个字节与文件头之间的偏移 */
  uint64	sh_size;		/* 8B section的长度（字节数） */
  uint32	sh_link;		/* 4B section的头部表索引链接 */
  uint32	sh_info;		/* 4B section的附加信息 */
  uint64	sh_addralign;	/* 8B section的对齐方式 */
  uint64	sh_entsize;		/* 8B section的每个表项的长度（字节数） */
} elf_sect_header;          /* total 64B */

// symbol               added for @lab1_challenge1
typedef struct elf_sym_t
{
    uint32 st_name;         /* 4B Symbol name (string tbl index) */ 
    unsigned char st_info;  /* 1B Symbol type and binding */
    unsigned char st_other; /* 1B Symbol visibility */
    uint16 st_shndx;        /* 2B Section index */
    uint64 st_value;        /* 8B Symbol value */
    uint64 st_size;         /* 8B Symbol size */
} elf_sym;                  /* total 24B */

typedef struct func_info_t{
    char func_name[32];
    uint64 st_value;
    uint64 st_size;
}func_info;

#define SECTION_HEADER_SIZE 64
#define SYMBOL_SIZE 24


#define ELF_MAGIC 0x464C457FU  // "\x7FELF" in little endian
#define ELF_PROG_LOAD 1

typedef enum elf_status_t {
  EL_OK = 0,

  EL_EIO,
  EL_ENOMEM,
  EL_NOTELF,
  EL_ERR,

} elf_status;

typedef struct elf_ctx_t {
  void *info;
  elf_header ehdr;
} elf_ctx;

elf_status elf_init(elf_ctx *ctx, void *info);
elf_status elf_load(elf_ctx *ctx);

void load_bincode_from_host_elf(process *p);
// fuction to get function info added for @lab1_challenge1
void get_func_name(elf_ctx* ctx);
#endif
