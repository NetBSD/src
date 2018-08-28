/*	$NetBSD: cache.S,v 1.1 2018/08/24 02:01:06 jmcneill Exp $	*/

/*-
 * Copyright (c) 2014 Robin Randhawa
 * Copyright (c) 2015 The FreeBSD Foundation
 * All rights reserved.
 *
 * Portions of this software were developed by Andrew Turner
 * under sponsorship from the FreeBSD Foundation
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * $FreeBSD: head/sys/arm64/arm64/cpufunc_asm.S 313347 2017-02-06 17:50:09Z andrew $
 */

#include <aarch64/asm.h>

	.text
	.align	2

/*
 * Macro to handle the cache. This takes the start address in x0, length
 * in x1. It will corrupt x0, x1, x2, and x3.
 */
.macro cache_handle_range dcop = 0, ic = 0, icop = 0
.if \ic == 0
	mrs	x3, ctr_el0
	ubfx	x3, x3, #16, #4		/* x3 = D cache shift */
	mov	x2, #4			/* size of word */
	lsl	x3, x2, x3		/* x3 = D cache line size */
.else
	mrs	x3, ctr_el0
	ubfx	x2, x3, #16, #4		/* x2 = D cache shift */
	and	x3, x3, #15		/* x3 = I cache shift */
	cmp	x3, x2
	bcs	1f
	mov	x3, x2
1:					/* x3 = MAX(IcacheShift,DcacheShift) */
	mov	x2, #4			/* size of word */
	lsl	x3, x2, x3		/* x3 = cache line size */
.endif
	sub	x4, x3, #1		/* Get the address mask */
	and	x2, x0, x4		/* Get the low bits of the address */
	add	x1, x1, x2		/* Add these to the size */
	bic	x0, x0, x4		/* Clear the low bit of the address */
1:
	dc	\dcop, x0
	dsb	ish
.if \ic != 0
	ic	\icop, x0
	dsb	ish
.endif
	add	x0, x0, x3		/* Move to the next line */
	subs	x1, x1, x3		/* Reduce the size */
	b.hi	1b			/* Check if we are done */
.if \ic != 0
	isb
.endif
	ret
.endm


/*
 * void aarch64_dcache_wbinv_range(vaddr_t, vsize_t)
 */
ENTRY(aarch64_dcache_wbinv_range)
	cache_handle_range	dcop = civac
END(aarch64_dcache_wbinv_range)

/*
 * void aarch64_icache_inv_all(void)
 */
ENTRY(aarch64_icache_inv_all)
	dsb	ish
	ic	ialluis
	dsb	ish
	isb
	ret
END(aarch64_icache_inv_all)
