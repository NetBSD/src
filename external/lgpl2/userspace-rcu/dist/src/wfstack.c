// SPDX-FileCopyrightText: 2010-2012 Mathieu Desnoyers <mathieu.desnoyers@efficios.com>
//
// SPDX-License-Identifier: LGPL-2.1-or-later

/*
 * Userspace RCU library - Stack with wait-free push, blocking traversal.
 */

/* Do not #define _LGPL_SOURCE to ensure we can emit the wrapper symbols */
#include "urcu/wfstack.h"
#include "urcu/static/wfstack.h"

/*
 * library wrappers to be used by non-LGPL compatible source code.
 */

void cds_wfs_node_init(struct cds_wfs_node *node)
{
	_cds_wfs_node_init(node);
}

void cds_wfs_init(struct cds_wfs_stack *s)
{
	_cds_wfs_init(s);
}

void cds_wfs_destroy(struct cds_wfs_stack *s)
{
	_cds_wfs_destroy(s);
}

void __cds_wfs_init(struct __cds_wfs_stack *s)
{
	___cds_wfs_init(s);
}

bool cds_wfs_empty(cds_wfs_stack_const_ptr_t u_stack)
{
	return _cds_wfs_empty(u_stack);
}

int cds_wfs_push(cds_wfs_stack_ptr_t u_stack, struct cds_wfs_node *node)
{
	return _cds_wfs_push(u_stack, node);
}

struct cds_wfs_node *cds_wfs_pop_blocking(struct cds_wfs_stack *s)
{
	return _cds_wfs_pop_blocking(s);
}

struct cds_wfs_node *
	cds_wfs_pop_with_state_blocking(struct cds_wfs_stack *s, int *state)
{
	return _cds_wfs_pop_with_state_blocking(s, state);
}

struct cds_wfs_head *cds_wfs_pop_all_blocking(struct cds_wfs_stack *s)
{
	return _cds_wfs_pop_all_blocking(s);
}

struct cds_wfs_node *cds_wfs_first(struct cds_wfs_head *head)
{
	return _cds_wfs_first(head);
}

struct cds_wfs_node *cds_wfs_next_blocking(struct cds_wfs_node *node)
{
	return _cds_wfs_next_blocking(node);
}

struct cds_wfs_node *cds_wfs_next_nonblocking(struct cds_wfs_node *node)
{
	return _cds_wfs_next_nonblocking(node);
}

void cds_wfs_pop_lock(struct cds_wfs_stack *s)
{
	_cds_wfs_pop_lock(s);
}

void cds_wfs_pop_unlock(struct cds_wfs_stack *s)
{
	_cds_wfs_pop_unlock(s);
}

struct cds_wfs_node *__cds_wfs_pop_blocking(cds_wfs_stack_ptr_t u_stack)
{
	return ___cds_wfs_pop_blocking(u_stack);
}

struct cds_wfs_node *
	__cds_wfs_pop_with_state_blocking(cds_wfs_stack_ptr_t u_stack,
		int *state)
{
	return ___cds_wfs_pop_with_state_blocking(u_stack, state);
}

struct cds_wfs_node *__cds_wfs_pop_nonblocking(cds_wfs_stack_ptr_t u_stack)
{
	return ___cds_wfs_pop_nonblocking(u_stack);
}

struct cds_wfs_node *
	__cds_wfs_pop_with_state_nonblocking(cds_wfs_stack_ptr_t u_stack,
		int *state)
{
	return ___cds_wfs_pop_with_state_nonblocking(u_stack, state);
}

struct cds_wfs_head *__cds_wfs_pop_all(cds_wfs_stack_ptr_t u_stack)
{
	return ___cds_wfs_pop_all(u_stack);
}
