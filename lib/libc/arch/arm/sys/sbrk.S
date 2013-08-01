/*	$NetBSD: sbrk.S,v 1.10 2013/08/01 05:32:33 matt Exp $	*/

/*-
 * Copyright (c) 1990 The Regents of the University of California.
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
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	from: @(#)sbrk.s	5.1 (Berkeley) 4/23/90
 */

#include "SYS.h"

	.globl	_C_LABEL(_end)

#ifdef WEAK_ALIAS
WEAK_ALIAS(sbrk, _sbrk)
#endif

	.data
	.align	0
	.globl	_C_LABEL(__curbrk)
	.hidden	_C_LABEL(__curbrk)
	.type	_C_LABEL(__curbrk),#object
_C_LABEL(__curbrk):
	.word	_C_LABEL(_end)

/*
 * Change the data segment size
 */
ENTRY(_sbrk)
	/* get address or PC offset to __curbrk */
	ldr	r2, .Lcurbrk
#ifdef PIC
	add	r2, pc, r2
.L1:
#endif

	/* Get the current brk address */
	ldr	r1, [r2]

	/* Calculate new value */
	mov	r3, r0
	add	r0, r0, r1
	SYSTRAP(break)
	bcs	CERROR

	/* Store new curbrk value */
	ldr	r0, [r2]
	add	r1, r0, r3
	str	r1, [r2]

	/* Return old curbrk value */
	RET

	.align	0
.Lcurbrk:
#ifdef PIC
	.word	_C_LABEL(__curbrk) - (.L1+4)
#else
	.word	_C_LABEL(__curbrk)
#endif
