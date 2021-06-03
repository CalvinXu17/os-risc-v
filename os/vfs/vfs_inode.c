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

#include "vfs.h"
#include "kmalloc.h"
#include "list.h"

list k_vfs_inode_list = {.prev=&k_vfs_inode_list, .next=&k_vfs_inode_list};

/*
    1. filesystem
    assume that we have registered a yaffs and mount it on /fs/yaffs,
    we wanna find the inode of file /fs/yaffs/somedir/thefile,
    we will return the inode of /fs/yaffs, and the relative_path will be somedir/thefile
    2. device
    if we are find a device's inode, like /dev/block/nand, the path must just be the path
    of the device(/dev/block/nand), but on this condition, we would return a relative_path,
    because device operation does not need a relative path.
 */
/*
    I don't think an IOT system will have quit a lot of inodes, a linear search algorithm
    should just be OK(mainly because it's simple ^_^).
 */
static vfs_inode_t *vfs_inode_search(const char *path, const char **relative_path)
{
    char *name = NULL;
    int path_len, name_len;
    vfs_inode_t *inode = NULL;
    list *l;
    path_len = strlen(path);

    vfs_inode_t *max_len_inode = NULL;
    int max_len = -1;

    for(l = k_vfs_inode_list.next; l != &k_vfs_inode_list; l=l->next) {
        inode = GET_STRUCT_ENTRY(l, vfs_inode_t, list);
        name = (char *)inode->name;
        name_len = strlen(name);

        /* eg. name is /dev/nand, path is /dev/na */
        if (path_len < name_len) {
            continue;
        }

        /* 1. name is /dev/nand, path is /dev/nand
           2. name is /dev/nand, path is /dev/nand/invalid
           3. name is /dev/nand, path is /dev/nand_tail
           4. name is /fs/ramfs, path is /fs/ramfs
           5. name is /fs/ramfs, path is /fs/ramfs/somefile
           6. name is /fs/ramfs, path is /fs/ramfs_tail
         */
        if (VFS_INODE_IS_DEVICE(inode)) {
            /* for a device, name and the path should just be the same, situation 1 */
            if (path_len == name_len &&
                strncmp(name, path, name_len) == 0) {
                return inode;
            }
        } else if (VFS_INODE_IS_FILESYSTEM(inode)) {
            if (path_len == name_len &&
                strncmp(name, path, name_len) == 0) {
                /* for a filesystem, situation 4 */
                if (relative_path) {
                    *relative_path = &path[name_len];
                }
                return inode;
            } else if (path_len > name_len &&
                        strncmp(name, path, name_len) == 0 &&
                        path[name_len] == '/') {
                /* for a filesystem, situation 5 */
                if (relative_path) {
                    *relative_path = &path[name_len];
                }
                if(name_len > max_len)
                {
                    max_len = name_len;
                    max_len_inode = inode;
                }
            }
        }
    }
    if(max_len_inode) return max_len_inode;
    else return NULL;
}

vfs_inode_t *vfs_mount_inode_search(const char *path, const char **relative_path)
{
    char *name = NULL;
    int path_len, name_len;
    vfs_inode_t *inode = NULL;
    list *l;
    path_len = strlen(path);

    for(l = k_vfs_inode_list.next; l != &k_vfs_inode_list; l=l->next) {
        inode = GET_STRUCT_ENTRY(l, vfs_inode_t, list);
        name = (char *)inode->name;
        name_len = strlen(name);

        /* eg. name is /dev/nand, path is /dev/na */
        if (path_len < name_len) {
            continue;
        }

        /* 1. name is /dev/nand, path is /dev/nand
           2. name is /dev/nand, path is /dev/nand/invalid
           3. name is /dev/nand, path is /dev/nand_tail
           4. name is /fs/ramfs, path is /fs/ramfs
           5. name is /fs/ramfs, path is /fs/ramfs/somefile
           6. name is /fs/ramfs, path is /fs/ramfs_tail
         */
        if (VFS_INODE_IS_DEVICE(inode)) {
            /* for a device, name and the path should just be the same, situation 1 */
            if (path_len == name_len &&
                strncmp(name, path, name_len) == 0) {
                return inode;
            }
        } else if (VFS_INODE_IS_FILESYSTEM(inode)) {
            if (path_len == name_len &&
                strncmp(name, path, name_len) == 0) {
                /* for a filesystem, situation 4 */
                if (relative_path) {
                    *relative_path = NULL;
                }
                return inode;
            } else if (path_len == name_len &&
                        strncmp(name, path, name_len) == 0 &&
                        path[name_len] == '/') {
                /* for a filesystem, situation 5 */
                if (relative_path) {
                    *relative_path = &path[name_len];
                }
                return inode;
            }
        }
    }
    return NULL;
}

static vfs_inode_t *vfs_inode_create(const char *name, int name_len)
{
    vfs_inode_t *inode = NULL;

    inode = (vfs_inode_t *)kmalloc(VFS_INODE_SIZE(name_len));
    if (!inode) {
        return NULL;
    }

    strncpy(inode->name, name, name_len);
    VFS_INODE_TYPE_SET(inode, VFS_INODE_TYPE_UNUSED);
    init_list(&inode->list);
    add_before(&k_vfs_inode_list, &inode->list);
    return inode;
}

void vfs_inode_free(vfs_inode_t *inode)
{
    del_list(&inode->list);
    kfree((void *)inode);
}

void vfs_inode_refinc(vfs_inode_t *inode)
{
    if (inode->refs < VFS_INODE_REFS_MAX) {
        ++inode->refs;
    }
}

vfs_inode_t *vfs_inode_find(const char *path, const char **relative_path)
{
    vfs_inode_t *inode = NULL;

    inode = vfs_inode_search(path, relative_path);
    if (!inode) {
        return NULL;
    }

    return inode;
}

vfs_inode_t *vfs_mount_inode_find(const char *path, const char **relative_path)
{
    vfs_inode_t *inode = NULL;

    inode = vfs_mount_inode_search(path, relative_path);
    if (!inode) {
        return NULL;
    }

    return inode;
}

int vfs_inode_is_busy(vfs_inode_t *inode)
{
    return inode->refs > 1;
}

vfs_inode_t *vfs_inode_alloc(const char *name)
{
    int name_len;
    vfs_inode_t *inode = NULL;

    if (!name) {
        return NULL;
    }

    name_len = strlen(name);
    if (name_len > VFS_INODE_NAME_MAX) {
        return NULL;
    }

    inode = vfs_inode_create(name, name_len);
    if (!inode) {
        return NULL;
    }

    ++inode->refs;

    return inode;
}

static void vfs_inode_refdec(vfs_inode_t *inode)
{
    if (inode->refs) {
        --inode->refs;
    }
}

void vfs_inode_release(vfs_inode_t *inode)
{
    vfs_inode_refdec(inode);

    if (inode->refs == 0) {
        vfs_inode_free(inode);
    }
}

