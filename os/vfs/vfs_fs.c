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
#include "list.h"
#include "kmalloc.h"
#include "string.h"

list k_vfs_fsmap_list = {.prev=&k_vfs_fsmap_list, .next=&k_vfs_fsmap_list};

static vfs_fsmap_t *vfs_fsmap_get(const char *fs_name)
{
    char *name = NULL;
    vfs_fsmap_t *fsmap = NULL;
    list *l;
    for(l = k_vfs_fsmap_list.next; l != &k_vfs_fsmap_list; l = l->next)
    {
        fsmap = GET_STRUCT_ENTRY(l, vfs_fsmap_t, list);
        name = (char *)fsmap->name;
        if (strlen(name) == strlen(fs_name) &&
            strncmp(name, fs_name, strlen(name)) == 0) {
            return fsmap;
        }
    }
    return NULL;
}

static inline void vfs_fsmap_set(vfs_fsmap_t *fsmap, const char *fs_name, vfs_fs_ops_t *ops)
{
    fsmap->name = fs_name;
    fsmap->ops  = ops;
    init_list(&fsmap->list);
    add_before(&k_vfs_fsmap_list, &fsmap->list);
}

static inline void vfs_fsmap_free(vfs_fsmap_t *fsmap)
{
    del_list(&fsmap->list);
    kfree((void *)fsmap);
}

/*
    Actually the "section table hack" is much more cool and elegant, but with that
    we must deel with the link script, that will introduce coding complication,
    especially develop with IDE. So, linked list with dynamic memory is a compromised
    solution here.
 */
vfs_err_t vfs_fs_register(const char *fs_name, vfs_fs_ops_t *ops)
{
    vfs_fsmap_t *fsmap = NULL;

    if (!fs_name || !ops) {
        return VFS_ERR_PARA_INVALID;
    }

    fsmap = vfs_fsmap_get(fs_name);
    if (fsmap) {
        return VFS_ERR_FS_ALREADY_REGISTERED;
    }

    fsmap = (vfs_fsmap_t *)kmalloc(sizeof(vfs_fsmap_t));
    if (!fsmap) {
        return VFS_ERR_OUT_OF_MEMORY;
    }

    vfs_fsmap_set(fsmap, fs_name, ops);
    return VFS_ERR_NONE;
}

vfs_err_t vfs_fs_unregister(const char *fs_name)
{
    vfs_fsmap_t *fsmap = NULL;

    if (!fs_name) {
        return VFS_ERR_PARA_INVALID;
    }

    fsmap = vfs_fsmap_get(fs_name);
    if (!fsmap) {
        return VFS_ERR_FS_NOT_REGISTERED;
    }

    kfree(fsmap);
    return VFS_ERR_NONE;
}

/*
    vfs_mount("/dev/block/nand", "/fs/yaffs", "yaffs");
    we mount a filesystem named "yaffs" to directory "fs/yaffs", using device "/dev/block/nand"
 */
vfs_err_t vfs_fs_mount(const char *device_path, const char *dir, const char *fs_name)
{
    vfs_fsmap_t *fsmap = NULL;
    vfs_inode_t *device_inode = NULL;
    vfs_inode_t *fs_inode = NULL;

    fsmap = vfs_fsmap_get(fs_name);
    if (!fsmap) {
        return VFS_ERR_FS_NOT_REGISTERED;
    }

    device_inode = vfs_inode_find(device_path, NULL);
    if (!device_inode) {
        return VFS_ERR_DEVICE_NOT_REGISTERED;
    }

    if (!VFS_INODE_IS_DEVICE(device_inode)) {
        return VFS_ERR_INODE_INVALID;
    }

    fs_inode = vfs_mount_inode_find(dir, NULL);
    if (fs_inode) {
        if (VFS_INODE_IS_FILESYSTEM(fs_inode)) {
            return VFS_ERR_FS_ALREADY_MOUNTED;
        } else {
            return VFS_ERR_INODE_INAVALIABLE;
        }
    }

    fs_inode = vfs_inode_alloc(dir);
    if (!fs_inode) {
        return VFS_ERR_INODE_CREATE_FAILED;
    }

    VFS_INODE_TYPE_SET(fs_inode, VFS_INODE_TYPE_FILESYSTEM);
    fs_inode->ops.fs_ops = fsmap->ops;

    if (!fs_inode->ops.fs_ops->bind) {
        return VFS_ERR_OPS_NULL;
    }

    if (fs_inode->ops.fs_ops->bind(fs_inode, device_inode) != 0) {
        return VFS_ERR_OPS_FAILED;
    }

    return VFS_ERR_NONE;
}

vfs_err_t vfs_fs_umount(const char *dir)
{
    vfs_inode_t *inode = NULL;

    inode = vfs_mount_inode_find(dir, NULL);
    
    if (!inode) {
        return VFS_ERR_FS_NOT_MOUNT;
    }

    if (!VFS_INODE_IS_FILESYSTEM(inode)) {
        return VFS_ERR_INODE_INAVALIABLE;
    }

    if (vfs_inode_is_busy(inode)) {
        return VFS_ERR_INODE_BUSY;
    }

    if (inode->ops.fs_ops->unbind && inode->ops.fs_ops->unbind(inode) != 0) {
        return VFS_ERR_OPS_FAILED;
    }
    
    vfs_inode_release(inode);
    return VFS_ERR_NONE;
}

vfs_err_t vfs_fs_mkfs(const char *device_path, const char *fs_name, int opt, unsigned long arg)
{
    vfs_fsmap_t *fsmap = NULL;
    vfs_inode_t *device_inode = NULL;
    vfs_inode_t *fs_inode = NULL;

    fsmap = vfs_fsmap_get(fs_name);
    if (!fsmap) {
        return VFS_ERR_FS_NOT_REGISTERED;
    }

    device_inode = vfs_inode_find(device_path, NULL);
    if (!device_inode) {
        return VFS_ERR_DEVICE_NOT_REGISTERED;
    }

    if (!VFS_INODE_IS_DEVICE(device_inode)) {
        return VFS_ERR_INODE_INVALID;
    }

    fs_inode->ops.fs_ops = fsmap->ops;

    if (!fsmap->ops->mkfs) {
        return VFS_ERR_OPS_NULL;
    }

    if (fsmap->ops->mkfs(device_inode, opt, arg) != 0) {
        return VFS_ERR_OPS_FAILED;
    }

    return VFS_ERR_NONE;
}

