/*	$NetBSD: idr.h,v 1.1.2.8 2014/01/15 13:51:48 riastradh Exp $	*/

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

#ifndef _LINUX_IDR_H_
#define _LINUX_IDR_H_

#include <sys/types.h>
#include <sys/rwlock.h>
#include <sys/rbtree.h>

/* XXX Stupid expedient algorithm should be replaced by something better.  */

struct idr {
	krwlock_t idr_lock;
	rb_tree_t idr_tree;
	struct idr_node *idr_temp;
};

/* XXX Make the nm output a little more greppable...  */
#define	idr_init		linux_idr_init
#define	idr_destroy		linux_idr_destroy
#define	idr_find		linux_idr_find
#define	idr_replace		linux_idr_replace
#define	idr_remove		linux_idr_remove
#define	idr_remove_all		linux_idr_remove_all
#define	idr_pre_get		linux_idr_pre_get
#define	idr_get_new_above	linux_idr_get_new_above
#define	idr_for_each		linux_idr_for_each

void	idr_init(struct idr *);
void	idr_destroy(struct idr *);
void	*idr_find(struct idr *, int);
void	*idr_replace(struct idr *, void *, int);
void	idr_remove(struct idr *, int);
void	idr_remove_all(struct idr *);
int	idr_pre_get(struct idr *, int);
int	idr_get_new_above(struct idr *, void *, int, int *);
int	idr_for_each(struct idr *, int (*)(int, void *, void *), void *);

#endif  /* _LINUX_IDR_H_ */
