/*	$NetBSD: _context_u.S,v 1.1.2.1 2001/11/13 20:21:23 briggs Exp $	*/

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
	stw	 1, (_REG_R0 +  1*4)(reg)		; \
	stw	 2, (_REG_R0 +  2*4)(reg)		; \
	stw	14, (_REG_R0 + 14*4)(reg)		; \
	stw	15, (_REG_R0 + 15*4)(reg)		; \
	stw	16, (_REG_R0 + 16*4)(reg)		; \
	stw	17, (_REG_R0 + 17*4)(reg)		; \
	stw	18, (_REG_R0 + 18*4)(reg)		; \
	stw	19, (_REG_R0 + 19*4)(reg)		; \
	stw	20, (_REG_R0 + 20*4)(reg)		; \
	stw	21, (_REG_R0 + 21*4)(reg)		; \
	stw	22, (_REG_R0 + 22*4)(reg)		; \
	stw	23, (_REG_R0 + 23*4)(reg)		; \
	stw	24, (_REG_R0 + 24*4)(reg)		; \
	stw	25, (_REG_R0 + 25*4)(reg)		; \
	stw	26, (_REG_R0 + 26*4)(reg)		; \
	stw	27, (_REG_R0 + 27*4)(reg)		; \
	stw	28, (_REG_R0 + 28*4)(reg)		; \
	stw	29, (_REG_R0 + 29*4)(reg)		; \
	stw	30, (_REG_R0 + 30*4)(reg)		; \
	stw	31, (_REG_R0 + 31*4)(reg)		; \
	mfcr	 0					; \
	stw	 0, _REG_CR(reg)			; \
	mflr	 0					; \
	stw	 0, _REG_LR(reg)			; \
	/* PC/SRR0 is not a user-accessible register */	; \
	stw	 0, _REG_PC(reg)			; \
	mfctr	 0					; \
	stw	 0, _REG_CTR(reg)			; \
	mfxer	 0					; \
	stw	 0, _REG_XER(reg)			; \
	li	 8, 1					; \
	slwi	 9, 8, _UC_USER_BIT			; \
	ori	 9, 9, _UC_CPU				; \
	stw	 9, (UC_FLAGS)(reg)

#define SETC(reg)					  \
	lwz	 1, (_REG_R0 +  1*4)(reg)		; \
	lwz	 2, (_REG_R0 +  2*4)(reg)		; \
	lwz	 3, (_REG_R0 +  3*4)(reg)		; \
	lwz	 5, (_REG_R0 +  5*4)(reg)		; \
	lwz	 6, (_REG_R0 +  6*4)(reg)		; \
	lwz	 7, (_REG_R0 +  7*4)(reg)		; \
	lwz	 8, (_REG_R0 +  8*4)(reg)		; \
	lwz	 9, (_REG_R0 +  9*4)(reg)		; \
	lwz	10, (_REG_R0 + 10*4)(reg)		; \
	lwz	11, (_REG_R0 + 11*4)(reg)		; \
	lwz	12, (_REG_R0 + 12*4)(reg)		; \
	lwz	13, (_REG_R0 + 13*4)(reg)		; \
	lwz	14, (_REG_R0 + 14*4)(reg)		; \
	lwz	15, (_REG_R0 + 15*4)(reg)		; \
	lwz	16, (_REG_R0 + 16*4)(reg)		; \
	lwz	17, (_REG_R0 + 17*4)(reg)		; \
	lwz	18, (_REG_R0 + 18*4)(reg)		; \
	lwz	19, (_REG_R0 + 19*4)(reg)		; \
	lwz	20, (_REG_R0 + 20*4)(reg)		; \
	lwz	21, (_REG_R0 + 21*4)(reg)		; \
	lwz	22, (_REG_R0 + 22*4)(reg)		; \
	lwz	23, (_REG_R0 + 23*4)(reg)		; \
	lwz	24, (_REG_R0 + 24*4)(reg)		; \
	lwz	25, (_REG_R0 + 25*4)(reg)		; \
	lwz	26, (_REG_R0 + 26*4)(reg)		; \
	lwz	27, (_REG_R0 + 27*4)(reg)		; \
	lwz	28, (_REG_R0 + 28*4)(reg)		; \
	lwz	29, (_REG_R0 + 29*4)(reg)		; \
	lwz	30, (_REG_R0 + 30*4)(reg)		; \
	lwz	31, (_REG_R0 + 31*4)(reg)		; \
	lwz	 0, _REG_CR(reg)			; \
	mtcr	 0					; \
	lwz	 0, _REG_LR(reg)			; \
	mtlr	 0					; \
	/* PC/SRR0 is not a user-accessible register */	; \
	/* yet CTR is callee-saved, so we snarf CTR  */	; \
	lwz	 0, _REG_PC(reg)			; \
	mtctr	 0					; \
	lwz	 0, _REG_XER(reg)			; \
	mtxer	 0					; \
	lwz	 4, (_REG_R0 +  4*4)(reg)		; \
	bctr						; \
	/* NOTREACHED */

ENTRY(_getcontext_u)
	GETC(3)
	xor 3,3,3
	blr

ENTRY(_setcontext_u)
	mr 4,3
	SETC(4)

ENTRY(_swapcontext_u)
	GETC(3)
	SETC(4)
