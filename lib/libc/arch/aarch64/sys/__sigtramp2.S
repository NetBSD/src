/* $NetBSD: __sigtramp2.S,v 1.2 2020/10/13 01:59:55 kamil Exp $ */

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
 * The ARM signal trampoline is invoked only to return from
 * the signal; the kernel calls the signal handler directly.
 *
 * On entry, the stack looks like:
 *
 *		ucontext structure
 *	sp->	siginfo structure
 * and x28 points to the ucontext
 *
 * The unwind entry includes the one byte prior to the trampoline
 * because the unwinder will look up (return PC - 1) while unwinding.
 * Normally (return PC - 1) computes an address inside the call
 * instruction that created the child frame, but here there is no call
 * instruction so we have to manually add padding.
 */
	.cfi_startproc simple
	.cfi_signal_frame
	.cfi_def_cfa x28, 0
	.cfi_offset x0, _UC_REGS_X0
	.cfi_offset x1, _UC_REGS_X1
	.cfi_offset x2, _UC_REGS_X2
	.cfi_offset x3, _UC_REGS_X3
	.cfi_offset x4, _UC_REGS_X4
	.cfi_offset x5, _UC_REGS_X5
	.cfi_offset x6, _UC_REGS_X6
	.cfi_offset x7, _UC_REGS_X7
	.cfi_offset x8, _UC_REGS_X8
	.cfi_offset x9, _UC_REGS_X9
	.cfi_offset x10, _UC_REGS_X10
	.cfi_offset x11, _UC_REGS_X11
	.cfi_offset x12, _UC_REGS_X12
	.cfi_offset x13, _UC_REGS_X13
	.cfi_offset x14, _UC_REGS_X14
	.cfi_offset x15, _UC_REGS_X15
	.cfi_offset x16, _UC_REGS_X16
	.cfi_offset x17, _UC_REGS_X17
	.cfi_offset x18, _UC_REGS_X18
	.cfi_offset x19, _UC_REGS_X19
	.cfi_offset x20, _UC_REGS_X20
	.cfi_offset x21, _UC_REGS_X21
	.cfi_offset x22, _UC_REGS_X22
	.cfi_offset x23, _UC_REGS_X23
	.cfi_offset x24, _UC_REGS_X24
	.cfi_offset x25, _UC_REGS_X25
	.cfi_offset x26, _UC_REGS_X26
	.cfi_offset x27, _UC_REGS_X27
	.cfi_offset x28, _UC_REGS_X28
	.cfi_offset x29, _UC_REGS_X29
	.cfi_offset x30, _UC_REGS_X30
	/* The unwinder will use the CFA to restore X31 (SP). */
	nop
ENTRY_NP(__sigtramp_siginfo_2)
	mov	x0, x28				/* set the arg         */
	SYSTRAP(setcontext)			/* and call setcontext */

	/* If that failed, exit with the error code. */
	SYSTRAP(exit)
	.cfi_endproc
END(__sigtramp_siginfo_2)
