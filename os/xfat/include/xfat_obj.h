/**
 * 本源码配套的课程为 - 从0到1动手写FAT32文件系统。每个例程对应一个课时，尽可能注释。
 * 作者：李述铜
 * 课程网址：http://01ketang.cc
 * 版权声明：本源码非开源，二次开发，或其它商用前请联系作者。
 */
#ifndef XFAT_OBJ_H
#define XFAT_OBJ_H

#include "xtypes.h"

#define to_type(obj, type_p)    ((type_p *)(obj))
#define to_obj(obj)             ((xfat_obj_t *)(obj))

typedef enum _xfat_obj_type_t {
    XFAT_OBJ_DISK,
    XFAT_OBJ_FAT,
    XFAT_OBJ_FILE,
}xfat_obj_type_t;;

struct _xfat_obj_t {
    xfat_obj_type_t type;
};
typedef struct _xfat_obj_t xfat_obj_t;

xfat_err_t xfat_obj_init(xfat_obj_t* obj, xfat_obj_type_t type);

#endif
