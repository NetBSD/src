/* $NetBSD: pmap_page.S,v 1.1 2014/08/10 05:47:37 matt Exp $ */

/*-
 * Copyright (c) 2014 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Matt Thomas of 3am Software Foundry.
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

#include <aarch64/asm.h>

#include "assym.h"

RCSID("$NetBSD: pmap_page.S,v 1.1 2014/08/10 05:47:37 matt Exp $");

/* LINTSTUB: void pmap_zero_page(paddr_t pa); */
ENTRY(pmap_zero_page)
	movk	x0, #0xffff, lsl #48	/* convert to direct mapped */
        add	x4, x0, #(PAGE_SIZE >> 12), lsl #12
	mrs	x2, dczid_el0
	tbnz	x2, #4, 2f

	mov	x3, #16
	lsl	x3, x3, x2

1:	dc	zva, x0
	add	x0, x0, x3
	cmp	x0, x4
	b.ne	1b
	ret

2:	stnp	xzr, xzr, [x0, #0]
	stnp	xzr, xzr, [x0, #16]
	stnp	xzr, xzr, [x0, #32]
	stnp	xzr, xzr, [x0, #48]
	add	x0, x0, #64
	cmp	x0, x4
	b.ne	2b
	ret
END(pmap_zero_page)

/* LINTSTUB: void pmap_zero_page(paddr_t src_pa, paddr_t dst_pa); */
ENTRY(pmap_copy_page)
	movk	x0, #0xffff, lsl #48	/* convert to direct mapped */
	movk	x1, #0xffff, lsl #48	/* convert to direct mapped */
        add	x10, x0, #(PAGE_SIZE >> 12), lsl #12

1:	ldnp	x2, x3, [x0, #0]
	ldnp	x4, x5, [x0, #16]
	ldnp	x6, x7, [x0, #32]
	ldnp	x8, x9, [x0, #48]
	add	x0, x0, #64
	stnp	x2, x3, [x1, #0]
	stnp	x4, x5, [x1, #16]
	stnp	x6, x7, [x1, #32]
	stnp	x8, x9, [x1, #48]
	add	x1, x1, #64
	cmp	x0, x10
	b.ne	1b
	ret
END(pmap_copy_page)
