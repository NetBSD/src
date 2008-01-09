/* $NetBSD: rb.c,v 1.11.26.1 2008/01/09 01:56:34 matt Exp $ */

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
#ifdef RBDEBUG
#define	KASSERT(s)	assert(s)
#else
#define KASSERT(s)	(void) 0
#endif
#else
#include <lib/libkern/libkern.h>
#endif
#include "rb.h"

static void rb_tree_insert_rebalance(struct rb_tree *, struct rb_node *);
static void rb_tree_removal_rebalance(struct rb_tree *, struct rb_node *,
	unsigned int);
#ifdef RBDEBUG
static const struct rb_node *rb_tree_iterate_const(const struct rb_tree *,
	const struct rb_node *, const unsigned int);
static bool rb_tree_check_node(const struct rb_tree *, const struct rb_node *,
	const struct rb_node *, bool);
#else
#define	rb_tree_check_node(a, b, c, d)	true
#endif

/*
 * Rather than testing for the NULL everywhere, all terminal leaves are
 * pointed to this node (and that includes itself).  Note that by setting
 * it to be const, that on some architectures trying to write to it will
 * cause a fault.
 */
static const struct rb_node sentinel_node = {
	.rb_nodes[0] = __UNCONST(&sentinel_node),
	.rb_nodes[1] = __UNCONST(&sentinel_node),
	.rb_sentinel = 1
};

void
rb_tree_init(struct rb_tree *rbt, const struct rb_tree_ops *ops)
{
	rbt->rbt_ops = ops;
	*((const struct rb_node **)&rbt->rbt_root) = &sentinel_node;
	RB_TAILQ_INIT(&rbt->rbt_nodes);
#ifndef RBSMALL
	rbt->rbt_minmax[RB_DIR_LEFT] = rbt->rbt_root;	/* minimum node */
	rbt->rbt_minmax[RB_DIR_RIGHT] = rbt->rbt_root;	/* maximum node */
#endif
#ifdef RBSTATS
	rbt->rbt_count = 0;
	rbt->rbt_insertions = 0;
	rbt->rbt_removals = 0;
	rbt->rbt_insertion_rebalance_calls = 0;
	rbt->rbt_insertion_rebalance_passes = 0;
	rbt->rbt_removal_rebalance_calls = 0;
	rbt->rbt_removal_rebalance_passes = 0;
#endif
}

struct rb_node *
rb_tree_find_node(struct rb_tree *rbt, const void *key)
{
	rb_compare_key_fn compare_key = rbt->rbt_ops->rb_compare_key;
	struct rb_node *parent = rbt->rbt_root;

	while (!RB_SENTINEL_P(parent)) {
		const signed int diff = (*compare_key)(parent, key);
		if (diff == 0)
			return parent;
		parent = parent->rb_nodes[diff > 0];
	}

	return NULL;
}
 
struct rb_node *
rb_tree_find_node_geq(struct rb_tree *rbt, const void *key)
{
	rb_compare_key_fn compare_key = rbt->rbt_ops->rb_compare_key;
	struct rb_node *parent = rbt->rbt_root;
	struct rb_node *last = NULL;

	while (!RB_SENTINEL_P(parent)) {
		const signed int diff = (*compare_key)(parent, key);
		if (diff == 0)
			return parent;
		if (diff < 0)
			last = parent;
		parent = parent->rb_nodes[diff > 0];
	}

	return last;
}
 
struct rb_node *
rb_tree_find_node_leq(struct rb_tree *rbt, const void *key)
{
	rb_compare_key_fn compare_key = rbt->rbt_ops->rb_compare_key;
	struct rb_node *parent = rbt->rbt_root;
	struct rb_node *last = NULL;

	while (!RB_SENTINEL_P(parent)) {
		const signed int diff = (*compare_key)(parent, key);
		if (diff == 0)
			return parent;
		if (diff > 0)
			last = parent;
		parent = parent->rb_nodes[diff > 0];
	}

	return last;
}

