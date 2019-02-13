/*	$NetBSD: nvmm_x86_vmxfunc.S,v 1.1 2019/02/13 16:03:16 maxv Exp $	*/

/*
 * Copyright (c) 2018 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Maxime Villard.
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

/* Override user-land alignment before including asm.h */
#define	ALIGN_DATA	.align	8
#define ALIGN_TEXT	.align 16,0x90
#define _ALIGN_TEXT	ALIGN_TEXT

#define _LOCORE
#include "assym.h"
#include <machine/asm.h>
#include <machine/segments.h>
#include <x86/specialreg.h>

#define ASM_NVMM
#include <dev/nvmm/x86/nvmm_x86.h>

	.text

/*
 * %rdi = *pa
 */
ENTRY(_vmx_vmxon)
	vmxon	(%rdi)
	jz	.Lfail_vmxon
	jc	.Lfail_vmxon
	xorq	%rax,%rax
	retq
.Lfail_vmxon:
	movq	$-1,%rax
	retq
END(_vmx_vmxon)

/*
 * no arg
 */
ENTRY(_vmx_vmxoff)
	vmxoff
	jz	.Lfail_vmxoff
	jc	.Lfail_vmxoff
	xorq	%rax,%rax
	retq
.Lfail_vmxoff:
	movq	$-1,%rax
	retq
END(_vmx_vmxoff)

/*
 * %rdi = op
 * %rsi = *descriptor
 */
ENTRY(_vmx_invept)
	invept	(%rsi),%rdi
	jz	.Linvept_failvalid
	jc	.Linvept_failinvalid
	xorq	%rax,%rax
	retq
.Linvept_failvalid:
	movq	$-1,%rax
	retq
.Linvept_failinvalid:
	movq	$-2,%rax
	retq
END(_vmx_invept)

/*
 * %rdi = op
 * %rsi = *descriptor
 */
ENTRY(_vmx_invvpid)
	invvpid	(%rsi),%rdi
	jz	.Linvvpid_failvalid
	jc	.Linvvpid_failinvalid
	xorq	%rax,%rax
	retq
.Linvvpid_failvalid:
	movq	$-1,%rax
	retq
.Linvvpid_failinvalid:
	movq	$-2,%rax
	retq
END(_vmx_invvpid)

/*
 * %rdi = op
 * %rsi = *val
 */
ENTRY(_vmx_vmread)
	vmread	%rdi,(%rsi)
	jz	.Lvmread_failvalid
	jc	.Lvmread_failinvalid
	xorq	%rax,%rax
	retq
.Lvmread_failvalid:
	movq	$-1,%rax
	retq
.Lvmread_failinvalid:
	movq	$-2,%rax
	retq
END(_vmx_vmread)

/*
 * %rdi = op
 * %rsi = val
 */
ENTRY(_vmx_vmwrite)
	vmwrite	%rsi,%rdi
	jz	.Lvmwrite_failvalid
	jc	.Lvmwrite_failinvalid
	xorq	%rax,%rax
	retq
.Lvmwrite_failvalid:
	movq	$-1,%rax
	retq
.Lvmwrite_failinvalid:
	movq	$-2,%rax
	retq
END(_vmx_vmwrite)

/*
 * %rdi = *pa
 */
ENTRY(_vmx_vmptrld)
	vmptrld	(%rdi)
	jz	.Lfail_vmptrld
	jc	.Lfail_vmptrld
	xorq	%rax,%rax
	retq
.Lfail_vmptrld:
	movq	$-1,%rax
	retq
END(_vmx_vmptrld)

/*
 * %rdi = *pa
 */
ENTRY(_vmx_vmptrst)
	vmptrst	(%rdi)
	jz	.Lfail_vmptrst
	jc	.Lfail_vmptrst
	xorq	%rax,%rax
	retq
.Lfail_vmptrst:
	movq	$-1,%rax
	retq
END(_vmx_vmptrst)

/*
 * %rdi = pa
 */
ENTRY(_vmx_vmclear)
	vmclear	(%rdi)
	jz	.Lfail_vmclear
	jc	.Lfail_vmclear
	xorq	%rax,%rax
	retq
.Lfail_vmclear:
	movq	$-1,%rax
	retq
END(_vmx_vmclear)

/* redef */
#define VMCS_HOST_RSP				0x00006C14

#define HOST_SAVE_GPRS		\
	pushq	%rbx		;\
	pushq	%rbp		;\
	pushq	%r12		;\
	pushq	%r13		;\
	pushq	%r14		;\
	pushq	%r15

#define HOST_RESTORE_GPRS	\
	popq	%r15		;\
	popq	%r14		;\
	popq	%r13		;\
	popq	%r12		;\
	popq	%rbp		;\
	popq	%rbx

#define HOST_SAVE_RAX		\
	pushq	%rax

#define HOST_RESTORE_RAX	\
	popq	%rax

#define HOST_SAVE_LDT		\
	sldtw	%ax		;\
	pushw	%ax

#define HOST_RESTORE_LDT	\
	popw	%ax		;\
	lldtw	%ax

/*
 * We don't save RAX (done manually), but we do restore it.
 */

