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
#define ELF_PROG_LOAD           1

// Flag bits for Proghdr flags
#define ELF_PROG_FLAG_EXEC      1
#define ELF_PROG_FLAG_WRITE     2
#define ELF_PROG_FLAG_READ      4

#endif