bool
rb_tree_insert_node(struct rb_tree *rbt, struct rb_node *self)
{
	rb_compare_nodes_fn compare_nodes = rbt->rbt_ops->rb_compare_nodes;
	struct rb_node *parent, *tmp;
	unsigned int position;
	bool rebalance;

	RBSTAT_INC(rbt->rbt_insertions);

	self->rb_sentinel = 0;		/* make sure this is zero */

	tmp = rbt->rbt_root;
	/*
	 * This is a hack.  Because rbt->rbt_root is just a struct rb_node *,
	 * just like rb_node->rb_nodes[RB_DIR_LEFT], we can use this fact to
	 * avoid a lot of tests for root and know that even at root,
	 * updating rb_node->rb_parent->rb_nodes[rb_node->rb_position] will
	 * rbt->rbt_root.
	 */
	parent = (struct rb_node *)&rbt->rbt_root;
	position = RB_DIR_LEFT;

	/*
	 * Find out where to place this new leaf.
	 */
	while (!RB_SENTINEL_P(tmp)) {
		const signed int diff = (*compare_nodes)(tmp, self);
		if (__predict_false(diff == 0)) {
			/*
			 * Node already exists; don't insert.
			 */
			return false;
		}
		parent = tmp;
		position = (diff > 0);
		tmp = parent->rb_nodes[position];
	}

#ifdef RBDEBUG
	{
		struct rb_node *prev = NULL, *next = NULL;

		if (position == RB_DIR_RIGHT)
			prev = parent;
		else if (tmp != rbt->rbt_root)
			next = parent;

		/*
		 * Verify our sequential position
		 */
		KASSERT(prev == NULL || !RB_SENTINEL_P(prev));
		KASSERT(next == NULL || !RB_SENTINEL_P(next));
		if (prev != NULL && next == NULL)
			next = TAILQ_NEXT(prev, rb_link);
		if (prev == NULL && next != NULL)
			prev = TAILQ_PREV(next, rb_node_qh, rb_link);
		KASSERT(prev == NULL || !RB_SENTINEL_P(prev));
		KASSERT(next == NULL || !RB_SENTINEL_P(next));
		KASSERT(prev == NULL || (*compare_nodes)(prev, self) > 0);
		KASSERT(next == NULL || (*compare_nodes)(self, next) > 0);
	}
#endif

	/*
	 * Initialize the node and insert as a leaf into the tree.
	 */
	self->rb_parent = parent;
	self->rb_position = position;
	RB_MARK_MOVED(self);
	if (__predict_false(parent == (struct rb_node *) &rbt->rbt_root)) {
		RB_MARK_ROOT(self);
		RB_MARK_BLACK(self);		/* root is always black */
#ifndef RBSMALL
		rbt->rbt_minmax[RB_DIR_LEFT] = self;
		rbt->rbt_minmax[RB_DIR_RIGHT] = self;
#endif
		rebalance = false;
	} else {
		KASSERT(position == RB_DIR_LEFT || position == RB_DIR_RIGHT);
#ifndef RBSMALL
		/*
		 * Keep track of the minimum and maximum nodes.  If our
		 * parent is a minmax node and we on their min/max side,
		 * we must be the new min/max node.
		 */
		RB_MARK_MOVED(parent);
		if (parent == rbt->rbt_minmax[position])
			rbt->rbt_minmax[position] = self;
#endif /* !RBSMALL */
		/*
		 * All new nodes are colored red.  We only need to rebalance
		 * if our parent is also red.
		 */
		RB_MARK_NONROOT(self);
		RB_MARK_RED(self);
		rebalance = RB_RED_P(parent);
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
	RBSTAT_INC(rbt->rbt_count);
#ifdef RBDEBUG
	if (RB_ROOT_P(self)) {
		RB_TAILQ_INSERT_HEAD(&rbt->rbt_nodes, self, rb_link);
	} else if (position == RB_DIR_LEFT) {
		KASSERT((*compare_nodes)(self, self->rb_parent) > 0);
		RB_TAILQ_INSERT_BEFORE(self->rb_parent, self, rb_link);
	} else {
		KASSERT((*compare_nodes)(self->rb_parent, self) > 0);
		RB_TAILQ_INSERT_AFTER(&rbt->rbt_nodes, self->rb_parent,
		    self, rb_link);
	}
#endif
	KASSERT(rb_tree_check_node(rbt, self, NULL, !rebalance));

	/*
	 * Rebalance tree after insertion
	 */
	if (rebalance) {
		rb_tree_insert_rebalance(rbt, self);
		KASSERT(rb_tree_check_node(rbt, self, NULL, true));
	}

	return true;
}

/*
 * Swap the location and colors of 'self' and its child @ which.  The child
 * can not be a sentinel node.  This is our rotation function.  However,
 * since it preserves coloring, it great simplifies both insertion and
 * removal since rotation almost always involves the exchanging of colors
 * as a separate step.
 */
static void
rb_tree_reparent_nodes(struct rb_tree *rbt, struct rb_node *old_father,
	const unsigned int which)
{
	const unsigned int other = which ^ RB_DIR_OTHER;
	struct rb_node * const grandpa = old_father->rb_parent;
	struct rb_node * const old_child = old_father->rb_nodes[which];
	struct rb_node * const new_father = old_child;
	struct rb_node * const new_child = old_father;
	struct { struct rb_properties rb_info; } tmp;

	KASSERT(which == RB_DIR_LEFT || which == RB_DIR_RIGHT);

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
	if (!RB_ROOT_P(old_father))
		RB_MARK_MOVED(grandpa);
	RB_MARK_MOVED(new_father);

	/*
	 * Exchange properties between new_father and new_child.  The only
	 * change is that new_child's position is now on the other side.
	 */
	RB_COPY_PROPERTIES(&tmp, old_child);
	RB_COPY_PROPERTIES(new_father, old_father);
	RB_COPY_PROPERTIES(new_child, &tmp);
	new_child->rb_position = other;
	RB_MARK_MOVED(new_child);

	/*
	 * Make sure to reparent the new child to ourself.
	 */
	if (!RB_SENTINEL_P(new_child->rb_nodes[which])) {
		new_child->rb_nodes[which]->rb_parent = new_child;
		new_child->rb_nodes[which]->rb_position = which;
		RB_MARK_MOVED(new_child->rb_nodes[which]);
	}

	KASSERT(rb_tree_check_node(rbt, new_father, NULL, false));
	KASSERT(rb_tree_check_node(rbt, new_child, NULL, false));
	KASSERT(RB_ROOT_P(new_father) || rb_tree_check_node(rbt, grandpa, NULL, false));
}

static void
rb_tree_insert_rebalance(struct rb_tree *rbt, struct rb_node *self)
{
	struct rb_node * father = self->rb_parent;
	struct rb_node * grandpa = father->rb_parent;
	struct rb_node * uncle;
	unsigned int which;
	unsigned int other;

	KASSERT(!RB_ROOT_P(self));
	KASSERT(RB_RED_P(self));
	KASSERT(RB_RED_P(father));
	RBSTAT_INC(rbt->rbt_insertion_rebalance_calls);

	for (;;) {
		KASSERT(!RB_SENTINEL_P(self));

		KASSERT(RB_RED_P(self));
		KASSERT(RB_RED_P(father));
		/*
		 * We are red and our parent is red, therefore we must have a
		 * grandfather and he must be black.
		 */
		grandpa = father->rb_parent;
		KASSERT(RB_BLACK_P(grandpa));
		KASSERT(RB_DIR_RIGHT == 1 && RB_DIR_LEFT == 0);
		which = (father == grandpa->rb_right);
		other = which ^ RB_DIR_OTHER;
		uncle = grandpa->rb_nodes[other];

		if (RB_BLACK_P(uncle))
			break;

		RBSTAT_INC(rbt->rbt_insertion_rebalance_passes);
		/*
		 * Case 1: our uncle is red
		 *   Simply invert the colors of our parent and
		 *   uncle and make our grandparent red.  And
		 *   then solve the problem up at his level.
		 */
		RB_MARK_BLACK(uncle);
		RB_MARK_BLACK(father);
		if (__predict_false(RB_ROOT_P(grandpa))) {
			/*
			 * If our grandpa is root, don't bother
			 * setting him to red, just return.
			 */
			KASSERT(RB_BLACK_P(grandpa));
			return;
		}
		RB_MARK_RED(grandpa);
		self = grandpa;
		father = self->rb_parent;
		KASSERT(RB_RED_P(self));
		if (RB_BLACK_P(father)) {
			/*
			 * If our greatgrandpa is black, we're done.
			 */
			KASSERT(RB_BLACK_P(rbt->rbt_root));
			return;
		}
	}

	KASSERT(!RB_ROOT_P(self));
	KASSERT(RB_RED_P(self));
	KASSERT(RB_RED_P(father));
	KASSERT(RB_BLACK_P(uncle));
	KASSERT(RB_BLACK_P(grandpa));
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
		rb_tree_reparent_nodes(rbt, father, other);
		KASSERT(father->rb_parent == self);
		KASSERT(self->rb_nodes[which] == father);
		KASSERT(self->rb_parent == grandpa);
		self = father;
		father = self->rb_parent;
	}
	KASSERT(RB_RED_P(self) && RB_RED_P(father));
	KASSERT(grandpa->rb_nodes[which] == father);
	/*
	 * Case 3: we are opposite a child of a black uncle.
	 *   Swap our parent and grandparent.  Since our grandfather
	 *   is black, our father will become black and our new sibling
	 *   (former grandparent) will become red.
	 */
	rb_tree_reparent_nodes(rbt, grandpa, which);
	KASSERT(self->rb_parent == father);
	KASSERT(self->rb_parent->rb_nodes[self->rb_position ^ RB_DIR_OTHER] == grandpa);
	KASSERT(RB_RED_P(self));
	KASSERT(RB_BLACK_P(father));
	KASSERT(RB_RED_P(grandpa));

	/*
	 * Final step: Set the root to black.
	 */
	RB_MARK_BLACK(rbt->rbt_root);
}

