/*	$NetBSD: _context_u.S,v 1.1.2.4 2001/11/20 18:31:21 briggs Exp $	*/

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
	stmw	r14, (_REG_R0 + 14*4)(reg)		; \
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
	li	 r9, 1					; \
	slwi	 r9, r9, _UC_USER_BIT			; \
	lwz	 r3, (UC_FLAGS)(r4)			; \
	and.	 r3, r3, r9				; \
	beq	 1f					; \
							; \
	lwz	 r1, (_REG_R0 +  1*4)(r4)		; \
	lmw	r14, (_REG_R0 + 14*4)(r4)		; \
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
	bctr						; \
1:							; \
	mr	r3, r4					; \
	b	PIC_PLT(_C_LABEL(setcontext))		; \
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
