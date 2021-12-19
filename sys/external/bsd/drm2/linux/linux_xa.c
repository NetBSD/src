/*	$NetBSD: linux_xa.c,v 1.3 2021/12/19 12:05:25 riastradh Exp $	*/

/*-
 * Copyright (c) 2021 The NetBSD Foundation, Inc.
 * All rights reserved.
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
__KERNEL_RCSID(0, "$NetBSD: linux_xa.c,v 1.3 2021/12/19 12:05:25 riastradh Exp $");

/*
 * This is a lame-o implementation of the Linux xarray data type, which
 * implements a map from 64-bit integers to pointers.  The operations
 * it supports are designed to be implemented by a radix tree, but
 * NetBSD's radixtree(9) doesn't quite support them all, and it's a bit
 * of work to implement them, so this just uses a red/black tree
 * instead at the cost of some performance in certain types of lookups
 * (and negative-lookups -- finding a free key).
 */

#include <sys/rbtree.h>

#include <linux/xarray.h>

struct node {
	struct rb_node	n_rb;
	uint64_t	n_key;
	void		*n_datum;
};

static int
compare_nodes(void *cookie, const void *va, const void *vb)
{
	const struct node *a = va, *b = vb;

	if (a->n_key < b->n_key)
		return -1;
	if (a->n_key > b->n_key)
		return +1;
	return 0;
}

static int
compare_node_key(void *cookie, const void *vn, const void *vk)
{
	const struct node *n = vn;
	const uint64_t *k = vk;

	if (n->n_key < *k)
		return -1;
	if (n->n_key > *k)
		return +1;
	return 0;
}

static const rb_tree_ops_t xa_rb_ops = {
	.rbto_compare_nodes = compare_nodes,
	.rbto_compare_key = compare_node_key,
	.rbto_node_offset = offsetof(struct node, n_rb),
};

const struct xa_limit xa_limit_32b = { .min = 0, .max = UINT32_MAX };

void
xa_init_flags(struct xarray *xa, gfp_t gfp)
{

	mutex_init(&xa->xa_lock, MUTEX_DEFAULT, IPL_VM);
	rb_tree_init(&xa->xa_tree, &xa_rb_ops);
	xa->xa_gfp = gfp;
}

void
xa_destroy(struct xarray *xa)
{
	struct node *n;

	/*
	 * Linux allows xa to remain populated on destruction; it is
	 * our job to free any internal node structures.
	 */
	while ((n = RB_TREE_MIN(&xa->xa_tree)) != NULL) {
		rb_tree_remove_node(&xa->xa_tree, n);
		kmem_free(n, sizeof(*n));
	}
	mutex_destroy(&xa->xa_lock);
}

void *
xa_load(struct xarray *xa, unsigned long key)
{
	const uint64_t key64 = key;
	struct node *n;

	/* XXX pserialize */
	mutex_enter(&xa->xa_lock);
	n = rb_tree_find_node(&xa->xa_tree, &key64);
	mutex_exit(&xa->xa_lock);

	return n ? n->n_datum : NULL;
}

void *
xa_store(struct xarray *xa, unsigned long key, void *datum, gfp_t gfp)
{
	struct node *n, *collision;

	KASSERT(datum != NULL);
	KASSERT(((uintptr_t)datum & 0x3) == 0);

	n = kmem_zalloc(sizeof(*n), gfp & __GFP_WAIT ? KM_SLEEP : KM_NOSLEEP);
	if (n == NULL)
		return XA_ERROR(-ENOMEM);
	n->n_key = key;
	n->n_datum = datum;

	mutex_enter(&xa->xa_lock);
	collision = rb_tree_insert_node(&xa->xa_tree, n);
	mutex_exit(&xa->xa_lock);

	if (collision != n) {
		datum = collision->n_datum;
		kmem_free(collision, sizeof(*collision));
	}
	return datum;
}

int
xa_alloc(struct xarray *xa, uint32_t *idp, void *datum, struct xa_limit limit,
    gfp_t gfp)
{
	uint64_t key64 = limit.min;
	struct node *n, *n1, *collision __diagused;
	int error;

	KASSERTMSG(limit.min < limit.max, "min=%"PRIu32" max=%"PRIu32,
	    limit.min, limit.max);

	n = kmem_zalloc(sizeof(*n), gfp & __GFP_WAIT ? KM_SLEEP : KM_NOSLEEP);
	if (n == NULL)
		return -ENOMEM;
	n->n_datum = datum;

	mutex_enter(&xa->xa_lock);
	while ((n1 = rb_tree_find_node_geq(&xa->xa_tree, &key64)) != NULL &&
	    n1->n_key == key64) {
		if (key64 == limit.max) {
			error = -EBUSY;
			goto out;
		}
		KASSERT(key64 < UINT32_MAX);
		key64++;
	}
	/* Found a hole -- insert in it.  */
	KASSERT(n1 == NULL || n1->n_key > key64);
	n->n_key = key64;
	collision = rb_tree_insert_node(&xa->xa_tree, n);
	KASSERT(collision == n);
	error = 0;
out:	mutex_exit(&xa->xa_lock);

	if (error)
		return error;
	*idp = key64;
	return 0;
}

void *
xa_find(struct xarray *xa, unsigned long *startp, unsigned long max,
    unsigned tagmask)
{
	uint64_t key64 = *startp;
	struct node *n = NULL;

	KASSERT(tagmask == -1);	/* not yet supported */

	mutex_enter(&xa->xa_lock);
	n = rb_tree_find_node_geq(&xa->xa_tree, &key64);
	mutex_exit(&xa->xa_lock);

	if (n == NULL || n->n_key > max)
		return NULL;

	*startp = n->n_key;
	return n->n_datum;
}

void *
xa_find_after(struct xarray *xa, unsigned long *startp, unsigned long max,
    unsigned tagmask)
{
	unsigned long start = *startp + 1;
	void *found;

	if (start == max)
		return NULL;
	found = xa_find(xa, &start, max, tagmask);
	if (found)
		*startp = start;
	return found;
}

void *
xa_erase(struct xarray *xa, unsigned long key)
{
	uint64_t key64 = key;
	struct node *n;
	void *datum = NULL;

	mutex_enter(&xa->xa_lock);
	n = rb_tree_find_node(&xa->xa_tree, &key64);
	if (n)
		rb_tree_remove_node(&xa->xa_tree, n);
	mutex_exit(&xa->xa_lock);

	if (n) {
		datum = n->n_datum;
		kmem_free(n, sizeof(*n));
	}
	return datum;
}
