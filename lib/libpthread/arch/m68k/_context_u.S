/*	$NetBSD: _context_u.S,v 1.1.2.4 2001/11/20 23:49:58 scw Exp $	*/

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

/*
 * Save the current context into the ucontext_t structure pointed to
 * by %a0. The value of %pc to save is passed in %a1.
 *
 * We only need to save the `callee' saved registers. In fact, saving
 * the %ccr here is probably overkill, but might as well put a sane
 * value in there...
 */
#define	GETC	\
	movw	%cc,%a0@(UC_REGS + (_REG_PS * 4) + 2)	;	\
	moveml	%d2-%d7,%a0@(UC_REGS + (_REG_D2 * 4))	;	\
	moveml	%a2-%a7,%a0@(UC_REGS + (_REG_A2 * 4))	;	\
	movl	%a1,%a0@(UC_REGS + (_REG_PC * 4))	;	\
	moveql	#_UC_CPU,%d0				;	\
	orl	%d0,%a0@(UC_FLAGS)

/*
 * Restore the complete context from the ucontext_t structure pointed
 * to by %a0. We can't short-cut caller saved registers here. In fact,
 * we have to do some gymnastics to avoid restoring the saved %sp until
 * all other saved registers have been read out. This is because there's
 * a chance the ucontext_t structure is allocated at the base of the new
 * context's stack.
 */
#define	SETC	\
	moveml	%a0@(UC_REGS + (_REG_D0 * 4)),%d0-%d7	;	\
	moveml	%a0@(UC_REGS + (_REG_A2 * 4)),%a2-%a6	;	\
	movl	%a0@(UC_REGS + (_REG_SP * 4)),%a1	;	\
	movl	%a0@(UC_REGS + (_REG_PC * 4)),%a1@-	;	\
	movl	%a0@(UC_REGS + (_REG_A1 * 4)),%a1@-	;	\
	movw	%a0@(UC_REGS + (_REG_PS * 4) + 2),%a1@-	;	\
	movl	%a0@(UC_REGS + (_REG_A0 * 4)),%a0	;	\
	movw	%a1@+,%cc				;	\
	movl	%a1,%sp					;	\
	movl	%sp@+,%a1				;	\
	rts

/*
 * int _getcontext_u(ucontext_t *ctx)
 *	Store the current context in the provided ctx structure.
 */
ENTRY_NOPROFILE(_getcontext_u)
	movl	%sp@+,%a1
	movl	%sp@,%a0
	GETC
	moveql	#0,%d0
	jmp	%a1@

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
	movl	%sp@+,%a1
	movl	%sp@,%a0
	GETC
	movl	%sp@(4),%a0
	SETC
