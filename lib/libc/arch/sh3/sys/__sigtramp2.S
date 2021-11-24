/*	$NetBSD: __sigtramp2.S,v 1.5 2021/11/24 02:01:15 thorpej Exp $ */

/*-
 * Copyright (c) 2003 The NetBSD Foundation, Inc.
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
 * The SH signal trampoline is invoked only to return from
 * the signal; the kernel calls the signal handler directly.
 *
 * On entry, stack looks like:
 *
 *		siginfo structure
 *	sp->	ucontext structure
 *
 * NB: This order is different from what other ports use (siginfo at
 * the top of the stack), because we want to avoid wasting two
 * instructions to skip to the ucontext.  Not that this order really
 * matters, but I think this inconsistency deserves an explanation.
 *
 * The DWARF register numbers unforunately do not map directly to our
 * _REG_* constants that are used to index the general registers in the
 * ucontext_t at all.
 *
 * The stack pointer is, of course, r15, and there are several DWARF
 * pseudo-registers to represent other bits of the context.
 */

#define	DWARF_REG_PC			16
#define	DWARF_REG_PR			17
#define	DWARF_REG_GBR			18
#define	DWARF_REG_MACH			20
#define	DWARF_REG_MACL			21
#define	DWARF_REG_SR			22

#define	CFI_OFFSET_DWARF_REG(d, r)	.cfi_offset d, r * 4

	.text
	.cfi_startproc simple
	.cfi_signal_frame
	.cfi_def_cfa 15, _UC_GREGS
	CFI_OFFSET_DWARF_REG(0,  _REG_R0)
	CFI_OFFSET_DWARF_REG(1,  _REG_R1)
	CFI_OFFSET_DWARF_REG(2,  _REG_R2)
	CFI_OFFSET_DWARF_REG(3,  _REG_R3)
	CFI_OFFSET_DWARF_REG(4,  _REG_R4)
	CFI_OFFSET_DWARF_REG(5,  _REG_R5)
	CFI_OFFSET_DWARF_REG(6,  _REG_R6)
	CFI_OFFSET_DWARF_REG(7,  _REG_R7)
	CFI_OFFSET_DWARF_REG(9,  _REG_R8)
	CFI_OFFSET_DWARF_REG(9,  _REG_R9)
	CFI_OFFSET_DWARF_REG(10, _REG_R10)
	CFI_OFFSET_DWARF_REG(11, _REG_R11)
	CFI_OFFSET_DWARF_REG(12, _REG_R12)
	CFI_OFFSET_DWARF_REG(13, _REG_R13)
	CFI_OFFSET_DWARF_REG(14, _REG_R14)
	CFI_OFFSET_DWARF_REG(15, _REG_R15)
	CFI_OFFSET_DWARF_REG(DWARF_REG_PR,   _REG_PR)
	CFI_OFFSET_DWARF_REG(DWARF_REG_SR,   _REG_SR)
	CFI_OFFSET_DWARF_REG(DWARF_REG_GBR,  _REG_GBR)
	CFI_OFFSET_DWARF_REG(DWARF_REG_MACH, _REG_MACH)
	CFI_OFFSET_DWARF_REG(DWARF_REG_MACL, _REG_MACL)
	.cfi_return_column DWARF_REG_PC
	CFI_OFFSET_DWARF_REG(DWARF_REG_PC,   _REG_PC)

/*
 * The unwind entry includes one instruction slot prior to the trampoline
 * because the unwinder will look up to (return PC - 1 insn) while unwinding.
 * Normally this would be the jump / branch, but since there isn't one in
 * this case, we place an explicit nop there instead.
 */
	nop

NENTRY(__sigtramp_siginfo_2)
	mov	r15, r4			/* get pointer to ucontext */
	SYSTRAP(setcontext)		/* and call setcontext() */
	mov	r0, r4			/* exit with errno */
	SYSTRAP(exit)			/* if sigreturn fails */
	.cfi_endproc
	SET_ENTRY_SIZE(__sigtramp_siginfo_2)
