/*	$NetBSD: srt0.S,v 1.1.2.2 2002/02/28 04:12:00 nathanw Exp $	*/

/*
 * Copyright (c) 2001 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Jason R. Thorpe for Wasabi Systems, Inc.
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
 *	This product includes software developed for the NetBSD Project by
 *	Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <machine/asm.h>

ASENTRY_NP(_start)
	b	_C_LABEL(startup)

ENTRY(cpufunc_id)
	mrc	p15, 0, r0, c0, c0, 0
	mov	pc, lr

	.data

	.global	_C_LABEL(sa110_cache_clean_addr)
_C_LABEL(sa110_cache_clean_addr):
	.word	0x00000000

	.global	_C_LABEL(sa110_cache_clean_size)
_C_LABEL(sa110_cache_clean_size):
	.word	0x00008000

	.text
Lsa110_cache_clean_addr:
	.word	_C_LABEL(sa110_cache_clean_addr)
Lsa110_cache_clean_size:
	.word	_C_LABEL(sa110_cache_clean_size)

ENTRY(sa110_cache_syncI)
	ldr	r2, Lsa110_cache_clean_addr
	ldmia	r2, {r0, r1}

1:	ldr	r2, [r0], #32			/* clean D$ */
	subs	r1, r1, #32
	bne	1b

	mcr	p15, 0, r0, c7, c10, 4		/* drain write buffer */
	mcr	p15, 0, r0, c7, c5, 0		/* flush I$ */

	mov	pc, lr
