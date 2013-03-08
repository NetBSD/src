#ifndef __COMPAT_LIST__
#define __COMPAT_LIST__

struct list_head {
	struct list_head	*next;
	struct list_head	*prev;
};

#define INIT_LIST_HEAD(ptr)	do {			\
	(ptr)->prev = (ptr);				\
	(ptr)->next = (ptr);				\
} while(0)

#define list_for_each_safe(pos, next, head)		\
	for ((pos) = (head)->next, (next) = (pos)->next->next; (pos) != (head); (pos) = (next), (next) = (next)->next)
#define list_for_each(pos, head)			\
	for ((pos) = (head)->next; (pos) != (head); (pos) = (pos)->next)
#define	list_entry(pos, type, head)	((type*)((intptr_t)(pos) - offsetof(type, head)))

#define	list_del(pos)		do {			\
	(pos)->prev->next = (pos)->next;		\
	(pos)->next->prev = (pos)->prev;		\
	(pos)->next = (pos)->prev = (pos);		\
} while(0)

#define	list_add(pos, head)	do {			\
	(pos)->prev = (head)->prev;			\
	(pos)->next = (head);				\
	(head)->prev = (pos);				\
} while (0)

#endif
