/*	$NetBSD: _context_u.S,v 1.3.14.2 2009/08/24 02:35:05 matt Exp $	*/

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
	REG_S	s0, _OFFSETOF_UC_GREGS_S0(reg)		; \
	REG_S	s1, _OFFSETOF_UC_GREGS_S1(reg)		; \
	REG_S	s2, _OFFSETOF_UC_GREGS_S2(reg)		; \
	REG_S	s3, _OFFSETOF_UC_GREGS_S3(reg)		; \
	REG_S	s4, _OFFSETOF_UC_GREGS_S4(reg)		; \
	REG_S	s5, _OFFSETOF_UC_GREGS_S5(reg)		; \
	REG_S	s6, _OFFSETOF_UC_GREGS_S6(reg)		; \
	REG_S	s7, _OFFSETOF_UC_GREGS_S7(reg)		; \
	REG_S	gp, _OFFSETOF_UC_GREGS_GP(reg)		; \
	REG_S	sp, _OFFSETOF_UC_GREGS_SP(reg)		; \
	REG_S	s8, _OFFSETOF_UC_GREGS_S8(reg)		; \
	REG_S	ra, _OFFSETOF_UC_GREGS_RA(reg)		; \
	REG_S	ra, _OFFSETOF_UC_GREGS_EPC(reg)		; \
	REG_EPILOGUE					; \
	li	t0, 1					; \
	sll	t0, t0, _UC_USER_BIT			; \
	ori	t0, t0, _UC_CPU				; \
	INT_S	t0, _OFFSETOF_UC_FLAGS(reg)

#define SETC(reg)					; \
	INT_L	t0, _OFFSETOF_UC_FLAGS(reg)		; \
	li	t1, 1					; \
	sll	t1, t1, _UC_USER_BIT			; \
	and	t0, t1, t0				; \
	beq	t0, zero, 1f				; \
	REG_PROLOGUE					; \
	REG_L	s0, _OFFSETOF_UC_GREGS_S0(reg)		; \
	REG_L	s1, _OFFSETOF_UC_GREGS_S1(reg)		; \
	REG_L	s2, _OFFSETOF_UC_GREGS_S2(reg)		; \
	REG_L	s3, _OFFSETOF_UC_GREGS_S3(reg)		; \
	REG_L	s4, _OFFSETOF_UC_GREGS_S4(reg)		; \
	REG_L	s5, _OFFSETOF_UC_GREGS_S5(reg)		; \
	REG_L	s6, _OFFSETOF_UC_GREGS_S6(reg)		; \
	REG_L	s7, _OFFSETOF_UC_GREGS_S7(reg)		; \
	REG_L	t9, _OFFSETOF_UC_GREGS_EPC(reg)		; \
	REG_L	gp, _OFFSETOF_UC_GREGS_GP(reg)		; \
	REG_L	sp, _OFFSETOF_UC_GREGS_SP(reg)		; \
	REG_L	s8, _OFFSETOF_UC_GREGS_S8(reg)		; \
	REG_L	ra, _OFFSETOF_UC_GREGS_RA(reg)		; \
	REG_EPILOGUE					; \
							; \
	/* part procedure call, part RET */		; \
	j	t9					; \
	nop						; \
	/* NOTREACHED */				; \
							; \
1:							; \
	move	a0, reg					; \
	PIC_TAILCALL(setcontext)
	/* NOTREACHED */

LEAF(_getcontext_u)
	PIC_PROLOGUE(_getcontext_u)
	GETC(a0)
	move	v0, zero
	PIC_RETURN()
END(_getcontext_u)

LEAF(_setcontext_u)
	PIC_PROLOGUE(_setcontext_u)
	SETC(a0)
END(_setcontext_u)

LEAF(_swapcontext_u)
	PIC_PROLOGUE(_swapcontext_u)
	GETC(a0)
	SETC(a1)
END(_swapcontext_u)
