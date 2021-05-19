#ifndef SCHED_H
#define SCHED_H

#include "type.h"
#include "process.h"
#include "spinlock.h"
#include "list.h"


extern struct proc_context sched_context[CPU_N]; // 保存scheduler调度器执行的上下文环境，切换到这里即切换到调度器运行
extern spinlock list_lock;
extern list ready_list;
extern list wait_list;
extern list dead_list;
extern list sleep_list;

struct semephore;

extern void direct_switch(struct proc_context *oldp, struct proc_context *newp);
void sched_init(void);
void scheduler(void);
void switch2sched(void);
void move_switch2sched(struct Process *proc, list *l); // 先加锁入队后切换到scheduler
void sleep(struct semephore *s);
void wakeup(struct semephore *s);

#endif