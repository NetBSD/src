/*	$NetBSD: _context_u.S,v 1.2 2003/01/18 10:34:19 thorpej Exp $	*/

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
#include "SYS.h"
#include <machine/asm.h>
#include <machine/mcontext.h>

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
#define GETC(reg)					; \
	REG_PROLOGUE					; \
	REG_S	s0, (UC_REGS + _REG_S0 * SZREG)(reg)	; \
	REG_S	s1, (UC_REGS + _REG_S1 * SZREG)(reg)	; \
	REG_S	s2, (UC_REGS + _REG_S2 * SZREG)(reg)	; \
	REG_S	s3, (UC_REGS + _REG_S3 * SZREG)(reg)	; \
	REG_S	s4, (UC_REGS + _REG_S4 * SZREG)(reg)	; \
	REG_S	s5, (UC_REGS + _REG_S5 * SZREG)(reg)	; \
	REG_S	s6, (UC_REGS + _REG_S6 * SZREG)(reg)	; \
	REG_S	s7, (UC_REGS + _REG_S7 * SZREG)(reg)	; \
	REG_S	gp, (UC_REGS + _REG_GP * SZREG)(reg)	; \
	REG_S	sp, (UC_REGS + _REG_SP * SZREG)(reg)	; \
	REG_S	s8, (UC_REGS + _REG_S8 * SZREG)(reg)	; \
	REG_S	ra, (UC_REGS + _REG_RA * SZREG)(reg)	; \
	REG_S	ra, (UC_REGS + _REG_EPC* SZREG)(reg)	; \
	REG_EPILOGUE					; \
	li	t0, 1					; \
	sll	t0, t0, _UC_USER_BIT			; \
	ori	t0, t0, _UC_CPU				; \
	sw	t0, (UC_FLAGS)(reg)

#define SETC(reg)					; \
	lw	t0, (UC_FLAGS)(reg)			; \
	li	t1, 1					; \
	sll	t1, t1, _UC_USER_BIT			; \
	and	t0, t1, t0				; \
	beq	t0, zero, 1f				; \
	REG_PROLOGUE					; \
	REG_L	s0, (UC_REGS + _REG_S0 * SZREG)(reg)	; \
	REG_L	s1, (UC_REGS + _REG_S1 * SZREG)(reg)	; \
	REG_L	s2, (UC_REGS + _REG_S2 * SZREG)(reg)	; \
	REG_L	s3, (UC_REGS + _REG_S3 * SZREG)(reg)	; \
	REG_L	s4, (UC_REGS + _REG_S4 * SZREG)(reg)	; \
	REG_L	s5, (UC_REGS + _REG_S5 * SZREG)(reg)	; \
	REG_L	s6, (UC_REGS + _REG_S6 * SZREG)(reg)	; \
	REG_L	s7, (UC_REGS + _REG_S7 * SZREG)(reg)	; \
	REG_L	s8, (UC_REGS + _REG_S8 * SZREG)(reg)	; \
	REG_L	t9, (UC_REGS + _REG_EPC * SZREG)(reg)	; \
	REG_L	gp, (UC_REGS + _REG_GP * SZREG)(reg)	; \
	REG_L	sp, (UC_REGS + _REG_SP * SZREG)(reg)	; \
	REG_L	ra, (UC_REGS + _REG_RA * SZREG)(reg)	; \
	REG_EPILOGUE					; \
							; \
	/* part procedure call, part RET */		; \
	j	t9					; \
	nop						; \
	/* NOTREACHED */				; \
							; \
1:							; \
	move	a0, reg					; \
	la	t9, _C_LABEL(setcontext)		; \
	jr	t9					; \
	nop						; \
	/* NOTREACHED */

LEAF(_getcontext_u)
	PIC_PROLOGUE(_getcontext_u, t9)
	GETC(a0)
	xor	v0, v0, v0
	j	ra
END(_getcontext_u)

LEAF(_setcontext_u)
	PIC_PROLOGUE(_setcontext_u, t9)
	SETC(a0)
END(_setcontext_u)

LEAF(_swapcontext_u)
	PIC_PROLOGUE(_swapcontext_u, t9)
	GETC(a0)
	SETC(a1)
END(_swapcontext_u)
