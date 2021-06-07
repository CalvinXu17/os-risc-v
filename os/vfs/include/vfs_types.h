#ifndef VFS_TYPES_H
#define VFS_TYPES_H

#include "type.h"

typedef void        VFS_DIR;

typedef int32     vfs_off_t;
typedef uint32    vfs_oflag_t;

typedef __builtin_va_list va_list;
#define va_start(ap, last)      (__builtin_va_start(ap, last))
#define va_arg(ap, type)        (__builtin_va_arg(ap, type))
#define va_end(ap)

#endif
