/*	$NetBSD: _context_u.S,v 1.1.2.3 2001/11/19 21:07:23 briggs Exp $	*/

/*
 * Copyright (c) 2001 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Allen Briggs for Wasabi Systems, Inc.
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
 *      This product includes software developed for the NetBSD Project by
 *      Wasabi Systems, Inc.
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

#define GETC(reg)					  \
	stw	 r1, (_REG_R0 +  1*4)(reg)		; \
	stw	 r2, (_REG_R0 +  2*4)(reg)		; \
	stw	r14, (_REG_R0 + 14*4)(reg)		; \
	stw	r15, (_REG_R0 + 15*4)(reg)		; \
	stw	r16, (_REG_R0 + 16*4)(reg)		; \
	stw	r17, (_REG_R0 + 17*4)(reg)		; \
	stw	r18, (_REG_R0 + 18*4)(reg)		; \
	stw	r19, (_REG_R0 + 19*4)(reg)		; \
	stw	r20, (_REG_R0 + 20*4)(reg)		; \
	stw	r21, (_REG_R0 + 21*4)(reg)		; \
	stw	r22, (_REG_R0 + 22*4)(reg)		; \
	stw	r23, (_REG_R0 + 23*4)(reg)		; \
	stw	r24, (_REG_R0 + 24*4)(reg)		; \
	stw	r25, (_REG_R0 + 25*4)(reg)		; \
	stw	r26, (_REG_R0 + 26*4)(reg)		; \
	stw	r27, (_REG_R0 + 27*4)(reg)		; \
	stw	r28, (_REG_R0 + 28*4)(reg)		; \
	stw	r29, (_REG_R0 + 29*4)(reg)		; \
	stw	r30, (_REG_R0 + 30*4)(reg)		; \
	stw	r31, (_REG_R0 + 31*4)(reg)		; \
	mfcr	 r0					; \
	stw	 r0, _REG_CR(reg)			; \
	mflr	 r0					; \
	stw	 r0, _REG_LR(reg)			; \
	/* PC/SRR0 is not a user-accessible register */	; \
	stw	 r0, _REG_PC(reg)			; \
	mfctr	 r0					; \
	stw	 r0, _REG_CTR(reg)			; \
	mfxer	 r0					; \
	stw	 r0, _REG_XER(reg)			; \
	li	 r8, 1					; \
	slwi	 r9, r8, _UC_USER_BIT			; \
	ori	 r9, r9, _UC_CPU			; \
	stw	 r9, (UC_FLAGS)(reg)

#define SETC						  \
	lwz	 r1, (_REG_R0 +  1*4)(r4)		; \
	lwz	 r2, (_REG_R0 +  2*4)(r4)		; \
	lwz	 r3, (_REG_R0 +  3*4)(r4)		; \
	lwz	 r5, (_REG_R0 +  5*4)(r4)		; \
	lwz	 r6, (_REG_R0 +  6*4)(r4)		; \
	lwz	 r7, (_REG_R0 +  7*4)(r4)		; \
	lwz	 r8, (_REG_R0 +  8*4)(r4)		; \
	lwz	 r9, (_REG_R0 +  9*4)(r4)		; \
	lwz	r10, (_REG_R0 + 10*4)(r4)		; \
	lwz	r11, (_REG_R0 + 11*4)(r4)		; \
	lwz	r12, (_REG_R0 + 12*4)(r4)		; \
	lwz	r13, (_REG_R0 + 13*4)(r4)		; \
	lwz	r14, (_REG_R0 + 14*4)(r4)		; \
	lwz	r15, (_REG_R0 + 15*4)(r4)		; \
	lwz	r16, (_REG_R0 + 16*4)(r4)		; \
	lwz	r17, (_REG_R0 + 17*4)(r4)		; \
	lwz	r18, (_REG_R0 + 18*4)(r4)		; \
	lwz	r19, (_REG_R0 + 19*4)(r4)		; \
	lwz	r20, (_REG_R0 + 20*4)(r4)		; \
	lwz	r21, (_REG_R0 + 21*4)(r4)		; \
	lwz	r22, (_REG_R0 + 22*4)(r4)		; \
	lwz	r23, (_REG_R0 + 23*4)(r4)		; \
	lwz	r24, (_REG_R0 + 24*4)(r4)		; \
	lwz	r25, (_REG_R0 + 25*4)(r4)		; \
	lwz	r26, (_REG_R0 + 26*4)(r4)		; \
	lwz	r27, (_REG_R0 + 27*4)(r4)		; \
	lwz	r28, (_REG_R0 + 28*4)(r4)		; \
	lwz	r29, (_REG_R0 + 29*4)(r4)		; \
	lwz	r30, (_REG_R0 + 30*4)(r4)		; \
	lwz	r31, (_REG_R0 + 31*4)(r4)		; \
	lwz	 r0, _REG_CR(r4)			; \
	mtcr	 r0					; \
	lwz	 r0, _REG_LR(r4)			; \
	mtlr	 r0					; \
	/* PC/SRR0 is not a user-accessible register */	; \
	/* yet CTR is callee-saved, so we snarf CTR  */	; \
	lwz	 r0, _REG_PC(r4)			; \
	mtctr	 r0					; \
	lwz	 r0, _REG_XER(r4)			; \
	mtxer	 r0					; \
	lwz	 r4, (_REG_R0 +  4*4)(r4)		; \
	bctr						; \
	/* NOTREACHED */

ENTRY(_getcontext_u)
	GETC(r3)
	xor r3,r3,r3
	blr

ENTRY(_setcontext_u)
	mr r4,r3
	SETC

ENTRY(_swapcontext_u)
	GETC(r3)
	SETC
