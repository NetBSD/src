/*	$NetBSD: __sigtramp3.S,v 1.3 2021/11/24 15:05:16 thorpej Exp $	*/

/*
 * Copyright (c) 2003 Matt Thomas <matt@3am-software.com>
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

#include "SYS.h"
#include "assym.h"

#ifdef SYSLIBC_SCCS
RCSID("$NetBSD: __sigtramp3.S,v 1.3 2021/11/24 15:05:16 thorpej Exp $")
#endif

/*
 * Signal trampoline; registers when called:
 *	pc, psl - obvious
 *	sp, ap - points to a CALLG argument list
 *	fp - address of signal handler
 *
 * Stack is set up like so:
 *		ucontext structure
 *		siginfo structure
 *		vvvv CALLG argument list vvvv
 *		12(ap) -- pointer to ucontext
 *		 8(ap) -- pointer to siginfo
 *		 4(ap) -- signal number
 *	sp ->	 0(ap) -- argument count
 *		^^^^ CALLG argument list ^^^^
 *
 * N.B. all of the DWARF register numbers match our _REG_* constants.
 * Also notice that while the %ap is adjusted inside the trampoline,
 * the %sp is not adjusted, and so the CFA base does not change.
 */

#define	CFI_OFFSET(r)			.cfi_offset r, r * 4

	.text
#if 0
	.cfi_startproc simple
	.cfi_signal_frame
	.cfi_def_cfa _REG_SP, 16 + SIZEOF_SIGINFO + UC_GREGS
	CFI_OFFSET(_REG_R0)
	CFI_OFFSET(_REG_R1)
	CFI_OFFSET(_REG_R2)
	CFI_OFFSET(_REG_R3)
	CFI_OFFSET(_REG_R4)
	CFI_OFFSET(_REG_R5)
	CFI_OFFSET(_REG_R6)
	CFI_OFFSET(_REG_R7)
	CFI_OFFSET(_REG_R8)
	CFI_OFFSET(_REG_R9)
	CFI_OFFSET(_REG_R10)
	CFI_OFFSET(_REG_R11)
	CFI_OFFSET(_REG_AP)
	CFI_OFFSET(_REG_FP)
	CFI_OFFSET(_REG_SP)
	CFI_OFFSET(_REG_PC)
	CFI_OFFSET(_REG_PSL)
#endif

/*
 * The unwind entry includes one instruction slot prior to the trampoline
 * because the unwinder will look up to (return PC - 1) while unwinding.
 * Normally this would be the jump / branch, but since there isn't one in
 * this case, we place an explicit nop there instead.
 */

#if 0
	nop
#endif
	_ALIGN_TEXT

	.globl	_C_LABEL(__sigtramp_siginfo_3)
_C_LABEL(__sigtramp_siginfo_3):
	nop; nop
	callg (%ap),(%fp)		# use global arg list
	addl2 $8,%ap			# arg is pointer to ucontext
	SYSTRAP(setcontext)		# exit from here
	halt				# illegal insn
#if 0
	.cfi_endproc
#endif
