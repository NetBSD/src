/*	$NetBSD: __sigtramp2.S,v 1.9 2020/10/19 11:29:26 kamil Exp $	*/

/*
 * Copyright (c) 2001 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Frank van der Linden for Wasabi Systems, Inc.
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
 *      This product includes software developed for the NetBSD Project by
 *      Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
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
 * The x86-64 signal trampoline is invoked only to return from
 * the signal; the kernel calls the signal handler directly.
 *
 * The unwind entry includes the one byte prior to the trampoline
 * because the unwinder will look up (return PC - 1) while unwinding.
 * Normally (return PC - 1) computes an address inside the call
 * instruction that created the child frame, but here there is no call
 * instruction so we have to manually add padding.
 */
	.cfi_startproc simple
	.cfi_signal_frame
	.cfi_def_cfa r15, 0
	.cfi_offset rax, UC_GREGS_RAX
	.cfi_offset rdx, UC_GREGS_RDX
	.cfi_offset rcx, UC_GREGS_RCX
	.cfi_offset rbx, UC_GREGS_RBX
	.cfi_offset rsi, UC_GREGS_RSI
	.cfi_offset rdi, UC_GREGS_RDI
	.cfi_offset rbp, UC_GREGS_RBP
	.cfi_offset rsp, UC_GREGS_RSP
	.cfi_offset r8,  UC_GREGS_R8
	.cfi_offset r9,  UC_GREGS_R9
	.cfi_offset r10, UC_GREGS_R10
	.cfi_offset r11, UC_GREGS_R11
	.cfi_offset r12, UC_GREGS_R12
	.cfi_offset r13, UC_GREGS_R13
	.cfi_offset r14, UC_GREGS_R14
	.cfi_offset r15, UC_GREGS_R15
	.cfi_offset rip, UC_GREGS_RIP
	nop
NENTRY(__sigtramp_siginfo_2)
	movq	%r15,%rdi
	movq	$SYS_setcontext, %rax
	syscall
	movq	$-1,%rdi /* if we return here, something is wrong */
	movq	$SYS_exit, %rax
	syscall
	.cfi_endproc
END(__sigtramp_siginfo_2)
