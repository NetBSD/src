/*	$NetBSD: _context_u.S,v 1.1.2.1 2001/11/18 23:35:20 scw Exp $	*/

/*-
 * Copyright (c) 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Nathan J. Williams and Steve C. Woodford.
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


#define	GETC	\
	moveml	%d2-%d7,%a0@(UC_REGS + (_REG_D2 * 4))	;	\
	moveml	%a2-%a6,%a0@(UC_REGS + (_REG_A2 * 4))	;	\
	movw	%cc,%d0					;	\
	movw	%d0,%a0@(UC_REGS + (_REG_PS * 4) + 2)	;	\
	movl	%a1,%a0@(UC_REGS + (_REG_PC * 4))	;	\
	lea	%sp@(4),%a1				;	\
	movl	%a1,%a0@(UC_REGS + (_REG_SP * 4))

#define	SETC	\
	moveml	%a0@(UC_REGS + (_REG_D2 * 4)),%d2-%d7	;	\
	moveml	%a0@(UC_REGS + (_REG_A2 * 4)),%a2-%a6	;	\
	movw	%a0@(UC_REGS + (_REG_PS * 4) + 2),%d0	;	\
	movw	%d0,%cc					;	\
	movl	%a0@(UC_REGS + (_REG_PC * 4)),%a1	;	\
	movl	%a0@(UC_REGS + (_REG_SP * 4)),%sp	;	\
	jmp	%a1@

/*
 * int _getcontext_u(ucontext_t *ctx)
 *	Store the current context in the provided ctx structure.
 */
ENTRY_NOPROFILE(_getcontext_u)
	movl	%sp@(4),%a0
	movl	%sp@,%a1
	GETC
	moveql	#0,%d0
	rts

/*
 * int _setcontext_u(const ucontext_t *ctx)
 *	Make the context stored in ctx current.
 */
ENTRY_NOPROFILE(_setcontext_u)
	movl	%sp@(4),%a0
	SETC

/*
 * int _swapcontext_u(ucontext_t *from, const ucontext_t *to)
 *	Save the current context in `from' before switching to the
 *	new context in `to'.
 */
ENTRY_NOPROFILE(_swapcontext_u)
	movl	%sp@(4),%a0
	movl	%sp@,%a1
	GETC
	movl	%sp@(8),%a0
	SETC
