/*	$NetBSD: sigsetjmp.S,v 1.2 2000/08/04 08:47:04 tsubai Exp $	*/

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
 *	from: @(#)setjmp.s	5.1 (Berkeley) 4/23/90"
 */

#include <machine/asm.h>
#if defined(LIBC_SCCS)
	RCSID("$NetBSD: sigsetjmp.S,v 1.2 2000/08/04 08:47:04 tsubai Exp $")
#endif

ENTRY(sigsetjmp)
	mov	r4, r0
	add	#4*9, r0	! &savemask
	mov.l	r5, @r0
	tst	r5, r5
	bt	1f
	
	sts.l	pr, @-r15
	mov.l	r4, @-r15
	mov	#0, r4
	mov.l	Lsigblock, r0
	jsr	@r0
	nop
	mov.l	@r15+, r4
	lds.l	@r15+, pr

	mov	r4, r1
	add	#4*10, r1
	mov.l	r0, @r1
	
1:
	add	#4*9, r4
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

	.align	2
Lsigblock:
	.long	_sigblock

ENTRY(siglongjmp)
	mov	r4, r0
	add	#4*9, r0
	mov.l	@r0, r0
	tst	r0, r0
	bt	1f

	mov.l	r4, @-r15
	mov.l	r5, @-r15
	mov	r4, r0
	add	#4*10, r0
	mov.l	@r0, r4
	mov.l	Lsigsetmask, r0
	jsr	@r0
	nop
	mov.l	@r15+, r5
	mov.l	@r15+, r4

1:	
	mov	r5, r0
	lds.l	@r4+, pr
	mov.l	@r4+, r8
	mov.l	@r4+, r9
	mov.l	@r4+, r10
	mov.l	@r4+, r11
	mov.l	@r4+, r12
	mov.l	@r4+, r13
	mov.l	@r4+, r14
	mov.l	@r4+, r15
	tst	r0, r0
	bf	.L0
	add	#1, r0
.L0:
	rts
	nop

	.align 2
Lsigsetmask :
	.long	_sigsetmask
