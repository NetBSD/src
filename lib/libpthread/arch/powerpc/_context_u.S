/*	$NetBSD: _context_u.S,v 1.3 2003/06/12 19:09:45 nathanw Exp $	*/

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

/*
 * According to the SVR4 ABI for PPC, r1, r14-r31, LR, and portions of the
 * CR are non-volatile.  I.e., they need to be preserved across the function
 * call.
 *
 * If _getcontext_u is called, set the UC_USER_BIT in the UC_FLAGS.  If
 * _setcontext_u sees that bit, then it will just restore this part of the
 * context, otherwise, it will call through to setcontext(2).
 *
 * Note: 0xd032 (put in MSR) is a magic constant that keeps the kernel
 * setcontext(2) happy as it compares this value to srr1 to see if it
 * should return EINVAL.
 */
#define GETC(reg)					  \
	stw	 %r1, (_REG_R0 +  1*4)(reg)		; \
	stmw	%r14, (_REG_R0 + 14*4)(reg)		; \
	mfcr	 %r0					; \
	stw	 %r0, _REG_CR(reg)			; \
	mflr	 %r0					; \
	stw	 %r0, _REG_LR(reg)			; \
	stw	 %r0, _REG_PC(reg)			; \
							; \
	mffs	%r0					; \
	stfd	%r0, (_REG_FPSCR - 4)(reg)		; \
	stfd	%r14, (_REG_FP0 + 14*8)(reg)		; \
	stfd	%r15, (_REG_FP0 + 15*8)(reg)		; \
	stfd	%r16, (_REG_FP0 + 16*8)(reg)		; \
	stfd	%r17, (_REG_FP0 + 17*8)(reg)		; \
	stfd	%r18, (_REG_FP0 + 18*8)(reg)		; \
	stfd	%r19, (_REG_FP0 + 19*8)(reg)		; \
	stfd	%r20, (_REG_FP0 + 20*8)(reg)		; \
	stfd	%r21, (_REG_FP0 + 21*8)(reg)		; \
	stfd	%r22, (_REG_FP0 + 22*8)(reg)		; \
	stfd	%r23, (_REG_FP0 + 23*8)(reg)		; \
	stfd	%r24, (_REG_FP0 + 24*8)(reg)		; \
	stfd	%r25, (_REG_FP0 + 25*8)(reg)		; \
	stfd	%r26, (_REG_FP0 + 26*8)(reg)		; \
	stfd	%r27, (_REG_FP0 + 27*8)(reg)		; \
	stfd	%r28, (_REG_FP0 + 28*8)(reg)		; \
	stfd	%r29, (_REG_FP0 + 29*8)(reg)		; \
	stfd	%r30, (_REG_FP0 + 30*8)(reg)		; \
	stfd	%r31, (_REG_FP0 + 31*8)(reg)		; \
							; \
	li	 %r8, 1					; \
	stw	 %r8, (_UC_FPVALID)(reg)		; \
	slwi	 %r9, %r8, _UC_USER_BIT			; \
	ori	 %r9, %r9, (_UC_CPU | _UC_FPU)		; \
	stw	 %r9, (UC_FLAGS)(reg)

#define SETC						  \
	li	 %r9, 1					; \
	slwi	 %r9, %r9, _UC_USER_BIT			; \
	lwz	 %r3, (UC_FLAGS)(%r4)			; \
	and.	 %r9, %r3, %r9				; \
	beq	 2f					; \
							; \
	lwz	 %r1, (_REG_R0 +  1*4)(%r4)		; \
	lmw	%r14, (_REG_R0 + 14*4)(%r4)		; \
	lwz	 %r0, _REG_LR(%r4)			; \
	mtlr	 %r0					; \
	lwz	 %r0, _REG_PC(%r4)			; \
	mtctr	 %r0					; \
							; \
	andi.	 %r5, %r3, _UC_FPU			; \
	beq	 1f					; \
							; \
	lwz	 %r5, (_UC_FPVALID)(%r4)		; \
	or.	 %r5, %r5, %r5				; \
	beq	 1f					; \
							; \
	lfd	%r14, (_REG_FP0 + 14*8)(%r4)		; \
	lfd	%r15, (_REG_FP0 + 15*8)(%r4)		; \
	lfd	%r16, (_REG_FP0 + 16*8)(%r4)		; \
	lfd	%r17, (_REG_FP0 + 17*8)(%r4)		; \
	lfd	%r18, (_REG_FP0 + 18*8)(%r4)		; \
	lfd	%r19, (_REG_FP0 + 19*8)(%r4)		; \
	lfd	%r20, (_REG_FP0 + 20*8)(%r4)		; \
	lfd	%r21, (_REG_FP0 + 21*8)(%r4)		; \
	lfd	%r22, (_REG_FP0 + 22*8)(%r4)		; \
	lfd	%r23, (_REG_FP0 + 23*8)(%r4)		; \
	lfd	%r24, (_REG_FP0 + 24*8)(%r4)		; \
	lfd	%r25, (_REG_FP0 + 25*8)(%r4)		; \
	lfd	%r26, (_REG_FP0 + 26*8)(%r4)		; \
	lfd	%r27, (_REG_FP0 + 27*8)(%r4)		; \
	lfd	%r28, (_REG_FP0 + 28*8)(%r4)		; \
	lfd	%r29, (_REG_FP0 + 29*8)(%r4)		; \
	lfd	%r30, (_REG_FP0 + 30*8)(%r4)		; \
	lfd	%r31, (_REG_FP0 + 31*8)(%r4)		; \
	lfd	%r0, (_REG_FPSCR - 4)(%r4)		; \
	mtfsf	0xff, %r0				; \
							; \
1:							; \
	lwz	 %r0, _REG_CR(%r4)			; \
	mtcr	 %r0					; \
	bctr						; \
2:							; \
	mr	%r3, %r4				; \
	b	PIC_PLT(_C_LABEL(setcontext))		; \
	/* NOTREACHED */

ENTRY(_getcontext_u)
	GETC(%r3)
	xor %r3,%r3,%r3
	blr

ENTRY(_setcontext_u)
	mr %r4,%r3
	SETC

ENTRY(_swapcontext_u)
	GETC(%r3)
	SETC
