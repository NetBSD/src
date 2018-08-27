/*	$NetBSD: idr.h,v 1.4 2018/08/27 06:54:51 riastradh Exp $	*/

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
#include <sys/queue.h>

#include <linux/gfp.h>

/* XXX Stupid expedient algorithm should be replaced by something better.  */

struct idr {
	kmutex_t	idr_lock;
	rb_tree_t	idr_tree;
};

/* XXX Make the nm output a little more greppable...  */
#define	idr_alloc		linux_idr_alloc
#define	idr_destroy		linux_idr_destroy
#define	idr_find		linux_idr_find
#define	idr_for_each		linux_idr_for_each
#define	idr_init		linux_idr_init
#define	idr_is_empty		linux_idr_is_empty
#define	idr_preload		linux_idr_preload
#define	idr_preload_end		linux_idr_preload_end
#define	idr_remove		linux_idr_remove
#define	idr_replace		linux_idr_replace

int	linux_idr_module_init(void);
void	linux_idr_module_fini(void);

void	idr_init(struct idr *);
void	idr_destroy(struct idr *);
bool	idr_is_empty(struct idr *);
void	*idr_find(struct idr *, int);
void	*idr_replace(struct idr *, void *, int);
void	idr_remove(struct idr *, int);
void	idr_preload(gfp_t);
int	idr_alloc(struct idr *, void *, int, int, gfp_t);
void	idr_preload_end(void);
int	idr_for_each(struct idr *, int (*)(int, void *, void *), void *);

struct ida {
	struct idr	ida_idr;
};

static inline void
ida_init(struct ida *ida)
{

	idr_init(&ida->ida_idr);
}

static inline void
ida_destroy(struct ida *ida)
{

	idr_destroy(&ida->ida_idr);
}

static inline void
ida_remove(struct ida *ida, int id)
{

	idr_remove(&ida->ida_idr, id);
}

static inline int
ida_simple_get(struct ida *ida, unsigned start, unsigned end, gfp_t gfp)
{
	int id;

	KASSERT(start <= INT_MAX);
	KASSERT(end <= INT_MAX);

	idr_preload(gfp);
	id = idr_alloc(&ida->ida_idr, NULL, start, end, gfp);
	idr_preload_end();

	return id;
}

#endif  /* _LINUX_IDR_H_ */
