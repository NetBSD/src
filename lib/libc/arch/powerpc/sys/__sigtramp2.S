/*	$NetBSD: __sigtramp2.S,v 1.5 2021/11/27 10:00:01 skrll Exp $	*/

/*-
 * Copyright (c) 2002 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe.
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

#if defined(LIBC_SCCS) && !defined(lint)
__RCSID("$NetBSD: __sigtramp2.S,v 1.5 2021/11/27 10:00:01 skrll Exp $")
#endif /* LIBC_SCCS && !lint */

/*
 * On entry to user space, stack and registers look like:
 *
 *	r3	signal number
 *	r4	pointer to siginfo structure
 *	r5	pointer to ucontext structure
 *	r30	pointer to ucontext structure
 *	srr0	address of handler
 *	lr	address of this trampoline
 *
 * We use r30 as the CFA base register since it's been conveniently set up
 * for us that way.  The DWARF register numbers used in the .eh_frame are:
 *
 *   0 - 31	GPRs
 *  32 - 63	FPRs
 *  65		LR
 *  66		CTR
 *  68-75	CRs
 *  76		XER
 *  77 - 108	AltiVec regs
 *  99		signal trampoline return address
 * 109		VRSAVE
 * 110		VCSR
 *
 * N.B. Only one CR slot is used (CR2) in the SVR4 ABI.
 */

#define	DWARF_LR_REG			65
#define	DWARF_CTR_REG			66
#define	DWARF_CR2_REG			70
#define	DWARF_XER_REG			76
#define	DWARF_SIGRETURN_REG		99

#define	CFI_OFFSET_DWARF_REG(d, r)	.cfi_offset d, r * 4
#define	CFI_OFFSET(r)			CFI_OFFSET_DWARF_REG(r, r)

	.text
	.cfi_startproc simple
	.cfi_signal_frame
	.cfi_def_cfa _REG_R30, UC_GREGS
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
	CFI_OFFSET(_REG_R12)
	CFI_OFFSET(_REG_R13)
	CFI_OFFSET(_REG_R14)
	CFI_OFFSET(_REG_R15)
	CFI_OFFSET(_REG_R16)
	CFI_OFFSET(_REG_R17)
	CFI_OFFSET(_REG_R18)
	CFI_OFFSET(_REG_R19)
	CFI_OFFSET(_REG_R20)
	CFI_OFFSET(_REG_R21)
	CFI_OFFSET(_REG_R22)
	CFI_OFFSET(_REG_R23)
	CFI_OFFSET(_REG_R24)
	CFI_OFFSET(_REG_R25)
	CFI_OFFSET(_REG_R26)
	CFI_OFFSET(_REG_R27)
	CFI_OFFSET(_REG_R28)
	CFI_OFFSET(_REG_R29)
	CFI_OFFSET(_REG_R30)
	CFI_OFFSET(_REG_R31)
	CFI_OFFSET_DWARF_REG(DWARF_CR2_REG, _REG_CR)
	CFI_OFFSET_DWARF_REG(DWARF_LR_REG, _REG_LR)
	CFI_OFFSET_DWARF_REG(DWARF_CTR_REG, _REG_CTR)
	CFI_OFFSET_DWARF_REG(DWARF_XER_REG, _REG_XER)
	.cfi_return_column DWARF_SIGRETURN_REG
	CFI_OFFSET_DWARF_REG(DWARF_SIGRETURN_REG, _REG_PC)

/*
 * The unwind entry includes one instruction slot prior to the trampoline
 * because the unwinder will look up to (return PC - 1 insn) while unwinding.
 * Normally this would be the jump / branch, but since there isn't one in
 * this case, we place an explicit nop there instead.
 */
	nop

ENTRY_NOPROFILE(__sigtramp_siginfo_2)
	mr	%r3,%r30		/* restore ucontext pointer */
	_DOSYSCALL(setcontext)		/* restore machine state */
	_DOSYSCALL(exit)		/* or exit with errno if failed */
	.cfi_endproc
END(__sigtramp_siginfo_2)
