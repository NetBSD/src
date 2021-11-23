/* $NetBSD: __sigtramp2.S,v 1.7 2021/11/23 02:49:56 thorpej Exp $ */

/*-
 * Copyright (c) 2014 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Matt Thomas of 3am Software Foundry.
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
 * The ARM64 signal trampoline is invoked only to return from
 * the signal; the kernel calls the signal handler directly.
 *
 * On entry, the stack looks like:
 *
 *		ucontext structure
 *	sp->	siginfo structure
 * and x28 points to the ucontext
 *
 * A DWARF pseudo-register is used for the return address from the
 * signal trampoline.
 */

#if defined(__LIBGCC_DWARF_ALT_FRAME_RETURN_COLUMN__)
#define	DWARF_SIGRETURN_REG		__LIBGCC_DWARF_ALT_FRAME_RETURN_COLUMN__
#else
#define	DWARF_SIGRETURN_REG		96
#endif

#define	CFI_OFFSET_DWARF_REG(d, r)	.cfi_offset d, r * 8
#define	CFI_OFFSET(r)			CFI_OFFSET_DWARF_REG(r, r)

	.text
	.cfi_startproc simple
	.cfi_signal_frame
	.cfi_def_cfa _REG_X28, _UC_GREGS
	CFI_OFFSET(_REG_X0)
	CFI_OFFSET(_REG_X1)
	CFI_OFFSET(_REG_X2)
	CFI_OFFSET(_REG_X3)
	CFI_OFFSET(_REG_X4)
	CFI_OFFSET(_REG_X5)
	CFI_OFFSET(_REG_X6)
	CFI_OFFSET(_REG_X7)
	CFI_OFFSET(_REG_X8)
	CFI_OFFSET(_REG_X9)
	CFI_OFFSET(_REG_X10)
	CFI_OFFSET(_REG_X11)
	CFI_OFFSET(_REG_X12)
	CFI_OFFSET(_REG_X13)
	CFI_OFFSET(_REG_X14)
	CFI_OFFSET(_REG_X15)
	CFI_OFFSET(_REG_X16)
	CFI_OFFSET(_REG_X17)
	CFI_OFFSET(_REG_X18)
	CFI_OFFSET(_REG_X19)
	CFI_OFFSET(_REG_X20)
	CFI_OFFSET(_REG_X21)
	CFI_OFFSET(_REG_X22)
	CFI_OFFSET(_REG_X23)
	CFI_OFFSET(_REG_X24)
	CFI_OFFSET(_REG_X25)
	CFI_OFFSET(_REG_X26)
	CFI_OFFSET(_REG_X27)
	CFI_OFFSET(_REG_X28)
	CFI_OFFSET(_REG_X29)
	CFI_OFFSET(_REG_X30)	/* a.k.a. _REG_LR */
	CFI_OFFSET(_REG_X31)	/* a.k.a. _REG_SP */
	.cfi_return_column DWARF_SIGRETURN_REG
	CFI_OFFSET_DWARF_REG(DWARF_SIGRETURN_REG, _REG_PC)

/*
 * The unwind entry includes the one instruction prior to the trampoline
 * because the unwinder will look up (return PC - 1) while unwinding.
 * Normally this would be the jump / branch, but since there isn't one in
 * this case, we place an explicit nop there instead.
 */
	nop

ENTRY_NP(__sigtramp_siginfo_2)
	mov	x0, x28				/* set the arg         */
	SYSTRAP(setcontext)			/* and call setcontext */

	/* If that failed, exit with the error code. */
	SYSTRAP(exit)
	.cfi_endproc
END(__sigtramp_siginfo_2)
