// SPDX-FileCopyrightText: 2010 Mathieu Desnoyers <mathieu.desnoyers@efficios.com>
//
// SPDX-License-Identifier: LGPL-2.1-or-later

#ifndef _URCU_RCULFSTACK_H
#define _URCU_RCULFSTACK_H

/*
 * Userspace RCU library - Lock-Free RCU Stack
 */

#include <urcu/compiler.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef CDS_LFS_RCU_DEPRECATED
#define CDS_LFS_RCU_DEPRECATED	\
	CDS_DEPRECATED("urcu/rculfstack.h is deprecated. Please use urcu/lfstack.h instead.")
#endif

struct cds_lfs_node_rcu {
	struct cds_lfs_node_rcu *next;
};

struct cds_lfs_stack_rcu {
	struct cds_lfs_node_rcu *head;
};

#ifdef _LGPL_SOURCE

#include <urcu/static/rculfstack.h>

static inline CDS_LFS_RCU_DEPRECATED
void cds_lfs_node_init_rcu(struct cds_lfs_node_rcu *node)
{
	_cds_lfs_node_init_rcu(node);
}

static inline
void cds_lfs_init_rcu(struct cds_lfs_stack_rcu *s)
{
	_cds_lfs_init_rcu(s);
}

static inline CDS_LFS_RCU_DEPRECATED
int cds_lfs_push_rcu(struct cds_lfs_stack_rcu *s,
			struct cds_lfs_node_rcu *node)
{
	return _cds_lfs_push_rcu(s, node);
}

static inline CDS_LFS_RCU_DEPRECATED
struct cds_lfs_node_rcu *cds_lfs_pop_rcu(struct cds_lfs_stack_rcu *s)
{
	return _cds_lfs_pop_rcu(s);
}

#else /* !_LGPL_SOURCE */

extern CDS_LFS_RCU_DEPRECATED
void cds_lfs_node_init_rcu(struct cds_lfs_node_rcu *node);
extern CDS_LFS_RCU_DEPRECATED
void cds_lfs_init_rcu(struct cds_lfs_stack_rcu *s);
extern CDS_LFS_RCU_DEPRECATED
int cds_lfs_push_rcu(struct cds_lfs_stack_rcu *s,
			struct cds_lfs_node_rcu *node);

/*
 * Should be called under rcu read lock critical section.
 *
 * The caller must wait for a grace period to pass before freeing the returned
 * node or modifying the cds_lfs_node_rcu structure.
 * Returns NULL if stack is empty.
 */
extern CDS_LFS_RCU_DEPRECATED
struct cds_lfs_node_rcu *cds_lfs_pop_rcu(struct cds_lfs_stack_rcu *s);

#endif /* !_LGPL_SOURCE */

#ifdef __cplusplus
}
#endif

#endif /* _URCU_RCULFSTACK_H */
