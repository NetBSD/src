/*	$NetBSD: radix-tree.h,v 1.7 2021/12/19 11:51:43 riastradh Exp $	*/

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

#ifndef _LINUX_RADIX_TREE_H_
#define _LINUX_RADIX_TREE_H_

#include <sys/radixtree.h>

#include <linux/gfp.h>

#define	INIT_RADIX_TREE			linux_INIT_RADIX_TREE
#define	radix_tree_delete		linux_radix_tree_delete
#define	radix_tree_deref_slot		linux_radix_tree_deref_slot
#define	radix_tree_empty		linux_radix_tree_empty
#define	radix_tree_insert		linux_radix_tree_insert
#define	radix_tree_iter_delete		linux_radix_tree_iter_delete
#define	radix_tree_iter_init		linux_radix_tree_iter_init
#define	radix_tree_lookup		linux_radix_tree_lookup
#define	radix_tree_next_chunk		linux_radix_tree_next_chunk
#define	radix_tree_next_slot		linux_radix_tree_next_slot

struct radix_tree_root {
	struct radix_tree rtr_tree;
};

struct radix_tree_iter {
	unsigned long		index;
	struct radix_tree	*rti_tree;
};

void	INIT_RADIX_TREE(struct radix_tree_root *, gfp_t);

int	radix_tree_insert(struct radix_tree_root *, unsigned long, void *);
void *	radix_tree_delete(struct radix_tree_root *, unsigned long);

bool	radix_tree_empty(struct radix_tree_root *);
void *	radix_tree_lookup(const struct radix_tree_root *, unsigned long);
void *	radix_tree_deref_slot(void **);

void **	radix_tree_iter_init(struct radix_tree_iter *, unsigned long);
void **	radix_tree_next_chunk(const struct radix_tree_root *,
	    struct radix_tree_iter *, unsigned);
void **	radix_tree_next_slot(void **, struct radix_tree_iter *, unsigned);
void	radix_tree_iter_delete(struct radix_tree_root *,
	    struct radix_tree_iter *, void **);

#define	radix_tree_for_each_slot(N, T, I, S)				      \
	for ((N) = radix_tree_iter_init((I), (S));			      \
		(N) || ((N) = radix_tree_next_chunk((T), (I), 0));	      \
		(N) = radix_tree_next_slot((N), (I), 0))

#endif  /* _LINUX_RADIX_TREE_H_ */
