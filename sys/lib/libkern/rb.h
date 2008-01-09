/* $NetBSD: rb.h,v 1.7.26.1 2008/01/09 01:56:35 matt Exp $ */

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
#ifndef _LIBKERN_RB_H_
#define	_LIBKERN_RB_H_

#include <sys/types.h>
#include <sys/queue.h>
#include <sys/endian.h>

struct rb_node {
	struct rb_node *rb_nodes[2];
#define	RB_DIR_LEFT	0
#define	RB_DIR_RIGHT	1
#define	RB_DIR_OTHER	1
#define	rb_left		rb_nodes[RB_DIR_LEFT]
#define	rb_right	rb_nodes[RB_DIR_RIGHT]
	struct rb_node *rb_parent;
	struct rb_properties {
#if BYTE_ORDER == LITTLE_ENDIAN
		unsigned long int s_data : 8 * sizeof(unsigned long int) - 5;
		unsigned long int s_moved : 1;
		unsigned long int s_root : 1;
		unsigned long int s_position : 1;
		unsigned long int s_color : 1;
		unsigned long int s_sentinel : 1;
#endif
#if BYTE_ORDER == BIG_ENDIAN
		unsigned long int s_sentinel : 1;
		unsigned long int s_color : 1;
		unsigned long int s_position : 1;
		unsigned long int s_root : 1;
		unsigned long int s_moved : 1;
		unsigned long int s_data : 8 * sizeof(unsigned long int) - 5;
#endif
	} rb_info;
#define	rb_moved			rb_info.s_moved
#define	rb_root				rb_info.s_root
#define	rb_position			rb_info.s_position
#define	rb_color			rb_info.s_color
#define	rb_sentinel			rb_info.s_sentinel
#define	rb_data				rb_info.s_data
#define	RB_SENTINEL_P(rb)		((rb)->rb_sentinel + 0)
#define	RB_LEFT_SENTINEL_P(rb)		((rb)->rb_left->rb_sentinel + 0)
#define	RB_RIGHT_SENTINEL_P(rb)		((rb)->rb_right->rb_sentinel + 0)
#define	RB_PARENT_SENTINEL_P(rb)	((rb)->rb_parent->rb_sentinel + 0)
#define	RB_CHILDLESS_P(rb)		(RB_LEFT_SENTINEL_P(rb) \
					 && RB_RIGHT_SENTINEL_P(rb))
#define	RB_TWOCHILDREN_P(rb)		(!RB_LEFT_SENTINEL_P(rb) \
					 && !RB_RIGHT_SENTINEL_P(rb))
#define	RB_ROOT_P(rb)			((rb)->rb_root != false)
#define	RB_RED_P(rb)			((rb)->rb_color + 0)
#define	RB_BLACK_P(rb)			(!(rb)->rb_color)
#define	RB_MOVED_P(rb)			((rb)->rb_moved + 0)
#ifdef RBSMALL
#define	RB_MARK_UNMOVED(rb)		((void)0)
#define	RB_MARK_MOVED(rb)		((void)0)
#else
#define	RB_MARK_UNMOVED(rb)		((void)((rb)->rb_moved = 0))
#define	RB_MARK_MOVED(rb)		((void)((rb)->rb_moved = 1))
#endif
#define	RB_MARK_RED(rb)			((void)((rb)->rb_color = 1))
#define	RB_MARK_BLACK(rb)		((void)((rb)->rb_color = 0))
#define	RB_INVERT_COLOR(rb)		((void)((rb)->rb_color ^= 1))
#define	RB_MARK_ROOT(rb)		((void)((rb)->rb_root = 1))
#define	RB_MARK_NONROOT(rb)		((void)((rb)->rb_root = 0))
#define	RB_ZERO_PROPERTIES(rb)		((void)((rb)->rb_sentinel = 0, \
						(rb)->rb_color = 0, \
						(rb)->rb_position = 0, \
						(rb)->rb_root = 0, \
						(rb)->rb_moved = 0))
