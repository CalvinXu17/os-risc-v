/*----------------------------------------------------------------------------
 * Tencent is pleased to support the open source community by making TencentOS
 * available.
 *
 * Copyright (C) 2019 THL A29 Limited, a Tencent company. All rights reserved.
 * If you have downloaded a copy of the TencentOS binary from Tencent, please
 * note that the TencentOS binary is licensed under the BSD 3-Clause License.
 *
 * If you have downloaded a copy of the TencentOS source code from Tencent,
 * please note that TencentOS source code is licensed under the BSD 3-Clause
 * License, except for the third-party components listed below which are
 * subject to different license terms. Your integration of TencentOS into your
 * own projects may require compliance with the BSD 3-Clause License, as well
 * as the other licenses applicable to the third-party components included
 * within TencentOS.
 *---------------------------------------------------------------------------*/

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
