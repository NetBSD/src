/*	$NetBSD: rbtree.h,v 1.18 2022/02/27 14:18:34 riastradh Exp $	*/

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

/*
 * Several of these functions take const inputs and return non-const
 * outputs.  That is a deliberate choice.  It would be better if these
 * functions could be const-polymorphic -- return const if given const,
 * return non-const if given non-const -- but C doesn't let us express
 * that.  We are using them to adapt Linux code that is defined in
 * terms of token-substitution macros, without types of their own,
 * which happen to work out with both const and non-const variants.
 * Presumably the Linux code compiles upstream and has some level of
 * const type-checking in Linux, so this abuse of __UNCONST does not
 * carry substantial risk over to this code here.
 */

static inline bool
RB_EMPTY_ROOT(const struct rb_root *root)
{

	return RB_TREE_MIN(__UNCONST(&root->rbr_tree)) == NULL;
}

static inline struct rb_node *
rb_first(const struct rb_root *root)
{
	char *vnode = RB_TREE_MIN(__UNCONST(&root->rbr_tree));

	if (vnode)
		vnode += root->rbr_tree.rbt_ops->rbto_node_offset;
	return (struct rb_node *)vnode;
}

static inline struct rb_node *
rb_next2(const struct rb_root *root, const struct rb_node *rbnode)
{
	char *vnode = (char *)__UNCONST(rbnode);

	vnode -= root->rbr_tree.rbt_ops->rbto_node_offset;
	vnode = RB_TREE_NEXT(__UNCONST(&root->rbr_tree), vnode);
	if (vnode)
		vnode += root->rbr_tree.rbt_ops->rbto_node_offset;
	return (struct rb_node *)vnode;
}

static inline struct rb_node *
rb_last(const struct rb_root *root)
{
	char *vnode = RB_TREE_MAX(__UNCONST(&root->rbr_tree));

	if (vnode)
		vnode += root->rbr_tree.rbt_ops->rbto_node_offset;
	return (struct rb_node *)vnode;
}

static inline struct rb_node *
rb_first_cached(const struct rb_root_cached *root)
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
 * This violates the abstraction of rbtree(3) for postorder traversal
 * -- children first, then parents -- so it is safe for cleanup code
 * that just frees all the nodes without removing them from the tree.
 */
static inline struct rb_node *
rb_first_postorder(const struct rb_root *root)
{
	struct rb_node *node, *child;

	if ((node = root->rbr_tree.rbt_root) == NULL)
		return NULL;
	for (;; node = child) {
		if ((child = node->rb_left) != NULL)
			continue;
		if ((child = node->rb_right) != NULL)
			continue;
		return node;
	}
}

static inline struct rb_node *
rb_next2_postorder(const struct rb_root *root, struct rb_node *node)
{
	struct rb_node *parent, *child;

	if (node == NULL)
		return NULL;

	/*
	 * If we're at the root, there are no more siblings and no
	 * parent, so post-order iteration is done.
	 */
	if (RB_ROOT_P(&root->rbr_tree, node))
		return NULL;
	parent = RB_FATHER(node);	/* kinda sexist, innit */
	KASSERT(parent != NULL);

	/*
	 * If we're the right child, we've already processed the left
	 * child (which may be gone by now), so just return the parent.
	 */
	if (RB_RIGHT_P(node))
		return parent;

	/*
	 * Otherwise, move down to the leftmost child of our right
	 * sibling -- or return the parent if there is none.
	 */
	if ((node = parent->rb_right) == NULL)
		return parent;
	for (;; node = child) {
		if ((child = node->rb_left) != NULL)
			continue;
		if ((child = node->rb_right) != NULL)
			continue;
		return node;
	}
}

/*
 * Extension to Linux API, which allows copying a struct rb_root object
 * with `=' or `memcpy' and no additional relocation.
 */
static inline void
rb_move(struct rb_root *to, struct rb_root *from)
{
	struct rb_node *root;

	*to = *from;
	memset(from, 0, sizeof(*from)); /* paranoia */
	if ((root = to->rbr_tree.rbt_root) == NULL)
		return;

	/*
	 * The root node's `parent' is a strict-aliasing-unsafe hack
	 * pointing at the root of the tree.
	 */
	RB_SET_FATHER(root, (struct rb_node *)(void *)&to->rbr_tree.rbt_root);
}

#define	rbtree_postorder_for_each_entry_safe(ENTRY, TMP, ROOT, FIELD)	      \
	for ((ENTRY) = rb_entry_safe(rb_first_postorder(ROOT),		      \
		    __typeof__(*(ENTRY)), FIELD);			      \
		((ENTRY) != NULL &&					      \
		    ((TMP) = rb_entry_safe(rb_next2_postorder((ROOT),	      \
			    &(ENTRY)->FIELD), __typeof__(*(ENTRY)), FIELD),   \
			1));						      \
		(ENTRY) = (TMP))

#endif  /* _LINUX_RBTREE_H_ */
