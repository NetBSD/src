/*	$NetBSD: brk.S,v 1.3 2000/06/26 06:25:38 kleink Exp $	*/

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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
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
 *	from: @(#)brk.s	5.2 (Berkeley) 12/17/90
 */

#include "SYS.h"

	.globl	_end
	.globl	curbrk

#ifdef WEAK_ALIAS
WEAK_ALIAS(brk, _brk)
#endif

	.data
	.align	0
	.globl	_C_LABEL(__minbrk)
	.type	_C_LABEL(__minbrk),#object
_C_LABEL(__minbrk):
	.word	_end

/*
 * Change the data segment size
 */
ENTRY(_brk)
#ifdef PIC
	/* Setup the GOT */
	ldr	r3, Lgot
	add	r3, pc, r3
L1:
	ldr	r1, Lminbrk
	ldr	r1, [r3, r1]
#else
	ldr	r1, Lminbrk
#endif
	/* Get the minimum allowable brk address */
	ldr	r1, [r1]

	/*
	 * Valid the address specified and set to the minimum
	 * if the address is below minbrk.
	 */
	cmp	r0, r1
	movlt	r0, r1
	mov	r2, r0
	swi	SYS_break
	bcs	cerror

#ifdef PIC
	ldr	r1, Lcurbrk
	ldr	r1, [r3, r1]
#else
	ldr	r1, Lcurbrk
#endif
	/* Store the new address in curbrk */
	str	r2, [r1]

	/* Return 0 for success */
	mov	r0, #0x00000000
#ifdef __APCS_26__
	movs	r15, r14
#else
	mov	r15, r14
#endif

	.align	0
#ifdef PIC
Lgot:
	.word	__GLOBAL_OFFSET_TABLE_+. - (L1+4)
#endif
Lminbrk:
	.word	_C_LABEL(__minbrk)
Lcurbrk:
	.word	curbrk