static void
rb_tree_prune_node(struct rb_tree *rbt, struct rb_node *self, bool rebalance)
{
	const unsigned int which = self->rb_position;
	struct rb_node *father = self->rb_parent;

	KASSERT(rebalance || (RB_ROOT_P(self) || RB_RED_P(self)));
	KASSERT(!rebalance || RB_BLACK_P(self));
	KASSERT(RB_CHILDLESS_P(self));
	KASSERT(rb_tree_check_node(rbt, self, NULL, false));

	/*
	 * Since we are childless, we know that self->rb_left is pointing
	 * to the sentinel node.
	 */
	father->rb_nodes[which] = self->rb_left;

	/*
	 * Remove ourselves from the node list, decrement the count,
	 * and update min/max.
	 */
	RB_TAILQ_REMOVE(&rbt->rbt_nodes, self, rb_link);
	RBSTAT_DEC(rbt->rbt_count);
#ifndef RBSMALL
	if (__predict_false(rbt->rbt_minmax[self->rb_position] == self)) {
		rbt->rbt_minmax[self->rb_position] = father;
		/*
		 * When removing the root, rbt->rbt_minmax[RB_DIR_LEFT] is
		 * updated automatically, but we also need to update 
		 * rbt->rbt_minmax[RB_DIR_RIGHT];
		 */
		if (__predict_false(RB_ROOT_P(self))) {
			rbt->rbt_minmax[RB_DIR_RIGHT] = father;
		}
	}
	self->rb_sentinel = 1;	/* so remove_node will fail */
#endif
	RB_MARK_MOVED(father);

	/*
	 * Rebalance if requested.
	 */
	if (rebalance)
		rb_tree_removal_rebalance(rbt, father, which);
	KASSERT(RB_ROOT_P(self) || rb_tree_check_node(rbt, father, NULL, true));
}

/*
 * When deleting an interior node
 */
static void
rb_tree_swap_prune_and_rebalance(struct rb_tree *rbt, struct rb_node *self,
	struct rb_node *standin)
{
	const unsigned int standin_which = standin->rb_position;
	unsigned int standin_other = standin_which ^ RB_DIR_OTHER;
	struct rb_node *standin_son;
	struct rb_node *standin_father = standin->rb_parent;
	bool rebalance = RB_BLACK_P(standin);

	if (standin->rb_parent == self) {
		/*
		 * As a child of self, any childen would be opposite of
		 * our parent.
		 */
		KASSERT(RB_SENTINEL_P(standin->rb_nodes[standin_other]));
		standin_son = standin->rb_nodes[standin_which];
	} else {
		/*
		 * Since we aren't a child of self, any childen would be
		 * on the same side as our parent.
		 */
		KASSERT(RB_SENTINEL_P(standin->rb_nodes[standin_which]));
		standin_son = standin->rb_nodes[standin_other];
	}

	/*
	 * the node we are removing must have two children.
	 */
	KASSERT(RB_TWOCHILDREN_P(self));
	/*
	 * If standin has a child, it must be red.
	 */
	KASSERT(RB_SENTINEL_P(standin_son) || RB_RED_P(standin_son));

