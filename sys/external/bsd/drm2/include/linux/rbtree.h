/*	$NetBSD: rbtree.h,v 1.13 2021/12/19 11:45:13 riastradh Exp $	*/

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

#ifndef _LINUX_RBTREE_H_
#define _LINUX_RBTREE_H_

#include <sys/rbtree.h>

#include <lib/libkern/libkern.h>

struct rb_root {
	struct rb_tree	rbr_tree;
};

struct rb_root_cached {
	struct rb_root	rb_root; /* Linux API name */
};

#define	rb_entry(P, T, F) container_of(P, T, F)
#define	rb_entry_safe(P, T, F)						      \
({									      \
	__typeof__(P) __p = (P);					      \
	__p ? container_of(__p, T, F) : NULL;				      \
})

static inline bool
RB_EMPTY_ROOT(struct rb_root *root)
{

	return RB_TREE_MIN(&root->rbr_tree) == NULL;
}

static inline struct rb_node *
rb_first(struct rb_root *root)
{
	char *vnode = RB_TREE_MIN(&root->rbr_tree);

	if (vnode)
		vnode += root->rbr_tree.rbt_ops->rbto_node_offset;
	return (struct rb_node *)vnode;
}

static inline struct rb_node *
rb_next2(struct rb_root *root, struct rb_node *rbnode)
{
	char *vnode = (char *)rbnode;

	vnode -= root->rbr_tree.rbt_ops->rbto_node_offset;
	return RB_TREE_NEXT(&root->rbr_tree, vnode);
}

static inline struct rb_node *
rb_last(struct rb_root *root)
{
	char *vnode = RB_TREE_MAX(&root->rbr_tree);

	if (vnode)
		vnode += root->rbr_tree.rbt_ops->rbto_node_offset;
	return (struct rb_node *)vnode;
}

static inline struct rb_node *
rb_first_cached(struct rb_root_cached *root)
{
	return rb_first(&root->rb_root);
}

static inline void
rb_erase(struct rb_node *rbnode, struct rb_root *root)
{
	struct rb_tree *tree = &root->rbr_tree;
	void *node = (char *)rbnode - tree->rbt_ops->rbto_node_offset;

	rb_tree_remove_node(tree, node);
}

static inline void
rb_erase_cached(struct rb_node *rbnode, struct rb_root_cached *root)
{
	rb_erase(rbnode, &root->rb_root);
}

static inline void
rb_replace_node(struct rb_node *old, struct rb_node *new, struct rb_root *root)
{
	void *vold = (char *)old - root->rbr_tree.rbt_ops->rbto_node_offset;
	void *vnew = (char *)new - root->rbr_tree.rbt_ops->rbto_node_offset;
	void *collision __diagused;

	rb_tree_remove_node(&root->rbr_tree, vold);
	collision = rb_tree_insert_node(&root->rbr_tree, vnew);
	KASSERT(collision == vnew);
}

static inline void
rb_replace_node_cached(struct rb_node *old, struct rb_node *new,
    struct rb_root_cached *root)
{
	rb_replace_node(old, new, &root->rb_root);
}

/*
 * XXX This is not actually postorder, but I can't fathom why you would
 * want postorder for an ordered tree; different insertion orders lead
 * to different traversal orders.
 */
#define	rbtree_postorder_for_each_entry_safe(NODE, TMP, ROOT, FIELD)	      \
	for ((NODE) = RB_TREE_MIN(&(ROOT)->rbr_tree);			      \
		((NODE) != NULL &&					      \
		    ((TMP) = rb_tree_iterate(&(ROOT)->rbr_tree, (NODE),	      \
			RB_DIR_RIGHT)));				      \
		(NODE) = (TMP))

#endif  /* _LINUX_RBTREE_H_ */