#define GUEST_SAVE_GPRS(reg)				\
	movq	%rbx,(NVMM_X64_GPR_RBX * 8)(reg)	;\
	movq	%rcx,(NVMM_X64_GPR_RCX * 8)(reg)	;\
	movq	%rdx,(NVMM_X64_GPR_RDX * 8)(reg)	;\
	movq	%r8,(NVMM_X64_GPR_R8 * 8)(reg)		;\
	movq	%r9,(NVMM_X64_GPR_R9 * 8)(reg)		;\
	movq	%r10,(NVMM_X64_GPR_R10 * 8)(reg)	;\
	movq	%r11,(NVMM_X64_GPR_R11 * 8)(reg)	;\
	movq	%r12,(NVMM_X64_GPR_R12 * 8)(reg)	;\
	movq	%r13,(NVMM_X64_GPR_R13 * 8)(reg)	;\
	movq	%r14,(NVMM_X64_GPR_R14 * 8)(reg)	;\
	movq	%r15,(NVMM_X64_GPR_R15 * 8)(reg)	;\
	movq	%rbp,(NVMM_X64_GPR_RBP * 8)(reg)	;\
	movq	%rdi,(NVMM_X64_GPR_RDI * 8)(reg)	;\
	movq	%rsi,(NVMM_X64_GPR_RSI * 8)(reg)

#define GUEST_RESTORE_GPRS(reg)				\
	movq	(NVMM_X64_GPR_RBX * 8)(reg),%rbx	;\
	movq	(NVMM_X64_GPR_RCX * 8)(reg),%rcx	;\
	movq	(NVMM_X64_GPR_RDX * 8)(reg),%rdx	;\
	movq	(NVMM_X64_GPR_R8 * 8)(reg),%r8		;\
	movq	(NVMM_X64_GPR_R9 * 8)(reg),%r9		;\
	movq	(NVMM_X64_GPR_R10 * 8)(reg),%r10	;\
	movq	(NVMM_X64_GPR_R11 * 8)(reg),%r11	;\
	movq	(NVMM_X64_GPR_R12 * 8)(reg),%r12	;\
	movq	(NVMM_X64_GPR_R13 * 8)(reg),%r13	;\
	movq	(NVMM_X64_GPR_R14 * 8)(reg),%r14	;\
	movq	(NVMM_X64_GPR_R15 * 8)(reg),%r15	;\
	movq	(NVMM_X64_GPR_RBP * 8)(reg),%rbp	;\
	movq	(NVMM_X64_GPR_RDI * 8)(reg),%rdi	;\
	movq	(NVMM_X64_GPR_RSI * 8)(reg),%rsi	;\
	movq	(NVMM_X64_GPR_RAX * 8)(reg),%rax

/*
 * %rdi = VA of guest GPR state
 */
ENTRY(vmx_vmlaunch)
	/* Save the Host GPRs. */
	HOST_SAVE_GPRS

	/* Disable Host interrupts. */
	cli

	/* Save the Host LDT. */
	HOST_SAVE_LDT

	/* Save the Host RAX. */
	movq	%rdi,%rax
	pushq	%rax

	/* Save the Host RSP. */
	movq	$VMCS_HOST_RSP,%rdi
	movq	%rsp,%rsi
	vmwrite	%rsi,%rdi

	/* Restore the Guest GPRs. */
	GUEST_RESTORE_GPRS(%rax)

	/* Run the VM. */
	vmlaunch

	/* Failure. */
	addq	$8,%rsp
	HOST_RESTORE_LDT
	sti
	HOST_RESTORE_GPRS
	movq	$-1,%rax
	retq
END(vmx_vmlaunch)

/*
 * %rdi = VA of guest GPR state
 */
ENTRY(vmx_vmresume)
	/* Save the Host GPRs. */
	HOST_SAVE_GPRS

	/* Disable Host interrupts. */
	cli

	/* Save the Host LDT. */
	HOST_SAVE_LDT

	/* Save the Host RAX. */
	movq	%rdi,%rax
	pushq	%rax

	/* Save the Host RSP. */
	movq	$VMCS_HOST_RSP,%rdi
	movq	%rsp,%rsi
	vmwrite	%rsi,%rdi

	/* Restore the Guest GPRs. */
	GUEST_RESTORE_GPRS(%rax)

	/* Run the VM. */
	vmresume

	/* Failure. */
	addq	$8,%rsp
	HOST_RESTORE_LDT
	sti
	HOST_RESTORE_GPRS
	movq	$-1,%rax
	retq
END(vmx_vmresume)

/*
 * The CPU jumps here after a #VMEXIT.
 */
ENTRY(vmx_resume_rip)
	/* Save the Guest GPRs. RAX done manually. */
	pushq	%rax
	movq	8(%rsp),%rax
	GUEST_SAVE_GPRS(%rax)
	popq	%rbx
	movq	%rbx,(NVMM_X64_GPR_RAX * 8)(%rax)
	addq	$8,%rsp

	/* Restore the Host LDT. */
	HOST_RESTORE_LDT

	/* Enable Host interrupts. */
	sti

	/* Restore the Host GPRs. */
	HOST_RESTORE_GPRS

	xorq	%rax,%rax
	retq
END(vmx_resume_rip)
