/* $NetBSD: rb.c,v 1.7 2006/09/06 20:01:57 thorpej Exp $ */

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

#if !defined(_KERNEL) && !defined(_STANDALONE)
#include <sys/types.h>
#include <stddef.h>
#include <assert.h>
#include <stdbool.h>
#define	KASSERT(s)	assert(s)
#else
#include <lib/libkern/libkern.h>
#endif
#include "rb.h"

static void rb_tree_swap_nodes(struct rb_tree *, struct rb_node *, int);
static void rb_tree_insert_rebalance(struct rb_tree *, struct rb_node *);
static void rb_tree_removal_rebalance(struct rb_tree *, struct rb_node *,
	unsigned int);
#ifndef NDEBUG
static bool rb_tree_check_node(const struct rb_tree *, const struct rb_node *,
	const struct rb_node *, bool);
#endif

/*
 * Rather than testing for the NULL everywhere, all terminal leaves are
 * pointed to this node.  Note that by setting it to be const, that on
 * some architectures trying to write to it will cause a fault.
 */
static const struct rb_node sentinel_node = { .rb_sentinel = 1 };

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
 * Swap the location and colors of 'self' and its child @ which.  The child
 * can not be a sentinel node.
 */
static void
rb_tree_swap_nodes(struct rb_tree *rbt, struct rb_node *old_father, int which)
{
	const unsigned int other = which ^ RB_OTHER;
	struct rb_node * const grandpa = old_father->rb_parent;
	struct rb_node * const old_child = old_father->rb_nodes[which];
	struct rb_node * const new_father = old_child;
	struct rb_node * const new_child = old_father;
	unsigned int properties;

	KASSERT(!RB_SENTINEL_P(old_child));
	KASSERT(old_child->rb_parent == old_father);

	KASSERT(rb_tree_check_node(rbt, old_father, NULL, false));
	KASSERT(rb_tree_check_node(rbt, old_child, NULL, false));
	KASSERT(RB_ROOT_P(old_father) || rb_tree_check_node(rbt, grandpa, NULL, false));

	/*
	 * Exchange descendant linkages.
	 */
	grandpa->rb_nodes[old_father->rb_position] = new_father;
	new_child->rb_nodes[which] = old_child->rb_nodes[other];
	new_father->rb_nodes[other] = new_child;

	/*
	 * Update ancestor linkages
	 */
	new_father->rb_parent = grandpa;
	new_child->rb_parent = new_father;

	/*
	 * Exchange properties between new_father and new_child.  The only
	 * change is that new_child's position is now on the other side.
	 */
	properties = old_child->rb_properties;
	new_father->rb_properties = old_father->rb_properties;
	new_child->rb_properties = properties;
	new_child->rb_position = other;

	/*
	 * Make sure to reparent the new child to ourself.
	 */
	if (!RB_SENTINEL_P(new_child->rb_nodes[which])) {
		new_child->rb_nodes[which]->rb_parent = new_child;
		new_child->rb_nodes[which]->rb_position = which;
	}

	KASSERT(rb_tree_check_node(rbt, new_father, NULL, false));
	KASSERT(rb_tree_check_node(rbt, new_child, NULL, false));
	KASSERT(RB_ROOT_P(new_father) || rb_tree_check_node(rbt, grandpa, NULL, false));
}

