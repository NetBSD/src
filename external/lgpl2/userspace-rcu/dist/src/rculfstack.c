// SPDX-FileCopyrightText: 2010 Mathieu Desnoyers <mathieu.desnoyers@efficios.com>
//
// SPDX-License-Identifier: LGPL-2.1-or-later

/*
 * Userspace RCU library - Lock-Free RCU Stack
 */

/* Remove deprecation warnings from LGPL wrapper build. */
#define CDS_LFS_RCU_DEPRECATED

/* Do not #define _LGPL_SOURCE to ensure we can emit the wrapper symbols */
#undef _LGPL_SOURCE
#include "urcu/rculfstack.h"
#define _LGPL_SOURCE
#include "urcu/static/rculfstack.h"

/*
 * library wrappers to be used by non-LGPL compatible source code.
 */


void cds_lfs_node_init_rcu(struct cds_lfs_node_rcu *node)
{
	_cds_lfs_node_init_rcu(node);
}

void cds_lfs_init_rcu(struct cds_lfs_stack_rcu *s)
{
	_cds_lfs_init_rcu(s);
}

int cds_lfs_push_rcu(struct cds_lfs_stack_rcu *s,
		struct cds_lfs_node_rcu *node)
{
	return _cds_lfs_push_rcu(s, node);
}

struct cds_lfs_node_rcu *cds_lfs_pop_rcu(struct cds_lfs_stack_rcu *s)
{
	return _cds_lfs_pop_rcu(s);
}