#define	RB_COPY_PROPERTIES(dst, src)	\
		((void)((dst)->rb_color = (src)->rb_color, \
			(dst)->rb_position = (src)->rb_position, \
			(dst)->rb_root = (src)->rb_root, \
			(dst)->rb_moved = (src)->rb_moved))
#ifdef RBDEBUG
	TAILQ_ENTRY(rb_node) rb_link;
#endif
};

#ifdef RBDEBUG
TAILQ_HEAD(rb_node_qh, rb_node);

#define	RB_TAILQ_REMOVE(a, b, c)		TAILQ_REMOVE(a, b, c)
#define	RB_TAILQ_INIT(a)			TAILQ_INIT(a)
#define	RB_TAILQ_INSERT_HEAD(a, b, c)		TAILQ_INSERT_HEAD(a, b, c)
#define	RB_TAILQ_INSERT_BEFORE(a, b, c)		TAILQ_INSERT_BEFORE(a, b, c)
#define	RB_TAILQ_INSERT_AFTER(a, b, c, d)	TAILQ_INSERT_AFTER(a, b, c, d)
#else
#define	RB_TAILQ_REMOVE(a, b, c)		do { } while (0)
#define	RB_TAILQ_INIT(a)			do { } while (0)
#define	RB_TAILQ_INSERT_HEAD(a, b, c)		do { } while (0)
#define	RB_TAILQ_INSERT_BEFORE(a, b, c)		do { } while (0)
#define	RB_TAILQ_INSERT_AFTER(a, b, c, d)	do { } while (0)
#endif /* RBDEBUG */

typedef signed int (*const rb_compare_nodes_fn)(const struct rb_node *,
    const struct rb_node *);
typedef signed int (*const rb_compare_key_fn)(const struct rb_node *,
    const void *);

struct rb_tree_ops {
	rb_compare_nodes_fn rb_compare_nodes;
	rb_compare_key_fn rb_compare_key;
};

struct rb_tree {
	struct rb_node *rbt_root;
	const struct rb_tree_ops *rbt_ops;
#ifndef RBSMALL
	struct rb_node *rbt_minmax[2];
#endif
#ifdef RBDEBUG
	struct rb_node_qh rbt_nodes;
#endif
#ifdef RBSTATS
	unsigned int rbt_count;
	unsigned int rbt_insertions;
	unsigned int rbt_removals;
	unsigned int rbt_insertion_rebalance_calls;
	unsigned int rbt_insertion_rebalance_passes;
	unsigned int rbt_removal_rebalance_calls;
	unsigned int rbt_removal_rebalance_passes;
#endif
};

#ifdef RBSTATS
#define	RBSTAT_INC(v)	((void)((v)++))
#define	RBSTAT_DEC(v)	((void)((v)--))
#else
#define	RBSTAT_INC(v)	do { } while (0)
#define	RBSTAT_DEC(v)	do { } while (0)
#endif

void	rb_tree_init(struct rb_tree *, const struct rb_tree_ops *);
bool	rb_tree_insert_node(struct rb_tree *, struct rb_node *);
struct rb_node	*
	rb_tree_find_node(struct rb_tree *, const void *);
struct rb_node	*
	rb_tree_find_node_geq(struct rb_tree *, const void *);
struct rb_node	*
	rb_tree_find_node_leq(struct rb_tree *, const void *);
void	rb_tree_remove_node(struct rb_tree *, struct rb_node *);
struct rb_node *
	rb_tree_iterate(struct rb_tree *, struct rb_node *, const unsigned int);
#ifdef RBDEBUG
void	rb_tree_check(const struct rb_tree *, bool);
#endif
#ifdef RBSTATS
void	rb_tree_depths(const struct rb_tree *, size_t *);
#endif

#endif	/* _LIBKERN_RB_H_*/
