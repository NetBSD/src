/*	$NetBSD: linux_idr.c,v 1.3.4.3 2017/12/03 11:38:00 jdolecek Exp $	*/

/*-
 * Copyright (c) 2013 The NetBSD Foundation, Inc.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: linux_idr.c,v 1.3.4.3 2017/12/03 11:38:00 jdolecek Exp $");

#include <sys/param.h>
#include <sys/atomic.h>
#include <sys/rbtree.h>

#include <linux/err.h>
#include <linux/idr.h>
#include <linux/slab.h>

struct idr_node {
	rb_node_t		in_rb_node;
	int			in_index;
	void			*in_data;
	SIMPLEQ_ENTRY(idr_node)	in_list;
};
SIMPLEQ_HEAD(idr_head, idr_node);

static struct {
	kmutex_t	lock;
	struct idr_head	preloaded_nodes;
	struct idr_head	discarded_nodes;
} idr_cache __cacheline_aligned;

int
linux_idr_module_init(void)
{

	mutex_init(&idr_cache.lock, MUTEX_DEFAULT, IPL_VM);
	SIMPLEQ_INIT(&idr_cache.preloaded_nodes);
	SIMPLEQ_INIT(&idr_cache.discarded_nodes);
	return 0;
}

void
linux_idr_module_fini(void)
{

	KASSERT(SIMPLEQ_EMPTY(&idr_cache.discarded_nodes));
	KASSERT(SIMPLEQ_EMPTY(&idr_cache.preloaded_nodes));
	mutex_destroy(&idr_cache.lock);
}

static signed int idr_tree_compare_nodes(void *, const void *, const void *);
static signed int idr_tree_compare_key(void *, const void *, const void *);

static const rb_tree_ops_t idr_rb_ops = {
	.rbto_compare_nodes = &idr_tree_compare_nodes,
	.rbto_compare_key = &idr_tree_compare_key,
	.rbto_node_offset = offsetof(struct idr_node, in_rb_node),
	.rbto_context = NULL,
};

static signed int
idr_tree_compare_nodes(void *ctx __unused, const void *na, const void *nb)
{
	const int a = ((const struct idr_node *)na)->in_index;
	const int b = ((const struct idr_node *)nb)->in_index;

	if (a < b)
		return -1;
	else if (b < a)
		 return +1;
	else
		return 0;
}

static signed int
idr_tree_compare_key(void *ctx __unused, const void *n, const void *key)
{
	const int a = ((const struct idr_node *)n)->in_index;
	const int b = *(const int *)key;

	if (a < b)
		return -1;
	else if (b < a)
		return +1;
	else
		return 0;
}

void
idr_init(struct idr *idr)
{

	mutex_init(&idr->idr_lock, MUTEX_DEFAULT, IPL_VM);
	rb_tree_init(&idr->idr_tree, &idr_rb_ops);
}

void
idr_destroy(struct idr *idr)
{

#if 0				/* XXX No rb_tree_destroy?  */
	rb_tree_destroy(&idr->idr_tree);
#endif
	mutex_destroy(&idr->idr_lock);
}

bool
idr_is_empty(struct idr *idr)
{

	return (RB_TREE_MIN(&idr->idr_tree) == NULL);
}

void *
idr_find(struct idr *idr, int id)
{
	const struct idr_node *node;
	void *data;

	mutex_spin_enter(&idr->idr_lock);
	node = rb_tree_find_node(&idr->idr_tree, &id);
	data = (node == NULL? NULL : node->in_data);
	mutex_spin_exit(&idr->idr_lock);

	return data;
}

void *
idr_replace(struct idr *idr, void *replacement, int id)
{
	struct idr_node *node;
	void *result;

	mutex_spin_enter(&idr->idr_lock);
	node = rb_tree_find_node(&idr->idr_tree, &id);
	if (node == NULL) {
		result = ERR_PTR(-ENOENT);
	} else {
		result = node->in_data;
		node->in_data = replacement;
	}
	mutex_spin_exit(&idr->idr_lock);

	return result;
}

