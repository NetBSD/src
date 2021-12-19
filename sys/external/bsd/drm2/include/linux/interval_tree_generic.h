/*	$NetBSD: interval_tree_generic.h,v 1.3 2021/12/19 11:00:18 riastradh Exp $	*/

/*-
 * Copyright (c) 2018 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Taylor R. Campbell.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef	_LINUX_INTERVAL_TREE_GENERIC_H_
#define	_LINUX_INTERVAL_TREE_GENERIC_H_

#define	INTERVAL_TREE_DEFINE(T, F, KT, KLAST, NSTART, NLAST, QUAL, PREFIX)    \
									      \
static inline int							      \
PREFIX##__compare_nodes(void *__cookie, const void *__va, const void *__vb)   \
{									      \
	const T *__na = __va;						      \
	const T *__nb = __vb;						      \
	const KT __astart = START(__na), __alast = LAST(__na);		      \
	const KT __bstart = START(__nb), __blast = LAST(__nb);		      \
									      \
	if (__astart < __bstart)					      \
		return -1;						      \
	if (__astart > __bstart)					      \
		return +1;						      \
	if (__alast < __blast)						      \
		return -1;						      \
	if (__alast > __blast)						      \
		return -1;						      \
	return 0;		       					      \
}									      \
									      \
static inline int							      \
PREFIX##__compare_key(void *__cookie, const void *__vn, const void *__vk)     \
{									      \
	const T *__n = __vn;						      \
	const KT *__k = __vk;						      \
	const KT __nstart = START(__n), __nlast = LAST(__n);		      \
									      \
	if (__nlast < *__k)						      \
		return -1;						      \
	if (*__k < __nstart)						      \
		return +1;						      \
	return 0;							      \
}									      \
									      \
static const rb_tree_ops_t PREFIX##__rbtree_ops = {			      \
	.rbto_compare_nodes = PREFIX##__compare_nodes,			      \
	.rbto_compare_key = PREFIX##__compare_key,			      \
	.rbto_node_offset = offsetof(T, F),				      \
};									      \
									      \
/* Not in Linux API, needed for us.  */					      \
QUAL void								      \
PREFIX##_init(struct rb_root_cached *__root)				      \
{									      \
	rb_tree_init(&__root->rb_root.rbr_tree, &PREFIX##__rbtree_ops);	      \
}									      \
									      \
QUAL void								      \
PREFIX##_insert(T *__node, struct rb_root_cached *__root)		      \
{									      \
	T *__collision __diagused;					      \
									      \
	__collision = rb_tree_insert_node(&__root->rb_root.rbr_tree, __node); \
	KASSERT(__collision == __node);					      \
}									      \
									      \
QUAL void								      \
PREFIX##_remove(T *__node, struct rb_root_cached *__root)		      \
{									      \
	rb_tree_remove_node(&__root->rb_root.rbr_tree, __node);		      \
}									      \
									      \
QUAL T *								      \
PREFIX##_iter_first(struct rb_root_cached *__root, KT __start, KT __last)     \
{									      \
	T *__node;							      \
									      \
	__node = rb_tree_find_node_geq(&__root->rb_root.rbr_tree, &__start);  \
	if (__node == NULL)						      \
		return NULL;						      \
	KASSERT(START(__node) <= __start);				      \
	if (__last < START(__node))					      \
		return NULL;						      \
									      \
	return __node;							      \
}

#endif	/* _LINUX_INTERVAL_TREE_GENERIC_H_ */
