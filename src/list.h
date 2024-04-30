#pragma once

#ifndef offsetof
#define offsetof(container, entry) __builtin_offsetof(container, entry)
#endif

#ifndef container_of
#define container_of(ptr, type, field) ((type *) ((char *) ptr - offsetof(type, field)))
#endif

struct list_head {
    struct list_head *next;
    struct list_head *prev;
};

static inline void INIT_LIST_HEAD(struct list_head *list)
{
	list->next = list->prev = list;
}

static inline int list_empty(struct list_head *head)
{
    return head->next == head;
}

static inline void list_del(struct list_head *node)
{
    node->prev->next = node->next;
    node->next->prev = node->prev;
}

static inline void list_del_init(struct list_head *node)
{
    list_del(node);
    INIT_LIST_HEAD(node);
}

static inline void __list_add(struct list_head *node, struct list_head *prev, struct list_head *next)
{
    prev->next = node;
    node->prev = prev;
    node->next = next;
    next->prev = node;
}

static inline void list_add(struct list_head *node, struct list_head *head)
{
    __list_add(node, head, head->next);
}

static inline void list_add_tail(struct list_head *node, struct list_head *head)
{
    __list_add(node, head->prev, head);
}


static inline void list_move(struct list_head *node, struct list_head *head)
{
    list_del(node);
    list_add(node, head);
}

static inline void list_move_tail(struct list_head *node, struct list_head *head)
{
    list_del(node);
    list_add_tail(node, head);
}

#define	list_entry(ptr, type, field)	container_of(ptr, type, field)

#define list_first_entry(ptr, type, member) \
    list_entry((ptr)->next, type, member)

#define list_for_each(node, head) \
    for (node = (head)->next; node != (head); node = node->next)

#define list_for_each_safe(node, n, head) \
    for (node = (head)->next, n = node->next; node != (head); node = n, n = node->next)

#define list_for_each_entry(container, head, field) \
    for (container = list_entry((head)->next, typeof(*container), field); \
        &container->field != (head); \
        container = list_entry(container->field.next, typeof(*container), field))
