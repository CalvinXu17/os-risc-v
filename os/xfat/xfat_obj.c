/**
 * 本源码配套的课程为 - 从0到1动手写FAT32文件系统。每个例程对应一个课时，尽可能注释。
 * 作者：李述铜
 * 课程网址：http://01ketang.cc
 * 版权声明：本源码非开源，二次开发，或其它商用前请联系作者。
 */
 #include "xfat_obj.h"

xfat_err_t xfat_obj_init(xfat_obj_t* obj, xfat_obj_type_t type) {
    obj->type = type;
    return FS_ERR_OK;
}
