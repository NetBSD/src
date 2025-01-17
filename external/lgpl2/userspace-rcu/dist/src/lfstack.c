// SPDX-FileCopyrightText: 2010-2012 Mathieu Desnoyers <mathieu.desnoyers@efficios.com>
//
// SPDX-License-Identifier: LGPL-2.1-or-later

/*
 * Userspace RCU library - Lock-Free Stack
 */

/* Do not #define _LGPL_SOURCE to ensure we can emit the wrapper symbols */
#undef _LGPL_SOURCE
#include "urcu/lfstack.h"
#define _LGPL_SOURCE
#include "urcu/static/lfstack.h"

/*
 * library wrappers to be used by non-LGPL compatible source code.
 */

void cds_lfs_node_init(struct cds_lfs_node *node)
{
	_cds_lfs_node_init(node);
}

void cds_lfs_init(struct cds_lfs_stack *s)
{
	_cds_lfs_init(s);
}

void cds_lfs_destroy(struct cds_lfs_stack *s)
{
	_cds_lfs_destroy(s);
}

void __cds_lfs_init(struct __cds_lfs_stack *s)
{
	___cds_lfs_init(s);
}

bool cds_lfs_empty(cds_lfs_stack_const_ptr_t s)
{
	return _cds_lfs_empty(s);
}

bool cds_lfs_push(cds_lfs_stack_ptr_t s, struct cds_lfs_node *node)
{
	return _cds_lfs_push(s, node);
}

struct cds_lfs_node *cds_lfs_pop_blocking(struct cds_lfs_stack *s)
{
	return _cds_lfs_pop_blocking(s);
}

struct cds_lfs_head *cds_lfs_pop_all_blocking(struct cds_lfs_stack *s)
{
	return _cds_lfs_pop_all_blocking(s);
}

void cds_lfs_pop_lock(struct cds_lfs_stack *s)
{
	_cds_lfs_pop_lock(s);
}

void cds_lfs_pop_unlock(struct cds_lfs_stack *s)
{
	_cds_lfs_pop_unlock(s);
}

struct cds_lfs_node *__cds_lfs_pop(cds_lfs_stack_ptr_t s)
{
	return ___cds_lfs_pop(s);
}

struct cds_lfs_head *__cds_lfs_pop_all(cds_lfs_stack_ptr_t s)
{
	return ___cds_lfs_pop_all(s);
}
