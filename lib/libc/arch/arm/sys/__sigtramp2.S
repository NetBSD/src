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

/*
 * The ARM signal trampoline is invoked only to return from
 * the signal; the kernel calls the signal handler directly.
 *
 * On entry, the stack looks like:
 *
 *		ucontext structure
 *	sp->	siginfo structure
 * and r5 points to the ucontext
 */

#define	CFI_OFFSET(r)			.cfi_offset r, r * 4

	_TEXT_SECTION
	.cfi_startproc
	.cfi_signal_frame
	.cfi_def_cfa _REG_SP, SIZEOF_SIGINFO + _UC_GREGS
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
	CFI_OFFSET(_REG_R13)	/* a.k.a. _REG_SP */
	CFI_OFFSET(_REG_R14)	/* a.k.a. _REG_LR */
	.cfi_return_column _REG_R15
	CFI_OFFSET(_REG_R15)	/* a.k.a. _REG_PC */

/*
 * The unwind entry includes the one instruction prior to the trampoline
 * because the unwinder will look up (return PC - 1) while unwinding.
 * Normally this would be the jump / branch, but since there isn't one in
 * this case, we place an explicit nop there instead.
 */
	nop

ENTRY_NP(__sigtramp_siginfo_2)
	mov r0, r5				/* set the arg         */
	SYSTRAP(setcontext)			/* and call setcontext */

	/* If that failed, exit with the error code. */
	SYSTRAP(exit)
	.cfi_endproc
END(__sigtramp_siginfo_2)
