/*	$NetBSD: locore.S,v 1.2 2002/02/28 01:53:42 uch Exp $	*/

/*-
 * Copyright (c) 2002 The NetBSD Foundation, Inc.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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

#include "assym.h"

#include <sh3/asm.h>
#include <sh3/locore.h>
#include <sh3/trapreg.h>
/*
 * void start(int argc, char *argv[], struct bootinfo *bi) 
 *    __attribute__((__noreturn__)):
 *	Kernel entry point. setup bootstrap stack pointer.
 *	Don't pollute r4, r5, r6, since these are passed down to
 *	machine_startup().
 */
NENTRY(start)
	mov.l	_L.machine_startup, r0
	mov.l	_L.bootstrap_stack, r1
	jmp	@r0
	 mov	r1,	r15
	/* NOTREACHED */
	.align	2
_L.machine_startup:	.long	_C_LABEL(machine_startup)
_L.bootstrap_stack:	.long	_C_LABEL(start)

/*
 * void interrupt_exp(int, int, int, int, stuct trapframe)
 *    __attribute__((__noreturn__)):
 *	on entry, SR.BL = 1, SR_RB = 0, all regsiters are saved,
 *	stack is already setuped.	
 */
	.align	2
recurse:
	stc	sr,	r4
	ldc	r5,	spc	
	ldc	r4,	ssr
	RECURSEENTRY
	bra	1f
	 nop
NENTRY(interrupt_exp)
1:
	MOV	(INTEVT, r0)
	mov.l	@r0,	r0
	mov.l	r0, @(TF_TRAPNO, r15)	/* trapframe->tf_trapno = INTEVT */
	__INTR_MASK(r0, r1)		/* mask all interrupt */
	__EXCEPTION_UNBLOCK(r0, r1)	/* enable exception for TLB handling */
	mov.l	_L.intrhandler, r0
	jsr	@r0
 	 nop
	tst	r0,	r0	/* intrhandler() == 0, fast intr return */
	bt	2f

	mov.l	_L.check_ipending, r0
	jsr	@r0
	 nop
	tst	r0,	r0
	bf	1b		/* handle pending interrupt. */

	/* Check for ASTs on exit to user mode. */
	mov.l	_L.ast,	r0
	jsr	@r0
	 mov	r15,	r4
2:	
	EXCEPTION_RETURN
	/* NOTREACHED */
	.align	2
REG_SYMBOL(INTEVT)
_L.intrhandler:		.long	_C_LABEL(intrhandler)
_L.ast:			.long	_C_LABEL(ast)

NENTRY(Xspllower)
	sts.l	pr,	@-r15

restart:
	__INTR_MASK(r0, r1)
	__EXCEPTION_UNBLOCK(r0, r1)
	mov.l	_L.check_ipending, r0
	jsr	@r0
	 nop
	tst	r0,	r0
	bt	1f

	mov.l	_L.restart, r5 
	mov.l	_L.recurse, r0
	jmp	@r0
	 nop

1:
	__INTR_UNMASK(r0, r1)
	lds.l	@r15+,	pr
	rts
	 nop

	.align	2
_L.check_ipending:	.long	_C_LABEL(check_ipending)
_L.recurse:		.long	recurse
_L.restart:		.long	restart

	.data
	.align	2
	.globl	_C_LABEL(intrcnt), _C_LABEL(eintrcnt)
	.globl	_C_LABEL(intrnames), _C_LABEL(eintrnames)
_C_LABEL(intrcnt):
_C_LABEL(eintrcnt):
_C_LABEL(intrnames):
_C_LABEL(eintrnames):
