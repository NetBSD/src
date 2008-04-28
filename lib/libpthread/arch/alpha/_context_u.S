/*	$NetBSD: _context_u.S,v 1.4.8.2 2008/04/28 20:23:03 martin Exp $	*/

/*-
 * Copyright (c) 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Nathan J. Williams.
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
#include "assym.h"

/*
 * Only save the callee-save regs, and set a special flag in uc_flags
 * that says so.
 */
#define GETC(reg)					; \
	stq	s0, (UC_GREGS + _REG_S0 * 8)(reg)	; \
	stq	s1, (UC_GREGS + _REG_S1 * 8)(reg)	; \
	stq	s2, (UC_GREGS + _REG_S2 * 8)(reg)	; \
	stq	s3, (UC_GREGS + _REG_S3 * 8)(reg)	; \
	stq	s4, (UC_GREGS + _REG_S4 * 8)(reg)	; \
	stq	s5, (UC_GREGS + _REG_S5 * 8)(reg)	; \
	stq	s6, (UC_GREGS + _REG_S6 * 8)(reg)	; \
	stq	sp, (UC_GREGS + _REG_SP * 8)(reg)	; \
	stq	ra, (UC_GREGS + _REG_RA * 8)(reg)	; \
	stq	ra, (UC_GREGS + _REG_PC * 8)(reg)	; \
	call_pal PAL_rdunique				; \
	stq	v0, (UC_GREGS + _REG_UNIQUE * 8)(reg)	; \
	stt	$f2, (UC_FPREGS + 2 * 8)(reg)		; \
	stt	$f3, (UC_FPREGS + 3 * 8)(reg)		; \
	stt	$f4, (UC_FPREGS + 4 * 8)(reg)		; \
	stt	$f5, (UC_FPREGS + 5 * 8)(reg)		; \
	stt	$f6, (UC_FPREGS + 6 * 8)(reg)		; \
	stt	$f7, (UC_FPREGS + 7 * 8)(reg)		; \
	stt	$f8, (UC_FPREGS + 8 * 8)(reg)		; \
	stt	$f9, (UC_FPREGS + 9 * 8)(reg)		; \
	ldiq	t0, 8					; \
	stq	t0, (UC_GREGS + _REG_PS * 8)(reg)	; \
	ldiq	t0, 1					; \
	sll	t0, _UC_USER_BIT, t0			; \
	bis	t0, (_UC_CPU|_UC_FPU|_UC_UNIQUE), t0	; \
	stl	t0, (UC_FLAGS)(reg)

#define SETC(reg)					; \
	ldl	t0, (UC_FLAGS)(reg)			; \
	ldiq	t1, 1					; \
	sll	t1, _UC_USER_BIT, t1			; \
	and	t0, t1, t0				; \
	beq	t0, 1f					; \
							; \
	/* _UC_USER implies _UC_UNIQUE */		; \
	ldq	a0, (UC_GREGS + _REG_UNIQUE * 8)(reg)	; \
	call_pal PAL_wrunique				; \
	ldq	s0, (UC_GREGS + _REG_S0 * 8)(reg)	; \
	ldq	s1, (UC_GREGS + _REG_S1 * 8)(reg)	; \
	ldq	s2, (UC_GREGS + _REG_S2 * 8)(reg)	; \
	ldq	s3, (UC_GREGS + _REG_S3 * 8)(reg)	; \
	ldq	s4, (UC_GREGS + _REG_S4 * 8)(reg)	; \
	ldq	s5, (UC_GREGS + _REG_S5 * 8)(reg)	; \
	ldq	s6, (UC_GREGS + _REG_S6 * 8)(reg)	; \
	ldq	ra, (UC_GREGS + _REG_RA * 8)(reg)	; \
	ldq	t12, (UC_GREGS + _REG_PC * 8)(reg)	; \
	ldt	$f2, (UC_FPREGS + 2 * 8)(reg)		; \
	ldt	$f3, (UC_FPREGS + 3 * 8)(reg)		; \
	ldt	$f4, (UC_FPREGS + 4 * 8)(reg)		; \
	ldt	$f5, (UC_FPREGS + 5 * 8)(reg)		; \
	ldt	$f6, (UC_FPREGS + 6 * 8)(reg)		; \
	ldt	$f7, (UC_FPREGS + 7 * 8)(reg)		; \
	ldt	$f8, (UC_FPREGS + 8 * 8)(reg)		; \
	ldt	$f9, (UC_FPREGS + 9 * 8)(reg)		; \
	ldq	sp, (UC_GREGS + _REG_SP * 8)(reg)	; \
							; \
	/* part procedure call, part RET */		; \
	jmp	zero, (t12)				; \
	/* NOTREACHED */				; \
1:	mov	reg, a0					; \
	CALL(setcontext)				; \
	/* NOTREACHED */

LEAF(_getcontext_u,1)
	GETC(a0)
	mov zero, v0
	RET
	END(_getcontext_u)

NESTED(_setcontext_u,1,0,ra,0,0)
	LDGP(pv)
	mov	a0, a1
	SETC(a1)
	END(_setcontext_u)

NESTED(_swapcontext_u,2,0,ra,0,0)
	LDGP(pv)
	GETC(a0)
	SETC(a1)
	END(_swapcontext_u)

	.align 2
LEAF(pthread__ras_simple_lock_init, 1)
	stl	zero, 0(a0)
	nop
	RET
END(pthread__ras_simple_lock_init)

	.align 2
LEAF(pthread__ras_simple_lock_try, 1)
	ldiq	t0, 1
pthread__lock_ras_start:	.globl	pthread__lock_ras_start
	ldl	v0, 0(a0)
	stl	t0, 0(a0)
pthread__lock_ras_end:		.globl	pthread__lock_ras_end
	cmpeq	v0, 0, v0
	RET	
END(pthread__ras_simple_lock_try)

	.align 2
LEAF(pthread__ras_simple_unlock, 1)
	stl	zero, 0(a0)
	nop
	RET
END(pthread__ras_simple_unlock)
