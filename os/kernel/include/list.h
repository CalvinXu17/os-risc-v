#ifndef LIST_H
#define LIST_H

#define VAROFFSET(type,mem) ((unsigned long)(&((type *)0)->mem))
#define GET_STRUCT_ENTRY(ptr, type, mem) ((type *)((char *)ptr - VAROFFSET(type, mem)))

typedef struct _list
{
    struct _list *prev;
    struct _list *next;
} list;

static inline void init_list(list *l) __attribute__((always_inline));
static inline void add_after(list *l, list *newl) __attribute__((always_inline));
static inline void add_before(list *l, list *newl) __attribute__((always_inline));
static inline void add_list(list *l, list *newl) __attribute__((always_inline));
static inline void del_list(list *del) __attribute__((always_inline));

static inline void init_list(list *l)
{
    l->prev = l->next = l;
}

static inline void add_after(list *l, list *newl)
{
    newl->next = l->next;
    newl->prev = l;
    l->next->prev = newl;
    l->next = newl;
}

static inline void add_before(list *l, list *newl)
{
    add_after(l->prev, newl);
}

static inline void add_list(list *l, list *newl)
{
    add_after(l, newl);
}

static inline void del_list(list *del)
{
    del->prev->next = del->next;
    del->next->prev = del->prev;
    init_list(del);
}

static inline int is_empty_list(list *l)
{
    return l->next == l && l->prev == l;
}

#endif