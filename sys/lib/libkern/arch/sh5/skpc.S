/*	$NetBSD: skpc.S,v 1.1 2002/10/17 11:53:33 scw Exp $	*/

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
 * int skpc(int mask, size_t size, u_char *cp)
 *
 * XXX: Room for some optimising here, specifically to use quad loads.
 */
ENTRY(skpc)
	andi	r2, 0xff, r2
#ifndef _LP64
	add.l	r4, r63, r4
	addz.l	r3, r4, r3		/* r3 = &cp[size] */
#else
	add	r3, r4, r3		/* r3 = &cp[size] */
#endif
	pta/l	1f, tr1
	pta/u	2f, tr2
	ptabs/u	r18, tr0
1:	bgeu/u	r4, r3, tr2		/* Break loop if reached end of "cp" */
	ld.ub	r4, 0, r0		/* r0 = *cp */
	addi	r4, 1, r4		/* cp++ */
	beq/l	r0, r2, tr1		/* Back around as long as r0 == mask */
	addi	r4, -1, r4		/* Correct for overshoot */
2:	sub	r3, r4, r2		/* return val = r3 - cp */
	blink	tr0, r63
