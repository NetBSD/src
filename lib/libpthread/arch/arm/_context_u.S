/*	$NetBSD: _context_u.S,v 1.2 2003/01/18 10:34:18 thorpej Exp $	*/

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
#include <arm/armreg.h>
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
 * NOTE: Clobbers r2.
 */
#define	GETC(reg)							\
	/*								\
	 * We're only saving the callee-saved registers, here,		\
	 * so make a note of it for SETC() later.			\
	 *								\
	 * And, yes, even if we're only saving callee-saved regs,	\
	 * we need to provide a valid CPSR in case the context is	\
	 * used in a setcontext(2) system call later.  (The CPSR	\
	 * is only used in 32-bit mode, and ignored in 26-bit mode.)	\
	 *								\
	 * We play a trick with the PC, here.  We don't save the	\
	 * LR (it's caller-save), but we instead stash it in the	\
	 * PC slot in the ucontext so that when the context is		\
	 * resumed, we magically appear back at the call site.		\
	 */								\
	mov	r2, #(_UC_USER | _UC_CPU)			;	\
	str	r2, [reg, #(UC_FLAGS)]				;	\
	mov	r2, #0						;	\
	mrs	r2, cpsr	/* No-op on 26-bit systems. */	;	\
	str	r2, [reg, #(_REG_CPSR)]				;	\
	str	lr, [reg, #(_REG_PC)]				;	\
	add	reg, reg, #(_REG_R4)				;	\
	stmia	reg, {r4-r13}

/*
 * NOTE: Clobbers r2.
 */
#define	SETC(reg)							\
	/*								\
	 * If _UC_USER is set, we only need to restore the		\
	 * callee-saved registers (i.e. we were saved by		\
	 * _getcontext_u().  If it is not set, we need to		\
	 * restore the caller-saved regs, too.				\
	 */								\
	ldr	r2, [reg, #(UC_FLAGS)]				;	\
	tst	r2, #(_UC_USER)					;	\
									\
	/* _UC_USER case */					;	\
	addne	reg, reg, #(_REG_R4)				;	\
	ldmneia	reg, {r4-r15}					;	\
	/* NOTREACHED */						\
									\
	/* ! _UC_USER case */						\
	teq	r0, r0		/* set Z */			;	\
	teq	pc, r15		/* EQ => 32, NE => 26 */	;	\
	bne	1f						;	\
									\
	/* 32-bit */							\
	ldr	r2, [reg, #(_REG_CPSR)]				;	\
	add	reg, reg, #(_REG_R0)				;	\
	msr	cpsr, r2					;	\
	ldmia	reg, {r0-r15}					;	\
	/* NOTREACHED */						\
									\
1:	/* 26-bit */							\
	add	reg, reg, #(_REG_R0)				;	\
	ldmia	reg, {r0-r15}^					;	\
	/* NOTREACHED */

ENTRY(_getcontext_u)
	GETC(r0)
	mov	r0, #0x00000000
#ifdef __APCS_26__
	movs	pc, lr
#else
	mov	pc, lr
#endif

ENTRY(_setcontext_u)
	SETC(r0)

ENTRY(_swapcontext_u)
	GETC(r0)
	SETC(r1)
