#ifndef ELF_H
#define ELF_H

#include "type.h"

#define ELF_MAGIC 0x464C457FU  // "\x7FELF" in little endian

// File header
struct elfhdr {
  uint magic;       // must equal ELF_MAGIC
  uchar elf[12];
  ushort type;
  ushort machine;
  uint version;
  uint64 entry;     // 程序的入口的虚拟地址
  uint64 phoff;     // 程序表头相对于文件头的偏移量
  uint64 shoff;     // 节头表相对于文件头的编移量
  uint flags;       
  ushort ehsize;    // ELF头部的大小
  ushort phentsize; // 每个段表条目大小
  ushort phnum;     // 有多少个段条目
  ushort shentsize; // 每个节头表的条目大小 
  ushort shnum;     // 有多少个节头表条目
  ushort shstrndx;  // 包含节名称的字符串
} __attribute__((__packed__));

// Program section header
struct proghdr {
  uint32 type;
  uint32 flags;
  uint64 off;       // 段的第一字节相对文件头的偏移
  uint64 vaddr;
  uint64 paddr;
  uint64 filesz;    // 段在文件中的长度
  uint64 memsz;     // 段在内存中的长度
  uint64 align;
} __attribute__((__packed__));

// Values for Proghdr type
#define ELF_PROG_NULL           0
#define ELF_PROG_LOAD           1
#define ELF_PROG_DYNAMIC        2
#define ELF_PROG_INTERP         3
#define ELF_PROG_NOTE           4
#define ELF_PROG_SHLIB          5
#define ELF_PROG_PHDR           6
#define ELF_PROG_TLS            7
#define ELF_PROG_NUM            8
#define ELF_PROG_LOOS           0x60000000
#define ELF_PROG_GNU_EH_FRAME   0x6474e550
#define ELF_PROG_GNU_STACK      0x6474e551
#define ELF_PROG_GNU_RELRO      0x6474e552
#define ELF_PROG_LOSUNW         0x6ffffffa
#define ELF_PROG_SUNWBSS        0x6ffffffa
#define ELF_PROG_SUNWSTACK      0x6ffffffb
#define ELF_PROG_HISUNW         0x6fffffff
#define ELF_PROG_HIOS           0x6fffffff
#define ELF_PROG_LOPROC         0x70000000
#define ELF_PROG_HIPROC         0x7fffffff

// Flag bits for Proghdr flags
#define ELF_PROG_FLAG_EXEC      1
#define ELF_PROG_FLAG_WRITE     2
#define ELF_PROG_FLAG_READ      4

#endif