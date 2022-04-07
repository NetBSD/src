/*	$NetBSD: __sigtramp2.S,v 1.5 2022/04/07 20:23:44 andvar Exp $	*/

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

#if defined(SYSLIBC_SCCS) && !defined(lint)
	RCSID("$NetBSD: __sigtramp2.S,v 1.5 2022/04/07 20:23:44 andvar Exp $")
#endif /* SYSLIBC_SCCS and not lint */


/*
 * The MIPS signal trampoline is invoked only to return from
 * the signal; the kernel calls the signal handler directly.
 *
 * On entry, stack looks like:
 *
 *		ucontext structure	sp + sizeof(siginfo_t)
 *	sp->	siginfo structure
 *
 * The DWARF register numbers for the general purpose registers are the
 * same as the architected register numbers.  For MIPS, there is a DWARF
 * pseudo-register for signal handler return addresses, as well as for the
 * MDLO and MDHI registers.
 */

#define	DWARF_MDHI_REG			64
#define	DWARF_MDLO_REG			65

#if defined(__LIBGCC_DWARF_ALT_FRAME_RETURN_COLUMN__)
#define	DWARF_SIGRETURN_REG		__LIBGCC_DWARF_ALT_FRAME_RETURN_COLUMN__
#else
#define	DWARF_SIGRETURN_REG		66
#endif

#define	CFI_OFFSET_DWARF_REG(d, r)	.cfi_offset d, r * SZREG
#define	CFI_OFFSET(r)			CFI_OFFSET_DWARF_REG(r, r)

	.text
	.cfi_startproc
	.cfi_signal_frame
	.cfi_def_cfa _REG_SP, SIGINFO_SIZE + _UC_GREGS
	CFI_OFFSET(_REG_R0)
	CFI_OFFSET(_REG_AT)
	CFI_OFFSET(_REG_V0)
	CFI_OFFSET(_REG_V1)
	CFI_OFFSET(_REG_A0)
	CFI_OFFSET(_REG_A1)
	CFI_OFFSET(_REG_A2)
	CFI_OFFSET(_REG_A3)
	CFI_OFFSET(_REG_T0)
	CFI_OFFSET(_REG_T1)
	CFI_OFFSET(_REG_T2)
	CFI_OFFSET(_REG_T3)
	CFI_OFFSET(_REG_T4)
	CFI_OFFSET(_REG_T5)
	CFI_OFFSET(_REG_T6)
	CFI_OFFSET(_REG_T7)
	CFI_OFFSET(_REG_S0)
	CFI_OFFSET(_REG_S1)
	CFI_OFFSET(_REG_S2)
	CFI_OFFSET(_REG_S3)
	CFI_OFFSET(_REG_S4)
	CFI_OFFSET(_REG_S5)
	CFI_OFFSET(_REG_S6)
	CFI_OFFSET(_REG_S7)
	CFI_OFFSET(_REG_T8)
	CFI_OFFSET(_REG_T9)
	CFI_OFFSET(_REG_K0)
	CFI_OFFSET(_REG_K1)
	CFI_OFFSET(_REG_GP)
	CFI_OFFSET(_REG_SP)
	CFI_OFFSET(_REG_S8)
	CFI_OFFSET(_REG_RA)
	CFI_OFFSET_DWARF_REG(DWARF_MDHI_REG, _REG_MDHI)
	CFI_OFFSET_DWARF_REG(DWARF_MDLO_REG, _REG_MDLO)
	.cfi_return_column DWARF_SIGRETURN_REG
	CFI_OFFSET_DWARF_REG(DWARF_SIGRETURN_REG, _REG_EPC)

/*
 * The unwind entry includes one instruction slot prior to the trampoline
 * because the unwinder will look up to (return PC - 1 insn) while unwinding.
 * Normally this would be the jump / branch, but since there isn't one in
 * this case, we place an explicit nop there instead.
 */
	nop

LEAF_NOPROFILE(__sigtramp_siginfo_2)
	PTR_ADDU	a0, sp, SIGINFO_SIZE	/* address of ucontext */
	SYSTRAP(setcontext)			/* and do setcontext */
	move		a0, v0			/* exit with errno */
	SYSTRAP(exit)				/* if sigreturn fails */
	.cfi_endproc
END(__sigtramp_siginfo_2)
