/*	$NetBSD: __setjmp14.S,v 1.3.2.1 2001/10/08 20:17:58 nathanw Exp $	*/

/*-
 * Copyright (c) 1990 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * William Jolitz.
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
 *	from: @(#)setjmp.s	5.1 (Berkeley) 4/23/90
 */

#include <machine/asm.h>
#if defined(LIBC_SCCS)
	RCSID("$NetBSD: __setjmp14.S,v 1.3.2.1 2001/10/08 20:17:58 nathanw Exp $")
#endif

/*
 * C library -- _setjmp, _longjmp
 *
 *	longjmp(a,v)
 * will generate a "return(v)" from the last call to
 *	setjmp(a)
 * by restoring registers from the stack.
 * The previous signal state is restored.
 */

ENTRY(__setjmp14)
	sts.l	pr, @-r15
	mov.l	r4, @-r15
	mov	r4, r6
	mov	#1, r4		/* SIG_BLOCK */
	mov	#0, r5
	add	#4*10, r6	/* &sigmask */

	mov.l	L__sigprocmask14, r0
	jsr	@r0
	nop

	mov.l	@r15+, r4
	lds.l	@r15+, pr
	add	#4*10, r4
	mov	#1, r0
	mov.l	r0, @-r4	/* savemask */
	mov.l	r15, @-r4
	mov.l	r14, @-r4
	mov.l	r13, @-r4
	mov.l	r12, @-r4
	mov.l	r11, @-r4
	mov.l	r10, @-r4
	mov.l	r9, @-r4
	mov.l	r8, @-r4
	sts.l	pr, @-r4
	rts
	xor	r0, r0

ENTRY(__longjmp14)
	mov.l	r4, @-r15
	mov.l	r5, @-r15

	mov	r4, r5
	mov	#3, r4		/* SIG_SETMASK */
	add	#4*10, r5	/* &sigmask */
	mov	#0, r6
	mov.l	L__sigprocmask14, r0
	jsr	@r0
	nop
	mov.l	@r15+, r5
	mov.l	@r15+, r4

	lds.l	@r4+, pr
	mov.l	@r4+, r8
	mov.l	@r4+, r9
	mov.l	@r4+, r10
	mov.l	@r4+, r11
	mov.l	@r4+, r12
	mov.l	@r4+, r13
	mov.l	@r4+, r14
	mov.l	@r4+, r15
	mov	r5, r0
	tst	r0, r0
	bf	.L0
	add	#1, r0
.L0:
	rts
	nop

	.align	2
L__sigprocmask14:
	.long	_C_LABEL(__sigprocmask14)
