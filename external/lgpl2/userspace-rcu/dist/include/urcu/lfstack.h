// SPDX-FileCopyrightText: 2010-2012 Mathieu Desnoyers <mathieu.desnoyers@efficios.com>
//
// SPDX-License-Identifier: LGPL-2.1-or-later

#ifndef _URCU_LFSTACK_H
#define _URCU_LFSTACK_H

/*
 * Userspace RCU library - Lock-Free Stack
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <pthread.h>

/*
 * Lock-free stack.
 *
 * Stack implementing push, pop, pop_all operations, as well as iterator
 * on the stack head returned by pop_all.
 *
 * Synchronization table:
 *
 * External synchronization techniques described in the API below is
 * required between pairs marked with "X". No external synchronization
 * required between pairs marked with "-".
 *
 *                      cds_lfs_push  __cds_lfs_pop  __cds_lfs_pop_all
 * cds_lfs_push               -              -                  -
 * __cds_lfs_pop              -              X                  X
 * __cds_lfs_pop_all          -              X                  -
 *
 * cds_lfs_pop_blocking and cds_lfs_pop_all_blocking use an internal
 * mutex to provide synchronization.
 */

/*
 * struct cds_lfs_node is returned by cds_lfs_pop, and also used as
 * iterator on stack. It is not safe to dereference the node next
 * pointer when returned by cds_lfs_pop.
 */
struct cds_lfs_node {
	struct cds_lfs_node *next;
};

/*
 * struct cds_lfs_head is returned by __cds_lfs_pop_all, and can be used
 * to begin iteration on the stack. "node" needs to be the first field
 * of cds_lfs_head, so the end-of-stack pointer value can be used for
 * both types.
 */
struct cds_lfs_head {
	struct cds_lfs_node node;
};

struct __cds_lfs_stack {
	struct cds_lfs_head *head;
};

struct cds_lfs_stack {
	struct cds_lfs_head *head;
	pthread_mutex_t lock;
};

/*
 * In C, the transparent union allows calling functions that work on
 * both struct cds_lfs_stack and struct __cds_lfs_stack on any of those
 * two types.
 *
 * In C++, implement static inline wrappers using function overloading
 * to obtain an API similar to C.
 *
 * Avoid complaints from clang++ not knowing the transparent union
 * attribute.
 */
#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wignored-attributes"
#endif
typedef union {
	struct __cds_lfs_stack *_s;
	struct cds_lfs_stack *s;
} __attribute__((__transparent_union__)) cds_lfs_stack_ptr_t;

typedef union {
	const struct __cds_lfs_stack *_s;
	const struct cds_lfs_stack *s;
} __attribute__((__transparent_union__)) cds_lfs_stack_const_ptr_t;
#if defined(__clang__)
#pragma clang diagnostic pop
#endif

#ifdef _LGPL_SOURCE

#include <urcu/static/lfstack.h>

#define cds_lfs_node_init		_cds_lfs_node_init
#define cds_lfs_init			_cds_lfs_init
#define cds_lfs_destroy			_cds_lfs_destroy
#define __cds_lfs_init			___cds_lfs_init
#define cds_lfs_empty			_cds_lfs_empty
#define cds_lfs_push			_cds_lfs_push

/* Locking performed internally */
#define cds_lfs_pop_blocking		_cds_lfs_pop_blocking
#define cds_lfs_pop_all_blocking	_cds_lfs_pop_all_blocking

/* Synchronize pop with internal mutex */
#define cds_lfs_pop_lock		_cds_lfs_pop_lock
#define cds_lfs_pop_unlock		_cds_lfs_pop_unlock

/* Synchronization ensured by the caller. See synchronization table. */
#define __cds_lfs_pop			___cds_lfs_pop
#define __cds_lfs_pop_all		___cds_lfs_pop_all

#else /* !_LGPL_SOURCE */

/*
 * cds_lfs_node_init: initialize lock-free stack node.
 */
extern void cds_lfs_node_init(struct cds_lfs_node *node);

/*
 * cds_lfs_init: initialize lock-free stack (with locking). Pair with
 * cds_lfs_destroy().
 */
extern void cds_lfs_init(struct cds_lfs_stack *s);

/*
 * cds_lfs_destroy: destroy lock-free stack (with lock). Pair with
 * cds_lfs_init().
 */
extern void cds_lfs_destroy(struct cds_lfs_stack *s);

/*
 * __cds_lfs_init: initialize lock-free stack (without lock).
 * Don't pair with any destroy function.
 */
extern void __cds_lfs_init(struct __cds_lfs_stack *s);

/*
 * cds_lfs_empty: return whether lock-free stack is empty.
 *
 * No memory barrier is issued. No mutual exclusion is required.
 */
extern bool cds_lfs_empty(cds_lfs_stack_const_ptr_t s);

/*
 * cds_lfs_push: push a node into the stack.
 *
 * Does not require any synchronization with other push nor pop.
 *
 * Returns 0 if the stack was empty prior to adding the node.
 * Returns non-zero otherwise.
 */
extern bool cds_lfs_push(cds_lfs_stack_ptr_t s,
			struct cds_lfs_node *node);

/*
 * cds_lfs_pop_blocking: pop a node from the stack.
 *
 * Calls __cds_lfs_pop with an internal pop mutex held.
 */
extern struct cds_lfs_node *cds_lfs_pop_blocking(struct cds_lfs_stack *s);

/*
 * cds_lfs_pop_all_blocking: pop all nodes from a stack.
 *
 * Calls __cds_lfs_pop_all with an internal pop mutex held.
 */
extern struct cds_lfs_head *cds_lfs_pop_all_blocking(struct cds_lfs_stack *s);

