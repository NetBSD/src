/*	$NetBSD: _context_u.S,v 1.5 2006/01/04 17:44:53 uwe Exp $ */

/*
 * Copyright (c) 2003 Christian P. Groessler
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
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <machine/asm.h>
#include "assym.h"


/*
 * Enter debugger or die with SIGTRAP if "notreached" code is reached
 */
#define NOTREACHED	trapa #0xc3


/*
 * Only save/restore registers that are callee saved, i.e for which
 * gcc has call_used_regs[reg] == 0.
 */

#define GETC(uc)							  \
	mov.l	.L_uc_gregs_offset, r1					; \
	sts	pr, r0							; \
	add	uc, r1		/* uc->uc_mcontext.__gregs */		; \
	mov.l	r0, @((_REG_PC * 4), r1)				; \
	mov.l	r0, @((_REG_PR * 4), r1)				; \
									  \
	mov.l	r8,  @((_REG_R8  * 4), r1)				; \
	mov.l	r9,  @((_REG_R9  * 4), r1)				; \
	mov.l	r10, @((_REG_R10 * 4), r1)				; \
	mov.l	r11, @((_REG_R11 * 4), r1)				; \
	mov.l	r12, @((_REG_R12 * 4), r1)				; \
	mov.l	r13, @((_REG_R13 * 4), r1)				; \
	mov.l	r14, @((_REG_R14 * 4), r1)				; \
	/* _REG_R15 is too far to use @(disp, Rn) */			  \
									  \
	/* in Hitachi calling convention mac regs are callee saved */	  \
	sts	mach, r0						; \
	mov.l	r0, @((_REG_MACH * 4), r1)				; \
	sts	macl, r0						; \
	mov.l	r0, @((_REG_MACL * 4), r1)				; \
									  \
	mov.l	.L_uc_flags, r0						; \
									  \
	add	#(_REG_R15 * 4), r1					; \
	mov.l	r15, @r1						; \
									  \
	/* XXX:	FP registers fr12..fr15? */				  \
									  \
	mov.l	r0, @(UC_FLAGS, uc)


#define SETC(uc)							  \
	mov.l	@(UC_FLAGS, uc), r1					; \
	mov.l	.L_uc_user, r0						; \
	tst	r0, r1		/* ands and sets T if zero(!) */	; \
	bt	1f							; \
									  \
	/* a _UC_USER context */					  \
	mov.l	.L_uc_gregs_offset, r1					; \
	add	uc, r1		/* uc->uc_mcontext.__gregs */		; \
									  \
	mov.l	@((_REG_R8  * 4), r1), r8				; \
	mov.l	@((_REG_R9  * 4), r1), r9				; \
	mov.l	@((_REG_R10 * 4), r1), r10				; \
	mov.l	@((_REG_R11 * 4), r1), r11				; \
	mov.l	@((_REG_R12 * 4), r1), r12				; \
	mov.l	@((_REG_R13 * 4), r1), r13				; \
	mov.l	@((_REG_R14 * 4), r1), r14				; \
	/* _REG_R15 is too far to use @(disp, Rn) */			  \
									  \
	/* in Hitachi calling convention mac regs are callee saved */	  \
	mov.l	@((_REG_MACH * 4), r1), r0				; \
	mov.l	@((_REG_MACL * 4), r1), r2				; \
	lds	r0, mach						; \
	lds	r2, macl						; \
									  \
	/* XXX:	FP registers fr12..fr15? */				  \
									  \
	mov.l	@((_REG_PR * 4), r1), r0				; \
	mov.l	@((_REG_PC * 4), r1), r2				; \
	lds	r0, pr							; \
	add	#(_REG_R15 * 4), r1					; \
	jmp	@r2							; \
	 mov.l	@r1, r15						; \
	NOTREACHED							; \
									  \
1:	/* not a _UC_USER context, pass to setcontext(2) */		  \
	CALL_SETCONTEXT(uc)						  \
	/* NOTREACHED */

#ifdef PIC

/*
 * For PIC code we need a per-call offset to the setcontext.
 * Fortunately, since setcontext(2) does not return, we can
 * put the offset right after the call.  We also don't need to
 * save/restore r12.
 */
#define CALL_SETCONTEXT(uc)						  \
	mov.l	2f, r12							; \
	mov.l	3f, r1							; \
	mova	2f, r0							; \
.ifnc "uc","r4"								; \
	mov	uc, r4							; \
.endif									; \
1:	bsrf	r1							; \
	 add	r0, r12							; \
	NOTREACHED							; \
	.align 2							; \
2:	.long	_GLOBAL_OFFSET_TABLE_					; \
3:	CALL_DATUM(_C_LABEL(setcontext), 1b)

#else /* !PIC */

/*
 * For static code all calls to setcontext can share single location
 * with the address of setcontext (see below).
 */
#define CALL_SETCONTEXT(uc)						  \
	mov.l	.L_setcontext, r1					; \
	jsr	@r1							; \
.ifnc "uc","r4"								; \
	 mov	uc, r4							; \
.else									; \
	 nop								; \
.endif									; \
	NOTREACHED

#endif



/*
 * int _getcontext_u(ucontext_t *ctx)
 *	Store the current context in the provided ctx structure.
 */
NENTRY(_getcontext_u)
	GETC(r4)		/* r4 - ctx */
	rts
	 mov	#0, r0


/*
 * int _swapcontext_u(ucontext_t *from, const ucontext_t *to)
 *	Save the current context in `from' before switching to the
 *	new context in `to'.
 */
NENTRY(_swapcontext_u)
	GETC(r4)		/* r4 - from */
	mov	r5, r4		/* r5 - to   */
	/* FALLTHROUGH */


/*
 * int _setcontext_u(const ucontext_t *ctx)
 *	Make the context stored in ctx current.
 */
NENTRY(_setcontext_u)
	SETC(r4)		/* r4 - ctx */
	/* NOTREACHED */

	.align	2
.L_uc_gregs_offset:	.long	UC_REGS
.L_uc_flags:		.long	_UC_USER | _UC_CPU
.L_uc_user:		.long	_UC_USER
#ifndef PIC
.L_setcontext:		.long	_C_LABEL(setcontext)
#endif
