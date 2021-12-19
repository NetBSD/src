/*	$NetBSD: xarray.h,v 1.3 2021/12/19 10:36:40 riastradh Exp $	*/

/*-
 * Copyright (c) 2020 The NetBSD Foundation, Inc.
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

#ifndef	_LINUX_XARRAY_H_
#define	_LINUX_XARRAY_H_

#include <linux/slab.h>

struct xarray;
struct xa_limit;

struct xa_limit {
	uint32_t	max;
	uint32_t	min;
};

#define	xa_for_each(XA, INDEX, ENTRY)					      \
	for ((INDEX) = 0, (ENTRY) = xa_find((XA), &(INDEX), ULONG_MAX, 0);    \
		(ENTRY) != NULL;					      \
		(ENTRY) = xa_find_after((XA), &(INDEX), ULONG_MAX, 0))

#define	xa_alloc	linux_xa_alloc
#define	xa_find		linux_xa_find
#define	xa_find_after	linux_xa_find_after
#define	xa_store	linux_xa_store
#define	xa_limit_32b	linux_xa_limit_32b

int	xa_alloc(struct xarray *, uint32_t *, void *, struct xa_limit, gfp_t);
void *	xa_find(struct xarray *, unsigned long *, unsigned long, unsigned);
void *	xa_find_after(struct xarray *, unsigned long *, unsigned long,
	    unsigned);
void	xa_store(struct xarray *, unsigned long, void *, gfp_t);

extern const struct xa_limit xa_limit_32b;

#endif	/* _LINUX_XARRAY_H_ */
