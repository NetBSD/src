/*	$NetBSD: _context_u.S,v 1.1.2.3 2002/05/21 01:24:10 petrov Exp $	*/

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


#define NOTREACHED	illtrap	10

#if 0
#define DBP(n)		\
	or	%g0,n,%o3	; \
	mov	%i0,%o0		; \
	mov	%i1,%o1		; \
	call	_C_LABEL(pthread__md_debug); \
	 mov	%i2,%o2

#define DBR(n,a0,a1,a2)		\
	or	%g0,n,%o3	; \
	mov	a0,%o0		; \
	mov	a1,%o1		; \
	call	_C_LABEL(pthread__md_debug); \
	 mov	a2,%o2
#else
#define DBR(n,a0,a1,a2)
#define DBP(n)
#endif

	.global _C_LABEL(setcontext)

       /*
	* For voluntary context switches save/restore take care
	* of caller's window, the only registers which should be
	* saved are frame pointer and return address
	*/

#define GETC(reg)					; \
	stx	%i6, [ reg + UC_GREGS + _REG_O6 * 8 ]	; \
	add	%i7, 8, %l0				; \
	stx	%l0, [ reg + UC_GREGS + _REG_PC * 8 ]   ; \
							  \
	sethi	%hi(0x40000000), %l1			; \
	or	%l1, _UC_CPU, %l1			; \
							  \
	st	%l1, [ reg + UC_FLAGS ]

#define SETC(reg)					; \
	ld	[ reg + UC_FLAGS ], %l0			; \
	sethi	%hi(0x40000000), %l1			; \
	andcc	%l0, %l1, %g0				; \
	bz,a,pn	%icc,1f					; \
	 nop						; \
							  \
	ldx	[ reg + UC_GREGS + _REG_O6 * 8 ], %i6	; \
	ldx	[ reg + UC_GREGS + _REG_PC * 8 ], %i7   ; \
							; \
	jmpl %i7, %g0					; \
	 restore					; \
1:	call _C_LABEL(setcontext)			; \
	 mov	reg, %o0

ENTRY(_getcontext_u)
	save	%sp, -176, %sp
	flushw				! XXX
DBP(20)
	GETC(%i0)
	mov %g0, %i0
	ret
	 restore

ENTRY(_setcontext_u)
	save	%sp, -176, %sp
	flushw
DBP(21)
	mov	%i0, %o0
	mov	%i0, %o0
	SETC(%o0)
	NOTREACHED

ENTRY(_swapcontext_u)
	save	%sp, -176, %sp
	flushw
DBP(22)
	GETC(%i0)
	mov %i1, %o0
	SETC(%o0)
	NOTREACHED

