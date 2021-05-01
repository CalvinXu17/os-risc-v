#ifndef INODE_H
#define INODE_H

#include "list.h"
#include "type.h"
#include "super.h"
#include "dentry.h"

struct inode
{
    list i_list;
    list i_sb_list;
    list i_dentry;
    uint64 i_no; // 节点号
    int ref_cnt;
    super_block *i_sb;
    uint i_flags;
};

struct inode_operations
{
    /* 为dentry对象创造一个新的索引节点 */
    int (*create) (struct inode *,struct dentry *,int, struct nameidata *);
    /* 在特定文件夹中寻找索引节点，该索引节点要对应于dentry中给出的文件名 */
    struct dentry * (*lookup) (struct inode *,struct dentry *, struct nameidata *);
    /* 创建硬链接 */
    int (*link) (struct dentry *,struct inode *,struct dentry *);
    /* 从一个符号链接查找它指向的索引节点 */
    void * (*follow_link) (struct dentry *, struct nameidata *);
    /* 在 follow_link调用之后，该函数由VFS调用进行清除工作 */
    void (*put_link) (struct dentry *, struct nameidata *, void *);
    /* 该函数由VFS调用，用于修改文件的大小 */
    void (*truncate) (struct inode *);
};

#endif