/*-
 * Copyright (c) 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Matt Thomas <matt@3am-software.com>.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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

#include <sys/types.h>
#include <stddef.h>
#ifndef _KERNEL
#include <assert.h>
#define	KASSERT(s)	assert(s)
#endif
#include "rb.h"

static void rb_tree_rotate(struct rb_tree *, struct rb_node *, int);
static void rb_tree_insert_rebalance(struct rb_tree *, struct rb_node *);
static void rb_tree_removal_rebalance(struct rb_tree *, struct rb_node *);

/*
 * Rather than testing for the NULL everywhere, all terminal leaves are
 * pointed to this node.  Note that by setting it to be const, that on
 * some architectures trying to write to it will cause a fault.
 */
static const struct rb_node sentinel_node = {
	NULL, NULL, NULL, { NULL, NULL }, 0, 0, 1
};

void
rb_tree_init(struct rb_tree *rbt, rb_compare_nodes_fn compare_nodes,
	rb_compare_key_fn compare_key, rb_print_node_fn print_node)
{
	TAILQ_INIT(&rbt->rbt_nodes);
	rbt->rbt_count = 0;
	rbt->rbt_compare_nodes = compare_nodes;
	rbt->rbt_compare_key = compare_key;
	rbt->rbt_print_node = print_node;
	*((const struct rb_node **)&rbt->rbt_root) = &sentinel_node;
}

/*
 * Rotate the tree between     Y   and    X
 *                           X   c       a   Y
 *                          a b             b c
 */
void
rb_tree_rotate(struct rb_tree *rbt, struct rb_node *self, int which)
{
	const int other = which ^ RB_OTHER;
	struct rb_node * const parent = self->rb_parent;
	struct rb_node * const child = self->rb_nodes[other];
	
	assert(!child->rb_sentinel);
	assert(child->rb_parent == self);
#if 0
	(*rbt->rbt_print_node)(child, which ? "before-l " : "before-r ");
#endif

	if ((child->rb_parent = parent) == NULL) {
		assert(rbt->rbt_root == self);
		rbt->rbt_root = child;
	} else {
		parent->rb_nodes[self->rb_position] = child;
	}

	self->rb_nodes[other] = child->rb_nodes[which];
	if (!self->rb_nodes[other]->rb_sentinel)
		self->rb_nodes[other]->rb_parent = self;
	child->rb_nodes[which] = self;
	child->rb_position = self->rb_position;
	self->rb_parent = child;
	self->rb_position = which;
#if 0
	(*rbt->rbt_print_node)(self, which ? "after-l " : "after-r ");
#endif
}

void
rb_tree_insert_node(struct rb_tree *rbt, struct rb_node *self)
{
	struct rb_node *prev, *next, *tmp, *parent;
	struct rb_node **insert_p;
	u_int position;

	prev = NULL;
	next = NULL;
	parent = NULL;
	tmp = rbt->rbt_root;	

	/*
	 * Find out where to place this new leaf.
	 */
	while (!tmp->rb_sentinel) {
		const int diff = (*rbt->rbt_compare_nodes)(tmp, self);
		parent = tmp;
		assert(diff != 0);
		if (diff < 0) {
			position = RB_LEFT;
			next = parent->rb_left;
			prev = NULL;
		} else {
			position = RB_RIGHT;
			prev = parent->rb_right;
			next = NULL;
		}
		tmp = parent->rb_nodes[position];
	}

	/*
	 * Verify our sequential position
	 */
	if (prev != NULL && next == NULL)
		next = TAILQ_NEXT(prev, rb_link);
	if (prev == NULL && next != NULL)
		next = TAILQ_PREV(next, rb_node_qh, rb_link);
	assert(prev == NULL || (*rbt->rbt_compare_nodes)(prev, self) > 0);
	assert(next == NULL || (*rbt->rbt_compare_nodes)(self, next) < 0);

	/*
	 * Initialize the node and insert as a leaf into the tree.
	 */
	rbt->rbt_count++;
	self->rb_parent = parent;
	if (parent == NULL) {
		assert(rbt->rbt_root->rb_sentinel);
		self->rb_position = RB_PARENT;
		self->rb_left = rbt->rbt_root;
		self->rb_right = rbt->rbt_root;
		rbt->rbt_root = self;
	} else {
		self->rb_position = position;
		self->rb_left = parent->rb_nodes[position];
		self->rb_right = parent->rb_nodes[position];
		parent->rb_nodes[position] = self;
	}
	assert(self->rb_left == &sentinel_node &&
	    self->rb_right == &sentinel_node);

	/*
	 * Insert the new node into a sorted list for easy sequential access
	 */
	if (next != NULL) {
		TAILQ_INSERT_BEFORE(next, self, rb_link);
	} else {
		TAILQ_INSERT_TAIL(&rbt->rbt_nodes, self, rb_link);
	}

	/*
	 * Rebalance tree after insertation
	 */
	rb_tree_insert_rebalance(rbt, self);
}

