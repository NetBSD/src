/*	$NetBSD: _context_u.S,v 1.3 2008/04/28 20:23:02 martin Exp $	*/

/*
 * Copyright (c) 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Wayne Knowles
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
#include <machine/mcontext.h>
#include <sys/cdefs.h>
#include "assym.h"

/*
 * Define:
 *	int _getcontext_u(ucontext_t *ctx)
 *		Store the current context in the provided ctx structure.
 *		[only store the callee-saved registers]
 *	int _setcontext_u(const ucontext_t *ctx)
 *		Restore the current context from the provided ctx structure.
 *	int _swapcontext_u(ucontext_t *from_ctx, const ucontext_t *to_ctx)
 *		First, store the current context into from_ctx and then
 *		restore the current context from the to_ctx.
 */

/* Macro to Save the callee-save register set */
#define GETC(reg)					! \
	stw	%r3,  (UC_REGS +  3 * SZREG)(reg)	! \
	stw	%r4,  (UC_REGS +  4 * SZREG)(reg)	! \
	stw	%r5,  (UC_REGS +  5 * SZREG)(reg)	! \
	stw	%r6,  (UC_REGS +  6 * SZREG)(reg)	! \
	stw	%r7,  (UC_REGS +  7 * SZREG)(reg)	! \
	stw	%r8,  (UC_REGS +  8 * SZREG)(reg)	! \
	stw	%r9,  (UC_REGS +  9 * SZREG)(reg)	! \
	stw	%r10, (UC_REGS + 10 * SZREG)(reg)	! \
	stw	%r11, (UC_REGS + 11 * SZREG)(reg)	! \
	stw	%r12, (UC_REGS + 12 * SZREG)(reg)	! \
	stw	%r13, (UC_REGS + 13 * SZREG)(reg)	! \
	stw	%r14, (UC_REGS + 14 * SZREG)(reg)	! \
	stw	%r15, (UC_REGS + 15 * SZREG)(reg)	! \
	stw	%r16, (UC_REGS + 16 * SZREG)(reg)	! \
	stw	%r17, (UC_REGS + 17 * SZREG)(reg)	! \
	stw	%r18, (UC_REGS + 18 * SZREG)(reg)	! \
	stw	%dp,  (UC_REGS + 27 * SZREG)(reg)	! \
	stw	%sp,  (UC_REGS + 30 * SZREG)(reg)	! \
	stw	%rp,  (UC_REGS +  2 * SZREG)(reg)	! \
	stw	%rp,  (UC_REGS + 35 * SZREG)(reg)	! \
	ldil	L%_UC_USER, %r1				! \
	ldo	_UC_CPU(%r1), %r1 			! \
	stw	%r1, (UC_FLAGS)(reg)

#ifdef PIC
#define LDADDR						! \
	addil	LT%_C_LABEL(setcontext), %r19		! \
	ldo	RT%_C_LABEL(setcontext)(%r1), %r1	! \
	bb,<,n	%r1, 30, 99f				! \
	depi	0, 31, 2, %r1				! \
	ldw	0(%r1), %rp				! \
	ldw	4(%r1), %r19				! \
99:
#else
#define LDADDR						! \
	ldil	L%_C_LABEL(setcontext), %rp		! \
	ldo	R%_C_LABEL(setcontext)(%rp), %rp
#endif

#define SETC(reg)					! \
	ldw	(UC_FLAGS)(reg), %r1			! \
	bb,>=,n	%r1, (31 - _UC_USER_BIT), 1f		! \
	ldw	(UC_REGS +  3 * SZREG)(reg), %r3	! \
	ldw	(UC_REGS +  4 * SZREG)(reg), %r4	! \
	ldw	(UC_REGS +  5 * SZREG)(reg), %r5	! \
	ldw	(UC_REGS +  6 * SZREG)(reg), %r6	! \
	ldw	(UC_REGS +  7 * SZREG)(reg), %r7	! \
	ldw	(UC_REGS +  8 * SZREG)(reg), %r8	! \
	ldw	(UC_REGS +  9 * SZREG)(reg), %r9	! \
	ldw	(UC_REGS + 10 * SZREG)(reg), %r10	! \
	ldw	(UC_REGS + 11 * SZREG)(reg), %r11	! \
	ldw	(UC_REGS + 12 * SZREG)(reg), %r12	! \
	ldw	(UC_REGS + 13 * SZREG)(reg), %r13	! \
	ldw	(UC_REGS + 14 * SZREG)(reg), %r14	! \
	ldw	(UC_REGS + 15 * SZREG)(reg), %r15	! \
	ldw	(UC_REGS + 16 * SZREG)(reg), %r16	! \
	ldw	(UC_REGS + 17 * SZREG)(reg), %r17	! \
	ldw	(UC_REGS + 18 * SZREG)(reg), %r18	! \
	ldw	(UC_REGS + 27 * SZREG)(reg), %dp	! \
	ldw	(UC_REGS + 30 * SZREG)(reg), %sp	! \
	ldw	(UC_REGS +  2 * SZREG)(reg), %rp	! \
	/* part procedure call, part RET */		! \
	ldw	(UC_REGS + _REG_PCOQH * SZREG)(reg), %r1! \
	bv,n	%r0(%r1)				! \
	/* NOTREACHED */				! \
							! \
1:							! \
	LDADDR						! \
	bv,n	%r0(%rp)				! \
	/* NOTREACHED */

ENTRY(_getcontext_u, 0)
	GETC(%arg0)
	bv	%r0(%rp)
	copy	%r0, %ret0
EXIT(_getcontext_u)

ENTRY(_setcontext_u, 0)
	SETC(%arg0)
EXIT(_setcontext_u)

ENTRY(_swapcontext_u, 0)
	GETC(%arg0)
	SETC(%arg1)
EXIT(_swapcontext_u)

LEAF_ENTRY(pthread__ras_simple_lock_init)
	ldi	1,%ret0			/* 1 == unlocked */
	stw	%ret0,0(%arg0)
	stw	%ret0,4(%arg0)
	stw	%ret0,8(%arg0)
	stw	%ret0,12(%arg0)
	bv,n	%r0(%rp)
EXIT(pthread__ras_simple_lock_init)

	.global pthread__lock_ras_start
	.global pthread__lock_ras_end

LEAF_ENTRY(pthread__ras_simple_lock_try)
	ldo	15(%arg0),%arg0
	depi	0,31,4,%arg0

pthread__lock_ras_start:
	ldw	0(%arg0),%ret0
	stw	%r0,0(%arg0)		/* 0 == locked */
pthread__lock_ras_end:

	comiclr,= 0,%ret0,%ret0		/* if locked return 0 */
	ldi	1,%ret0			/* else return 1 */
	bv,n	%r0(%rp)
EXIT(pthread__ras_simple_lock_try)


LEAF_ENTRY(pthread__ras_simple_unlock)
	ldo	15(%arg0),%arg0
	depi	0,31,4,%arg0
	ldi	1,%ret0			/* 1 == unlocked */
	bv	%r0(%rp)
	stw	%ret0,0(%arg0)
EXIT(pthread__ras_simple_unlock)