	/*
	 * Verify things are sane.
	 */
	KASSERT(rb_tree_check_node(rbt, self, NULL, false));
	KASSERT(rb_tree_check_node(rbt, standin, NULL, false));

	if (__predict_false(RB_RED_P(standin_son))) {
		/*
		 * We know we have a red child so if we flip it to black
		 * we don't have to rebalance.
		 */
		KASSERT(rb_tree_check_node(rbt, standin_son, NULL, true));
		RB_MARK_BLACK(standin_son);
		rebalance = false;

		if (standin_father == self) {
			KASSERT(standin_son->rb_position == standin_which);
		} else {
			KASSERT(standin_son->rb_position == standin_other);
			/*
			 * Change the son's parentage to point to his grandpa.
			 */
			standin_son->rb_parent = standin_father;
			standin_son->rb_position = standin_which;
			RB_MARK_MOVED(standin_son);
		}
	}

	if (standin_father == self) {
		/*
		 * If we are about to delete the standin's father, then when
		 * we call rebalance, we need to use ourselves as our father.
		 * Otherwise remember our original father.  Also, sincef we are
		 * our standin's father we only need to reparent the standin's
		 * brother.
		 *
		 * |    R      -->     S    |
		 * |  Q   S    -->   Q   T  |
		 * |        t  -->          |
		 */
		KASSERT(RB_SENTINEL_P(standin->rb_nodes[standin_other]));
		KASSERT(!RB_SENTINEL_P(self->rb_nodes[standin_other]));
		KASSERT(self->rb_nodes[standin_which] == standin);
		/*
		 * Have our son/standin adopt his brother as his new son.
		 */
		standin_father = standin;
	} else {
		/*
		 * |    R          -->    S       .  |
		 * |   / \  |   T  -->   / \  |  /   |
		 * |  ..... | S    -->  ..... | T    |
		 *
		 * Sever standin's connection to his father.
		 */
		standin_father->rb_nodes[standin_which] = standin_son;
		/*
		 * Adopt the far son.
		 */
		standin->rb_nodes[standin_other] = self->rb_nodes[standin_other];
		standin->rb_nodes[standin_other]->rb_parent = standin;
		RB_MARK_MOVED(standin->rb_nodes[standin_other]);
		KASSERT(self->rb_nodes[standin_other]->rb_position == standin_other);
		/*
		 * Use standin_other because we need to preserve standin_which
		 * for the removal_rebalance.
		 */
		standin_other = standin_which;
	}

	/*
	 * Move the only remaining son to our standin.  If our standin is our
	 * son, this will be the only son needed to be moved.
	 */
	KASSERT(standin->rb_nodes[standin_other] != self->rb_nodes[standin_other]);
	standin->rb_nodes[standin_other] = self->rb_nodes[standin_other];
	standin->rb_nodes[standin_other]->rb_parent = standin;
	RB_MARK_MOVED(standin->rb_nodes[standin_other]);

	/*
	 * Now copy the result of self to standin and then replace
	 * self with standin in the tree.
	 */
	RB_COPY_PROPERTIES(standin, self);
	standin->rb_parent = self->rb_parent;
	standin->rb_parent->rb_nodes[standin->rb_position] = standin;
	RB_MARK_MOVED(standin);

	/*
	 * Remove ourselves from the node list, decrement the count,
	 * and update min/max.
	 */
	RB_TAILQ_REMOVE(&rbt->rbt_nodes, self, rb_link);
	RBSTAT_DEC(rbt->rbt_count);
#ifndef RBSMALL
	if (__predict_false(rbt->rbt_minmax[self->rb_position] == self))
		rbt->rbt_minmax[self->rb_position] = self->rb_parent;
	self->rb_sentinel = 1;
#endif

	KASSERT(rb_tree_check_node(rbt, standin, NULL, false));
	KASSERT(RB_PARENT_SENTINEL_P(standin)
		|| rb_tree_check_node(rbt, standin_father, NULL, false));
	KASSERT(RB_LEFT_SENTINEL_P(standin)
		|| rb_tree_check_node(rbt, standin->rb_left, NULL, false));
	KASSERT(RB_RIGHT_SENTINEL_P(standin)
		|| rb_tree_check_node(rbt, standin->rb_right, NULL, false));

	if (!rebalance)
		return;

	rb_tree_removal_rebalance(rbt, standin_father, standin_which);
	KASSERT(rb_tree_check_node(rbt, standin, NULL, true));
}

/*
 * We could do this by doing
 *	rb_tree_node_swap(rbt, self, which);
 *	rb_tree_prune_node(rbt, self, false);
 *
 * But it's more efficient to just evalate and recolor the child.
 */
static void
rb_tree_prune_blackred_branch(struct rb_tree *rbt, struct rb_node *self,
	unsigned int which)
{
	struct rb_node *father = self->rb_parent;
	struct rb_node *son = self->rb_nodes[which];

	KASSERT(which == RB_DIR_LEFT || which == RB_DIR_RIGHT);
	KASSERT(RB_BLACK_P(self) && RB_RED_P(son));
	KASSERT(!RB_TWOCHILDREN_P(son));
	KASSERT(RB_CHILDLESS_P(son));
	KASSERT(rb_tree_check_node(rbt, self, NULL, false));
	KASSERT(rb_tree_check_node(rbt, son, NULL, false));

	/*
	 * Remove ourselves from the tree and give our former child our
	 * properties (position, color, root).
	 */
	father->rb_nodes[self->rb_position] = son;
	son->rb_parent = father;
	RB_COPY_PROPERTIES(son, self);
	RB_MARK_MOVED(son);
	if (__predict_false(!RB_ROOT_P(son)))
		RB_MARK_MOVED(father);

	/*
	 * Remove ourselves from the node list, decrement the count,
	 * and update minmax.
	 */
	RB_TAILQ_REMOVE(&rbt->rbt_nodes, self, rb_link);
	RBSTAT_DEC(rbt->rbt_count);
#ifndef RBSMALL
	if (__predict_false(RB_ROOT_P(self))) {
		KASSERT(rbt->rbt_minmax[which] == son);
		rbt->rbt_minmax[which ^ RB_DIR_OTHER] = son;
	} else if (rbt->rbt_minmax[self->rb_position] == self) {
		rbt->rbt_minmax[self->rb_position] = son;
	}
	self->rb_sentinel = 1;
#endif

	KASSERT(RB_ROOT_P(self) || rb_tree_check_node(rbt, father, NULL, true));
	KASSERT(rb_tree_check_node(rbt, son, NULL, true));
}
/*
 *
 */
