/*	$NetBSD: gfp.h,v 1.3 2014/07/16 20:56:25 riastradh Exp $	*/

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

#ifndef	_LINUX_GFP_H_
#define	_LINUX_GFP_H_

/* GFP: `Get Free Page' */

#include <sys/param.h>
#include <sys/cdefs.h>

typedef int gfp_t;

#define	GFP_ATOMIC	(__GFP_HIGH)
#define	GFP_DMA32	(__GFP_DMA32)
#define	GFP_HIGHUSER	(__GFP_FS | __GFP_HARDWALL | __GFP_HIGHMEM | \
			    __GFP_IO | __GFP_WAIT)
#define	GFP_KERNEL	(__GFP_FS | __GFP_IO | __GFP_WAIT)
#define	GFP_TEMPORARY	(__GFP_FS | __GFP_IO | __GFP_RECLAIMABLE | __GFP_WAIT)
#define	GFP_USER	(__GFP_FS | __GFP_HARDWALL | __GFP_IO | __GFP_WAIT)

#define	GFP_NOWAIT	(GFP_ATOMIC & ~__GFP_HIGH)

#define	__GFP_COMP		__BIT(0)
#define	__GFP_DMA32		__BIT(1)
#define	__GFP_FS		__BIT(2)
#define	__GFP_HARDWALL		__BIT(3)
#define	__GFP_HIGH		__BIT(4)
#define	__GFP_HIGHMEM		__BIT(5)
#define	__GFP_IO		__BIT(6)
#define	__GFP_NORETRY		__BIT(7)
#define	__GFP_NOWARN		__BIT(8)
#define	__GFP_NO_KSWAPD		__BIT(9)
#define	__GFP_RECLAIMABLE	__BIT(10)
#define	__GFP_WAIT		__BIT(11)
#define	__GFP_ZERO		__BIT(12)

/* XXX Make the nm output a little more greppable...  */
#define	alloc_page	linux_alloc_page
#define	__free_page	linux___free_page

struct page;
struct page *	alloc_page(gfp_t);
void		__free_page(struct page *);

#endif	/* _LINUX_GFP_H_ */
