/*	$NetBSD: strncat.S,v 1.1.2.2 2002/12/10 06:25:47 thorpej Exp $	*/

/*
 * Copyright 2002 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Steve C. Woodford for Wasabi Systems, Inc.
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
 *      This product includes software developed for the NetBSD Project by
 *      Wasabi Systems, Inc.
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

/*
 * char *strncat(char *s, const char *append, size_t n)
 *
 * Note:
 * We assume that this routine is mostly called for short strings, and that
 * the overhead of aligning dst/src to permit long or quad copies is just
 * Not Worth The Effort.
 */
ENTRY(strncat)
#ifndef _LP64
	addi.l	r2, -1, r6
	add.l	r3, r63, r3
	addz.l	r4, r63, r4
#else
	addi	r2, -1, r6
#endif
	ptabs/u	r18, tr0
	beq/u	r4, r63, tr0
	pta/l	1f, tr1
	pta/u	2f, tr2
	pta/u	3f, tr3
	movi	0, r5
1:	ld.b	r6, 1, r0		/* Locate the end of string "s" */
	addi	r6, 1, r6
	bne/l	r0, r63, tr1
2:	ldx.b	r3, r5, r0		/* r0 = *append */
	stx.b	r6, r5, r0		/* *s = r0 */
	addi	r5, 1, r5		/* append++, s++ */
	beq/u	r5, r4, tr3		/* Bail if copied `n' bytes */
	bne/l	r0, r63, tr2		/* Loop until r0 == '\0' */
	blink	tr0, r63
3:	stx.b	r6, r5, r63
	blink	tr0, r63