void
rb_tree_remove_node(struct rb_tree *rbt, struct rb_node *self)
{
	struct rb_node *standin;
	unsigned int which;

	KASSERT(!RB_SENTINEL_P(self));
	RBSTAT_INC(rbt->rbt_removals);

	/*
	 * In the following diagrams, we (the node to be removed) are S.  Red
	 * nodes are lowercase.  T could be either red or black.
	 *
	 * Remember the major axiom of the red-black tree: the number of
	 * black nodes from the root to each leaf is constant across all
	 * leaves, only the number of red nodes varies.
	 *
	 * Thus removing a red leaf doesn't require any other changes to a
	 * red-black tree.  So if we must remove a node, attempt to rearrange
	 * the tree so we can remove a red node.
	 *
	 * The simpliest case is a childless red node or a childless root node:
	 *
	 * |    T  -->    T  |    or    |  R  -->  *  |
	 * |  s    -->  *    |
	 */
	if (RB_CHILDLESS_P(self)) {
		const bool rebalance = RB_BLACK_P(self) && !RB_ROOT_P(self);
		rb_tree_prune_node(rbt, self, rebalance);
		return;
	}
	KASSERT(!RB_CHILDLESS_P(self));
	if (!RB_TWOCHILDREN_P(self)) {
		/*
		 * The next simpliest case is the node we are deleting is
		 * black and has one red child.
		 *
		 * |      T  -->      T  -->      T  |
		 * |    S    -->  R      -->  R      |
		 * |  r      -->    s    -->    *    |
		 */
		which = RB_LEFT_SENTINEL_P(self) ? RB_DIR_RIGHT : RB_DIR_LEFT;
		KASSERT(RB_BLACK_P(self));
		KASSERT(RB_RED_P(self->rb_nodes[which]));
		KASSERT(RB_CHILDLESS_P(self->rb_nodes[which]));
		rb_tree_prune_blackred_branch(rbt, self, which);
		return;
	}
	KASSERT(RB_TWOCHILDREN_P(self));

	/*
	 * We invert these because we prefer to remove from the inside of
	 * the tree.
	 */
	which = self->rb_position ^ RB_DIR_OTHER;

	/*
	 * Let's find the node closes to us opposite of our parent
	 * Now swap it with ourself, "prune" it, and rebalance, if needed.
	 */
	standin = rb_tree_iterate(rbt, self, which);
	rb_tree_swap_prune_and_rebalance(rbt, self, standin);
}

