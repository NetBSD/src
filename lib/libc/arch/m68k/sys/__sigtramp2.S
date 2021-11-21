/*	$NetBSD: __sigtramp2.S,v 1.5 2021/11/21 23:58:09 thorpej Exp $ */

/*-
 * Copyright (c) 2003 The NetBSD Foundation, Inc.
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

#include "SYS.h"
#include "assym.h"

/*
 * The m68k signal trampoline is invoked only to return from
 * the signal; the kernel calls the signal handler directly.
 *
 * On entry, stack looks like:
 *
 *		ucontext structure			[12+sizeof(siginfo_t)]
 *		siginfo structure			[12]
 *		pointer to ucontext structure		[8]
 *		pointer to siginfo structure		[4]
 *	sp->	signal number				[0]
 *
 * The DWARF register numbers are 0-7 (dX), 8-15 (aX), 16-23 (fpX),
 * which maps nicely to _REG_D[0-7] and _REG_A[0-7].  For m68k, there
 * is a DWARF pseudo-register for the return address, and additionally
 * another DWARF pseudo-register for signal handler return addresses.
 * We will specify both return address pseudo-registers (without
 * explicitly specifying .cfi_return_column) to keep the compiler
 * run-time happy with whichever one it decides to use.
 */

#define	DWARF_RETURN_REG		24
#if defined(__LIBGCC_DWARF_ALT_FRAME_RETURN_COLUMN__)
#define	DWARF_SIGRETURN_REG		__LIBGCC_DWARF_ALT_FRAME_RETURN_COLUMN__
#else
#define	DWARF_SIGRETURN_REG		25
#endif

#define	CFI_OFFSET_DWARF_REG(d, r)	.cfi_offset d, r * 4
#define	CFI_OFFSET(r)			CFI_OFFSET_DWARF_REG(r, r)

	.text
	.cfi_startproc simple
	.cfi_signal_frame
	.cfi_def_cfa _REG_A7, 12 + SIZEOF_SIGINFO + UC_GREGS
	CFI_OFFSET(_REG_D0)
	CFI_OFFSET(_REG_D1)
	CFI_OFFSET(_REG_D2)
	CFI_OFFSET(_REG_D3)
	CFI_OFFSET(_REG_D4)
	CFI_OFFSET(_REG_D5)
	CFI_OFFSET(_REG_D6)
	CFI_OFFSET(_REG_D7)
	CFI_OFFSET(_REG_A0)
	CFI_OFFSET(_REG_A1)
	CFI_OFFSET(_REG_A2)
	CFI_OFFSET(_REG_A3)
	CFI_OFFSET(_REG_A4)
	CFI_OFFSET(_REG_A5)
	CFI_OFFSET(_REG_A6)
	CFI_OFFSET(_REG_A7)
	CFI_OFFSET_DWARF_REG(DWARF_RETURN_REG, _REG_PC)
	CFI_OFFSET_DWARF_REG(DWARF_SIGRETURN_REG, _REG_PC)

/*
 * The unwind entry includes one instruction slot prior to the trampoline
 * because the unwinder will look up to (return PC - 1 insn) while unwinding.
 * Normally this would be the jump / branch, but since there isn't one in
 * this case, we place an explicit nop there instead.
 */
	nop

ENTRY_NOPROFILE(__sigtramp_siginfo_2)
	movl	8(%sp),%a0	/* get pointer to ucontext */
	movl	%a0,4(%sp)	/* put it in the argument slot */
				/* fake return address already there */
	SYSTRAP(setcontext)
	movl	%d0,4(%sp)	/* error code */
	SYSTRAP(exit)		/* exit */
	.cfi_endproc
END(__sigtramp_siginfo_2)
