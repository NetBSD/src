/*	$NetBSD: ptrace.S,v 1.1 2000/12/29 20:14:03 bjh21 Exp $ */

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
 *	from: @(#)ptrace.s	5.1 (Berkeley) 4/23/90
 */

#include "SYS.h"

ENTRY(ptrace)
#ifdef _REENTRANT
	stmfd	sp!, {r0-r3, lr}
	bl	_C_LABEL(__errno)
	mov	r1, #0x00000000
	str	r1, [r0]
	ldmfd	sp!, {r0-r3, lr}
#else
	stmfd	sp!, {r0, r1}
#ifdef PIC
	/* Setup the GOT */
	ldr	r0, got
	add	r0, pc, r0
L1:
	ldr	r1, Lerrno
	ldr	r1, [r0, r1]
#else
	ldr	r1, Lerrno
#endif /* PIC */
	mov	r0, #0x00000000
	str	r0, [r1]
	ldmfd	sp!, {r0, r1}
#endif /* _REENTRANT */

	swi	SYS_ptrace
	bcs	cerror
#ifdef __APCS_26__
	movs	pc, lr
#else
	mov	pc, lr
#endif

#ifndef _REENTRANT
#ifdef PIC
	.align	0
got:
	.word	__GLOBAL_OFFSET_TABLE_ + (. - (L1+4))
#endif /* PIC */

Lerrno:
	.word	_C_LABEL(errno)
#endif /* !_REENTRANT */
