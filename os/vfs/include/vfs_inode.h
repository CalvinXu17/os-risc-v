#ifndef VFS_INODE_H
#define VFS_INODE_H

#include "type.h"
#include "list.h"

// why your inode's name so long?
#define VFS_INODE_NAME_MAX              31

typedef union vfs_inode_operations_un {
    vfs_chrdev_ops_t    *cd_ops;
    vfs_blkdev_ops_t    *bd_ops;
    vfs_fs_ops_t        *fs_ops;
} vfs_inode_ops_t;

#define VFS_INODE_TYPE_CHAR_DEVICE          0x00000000
#define VFS_INODE_TYPE_BLOCK_DEVICE         0x00000001
#define VFS_INODE_TYPE_FILESYSTEM           0x00000002
#define VFS_INODE_TYPE_UNUSED               0x00000003

#define VFS_INODE_TYPE_SET(inode, _type) \
    ((inode)->type = _type)

#define VFS_INODE_IS_TYPE(inode, _type) \
    ((inode)->type == _type)

#define VFS_INODE_IS_CHAR_DEVICE(inode)     VFS_INODE_IS_TYPE(inode, VFS_INODE_TYPE_CHAR_DEVICE)
#define VFS_INODE_IS_BLOCK_DEVICE(inode)    VFS_INODE_IS_TYPE(inode, VFS_INODE_TYPE_BLOCK_DEVICE)
#define VFS_INODE_IS_FILESYSTEM(inode)      VFS_INODE_IS_TYPE(inode, VFS_INODE_TYPE_FILESYSTEM)
#define VFS_INODE_IS_DEVICE(inode)          (VFS_INODE_IS_BLOCK_DEVICE(inode) || VFS_INODE_IS_CHAR_DEVICE(inode))

#define VFS_INODE_REFS_MAX                  (0x3F)

typedef struct vfs_inode_st {
    uchar               type : 2; /*< now we only support three kind of inode */
    uchar               refs : 6; /*< max 0x3F */
    vfs_inode_ops_t     ops;
    void               *private;
    list                list;
    char                name[1];
} vfs_inode_t;

#define VFS_INODE_SIZE(name_len)    (sizeof(vfs_inode_t) + name_len)

int vfs_inode_is_busy(vfs_inode_t *inode);

void vfs_inode_refinc(vfs_inode_t *inode);

vfs_inode_t *vfs_inode_find(const char *path, const char **relative_path);

vfs_inode_t *vfs_mount_inode_find(const char *path, const char **relative_path);

vfs_inode_t *vfs_inode_alloc(const char *name);

void vfs_inode_free(vfs_inode_t *inode);

void vfs_inode_release(vfs_inode_t *inode);

#endif