static void
rb_tree_removal_rebalance(struct rb_tree *rbt, struct rb_node *parent,
	unsigned int which)
{
	KASSERT(!RB_SENTINEL_P(parent));
	KASSERT(RB_SENTINEL_P(parent->rb_nodes[which]));
	KASSERT(which == RB_DIR_LEFT || which == RB_DIR_RIGHT);
	RBSTAT_INC(rbt->rbt_removal_rebalance_calls);

	while (RB_BLACK_P(parent->rb_nodes[which])) {
		unsigned int other = which ^ RB_DIR_OTHER;
		struct rb_node *brother = parent->rb_nodes[other];

		RBSTAT_INC(rbt->rbt_removal_rebalance_passes);

		KASSERT(!RB_SENTINEL_P(brother));
		/*
		 * For cases 1, 2a, and 2b, our brother's children must
		 * be black and our father must be black
		 */
		if (RB_BLACK_P(parent)
		    && RB_BLACK_P(brother->rb_left)
		    && RB_BLACK_P(brother->rb_right)) {
			if (RB_RED_P(brother)) {
				/*
				 * Case 1: Our brother is red, swap its
				 * position (and colors) with our parent. 
				 * This should now be case 2b (unless C or E
				 * has a red child which is case 3; thus no
				 * explicit branch to case 2b).
				 *
				 *    B         ->        D
				 *  A     d     ->    b     E
				 *      C   E   ->  A   C
				 */
				KASSERT(RB_BLACK_P(parent));
				rb_tree_reparent_nodes(rbt, parent, other);
				brother = parent->rb_nodes[other];
				KASSERT(!RB_SENTINEL_P(brother));
				KASSERT(RB_RED_P(parent));
				KASSERT(RB_BLACK_P(brother));
				KASSERT(rb_tree_check_node(rbt, brother, NULL, false));
				KASSERT(rb_tree_check_node(rbt, parent, NULL, false));
			} else {
				/*
				 * Both our parent and brother are black.
				 * Change our brother to red, advance up rank
				 * and go through the loop again.
				 *
				 *    B         ->   *B
				 * *A     D     ->  A     d
				 *      C   E   ->      C   E
				 */
				RB_MARK_RED(brother);
				KASSERT(RB_BLACK_P(brother->rb_left));
				KASSERT(RB_BLACK_P(brother->rb_right));
				if (RB_ROOT_P(parent))
					return;	/* root == parent == black */
				KASSERT(rb_tree_check_node(rbt, brother, NULL, false));
				KASSERT(rb_tree_check_node(rbt, parent, NULL, false));
				which = parent->rb_position;
				parent = parent->rb_parent;
				continue;
			}
		}
		/*
		 * Avoid an else here so that case 2a above can hit either
		 * case 2b, 3, or 4.
		 */
		if (RB_RED_P(parent)
		    && RB_BLACK_P(brother)
		    && RB_BLACK_P(brother->rb_left)
		    && RB_BLACK_P(brother->rb_right)) {
			KASSERT(RB_RED_P(parent));
			KASSERT(RB_BLACK_P(brother));
			KASSERT(RB_BLACK_P(brother->rb_left));
			KASSERT(RB_BLACK_P(brother->rb_right));
			/*
			 * We are black, our father is red, our brother and
			 * both nephews are black.  Simply invert/exchange the
			 * colors of our father and brother (to black and red
			 * respectively).
			 *
			 *	|    f        -->    F        |
			 *	|  *     B    -->  *     b    |
			 *	|      N   N  -->      N   N  |
			 */
			RB_MARK_BLACK(parent);
			RB_MARK_RED(brother);
			KASSERT(rb_tree_check_node(rbt, brother, NULL, true));
			break;		/* We're done! */
		} else {
			/*
			 * Our brother must be black and have at least one
			 * red child (it may have two).
			 */
			KASSERT(RB_BLACK_P(brother));
			KASSERT(RB_RED_P(brother->rb_nodes[which]) ||
				RB_RED_P(brother->rb_nodes[other]));
			if (RB_BLACK_P(brother->rb_nodes[other])) {
				/*
				 * Case 3: our brother is black, our near
				 * nephew is red, and our far nephew is black.
				 * Swap our brother with our near nephew.  
				 * This result in a tree that matches case 4.
				 * (Our father could be red or black).
				 *
				 *	|    F      -->    F      |
				 *	|  x     B  -->  x   B    |
				 *	|      n    -->        n  |
				 */
				KASSERT(RB_RED_P(brother->rb_nodes[which]));
				rb_tree_reparent_nodes(rbt, brother, which);
				KASSERT(brother->rb_parent == parent->rb_nodes[other]);
				brother = parent->rb_nodes[other];
				KASSERT(RB_RED_P(brother->rb_nodes[other]));
			}
			/*
			 * Case 4: our brother is black and our far nephew
			 * is red.  Swap our father and brother locations and
			 * change our far nephew to black.  (these can be
			 * done in either order so we change the color first).
			 * The result is a valid red-black tree and is a
			 * terminal case.  (again we don't care about the
			 * father's color)
			 *
			 * If the father is red, we will get a red-black-black
			 * tree:
			 *	|  f      ->  f      -->    b    |
			 *	|    B    ->    B    -->  F   N  |
			 *	|      n  ->      N  -->         |
			 *
			 * If the father is black, we will get an all black
			 * tree:
			 *	|  F      ->  F      -->    B    |
			 *	|    B    ->    B    -->  F   N  |
			 *	|      n  ->      N  -->         |
			 *
			 * If we had two red nephews, then after the swap,
			 * our former father would have a red grandson. 
			 */
			KASSERT(RB_BLACK_P(brother));
			KASSERT(RB_RED_P(brother->rb_nodes[other]));
			RB_MARK_BLACK(brother->rb_nodes[other]);
			rb_tree_reparent_nodes(rbt, parent, other);
			break;		/* We're done! */
		}
	}
	KASSERT(rb_tree_check_node(rbt, parent, NULL, true));
}

struct rb_node *
rb_tree_iterate(struct rb_tree *rbt, struct rb_node *self,
	const unsigned int direction)
{
	const unsigned int other = direction ^ RB_DIR_OTHER;
	KASSERT(direction == RB_DIR_LEFT || direction == RB_DIR_RIGHT);

	if (self == NULL) {
#ifndef RBSMALL
		if (RB_SENTINEL_P(rbt->rbt_root))
			return NULL;
		return rbt->rbt_minmax[direction];
#else
		self = rbt->rbt_root;
		if (RB_SENTINEL_P(self))
			return NULL;
		while (!RB_SENTINEL_P(self->rb_nodes[other]))
			self = self->rb_nodes[other];
		return self;
#endif /* !RBSMALL */
	}
	KASSERT(!RB_SENTINEL_P(self));
	/*
	 * We can't go any further in this direction.  We proceed up in the
	 * opposite direction until our parent is in direction we want to go.
	 */
	if (RB_SENTINEL_P(self->rb_nodes[direction])) {
		while (!RB_ROOT_P(self)) {
			if (other == self->rb_position)
				return self->rb_parent;
			self = self->rb_parent;
		}
		return NULL;
	}

	/*
	 * Advance down one in current direction and go down as far as possible
	 * in the opposite direction.
	 */
	self = self->rb_nodes[direction];
	KASSERT(!RB_SENTINEL_P(self));
	while (!RB_SENTINEL_P(self->rb_nodes[other]))
		self = self->rb_nodes[other];
	return self;
}

