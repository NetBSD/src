/*	$NetBSD: linux_idr.c,v 1.1.2.3 2013/07/24 02:07:09 riastradh Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: linux_idr.c,v 1.1.2.3 2013/07/24 02:07:09 riastradh Exp $");

#include <sys/param.h>
#include <sys/atomic.h>
#include <sys/kmem.h>
#include <sys/rbtree.h>

#include <linux/idr.h>

struct idr_node {
	rb_node_t in_rb_node;
	int in_index;
	void *in_data;
};

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

	rw_init(&idr->idr_lock);
	rb_tree_init(&idr->idr_tree, &idr_rb_ops);
	idr->idr_temp = NULL;
}

void
idr_destroy(struct idr *idr)
{

	if (idr->idr_temp != NULL) {
		/* XXX Probably shouldn't happen.  */
		kmem_free(idr->idr_temp, sizeof(*idr->idr_temp));
		idr->idr_temp = NULL;
	}
#if 0				/* XXX No rb_tree_destroy?  */
	rb_tree_destroy(&idr->idr_tree);
#endif
	rw_destroy(&idr->idr_lock);
}

void *
idr_find(struct idr *idr, int id)
{
	const struct idr_node *node;
	void *data;

	rw_enter(&idr->idr_lock, RW_READER);
	node = rb_tree_find_node(&idr->idr_tree, &id);
	data = (node == NULL? NULL : node->in_data);
	rw_exit(&idr->idr_lock);

	return data;
}

void
idr_remove(struct idr *idr, int id)
{
	struct idr_node *node;

	rw_enter(&idr->idr_lock, RW_WRITER);
	node = rb_tree_find_node(&idr->idr_tree, &id);
	KASSERT(node != NULL);
	rb_tree_remove_node(&idr->idr_tree, node);
	rw_exit(&idr->idr_lock);
	kmem_free(node, sizeof(*node));
}

void
idr_remove_all(struct idr *idr)
{
	struct idr_node *node;

	rw_enter(&idr->idr_lock, RW_WRITER);
	while ((node = rb_tree_iterate(&idr->idr_tree, NULL, RB_DIR_RIGHT)) !=
	    NULL) {
		rb_tree_remove_node(&idr->idr_tree, node);
		rw_exit(&idr->idr_lock);
		kmem_free(node, sizeof(*node));
		rw_enter(&idr->idr_lock, RW_WRITER);
	}
	rw_exit(&idr->idr_lock);
}

int
idr_pre_get(struct idr *idr, int flags __unused /* XXX */)
{
	struct idr_node *const temp = kmem_alloc(sizeof(*temp), KM_SLEEP);

	if (temp == NULL)
		return 0;

	rw_enter(&idr->idr_lock, RW_WRITER);
	if (idr->idr_temp == NULL)
		idr->idr_temp = temp;
	else
		kmem_free(temp, sizeof(*temp));
	rw_exit(&idr->idr_lock);

	return 1;
}

int
idr_get_new_above(struct idr *idr, void *data, int min_id, int *id)
{
	struct idr_node *node, *search, *collision __unused;
	int want_id = min_id;

	rw_enter(&idr->idr_lock, RW_WRITER);

	node = idr->idr_temp;
	if (node == NULL) {
		rw_exit(&idr->idr_lock);
		return -EAGAIN;
	}
	idr->idr_temp = NULL;

	search = rb_tree_find_node_geq(&idr->idr_tree, &min_id);
	while ((search != NULL) && (search->in_index == want_id)) {
		search = rb_tree_iterate(&idr->idr_tree, search, RB_DIR_RIGHT);
		want_id++;
	}

	node->in_index = want_id;
	node->in_data = data;

	collision = rb_tree_insert_node(&idr->idr_tree, node);
	KASSERT(collision == node);

	rw_exit(&idr->idr_lock);

	return 0;
}
