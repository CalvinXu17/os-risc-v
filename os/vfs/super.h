#ifndef SUPER_H
#define SUPER_H

#include "list.h"
#include "dentry.h"
#include "inode.h"
#include "spinlock.h"

struct super_operations;

struct super_block
{
    list sb_list; // 所有超级块的链表
    struct super_operations *s_ops;
    struct dentry *s_root;
    spinlock lock; // 锁
    int ref_cnt;  // 引用次数
    list s_inodes; // 指向全部inodes的链表

};

struct super_operations
{
    struct inode* (*alloc_indode)(struct super_block *sb); // 创建一个inode
    void (*destroy_inode)(struct inode *); // 释放inode
    int (*write_inode) (struct inode *, int);  // 向磁盘更新inode
    void (*delete_inode) (struct inode *); // 删除inode
    void (*write_super) (struct super_block *); // 向磁盘更新super_block 
};

#endif