/*	$NetBSD: _context_u.S,v 1.1.2.1 2001/09/25 19:49:05 nathanw Exp $	*/

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

#include <machine/asm.h>
#include "assym.h"

/*
 * Only save the callee-save regs, and set a special flag in uc_flags
 * that says so.
 */
#define GETC(reg)				; \
	stq	s0, (UC_REGS + R_S0 * 8)(reg)	; \
	stq	s1, (UC_REGS + R_S1 * 8)(reg)	; \
	stq	s2, (UC_REGS + R_S2 * 8)(reg)	; \
	stq	s3, (UC_REGS + R_S3 * 8)(reg)	; \
	stq	s4, (UC_REGS + R_S4 * 8)(reg)	; \
	stq	s5, (UC_REGS + R_S5 * 8)(reg)	; \
	stq	s6, (UC_REGS + R_S6 * 8)(reg)	; \
	stq	sp, (UC_REGS + R_SP * 8)(reg)	; \
	stq	ra, (UC_REGS + R_RA * 8)(reg)	; \
	stq	ra, (UC_PC)(reg)		; \
	ldiq	t0, 1				; \
	sll	t0, _UC_USER_BIT, t0		; \
	bis	t0, _UC_CPU, t0			; \
	stl	t0, (UC_FLAGS)(reg)

#define SETC(reg)				; \
	ldl	t0, (UC_FLAGS)(reg)		; \
	ldiq	t1, 1				; \
	sll	t1, _UC_USER_BIT, t1		; \
	and	t0, t1, t0			; \
	beq	t0, 1f				; \
						; \
	ldq	s0, (UC_REGS + R_S0 * 8)(reg)	; \
	ldq	s1, (UC_REGS + R_S1 * 8)(reg)	; \
	ldq	s2, (UC_REGS + R_S2 * 8)(reg)	; \
	ldq	s3, (UC_REGS + R_S3 * 8)(reg)	; \
	ldq	s4, (UC_REGS + R_S4 * 8)(reg)	; \
	ldq	s5, (UC_REGS + R_S5 * 8)(reg)	; \
	ldq	s6, (UC_REGS + R_S6 * 8)(reg)	; \
	ldq	ra, (UC_REGS + R_RA * 8)(reg)	; \
	ldq	t12, (UC_PC)(reg)		; \
	ldq	sp, (UC_REGS + R_SP * 8)(reg)	; \
						; \
	/* part procedure call, part RET */	; \
	jmp	zero, (t12)			; \
	/* NOTREACHED */			; \
1:	mov	reg, a0				; \
	CALL(setcontext)			; \
	/* NOTREACHED */

LEAF(_getcontext_u,1)
	GETC(a0)
	mov zero, v0
	RET
	END(_getcontext_u)

NESTED(_setcontext_u,1,0,ra,0,0)
	LDGP(pv)
	SETC(a0)
	END(_setcontext_u)

NESTED(_swapcontext_u,2,0,ra,0,0)
	LDGP(pv)
	GETC(a0)
	SETC(a1)
	END(_swapcontext_u)
