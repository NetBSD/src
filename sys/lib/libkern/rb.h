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

struct rb_node {
	struct rb_node *rb_nodes[3];
#define	RB_LEFT		0
#define	RB_RIGHT	1
#define	RB_OTHER	1
#define	RB_PARENT	2
#define	rb_left		rb_nodes[RB_LEFT]
#define	rb_right	rb_nodes[RB_RIGHT]
#define	rb_parent	rb_nodes[RB_PARENT]
	TAILQ_ENTRY(rb_node) rb_link;
	int rb_balance : 16;
	unsigned int rb_red : 1;
	unsigned int rb_sentinel : 1;
	unsigned int rb_position : 2;
};

TAILQ_HEAD(rb_node_qh, rb_node);

typedef int (*rb_compare_nodes_fn)(const struct rb_node *,
    const struct rb_node *);
typedef int (*rb_compare_key_fn)(const struct rb_node *, const void *);
typedef void (*rb_print_node_fn)(const struct rb_node *, const char *);

struct rb_tree {
	struct rb_node *rbt_root;
	struct rb_node_qh rbt_nodes;
	rb_compare_nodes_fn rbt_compare_nodes;
	rb_compare_key_fn rbt_compare_key;
	rb_print_node_fn rbt_print_node;
	u_int rbt_count;
};

void	rb_tree_init(struct rb_tree *, rb_compare_nodes_fn, rb_compare_key_fn,
	    rb_print_node_fn);
void	rb_tree_insert_node(struct rb_tree *, struct rb_node *);
struct rb_node	*rb_tree_find(struct rb_tree *, void *);
void	rb_tree_remove_node(struct rb_tree *, struct rb_node *);

#endif	/* _LIBKERN_RB_H_*/
