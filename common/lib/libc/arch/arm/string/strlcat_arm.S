/*-
 * Copyright (c) 2013 The NetBSD Foundation, Inc.
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

#include <machine/asm.h>

RCSID("$NetBSD: strlcat_arm.S,v 1.1 2013/01/23 07:02:45 matt Exp $")

/* LINTSTUB: size_t strlcat(char *, const char *, size_t) */
ENTRY(strlcat)
	push	{r4-r6, lr}
	mov	r4, r0			/* save dst */
	mov	r5, r1			/* save src */
	mov	r6, r2			/* save siz */
        
	mov	r1, r6			/* get siz */
	bl	PLT_SYM(strnlen)	/* find length of dst (up to siz) */

	sub	r2, r6, r0		/* get remaining space in dst */
	mov	r1, r5			/* restore src */
	mov	r5, r0			/* save remaining space */
	add	r0, r0, r4		/* set start to end of dst */
	bl	PLT_SYM(strlcpy)	/* attempt to copy src */
	add	r0, r0, r5		/* add remaining to strlcpy return */
	pop	{r4-r6, lr}		/* restore registers */
	RET				/* return */
END(strlcat)