void
rb_tree_insert_node(struct rb_tree *rbt, struct rb_node *self)
{
	struct rb_node *prev, *next, *parent, *tmp;
	unsigned int position;

	self->rb_properties = 0;
	prev = NULL;
	next = NULL;
	tmp = rbt->rbt_root;
	parent = (struct rb_node *)&rbt->rbt_root;
	position = RB_LEFT;

	/*
	 * Find out where to place this new leaf.
	 */
	while (!RB_SENTINEL_P(tmp)) {
		const int diff = (*rbt->rbt_compare_nodes)(tmp, self);
		parent = tmp;
		KASSERT(diff != 0);
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
	if (prev != NULL && !RB_SENTINEL_P(prev) && next == NULL)
		next = TAILQ_NEXT(prev, rb_link);
	if (prev == NULL && (next != NULL && !RB_SENTINEL_P(next)))
		next = TAILQ_PREV(next, rb_node_qh, rb_link);
	KASSERT(prev == NULL || RB_SENTINEL_P(prev) || (*rbt->rbt_compare_nodes)(prev, self) > 0);
	KASSERT(next == NULL || RB_SENTINEL_P(next) || (*rbt->rbt_compare_nodes)(self, next) < 0);

	/*
	 * Initialize the node and insert as a leaf into the tree.
	 */
	self->rb_parent = parent;
	self->rb_position = position;
	if (__predict_false(parent == (struct rb_node *) &rbt->rbt_root)) {
		RB_MARK_ROOT(self);
	} else {
		KASSERT(position == RB_LEFT || position == RB_RIGHT);
		KASSERT(!RB_ROOT_P(self)); 	/* Already done */
	}
	KASSERT(RB_SENTINEL_P(parent->rb_nodes[position]));
	self->rb_left = parent->rb_nodes[position];
	self->rb_right = parent->rb_nodes[position];
	parent->rb_nodes[position] = self;
	KASSERT(self->rb_left == &sentinel_node &&
	    self->rb_right == &sentinel_node);

	/*
	 * Insert the new node into a sorted list for easy sequential access
	 */
	rbt->rbt_count++;
	if (next != NULL && !RB_SENTINEL_P(next)) {
		TAILQ_INSERT_BEFORE(next, self, rb_link);
	} else {
		TAILQ_INSERT_TAIL(&rbt->rbt_nodes, self, rb_link);
	}

#if 0
	/*
	 * Validate the tree before we rebalance
	 */
	rb_tree_check(rbt, false);
#endif

	/*
	 * Rebalance tree after insertion
	 */
	rb_tree_insert_rebalance(rbt, self);

#if 0
	/*
	 * Validate the tree after we rebalanced
	 */
	rb_tree_check(rbt, true);
#endif
}

static void
rb_tree_insert_rebalance(struct rb_tree *rbt, struct rb_node *self)
{
	RB_MARK_RED(self);

	while (!RB_ROOT_P(self) && RB_RED_P(self->rb_parent)) {
		const unsigned int which =
		     (self->rb_parent == self->rb_parent->rb_parent->rb_left
			? RB_LEFT
			: RB_RIGHT);
		const unsigned int other = which ^ RB_OTHER;
		struct rb_node * father = self->rb_parent;
		struct rb_node * grandpa = father->rb_parent;
		struct rb_node * const uncle = grandpa->rb_nodes[other];

		KASSERT(!RB_SENTINEL_P(self));
		/*
		 * We are red and our parent is red, therefore we must have a
		 * grandfather and he must be black.
		 */
		KASSERT(RB_RED_P(self)
			&& RB_RED_P(father)
			&& RB_BLACK_P(grandpa));

		if (RB_RED_P(uncle)) {
			/*
			 * Case 1: our uncle is red
			 *   Simply invert the colors of our parent and
			 *   uncle and make our grandparent red.  And
			 *   then solve the problem up at his level.
			 */
			RB_MARK_BLACK(uncle);
			RB_MARK_BLACK(father);
			RB_MARK_RED(grandpa);
			self = grandpa;
			continue;
		}
		/*
		 * Case 2&3: our uncle is black.
		 */
		if (self == father->rb_nodes[other]) {
			/*
			 * Case 2: we are on the same side as our uncle
			 *   Swap ourselves with our parent so this case
			 *   becomes case 3.  Basically our parent becomes our
			 *   child.
			 */
			self = father;
			rb_tree_swap_nodes(rbt, self, other);
			father = self->rb_parent;
			grandpa = father->rb_parent;
		}
		KASSERT(RB_RED_P(self) && RB_RED_P(father));
		KASSERT(grandpa->rb_nodes[which] == father);
		/*
		 * Case 3: we are opposite a child of a black uncle.
		 *   Swap our parent and grandparent.  Since our grandfather
		 *   is black, our father will become black and our new sibling
		 *   (former grandparent) will become red.
		 */
		rb_tree_swap_nodes(rbt, grandpa, which);
		KASSERT(RB_RED_P(self) && RB_BLACK_P(father));
		break;
	}

	/*
	 * Final step: Set the root to black.
	 */
	RB_MARK_BLACK(rbt->rbt_root);
}

struct rb_node *
rb_tree_find(struct rb_tree *rbt, void *key)
{
	struct rb_node *parent = rbt->rbt_root;

	while (!RB_SENTINEL_P(parent)) {
		const int diff = (*rbt->rbt_compare_key)(parent, key);
		if (diff == 0)
			return parent;
		parent = parent->rb_nodes[diff > 0];
	}

	return NULL;
}

/*
 *
 */
void
rb_tree_remove_node(struct rb_tree *rbt, struct rb_node *self)
{
	/*
	 * Easy case, there are no children.  This means this node is
	 * at rank [N] (the bottom) of the red-black tree.  We can simple just
	 * prune it.  This leave the problem of identifying the node removed.
	 * Since the node "left" is a sentinel it doesn't carry any information.
	 * Instead we identify the removed node by parent and position.
	 */
	if (RB_CHILDLESS_P(self)) {
		KASSERT(rb_tree_check_node(rbt, self, NULL, false));
		self->rb_parent->rb_nodes[self->rb_position] = self->rb_left;
		if (RB_BLACK_P(self) && !RB_ROOT_P(self))
			rb_tree_removal_rebalance(rbt,
				    self->rb_parent, self->rb_position);
		KASSERT(RB_ROOT_P(self) || rb_tree_check_node(rbt, self->rb_parent, NULL, true));
		goto done;
	}


	/*
	 * If this node only has one valid child, that child must be itself
	 * childless.  This is because a red-black tree only has leaves at
	 * two ranks, [N-1] and [N].  If this node only has one child at this
	 * rank, this must be rank [N-1] since all nodes at rank [N] are
	 * childless.  Thus our child must be at rank [N] and therevore must
	 * be childless.
	 *
	 * Promoting this node's child to rank [N-1] and giving this node's
	 * color allows us to pretend we really removed a node of rank [N]. 
	 * Now if the child was red, even better since we can just return
	 * knowing that removing a red node from a red-black tree still
	 * leaves a valid red-black tree.
	 *
	 *        D        \         D         \         D
	 *    B       F     >    B       E      >    B       E
	 *  A   C   E   _  /   A   C   _   F   /   A   C   _   x
	 *
	 * We are removing F, but F has a child E.  So we swap E and F (in
	 * theory but we really replace F by E thereby removing F from the
	 * tree).
	 */
	if (!RB_TWOCHILDREN_P(self)) {
		const unsigned int which = RB_LEFT_SENTINEL_P(self) ? RB_RIGHT : RB_LEFT;
		const unsigned int other = which ^ RB_OTHER;
		struct rb_node * const new_self = self->rb_nodes[which];
		const bool was_black = RB_BLACK_P(new_self);

		KASSERT(RB_CHILDLESS_P(new_self));
		KASSERT(rb_tree_check_node(rbt, new_self, NULL, false));

		/*
		 * Copy self to new_self.
		 */
		self->rb_parent->rb_nodes[self->rb_position] = new_self;
		new_self->rb_parent = self->rb_parent;
		new_self->rb_properties = self->rb_properties;
		new_self->rb_nodes[which] = new_self->rb_nodes[other];
		KASSERT(rb_tree_check_node(rbt, new_self, NULL, false));
		if (was_black) {
			/*
			 * Now rebalance
			 */
			rb_tree_removal_rebalance(rbt, new_self, other);
		} else {
			/*
			 * If new_self was red, then self must have been black.
			 */
			KASSERT(RB_BLACK_P(self));
			/* RB_MARK_BLACK(new_self); */
		}
		KASSERT(rb_tree_check_node(rbt, new_self, NULL, true));
		goto done;
	}

	/*
	 * The node to be removed is in the interior of the red-black tree.
	 * This results in a difficulty in this red-black implementation
	 * since contrary to a traditional implemenation the interior nodes
	 * contain the keys while the leaves have no keys.
	 *
	 *        D              |             C
	 *   B         F         |        B         F
	 * A   C     E   G       |      A   x     E   G
	 * 1 2 3  4  5 6 7       |      1 2    3  5 6 7
	 *
	 * We need to remove D.  Whom do we swap with, C or E?  It doesn't
	 * matter since they are both at rank [N].  We put C as root and
	 * then pretend D was a child B.  Even though D's key would never
	 * have been valid at C, since it's now deleted it's key is
	 * irrevalent.  We simply claim to have deleted right(B).
	 *
	 *        D              |             E
	 *   B         F         |        B         F
	 * A         E   G       |      A         x   G
	 * 1 2    4  5 6 7       |      1 2    5    6 7
	 *
	 * We need to remove D.  Whom do we swap with?  E, of course.  E is
	 * the closest node of rank [N].  Always choose the highest rank you
	 * can.  We make E root and then pretend D was a child F.  Again D's
	 * key is irrevalent.  We simply claim to have deleted left(F).
	 *
	 *        D              |             D
	 *   B         F         |        B         G
	 * A             G       |      A         F
	 * 1 2    4    6 7       |      1 2    4  6 7
	 *
	 * We need to remove D.  Whom do we swap with?  B and F are equally
	 * bad since they both have one child.  So we swap one and that
	 * reduces to the previous case.
	 */
	if (RB_TWOCHILDREN_P(self)) {
		struct rb_node * const next = TAILQ_NEXT(self, rb_link);
		struct rb_node * const prev = TAILQ_PREV(self, rb_node_qh, rb_link);
		struct rb_node * new_self;
		struct rb_node * parent;
		unsigned int which;
		bool was_black;

		KASSERT(next != NULL && prev != NULL);
		KASSERT(!RB_TWOCHILDREN_P(next));
		KASSERT(!RB_TWOCHILDREN_P(prev));

		/*
		 * First, pick the childless victim.
		 * Now we two possible victims
		 * Otherwise, if root, pick from the side with more nodes. 
		 * Lastly, pick from the side closest to the root.
		 */
		if (RB_CHILDLESS_P(next) && !RB_CHILDLESS_P(prev)) {
			new_self = next;
		} else if (RB_CHILDLESS_P(prev) && !RB_CHILDLESS_P(next)) {
			new_self = prev;
		} else if (RB_ROOT_P(self)) {
			new_self = prev;
		} else {
			new_self = next;
		}
		KASSERT(new_self != rbt->rbt_root);
		KASSERT(!RB_TWOCHILDREN_P(new_self));

		if (RB_CHILDLESS_P(new_self)) {
			KASSERT(rb_tree_check_node(rbt, new_self, NULL, false));
			which = new_self->rb_position;
			parent = new_self->rb_parent;
			was_black = RB_BLACK_P(new_self);

			parent->rb_nodes[which] = new_self->rb_left;
			KASSERT(rb_tree_check_node(rbt, parent, NULL, false));
		} else {
			/*
			 * New self has one child.  And we know it's on the
			 * opposite side than what we want.  So we want to
			 * replace new_self with it and then self with new_self.
			 */
			which = new_self->rb_position;
			parent = new_self->rb_nodes[which ^ RB_OTHER];
			was_black = RB_BLACK_P(parent);

			KASSERT(RB_SENTINEL_P(new_self->rb_nodes[which]));
			KASSERT(!RB_SENTINEL_P(parent));
			KASSERT(RB_CHILDLESS_P(parent));

			/*
			 * Move parent to new_self's location in the tree.
			 */
			new_self->rb_parent->rb_nodes[new_self->rb_position] = parent;
			parent->rb_parent = new_self->rb_parent;
			parent->rb_properties = new_self->rb_properties;
			KASSERT(rb_tree_check_node(rbt, parent, NULL, false));
		}

		/*
		 * Move new_self to our location in the tree.
		 */
		self->rb_parent->rb_nodes[self->rb_position] = new_self;
		new_self->rb_left = self->rb_left;
		new_self->rb_right = self->rb_right;
		new_self->rb_parent = self->rb_parent;
		new_self->rb_properties = self->rb_properties;
		new_self->rb_left->rb_parent = new_self;
		new_self->rb_right->rb_parent = new_self;
		KASSERT(rb_tree_check_node(rbt, new_self, NULL, false));

		/*
		 * Rebalance if the node was 
		 */
		if (was_black)
			rb_tree_removal_rebalance(rbt, parent, which);
	}

  done:
	/*
	 * Remove ourselves from the node list and decrement the count.
	 */
	TAILQ_REMOVE(&rbt->rbt_nodes, self, rb_link);
	rbt->rbt_count--;
}

static void
rb_tree_removal_rebalance(struct rb_tree *rbt, struct rb_node *parent,
	unsigned int which)
{
	KASSERT(!RB_SENTINEL_P(parent));
	KASSERT(RB_SENTINEL_P(parent->rb_nodes[which]));

	while (RB_BLACK_P(parent->rb_nodes[which])) {
		unsigned int other = which ^ RB_OTHER;
		struct rb_node *brother = parent->rb_nodes[other];

		KASSERT(!RB_SENTINEL_P(brother));
		/*
		 * For cases 1, 2a, and 2b, our brother's children must
		 * be black.
		 */
		if (RB_BLACK_P(brother->rb_left)
		    && RB_BLACK_P(brother->rb_right)) {
			/*
			 * Case 1: Our brother is red, swap its position
			 * (and colors) with our parent.  This is now case 2b.
			 *
			 *    B         ->        D
			 *  x     d     ->    b     E
			 *      C   E   ->  x   C
			 */
			if (RB_RED_P(brother)) {
				KASSERT(RB_BLACK_P(parent));
				rb_tree_swap_nodes(rbt, parent, other);
				brother = parent->rb_nodes[other];
				KASSERT(RB_BLACK_P(parent->rb_nodes[which]));
				KASSERT(!RB_SENTINEL_P(brother));
				KASSERT(RB_RED_P(parent));
				KASSERT(RB_BLACK_P(brother->rb_left)
					&& RB_BLACK_P(brother->rb_right));
			}
			if (RB_BLACK_P(parent)) {
				/*
				 * Both our parent and brother are black.
				 * Change our brother to red, advance up rank
				 * and go through the loop again.
				 *
				 *    B         ->    B
				 *  A     D     ->  A     d
				 *      C   E   ->      C   E
				 */
				RB_MARK_RED(brother);
				if (RB_ROOT_P(parent))
					return;
				KASSERT(rb_tree_check_node(rbt, brother, NULL, false));
				KASSERT(rb_tree_check_node(rbt, parent, NULL, false));
				which = parent->rb_position;
				parent = parent->rb_parent;
			} else {
				KASSERT(RB_BLACK_P(brother));
				RB_MARK_BLACK(parent);
				RB_MARK_RED(brother);
				KASSERT(rb_tree_check_node(rbt, brother, NULL, true));
				KASSERT(rb_tree_check_node(rbt, parent, NULL, true));
				return;
			}
		} else {
			KASSERT(RB_BLACK_P(brother));
			KASSERT(!RB_CHILDLESS_P(brother));
			/*
			 * Case 3: our brother is black, our left nephew is
			 * red, and our right nephew is black.  Swap our
			 * brother with our left nephew.   This result in a
			 * tree that matches case 4.
			 *
			 *     B         ->       D
			 * A       D     ->   B     E
			 *       c   e   -> A   C
			 */
			if (RB_BLACK_P(brother->rb_nodes[other])) {
				KASSERT(RB_RED_P(brother->rb_nodes[which]));
				rb_tree_swap_nodes(rbt, brother, which);
				brother = parent->rb_nodes[other];
				KASSERT(RB_RED_P(brother->rb_nodes[other]));
			}
			/*
			 * Case 4: our brother is black and our right nephew
			 * is red.  Swap our parent and brother locations and
			 * change our right nephew to black.  (these can be
			 * done in either order so we change the color first).
			 * The result is a valid red-black tree and is a
			 * terminal case.
			 *
			 *     B         ->       D
			 * A       D     ->   B     E
			 *       c   e   -> A   C
			 */
			RB_MARK_BLACK(brother->rb_nodes[other]);
			rb_tree_swap_nodes(rbt, parent, other);
			KASSERT(rb_tree_check_node(rbt, parent, NULL, true));
			return;
		}
	}
	KASSERT(rb_tree_check_node(rbt, parent, NULL, true));
}

#ifndef NDEBUG
static bool
rb_tree_check_node(const struct rb_tree *rbt, const struct rb_node *self,
	const struct rb_node *prev, bool red_check)
{
	KASSERT(!self->rb_sentinel);
	KASSERT(self->rb_left);
	KASSERT(self->rb_right);
	KASSERT(prev == NULL || (*rbt->rbt_compare_nodes)(prev, self) > 0);

	/*
	 * Verify our relationship to our parent.
	 */
	if (RB_ROOT_P(self)) {
		KASSERT(self == rbt->rbt_root);
		KASSERT(self->rb_position == RB_LEFT);
		KASSERT(self->rb_parent->rb_nodes[RB_LEFT] == self);
		KASSERT(self->rb_parent == (const struct rb_node *) &rbt->rbt_root);
	} else {
		KASSERT(self != rbt->rbt_root);
		KASSERT(!RB_PARENT_SENTINEL_P(self));
		if (self->rb_position == RB_LEFT) {
			KASSERT((*rbt->rbt_compare_nodes)(self, self->rb_parent) > 0);
			KASSERT(self->rb_parent->rb_nodes[RB_LEFT] == self);
		} else {
			KASSERT((*rbt->rbt_compare_nodes)(self, self->rb_parent) < 0);
			KASSERT(self->rb_parent->rb_nodes[RB_RIGHT] == self);
		}
	}

	/*
	 * The root must be black.
	 * There can never be two adjacent red nodes. 
	 */
	if (red_check) {
		KASSERT(!RB_ROOT_P(self) || RB_BLACK_P(self));
		if (RB_RED_P(self)) {
			KASSERT(!RB_ROOT_P(self));
			KASSERT(RB_BLACK_P(self->rb_parent));
			KASSERT(RB_LEFT_SENTINEL_P(self)
				|| RB_BLACK_P(self->rb_left));
			KASSERT(RB_RIGHT_SENTINEL_P(self)
				|| RB_BLACK_P(self->rb_right));
		}
		/*
		 * A grandparent's children must be real nodes and not
		 * sentinels.  First check out grandparent.
		 */
		KASSERT(RB_ROOT_P(self)
			|| RB_ROOT_P(self->rb_parent)
			|| RB_TWOCHILDREN_P(self->rb_parent->rb_parent));
		/*
		 * If we are have grandchildren on our left, then
		 * we must have a child on our right.
		 */
		KASSERT(RB_LEFT_SENTINEL_P(self)
			|| RB_CHILDLESS_P(self->rb_left)
			|| !RB_RIGHT_SENTINEL_P(self));
		/*
		 * If we are have grandchildren on our right, then
		 * we must have a child on our left.
		 */
		KASSERT(RB_RIGHT_SENTINEL_P(self)
			|| RB_CHILDLESS_P(self->rb_right)
			|| !RB_LEFT_SENTINEL_P(self));
	}

	return true;
}

void
rb_tree_check(const struct rb_tree *rbt, bool red_check)
{
	const struct rb_node *self;
	const struct rb_node *prev;
	int counts[2] = { 0, 0 };
	unsigned int which = RB_LEFT;

	KASSERT(rbt->rbt_root == NULL || rbt->rbt_root->rb_position == RB_LEFT);

	prev = NULL;
	TAILQ_FOREACH(self, &rbt->rbt_nodes, rb_link) {
		rb_tree_check_node(rbt, self, prev, false);
		if (self == rbt->rbt_root) {
			which = RB_RIGHT;
		} else {
			counts[which]++;
		}
		prev = self;
	}
	KASSERT(rbt->rbt_count == counts[RB_RIGHT] + counts[RB_LEFT] + (which == RB_RIGHT));

	/*
	 * The root must be black.
	 * There can never be two adjacent red nodes. 
	 */
	if (red_check) {
		KASSERT(rbt->rbt_root == NULL || RB_BLACK_P(rbt->rbt_root));
		TAILQ_FOREACH(self, &rbt->rbt_nodes, rb_link) {
			rb_tree_check_node(rbt, self, NULL, true);
		}
	}
}
#endif