void
idr_remove(struct idr *idr, int id)
{
	struct idr_node *node;

	mutex_spin_enter(&idr->idr_lock);
	node = rb_tree_find_node(&idr->idr_tree, &id);
	KASSERTMSG((node != NULL), "idr %p has no entry for id %d", idr, id);
	rb_tree_remove_node(&idr->idr_tree, node);
	mutex_spin_exit(&idr->idr_lock);
	kfree(node);
}

void
idr_preload(gfp_t gfp)
{
	struct idr_node *node;

	if (ISSET(gfp, __GFP_WAIT))
		ASSERT_SLEEPABLE();

	node = kmalloc(sizeof(*node), gfp);
	KASSERT(node != NULL || !ISSET(gfp, __GFP_WAIT));
	if (node == NULL)
		return;

	mutex_spin_enter(&idr_cache.lock);
	SIMPLEQ_INSERT_TAIL(&idr_cache.preloaded_nodes, node, in_list);
	mutex_spin_exit(&idr_cache.lock);
}

int
idr_alloc(struct idr *idr, void *data, int start, int end, gfp_t gfp)
{
	int maximum = (end <= 0? INT_MAX : (end - 1));
	struct idr_node *node, *search, *collision __diagused;
	int id = start;

	/* Sanity-check inputs.  */
	if (ISSET(gfp, __GFP_WAIT))
		ASSERT_SLEEPABLE();
	if (__predict_false(start < 0))
		return -EINVAL;
	if (__predict_false(maximum < start))
		return -ENOSPC;

	/* Grab a node allocated by idr_preload.  */
	mutex_spin_enter(&idr_cache.lock);
	if (SIMPLEQ_EMPTY(&idr_cache.preloaded_nodes)) {
		mutex_spin_exit(&idr_cache.lock);
		return -ENOMEM;
	}
	node = SIMPLEQ_FIRST(&idr_cache.preloaded_nodes);
	SIMPLEQ_REMOVE_HEAD(&idr_cache.preloaded_nodes, in_list);
	mutex_spin_exit(&idr_cache.lock);

	/* Find an id.  */
	mutex_spin_enter(&idr->idr_lock);
	search = rb_tree_find_node_geq(&idr->idr_tree, &start);
	while ((search != NULL) && (search->in_index == id)) {
		if (maximum <= id) {
			id = -ENOSPC;
			goto out;
		}
		search = rb_tree_iterate(&idr->idr_tree, search, RB_DIR_RIGHT);
		id++;
	}
	node->in_index = id;
	node->in_data = data;
	collision = rb_tree_insert_node(&idr->idr_tree, node);
	KASSERT(collision == node);
out:	mutex_spin_exit(&idr->idr_lock);

	if (id < 0) {
		/* Discard the node on failure.  */
		mutex_spin_enter(&idr_cache.lock);
		SIMPLEQ_INSERT_HEAD(&idr_cache.discarded_nodes, node, in_list);
		mutex_spin_exit(&idr_cache.lock);
	}
	return id;
}

void
idr_preload_end(void)
{
	struct idr_head temp = SIMPLEQ_HEAD_INITIALIZER(temp);
	struct idr_node *node, *next;

	mutex_spin_enter(&idr_cache.lock);
	SIMPLEQ_FOREACH_SAFE(node, &idr_cache.discarded_nodes, in_list, next) {
		SIMPLEQ_REMOVE_HEAD(&idr_cache.discarded_nodes, in_list);
		SIMPLEQ_INSERT_HEAD(&temp, node, in_list);
	}
	mutex_spin_exit(&idr_cache.lock);

	SIMPLEQ_FOREACH_SAFE(node, &temp, in_list, next) {
		SIMPLEQ_REMOVE_HEAD(&temp, in_list);
		kfree(node);
	}
}

int
idr_for_each(struct idr *idr, int (*proc)(int, void *, void *), void *arg)
{
	struct idr_node *node;
	int error = 0;

	/* XXX Caller must exclude modifications.  */
	membar_consumer();
	RB_TREE_FOREACH(node, &idr->idr_tree) {
		error = (*proc)(node->in_index, node->in_data, arg);
		if (error)
			break;
	}

	return error;
}
