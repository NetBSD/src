/*	$NetBSD: __sigtramp2.S,v 1.5 2024/05/30 15:56:43 skrll Exp $	*/

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
	RCSID("$NetBSD: __sigtramp2.S,v 1.5 2024/05/30 15:56:43 skrll Exp $")
#endif /* SYSLIBC_SCCS and not lint */


/*
 * The RISC-V signal trampoline is invoked only to return from
 * the signal; the kernel calls the signal handler directly.
 *
 * On entry, stack looks like:
 *
 *	sp			->	siginfo_t structure
 *	sp + SIGINFO_SIZE	->	ucontext_t structure
 *
 * The DWARF register numbers for the general purpose registers are the
 * same as the architected register numbers.  For RISC-V, there is a DWARF
 * pseudo-register for signal handler return address.
 */

#if defined(__LIBGCC_DWARF_ALT_FRAME_RETURN_COLUMN__)
#define	DWARF_SIGRETURN_REG		__LIBGCC_DWARF_ALT_FRAME_RETURN_COLUMN__
#else
#define	DWARF_SIGRETURN_REG		64
#endif

	.text
	.cfi_startproc
	.cfi_signal_frame
	.cfi_def_cfa	sp, SIGINFO_SIZE
	.cfi_offset	ra,  UC_GREGS_RA
	.cfi_offset	sp,  UC_GREGS_SP
	.cfi_offset	gp,  UC_GREGS_GP
	.cfi_offset	tp,  UC_GREGS_TP
	.cfi_offset	t0,  UC_GREGS_T0
	.cfi_offset	t1,  UC_GREGS_T1
	.cfi_offset	t2,  UC_GREGS_T2
	.cfi_offset	s0,  UC_GREGS_S0
	.cfi_offset	s1,  UC_GREGS_S1
	.cfi_offset	a0,  UC_GREGS_A0
	.cfi_offset	a1,  UC_GREGS_A1
	.cfi_offset	a2,  UC_GREGS_A2
	.cfi_offset	a3,  UC_GREGS_A3
	.cfi_offset	a4,  UC_GREGS_A4
	.cfi_offset	a5,  UC_GREGS_A5
	.cfi_offset	a6,  UC_GREGS_A6
	.cfi_offset	a7,  UC_GREGS_A7
	.cfi_offset	s2,  UC_GREGS_S2
	.cfi_offset	s3,  UC_GREGS_S3
	.cfi_offset	s4,  UC_GREGS_S4
	.cfi_offset	s5,  UC_GREGS_S5
	.cfi_offset	s6,  UC_GREGS_S6
	.cfi_offset	s7,  UC_GREGS_S7
	.cfi_offset	s8,  UC_GREGS_S8
	.cfi_offset	s9,  UC_GREGS_S9
	.cfi_offset	s10, UC_GREGS_S10
	.cfi_offset	s11, UC_GREGS_S11
	.cfi_offset	t3,  UC_GREGS_T3
	.cfi_offset	t4,  UC_GREGS_T4
	.cfi_offset	t5,  UC_GREGS_T5
	.cfi_offset	t6,  UC_GREGS_T6
	.cfi_return_column DWARF_SIGRETURN_REG
	.cfi_offset	DWARF_SIGRETURN_REG, UC_GREGS_PC

/*
 * The unwind entry includes one instruction slot prior to the trampoline
 * because the unwinder will look up to (return PC - 1 insn) while unwinding.
 * Normally this would be the jump / branch, but since there isn't one in
 * this case, we place an explicit nop there instead.
 */
	nop

ENTRY_NP(__sigtramp_siginfo_2)
	addi		a0, sp, SIGINFO_SIZE	/* address of ucontext */
	SYSTRAP_NOERROR(setcontext)		/* and do setcontext */
	SYSTRAP(exit)				/* if sigreturn fails */
	.cfi_endproc
END(__sigtramp_siginfo_2)