void
rb_tree_insert_rebalance(struct rb_tree *rbt, struct rb_node *self)
{
	self->rb_red = 1;

	while (self != rbt->rbt_root && self->rb_parent->rb_red) {
		const int which =
		     (self->rb_parent == self->rb_parent->rb_parent->rb_left
			? RB_LEFT
			: RB_RIGHT);
		const int other = which ^ RB_OTHER;
		struct rb_node *uncle;

		assert(!self->rb_sentinel);
		/*
		 * We are red, our are parent is red, and our
		 * grandparent is black.
		 */
		uncle = self->rb_parent->rb_parent->rb_nodes[other];
		if (uncle->rb_red) {
			/*
			 * Case 1: our uncle is red
			 *   Simply invert the colors of our parent and
			 *   uncle and make our grandparent red.  And
			 *   then solve the problem up at his level.
			 */
			uncle->rb_red = 0;
			self->rb_parent->rb_red = 0;
			self->rb_parent->rb_parent->rb_red = 1;
			self = self->rb_parent->rb_parent;
		} else {
			/*
			 * Case 2&3: our uncle is black.
			 */
			if (self == self->rb_parent->rb_nodes[other]) {
				/*
				 * Case 2: we are on the same side as our uncle
				 *   Rotate parent away from uncle so this
				 *   case becomes case 3
				 */
				self = self->rb_parent;
				rb_tree_rotate(rbt, self, which);
			}
			/*
			 * Case 3: we are opposite a child of a black uncle.
			 *   Change parent to black and grandparent to
			 *   red.  Rotate grandparent away from ourself.
			 */
			self->rb_parent->rb_red = 0;
			self->rb_parent->rb_parent->rb_red = 1;
			rb_tree_rotate(rbt, self->rb_parent->rb_parent, other);
		}
	}

	/*
	 * Final step: Set the root to black.
	 */
	rbt->rbt_root->rb_red = 0;
}

struct rb_node *
rb_tree_lookup(struct rb_tree *rbt, void *key)
{
	struct rb_node *parent = rbt->rbt_root;

	while (!parent->rb_sentinel) {
		const int diff = (*rbt->rbt_compare_key)(parent, key);
		if (diff == 0)
			return parent;
		parent = parent->rb_nodes[diff > 0];
	}

	return NULL;
}

void
rb_tree_remove_node(struct rb_tree *rbt, struct rb_node *self)
{
	struct rb_node *child;
	/*
	 * Easy case, one or more children is NULL (leaf node or parent of
	 * leaf node).
	 */
	if (self->rb_left->rb_sentinel || self->rb_left->rb_sentinel) {
		const int which = (!self->rb_left->rb_sentinel ? RB_LEFT : RB_RIGHT);
		child = self->rb_nodes[which];
		if (self->rb_parent == NULL) {
			rbt->rbt_root = child;
		} else {
			self->rb_parent->rb_nodes[self->rb_position] = child;
		}
		if (child->rb_sentinel)
			child = self->rb_parent;
		else
			child->rb_parent = self->rb_parent;
	} else {
		struct rb_node *new_self;
		child = self->rb_right;
		while (!child->rb_left->rb_sentinel)
			child = child->rb_left;
		new_self = child;
		assert(new_self == TAILQ_NEXT(self, rb_link));
		
		/*
		 * Take new_self out of the tree (its only subnode can be on the
		 * right since we know the left subnode is NULL).
		 */
		child->rb_parent->rb_left = child->rb_right;
		if (!child->rb_right->rb_sentinel) {
			child->rb_right->rb_parent = child->rb_parent;
			child->rb_right->rb_position = RB_LEFT;
		}

		/*
		 * Take self out of the tree and insert new_self into its place.
		 */
		new_self->rb_right = self->rb_right;
		new_self->rb_left = self->rb_left;
		if (!new_self->rb_right->rb_sentinel)
			new_self->rb_right->rb_parent = new_self;
		if (!new_self->rb_left->rb_sentinel)
			new_self->rb_left->rb_parent = new_self;
		new_self->rb_red = self->rb_red;
		new_self->rb_parent = self->rb_parent;
		/*
		 * Update parent
		 */
		if (new_self->rb_parent == NULL) {
			rbt->rbt_root = new_self;
		} else {
			new_self->rb_parent->rb_nodes[self->rb_position] = new_self;
		}
	}
	TAILQ_REMOVE(&rbt->rbt_nodes, self, rb_link);
	rbt->rbt_count--;

	if (child != NULL) {
		assert(!child->rb_sentinel);
#if 0
		(*rbt->rbt_print_node)(child, "before ");
#endif
		rb_tree_removal_rebalance(rbt, child);
#if 0
		(*rbt->rbt_print_node)(child, "after ");
#endif
	}
}

void
rb_tree_removal_rebalance(struct rb_tree *rbt, struct rb_node *self)
{
	assert(!self->rb_sentinel);
	while (self->rb_parent != NULL && !self->rb_red) {
		struct rb_node *parent = self->rb_parent;
		int which = (self == parent->rb_left) ? RB_LEFT : RB_RIGHT;
		int other = which ^ RB_OTHER;
		struct rb_node *sibling = parent->rb_nodes[other];

		if (sibling->rb_red) {
			sibling->rb_red = 0;
			parent->rb_red = 1;
			rb_tree_rotate(rbt, parent, which);
			parent = self->rb_parent;
			sibling = parent->rb_nodes[other];
		}

		if (sibling->rb_sentinel ||
		    (!sibling->rb_left->rb_red && !sibling->rb_right->rb_red)) {
			sibling->rb_red = 1;
			self = parent;
			continue;
		}

		if (!sibling->rb_nodes[other]->rb_red) {
			sibling->rb_nodes[which]->rb_red = 0;
			sibling->rb_red = 1;
			rb_tree_rotate(rbt, sibling, other);
			parent = self->rb_parent;
			sibling = parent->rb_nodes[other];
		}

		sibling->rb_red = parent->rb_red;
		parent->rb_red = 0;
		sibling->rb_nodes[other]->rb_red = 0;
		rb_tree_rotate(rbt, parent, which);
		break;
	}
}