#ifdef RBDEBUG
static const struct rb_node *
rb_tree_iterate_const(const struct rb_tree *rbt, const struct rb_node *self,
	const unsigned int direction)
{
	const unsigned int other = direction ^ RB_DIR_OTHER;
	KASSERT(direction == RB_DIR_LEFT || direction == RB_DIR_RIGHT);

	if (self == NULL) {
#ifndef RBSMALL
		if (RB_SENTINEL_P(rbt->rbt_root))
			return NULL;
		return rbt->rbt_minmax[direction];
#else
		self = rbt->rbt_root;
		if (RB_SENTINEL_P(self))
			return NULL;
		while (!RB_SENTINEL_P(self->rb_nodes[other]))
			self = self->rb_nodes[other];
		return self;
#endif /* !RBSMALL */
	}
	KASSERT(!RB_SENTINEL_P(self));
	/*
	 * We can't go any further in this direction.  We proceed up in the
	 * opposite direction until our parent is in direction we want to go.
	 */
	if (RB_SENTINEL_P(self->rb_nodes[direction])) {
		while (!RB_ROOT_P(self)) {
			if (other == self->rb_position)
				return self->rb_parent;
			self = self->rb_parent;
		}
		return NULL;
	}

	/*
	 * Advance down one in current direction and go down as far as possible
	 * in the opposite direction.
	 */
	self = self->rb_nodes[direction];
	KASSERT(!RB_SENTINEL_P(self));
	while (!RB_SENTINEL_P(self->rb_nodes[other]))
		self = self->rb_nodes[other];
	return self;
}

static unsigned int
rb_tree_count_black(const struct rb_node *self)
{
	unsigned int left, right;

	if (RB_SENTINEL_P(self))
		return 0;

	left = rb_tree_count_black(self->rb_left);
	right = rb_tree_count_black(self->rb_right);

	KASSERT(left == right);

	return left + RB_BLACK_P(self);
}

