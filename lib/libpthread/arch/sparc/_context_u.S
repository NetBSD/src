/*	$NetBSD: _context_u.S,v 1.3 2008/04/28 20:23:02 martin Exp $	*/

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
#include <machine/trap.h>
#include "assym.h"

#define	FLUSHW			t T_FLUSHWIN
#define NOTREACHED		t T_DIV0

	.global _C_LABEL(setcontext)

       /*
	* For voluntary context switches save/restore take care
	* of caller's window, the only registers which should be
	* saved are frame pointer and return address
	*/

#define GETC(uc)					  \
	st	%i6, [uc + UC_GREGS + _REG_O6 * 4]	; \
	add	%i7, 8, %l0				; \
	st	%l0, [uc + UC_GREGS + _REG_PC * 4]	; \
	/* save nPC too, so that setcontext(2) works */	  \
	add	%i7, 12, %l0				; \
	st	%l0, [uc + UC_GREGS + _REG_nPC * 4]	; \
							  \
	set	_UC_CPU | _UC_USER, %l1			; \
	st	%l1, [uc + UC_FLAGS]

#define SETC(uc)					  \
	ld	[uc + UC_FLAGS], %l0			; \
	set	_UC_USER, %l1				; \
	btst	%l1, %l0				; \
	bz	1f					; \
	 nop						; \
							  \
	ld	[uc + UC_GREGS + _REG_PC * 4], %i7	; \
	ld	[uc + UC_GREGS + _REG_O6 * 4], %i6	; \
							; \
	jmp	%i7					; \
	 restore					; \
	NOTREACHED					; \
							  \
1:	call _C_LABEL(setcontext)			; \
	 mov	uc, %o0


ENTRY(_getcontext_u)
	save	%sp, -CCFSZ, %sp
	FLUSHW				! XXX
	GETC(%i0)
	ret
	 restore %g0, 0, %o0		! return (0);

ENTRY(_setcontext_u)
	save	%sp, -CCFSZ, %sp
	FLUSHW
	SETC(%i0)
	NOTREACHED

ENTRY(_swapcontext_u)
	save	%sp, -CCFSZ, %sp
	FLUSHW
	GETC(%i0)
	SETC(%i1)
	NOTREACHED

