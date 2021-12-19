/*	$NetBSD: rbtree.h,v 1.6 2021/12/19 01:44:26 riastradh Exp $	*/

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

struct rb_root {
	struct rb_tree	rbr_tree;
};

struct rb_root_cached {
	struct rb_root	rbrc_root;
};

static inline bool
RB_EMPTY_ROOT(struct rb_root *root)
{

	return RB_TREE_MIN(&root->rbr_tree) == NULL;
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
	rb_erase(rbnode, &root->rbrc_root);
}

#endif  /* _LINUX_RBTREE_H_ */