static bool
rb_tree_check_node(const struct rb_tree *rbt, const struct rb_node *self,
	const struct rb_node *prev, bool red_check)
{
	rb_compare_nodes_fn compare_nodes = rbt->rbt_ops->rb_compare_nodes;
	KASSERT(!self->rb_sentinel);
	KASSERT(self->rb_left);
	KASSERT(self->rb_right);
	KASSERT(prev == NULL || (*compare_nodes)(prev, self) > 0);

	/*
	 * Verify our relationship to our parent.
	 */
	if (RB_ROOT_P(self)) {
		KASSERT(self == rbt->rbt_root);
		KASSERT(self->rb_position == RB_DIR_LEFT);
		KASSERT(self->rb_parent->rb_nodes[RB_DIR_LEFT] == self);
		KASSERT(self->rb_parent == (const struct rb_node *) &rbt->rbt_root);
	} else {
		KASSERT(self != rbt->rbt_root);
		KASSERT(!RB_PARENT_SENTINEL_P(self));
		if (self->rb_position == RB_DIR_LEFT) {
			KASSERT((*compare_nodes)(self, self->rb_parent) > 0);
			KASSERT(self->rb_parent->rb_nodes[RB_DIR_LEFT] == self);
		} else {
			KASSERT((*compare_nodes)(self, self->rb_parent) < 0);
			KASSERT(self->rb_parent->rb_nodes[RB_DIR_RIGHT] == self);
		}
	}

	/*
	 * Verify our position in the linked list against the tree itself.
	 */
	{
		const struct rb_node *prev0 = rb_tree_iterate_const(rbt, self, RB_DIR_LEFT);
		const struct rb_node *next0 = rb_tree_iterate_const(rbt, self, RB_DIR_RIGHT);
		KASSERT(prev0 == TAILQ_PREV(self, rb_node_qh, rb_link));
		KASSERT(next0 == TAILQ_NEXT(self, rb_link));
#ifndef RBSMALL
		KASSERT(prev0 != NULL || self == rbt->rbt_minmax[RB_DIR_LEFT]);
		KASSERT(next0 != NULL || self == rbt->rbt_minmax[RB_DIR_RIGHT]);
#endif
	}

	/*
	 * The root must be black.
	 * There can never be two adjacent red nodes. 
	 */
	if (red_check) {
		KASSERT(!RB_ROOT_P(self) || RB_BLACK_P(self));
		(void) rb_tree_count_black(self);
		if (RB_RED_P(self)) {
			const struct rb_node *brother;
			KASSERT(!RB_ROOT_P(self));
			brother = self->rb_parent->rb_nodes[self->rb_position ^ RB_DIR_OTHER];
			KASSERT(RB_BLACK_P(self->rb_parent));
			/* 
			 * I'm red and have no children, then I must either
			 * have no brother or my brother also be red and
			 * also have no children.  (black count == 0)
			 */
			KASSERT(!RB_CHILDLESS_P(self)
				|| RB_SENTINEL_P(brother)
				|| RB_RED_P(brother)
				|| RB_CHILDLESS_P(brother));
			/*
			 * If I'm not childless, I must have two children
			 * and they must be both be black.
			 */
			KASSERT(RB_CHILDLESS_P(self)
				|| (RB_TWOCHILDREN_P(self)
				    && RB_BLACK_P(self->rb_left)
				    && RB_BLACK_P(self->rb_right)));
			/*
			 * If I'm not childless, thus I have black children,
			 * then my brother must either be black or have two
			 * black children.
			 */
			KASSERT(RB_CHILDLESS_P(self)
				|| RB_BLACK_P(brother)
				|| (RB_TWOCHILDREN_P(brother)
				    && RB_BLACK_P(brother->rb_left)
				    && RB_BLACK_P(brother->rb_right)));
		} else {
			/*
			 * If I'm black and have one child, that child must
			 * be red and childless.
			 */
			KASSERT(RB_CHILDLESS_P(self)
				|| RB_TWOCHILDREN_P(self)
				|| (!RB_LEFT_SENTINEL_P(self)
				    && RB_RIGHT_SENTINEL_P(self)
				    && RB_RED_P(self->rb_left)
				    && RB_CHILDLESS_P(self->rb_left))
				|| (!RB_RIGHT_SENTINEL_P(self)
				    && RB_LEFT_SENTINEL_P(self)
				    && RB_RED_P(self->rb_right)
				    && RB_CHILDLESS_P(self->rb_right)));

			/*
			 * If I'm a childless black node and my parent is
			 * black, my 2nd closet relative away from my parent
			 * is either red or has a red parent or red children.
			 */
			if (!RB_ROOT_P(self)
			    && RB_CHILDLESS_P(self)
			    && RB_BLACK_P(self->rb_parent)) {
				const unsigned int which = self->rb_position;
				const unsigned int other = which ^ RB_DIR_OTHER;
				const struct rb_node *relative0, *relative;

				relative0 = rb_tree_iterate_const(rbt,
				    self, other);
				KASSERT(relative0 != NULL);
				relative = rb_tree_iterate_const(rbt,
				    relative0, other);
				KASSERT(relative != NULL);
				KASSERT(RB_SENTINEL_P(relative->rb_nodes[which]));
#if 0
				KASSERT(RB_RED_P(relative)
					|| RB_RED_P(relative->rb_left)
					|| RB_RED_P(relative->rb_right)
					|| RB_RED_P(relative->rb_parent));
#endif
			}
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

		/*
		 * If we have a child on the left and it doesn't have two
		 * children make sure we don't have great-great-grandchildren on
		 * the right.
		 */
		KASSERT(RB_TWOCHILDREN_P(self->rb_left)
			|| RB_CHILDLESS_P(self->rb_right)
			|| RB_CHILDLESS_P(self->rb_right->rb_left)
			|| RB_CHILDLESS_P(self->rb_right->rb_left->rb_left)
			|| RB_CHILDLESS_P(self->rb_right->rb_left->rb_right)
			|| RB_CHILDLESS_P(self->rb_right->rb_right)
			|| RB_CHILDLESS_P(self->rb_right->rb_right->rb_left)
			|| RB_CHILDLESS_P(self->rb_right->rb_right->rb_right));

		/*
		 * If we have a child on the right and it doesn't have two
		 * children make sure we don't have great-great-grandchildren on
		 * the left.
		 */
		KASSERT(RB_TWOCHILDREN_P(self->rb_right)
			|| RB_CHILDLESS_P(self->rb_left)
			|| RB_CHILDLESS_P(self->rb_left->rb_left)
			|| RB_CHILDLESS_P(self->rb_left->rb_left->rb_left)
			|| RB_CHILDLESS_P(self->rb_left->rb_left->rb_right)
			|| RB_CHILDLESS_P(self->rb_left->rb_right)
			|| RB_CHILDLESS_P(self->rb_left->rb_right->rb_left)
			|| RB_CHILDLESS_P(self->rb_left->rb_right->rb_right));

		/*
		 * If we are fully interior node, then our predecessors and
		 * successors must have no children in our direction.
		 */
		if (RB_TWOCHILDREN_P(self)) {
			const struct rb_node *prev0;
			const struct rb_node *next0;

			prev0 = rb_tree_iterate_const(rbt, self, RB_DIR_LEFT);
			KASSERT(prev0 != NULL);
			KASSERT(RB_RIGHT_SENTINEL_P(prev0));

			next0 = rb_tree_iterate_const(rbt, self, RB_DIR_RIGHT);
			KASSERT(next0 != NULL);
			KASSERT(RB_LEFT_SENTINEL_P(next0));
		}
	}

	return true;
}

void
rb_tree_check(const struct rb_tree *rbt, bool red_check)
{
	const struct rb_node *self;
	const struct rb_node *prev;
#ifdef RBSTATS
	unsigned int count = 0;
#endif

	KASSERT(rbt->rbt_root != NULL);
	KASSERT(rbt->rbt_root->rb_position == RB_DIR_LEFT);

#if defined(RBSTATS) && !defined(RBSMALL)
	KASSERT(rbt->rbt_count > 1
	    || rbt->rbt_minmax[RB_DIR_LEFT] == rbt->rbt_minmax[RB_DIR_RIGHT]);
#endif

	prev = NULL;
	TAILQ_FOREACH(self, &rbt->rbt_nodes, rb_link) {
		rb_tree_check_node(rbt, self, prev, false);
#ifdef RBSTATS
		count++;
#endif
	}
#ifdef RBSTATS
	KASSERT(rbt->rbt_count == count);
#endif
	if (red_check) {
		KASSERT(RB_BLACK_P(rbt->rbt_root));
		KASSERT(RB_SENTINEL_P(rbt->rbt_root)
			|| rb_tree_count_black(rbt->rbt_root));

		/*
		 * The root must be black.
		 * There can never be two adjacent red nodes. 
		 */
		TAILQ_FOREACH(self, &rbt->rbt_nodes, rb_link) {
			rb_tree_check_node(rbt, self, NULL, true);
		}
	}
}
#endif /* RBDEBUG */

#ifdef RBSTATS
static void
rb_tree_mark_depth(const struct rb_tree *rbt, const struct rb_node *self,
	size_t *depths, size_t depth)
{
	if (RB_SENTINEL_P(self))
		return;

	if (RB_TWOCHILDREN_P(self)) {
		rb_tree_mark_depth(rbt, self->rb_left, depths, depth + 1);
		rb_tree_mark_depth(rbt, self->rb_right, depths, depth + 1);
		return;
	}
	depths[depth]++;
	if (!RB_LEFT_SENTINEL_P(self)) {
		rb_tree_mark_depth(rbt, self->rb_left, depths, depth + 1);
	}
	if (!RB_RIGHT_SENTINEL_P(self)) {
		rb_tree_mark_depth(rbt, self->rb_right, depths, depth + 1);
	}
}

void
rb_tree_depths(const struct rb_tree *rbt, size_t *depths)
{
	rb_tree_mark_depth(rbt, rbt->rbt_root, depths, 1);
}
#endif /* RBSTATS */
