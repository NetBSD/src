/*	$NetBSD: _context_u.S,v 1.1.2.1 2002/02/22 03:00:47 petrov Exp $	*/

/*-
 * Copyright (c) 2002 The NetBSD Foundation, Inc.
 * All rights reserved.
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


#define NOTREACHED	illtrap	210; \
	/* NOTREACHED */

	.global _C_LABEL(setcontext)

       /*
	* XXX need to check ABI for what has to be saved/restored
	* in function call, all user-level context switches are
	* voluntary(aren't they?)
	*
	* NOTE:	if saved context will be used for setcontext syscall,
	*	then it better be complete 
	*
	* save/flushw push previous(caller's) frame to stack,
	* so I should be able to return just restoring %fp and %i7
	*
	* global regs
	*
	* fpu context
	*/

#define GETC(reg)					; \
	stx	%g1, [ reg + UC_GREGS + _REG_G1 * 8 ]	; \
	stx	%g2, [ reg + UC_GREGS + _REG_G2 * 8 ]	; \
	stx	%g3, [ reg + UC_GREGS + _REG_G3 * 8 ]	; \
	stx	%g4, [ reg + UC_GREGS + _REG_G4 * 8 ]	; \
	stx	%g5, [ reg + UC_GREGS + _REG_G5 * 8 ]	; \
	stx	%g6, [ reg + UC_GREGS + _REG_G6 * 8 ]	; \
	stx	%g7, [ reg + UC_GREGS + _REG_G7 * 8 ]	; \
	stx	%i0, [ reg + UC_GREGS + _REG_O0 * 8 ]	; \
	stx	%i1, [ reg + UC_GREGS + _REG_O1 * 8 ]	; \
	stx	%i2, [ reg + UC_GREGS + _REG_O2 * 8 ]	; \
	stx	%i3, [ reg + UC_GREGS + _REG_O3 * 8 ]	; \
	stx	%i4, [ reg + UC_GREGS + _REG_O4 * 8 ]	; \
	stx	%i5, [ reg + UC_GREGS + _REG_O5 * 8 ]	; \
	stx	%i6, [ reg + UC_GREGS + _REG_O6 * 8 ]	; \
	stx	%i7, [ reg + UC_GREGS + _REG_O7 * 8 ]	; \
							; \
/*	ldx	[ %fp + 2047 + 15 * 8], %l0	*/	; \
	mov	%i7, %l0	; \
	add	%l0, 8, %l0	; \
	stx	%l0, [ reg + UC_GREGS + _REG_PC * 8 ]   ; \
	add	%l0, 4, %l0			        ; \
	stx	%l0, [ reg + UC_GREGS + _REG_nPC * 8 ]  ; \
							; \
	rd	%ccr, %l0				; \
	stx	%l0, [ reg + UC_GREGS + _REG_CCR * 8]   ; \
	rd	%y, %l0					; \
	stx	%l0, [ reg + UC_GREGS + _REG_Y * 8 ]	; \
	rd	%asi, %l0				; \
	stx	%l0, [ reg + UC_GREGS + _REG_ASI * 8 ]	; \
							; \
	rd	%fprs, %l0				; \
	stx	%l0, [ reg + UC_GREGS + _REG_FPRS * 8 ]	; \
				; \
	clr	%l1		; \
	mov	1, %l1		; \
	sll	%l1, _UC_USER_BIT, %l1			; \
	or	%l1, _UC_CPU, %l1			; \
	btst	FPRS_FEF, %l0	; \
	bz	1f		; \
	or	%l1, _UC_FPU, %l1			; \
	nop			; \
	nop			; \
1:				; \
	st	%l1, [ reg + UC_FLAGS ]

/*
 * XXX comment when done
 */
#define SETC(reg)					; \
	ld	[ reg + UC_FLAGS ], %l0			; \
	mov	1, %l1		; \
	sll	%l1, _UC_USER_BIT, %l1			; \
	andcc	%l0, %l1, %g0	; \
	bz,a,pn	%icc,2f		; \
	 nop			; \
	ldx	[ reg + UC_GREGS + _REG_G1 * 8 ], %g1	; \
	ldx	[ reg + UC_GREGS + _REG_G2 * 8 ], %g2	; \
	ldx	[ reg + UC_GREGS + _REG_G3 * 8 ], %g3	; \
	ldx	[ reg + UC_GREGS + _REG_G4 * 8 ], %g4	; \
	ldx	[ reg + UC_GREGS + _REG_G5 * 8 ], %g5	; \
	ldx	[ reg + UC_GREGS + _REG_G6 * 8 ], %g6	; \
	ldx	[ reg + UC_GREGS + _REG_G7 * 8 ], %g7	; \
	ldx	[ reg + UC_GREGS + _REG_O0 * 8 ], %i0 	; \
	ldx	[ reg + UC_GREGS + _REG_O1 * 8 ], %i1	; \
	ldx	[ reg + UC_GREGS + _REG_O2 * 8 ], %i2	; \
	ldx	[ reg + UC_GREGS + _REG_O3 * 8 ], %i3	; \
	ldx	[ reg + UC_GREGS + _REG_O4 * 8 ], %i4	; \
	ldx	[ reg + UC_GREGS + _REG_O5 * 8 ], %i5	; \
				; \
	ldx	[ reg + UC_GREGS + _REG_O6 * 8 ], %i6	; \
							; \
	ldx	[ reg + UC_GREGS + _REG_PC * 8 ], %i7   ; \
							; \
/*	ldx	[ %fp + 2047 + 14 * 8], %l0		; */ \
/*	mov	%l0, %fp	; */ \
							; \
/*	ldx	[ reg + UC_GREGS + _REG_Y * 8 ], %l0	; */ \
/*	wr	%l0, 0, %y				; */\
/*	ldx	[ reg + UC_GREGS + _REG_ASI * 8 ], %l0	; */\
/*	wr	%l0, 0, %asi				; */\
/*	ldx	[ reg + UC_GREGS + _REG_CCR * 8 ], %l0  ; */\
/*	wr	%l0, 0, %ccr				; */ \
							; \
/* XXX writing to fprs causes visible delay */		; \
/*	ldx	[ reg + UC_GREGS + _REG_FPRS * 8 ], %l0	; */\
/*	wr	%l0, 0, %fprs				; */\
				; \
	clr	%l1		; \
	mov	1, %l1		; \
	btst	FPRS_FEF, %l0	; \
	bz	1f		; \
	or	%l1, _UC_FPU, %l1; \
	nop			; \
	nop			; \
1:				; \
	jmpl %i7, %g0		; \
	 restore		;  \
2:	call _C_LABEL(setcontext)			; \
	 mov	reg, %o0	;

       /*
	* XXX does it need flushw here
	*/
ENTRY(_getcontext_u)
	save	%sp, -176, %sp
	flushw
	GETC(%i0)
	mov %g0, %i0
	ret
	 restore

ENTRY(_setcontext_u)
	save	%sp, -176, %sp
	flushw
	mov	%i0, %o0
	SETC(%o0)
	NOTREACHED

ENTRY(_swapcontext_u)
	save	%sp, -176, %sp
	flushw
	GETC(%i0)
	mov %i1, %o0
	SETC(%o0)
	NOTREACHED