/*
 * cds_lfs_pop_lock: lock stack pop-protection mutex.
 */
extern void cds_lfs_pop_lock(struct cds_lfs_stack *s);

/*
 * cds_lfs_pop_unlock: unlock stack pop-protection mutex.
 */
extern void cds_lfs_pop_unlock(struct cds_lfs_stack *s);

/*
 * __cds_lfs_pop: pop a node from the stack.
 *
 * Returns NULL if stack is empty.
 *
 * __cds_lfs_pop needs to be synchronized using one of the following
 * techniques:
 *
 * 1) Calling __cds_lfs_pop under rcu read lock critical section.
 *    Both __cds_lfs_pop and __cds_lfs_pop_all callers must wait for a
 *    grace period to pass before freeing the returned node or pushing
 *    the node back into the stack. It is valid to overwrite the content
 *    of cds_lfs_node immediately after __cds_lfs_pop and
 *    __cds_lfs_pop_all.
 * 2) Using mutual exclusion (e.g. mutexes) to protect __cds_lfs_pop
 *    and __cds_lfs_pop_all callers.
 * 3) Ensuring that only ONE thread can call __cds_lfs_pop() and
 *    __cds_lfs_pop_all(). (multi-provider/single-consumer scheme).
 */
extern struct cds_lfs_node *__cds_lfs_pop(cds_lfs_stack_ptr_t s);

/*
 * __cds_lfs_pop_all: pop all nodes from a stack.
 *
 * __cds_lfs_pop_all does not require any synchronization with other
 * push, nor with other __cds_lfs_pop_all, but requires synchronization
 * matching the technique used to synchronize __cds_lfs_pop:
 *
 * 1) If __cds_lfs_pop is called under rcu read lock critical section,
 *    both __cds_lfs_pop and __cds_lfs_pop_all callers must wait for a
 *    grace period to pass before freeing the returned node or pushing
 *    the node back into the stack. It is valid to overwrite the content
 *    of cds_lfs_node immediately after __cds_lfs_pop and
 *    __cds_lfs_pop_all. No RCU read-side critical section is needed
 *    around __cds_lfs_pop_all.
 * 2) Using mutual exclusion (e.g. mutexes) to protect __cds_lfs_pop and
 *    __cds_lfs_pop_all callers.
 * 3) Ensuring that only ONE thread can call __cds_lfs_pop() and
 *    __cds_lfs_pop_all(). (multi-provider/single-consumer scheme).
 */
extern struct cds_lfs_head *__cds_lfs_pop_all(cds_lfs_stack_ptr_t s);

#endif /* !_LGPL_SOURCE */

/*
 * cds_lfs_for_each: Iterate over all nodes returned by
 *                   __cds_lfs_pop_all.
 * @__head: node returned by __cds_lfs_pop_all (struct cds_lfs_head pointer).
 * @__node: node to use as iterator (struct cds_lfs_node pointer).
 *
 * Content written into each node before push is guaranteed to be
 * consistent, but no other memory ordering is ensured.
 */
#define cds_lfs_for_each(__head, __node)	\
	for (__node = &__head->node;		\
		__node != NULL;			\
		__node = __node->next)

/*
 * cds_lfs_for_each_safe: Iterate over all nodes returned by
 *                        __cds_lfs_pop_all, safe against node deletion.
 * @__head: node returned by __cds_lfs_pop_all (struct cds_lfs_head pointer).
 * @__node: node to use as iterator (struct cds_lfs_node pointer).
 * @__n: struct cds_lfs_node pointer holding the next pointer (used
 *       internally).
 *
 * Content written into each node before push is guaranteed to be
 * consistent, but no other memory ordering is ensured.
 */
#define cds_lfs_for_each_safe(__head, __node, __n)			   \
	for (__node = &__head->node, __n = (__node ? __node->next : NULL); \
		__node != NULL;						   \
		__node = __n, __n = (__node ? __node->next : NULL))

#ifdef __cplusplus
}

/*
 * In C++, implement static inline wrappers using function overloading
 * to obtain an API similar to C.
 */

static inline cds_lfs_stack_ptr_t cds_lfs_stack_cast(struct __cds_lfs_stack *s)
{
	cds_lfs_stack_ptr_t ret = {
		._s = s,
	};
	return ret;
}

static inline cds_lfs_stack_ptr_t cds_lfs_stack_cast(struct cds_lfs_stack *s)
{
	cds_lfs_stack_ptr_t ret = {
		.s = s,
	};
	return ret;
}

static inline cds_lfs_stack_const_ptr_t cds_lfs_stack_const_cast(const struct __cds_lfs_stack *s)
{
	cds_lfs_stack_const_ptr_t ret = {
		._s = s,
	};
	return ret;
}

static inline cds_lfs_stack_const_ptr_t cds_lfs_stack_const_cast(const struct cds_lfs_stack *s)
{
	cds_lfs_stack_const_ptr_t ret = {
		.s = s,
	};
	return ret;
}

template<typename T> static inline bool cds_lfs_empty(T s)
{
	return cds_lfs_empty(cds_lfs_stack_const_cast(s));
}

template<typename T> static inline bool cds_lfs_push(T s,
			struct cds_lfs_node *node)
{
	return cds_lfs_push(cds_lfs_stack_cast(s), node);
}

template<typename T> static inline struct cds_lfs_node *__cds_lfs_pop(T s)
{
	return __cds_lfs_pop(cds_lfs_stack_cast(s));
}

template<typename T> static inline struct cds_lfs_head *__cds_lfs_pop_all(T s)
{
	return __cds_lfs_pop_all(cds_lfs_stack_cast(s));
}

#endif	/* __cplusplus */

#endif /* _URCU_LFSTACK_H */
