/*	$NetBSD: _context_u.S,v 1.1.2.3 2001/11/20 03:18:44 thorpej Exp $	*/

/*
 * Copyright (c) 2001 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Jason R. Thorpe for Wasabi Systems, Inc.
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
 *	This product includes software developed for the NetBSD Project by
 *	Wasabi Systems, Inc.
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

/*
 * XXX XXX XXX
 * XXX Hardware FP context?
 * XXX Software FP context?
 * XXX XXX XXX
 */

/*
 * XXX We use MRS/MSR here.  Those are not available pre-ARMv3.
 * XXX Either the kernel will have to emulate them, or Ben Harris
 * XXX will have to fix this up to work with older processors.
 */

#ifdef __APCS_26__
#define	SAVE_FLAGS(reg, tmp)		/* nothing; in r15 */
#define	RESTORE_FLAGS(reg, tmp)		/* nothing; in r15 */
#else
#define	SAVE_FLAGS(reg, tmp)						\
	mrs	tmp, cpsr					;	\
	str	tmp, [reg, #(_REG_CPSR)]

#define	RESTORE_FLAGS(reg, tmp)						\
	ldr	tmp, [reg, #(_REG_CPSR)]			;	\
	msr	cpsr, tmp
#endif /* __APCS_26__ */

#define	GETC(reg, tmp)							\
	/*								\
	 * We're only saving the callee-saved registers, here,		\
	 * so make a note of it for SETC() later.			\
	 *								\
	 * Make sure to save the flags first, since ORR changes them.	\
	 */								\
	SAVE_FLAGS(reg, tmp)					;	\
	ldr	tmp, [reg, #(UC_FLAGS)]				;	\
	orr	tmp, tmp, #(_UC_USER)				;	\
	orr	tmp, tmp, #(_UC_CPU)				;	\
	str	tmp, [reg, #(UC_FLAGS)]				;	\
	add	reg, reg, #(_REG_R4)				;	\
	stmia	reg, {r4-r15}

#define	SETC(reg, tmp)							\
	/*								\
	 * If _UC_USER is set, we only need to restore the		\
	 * callee-saved registers (i.e. we were saved by		\
	 * _getcontext_u().  If it is not set, we need to		\
	 * restore the caller-saved regs, too.				\
	 *								\
	 * Restore flags after we fixup the pointer, since		\
	 * ADD changes them.						\
	 */								\
	ldr	tmp, [reg, #(UC_FLAGS)]				;	\
	tst	tmp, #(_UC_USER)				;	\
	beq	1f						;	\
									\
	/* _UC_USER case */					;	\
	add	reg, reg, #(_REG_R4)				;	\
	RESTORE_FLAGS(reg, tmp)					;	\
	ldmia	reg, {r4-r15}					;	\
	/* NOTREACHED */						\
									\
	/* ! _UC_USER case */						\
1:	add	reg, reg, #(_REG_R0)				;	\
	RESTORE_FLAGS(reg, tmp)					;	\
	ldmia	reg, {r0-r15}					;	\
	/* NOTREACHED */

ENTRY(_getcontext_u)
	GETC(r0, r1)
	mov	r0, #0x00000000
#ifdef __APCS_26__
	movs	pc, lr
#else
	mov	pc, lr
#endif

ENTRY(_setcontext_u)
	SETC(r0, r1)

ENTRY(_swapcontext_u)
	GETC(r0, r2)
	SETC(r1, r2)
