/*	$NetBSD: nvmm_x86_svmfunc.S,v 1.1 2018/11/07 07:43:08 maxv Exp $	*/

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

#define HOST_SAVE_MSR(msr)	\
	movq	$msr,%rcx	;\
	rdmsr			;\
	pushq	%rdx		;\
	pushq	%rax

#define HOST_RESTORE_MSR(msr)	\
	popq	%rax		;\
	popq	%rdx		;\
	movq	$msr,%rcx	;\
	wrmsr

#define HOST_SAVE_SEGREG(sreg)	\
	movw	sreg,%ax	;\
	pushw	%ax

#define HOST_RESTORE_SEGREG(sreg)\
	popw	%ax		;\
	movw	%ax,sreg

#define HOST_SAVE_TR		\
	strw	%ax		;\
	pushw	%ax

#define HOST_RESTORE_TR				\
	popw	%ax				;\
	movzwq	%ax,%rdx			;\
	movq	CPUVAR(GDT),%rax		;\
	andq	$~0x0200,4(%rax,%rdx, 1)	;\
	ltrw	%dx

#define HOST_SAVE_LDT		\
	sldtw	%ax		;\
	pushw	%ax

#define HOST_RESTORE_LDT	\
	popw	%ax		;\
	lldtw	%ax

/*
 * All GPRs except RAX and RSP, which are taken care of in VMCB.
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
	movq	(NVMM_X64_GPR_RSI * 8)(reg),%rsi

/*
 * %rdi = PA of VMCB
 * %rsi = VA of guest GPR state
 */
ENTRY(svm_vmrun)
	/* Save the Host GPRs. */
	HOST_SAVE_GPRS

	/* Disable Host interrupts. */
	clgi

	/* Save the Host TR. */
	HOST_SAVE_TR

	/* Save the variable Host MSRs. */
	HOST_SAVE_MSR(MSR_KERNELGSBASE)
	HOST_SAVE_MSR(MSR_GSBASE)
	HOST_SAVE_MSR(MSR_FSBASE)

	/* Reset the Host Segregs. */
	movq	$GSEL(GUDATA_SEL, SEL_UPL),%rax
	movw	%ax,%ds
	movw	%ax,%es
	xorq	%rax,%rax
	movw	%ax,%fs
	movw	%ax,%gs

	/* Save some Host Segregs. */
	HOST_SAVE_SEGREG(%fs)
	HOST_SAVE_SEGREG(%gs)

	/* Save the Host LDT. */
	HOST_SAVE_LDT

	/* Prepare RAX. */
	pushq	%rsi
	pushq	%rdi

	/* Restore the Guest GPRs. */
	movq	%rsi,%rax
	GUEST_RESTORE_GPRS(%rax)

	/* Set RAX. */
	popq	%rax

	/* Run the VM. */
	vmload	%rax
	vmrun	%rax
	vmsave	%rax

	/* Get RAX. */
	popq	%rax

	/* Save the Guest GPRs. */
	GUEST_SAVE_GPRS(%rax)

	/* Restore the Host LDT. */
	HOST_RESTORE_LDT

	/* Restore the Host Segregs. */
	HOST_RESTORE_SEGREG(%gs)
	HOST_RESTORE_SEGREG(%fs)

	/* Restore the variable Host MSRs. */
	HOST_RESTORE_MSR(MSR_FSBASE)
	HOST_RESTORE_MSR(MSR_GSBASE)
	HOST_RESTORE_MSR(MSR_KERNELGSBASE)

	/* Restore the Host TR. */
	HOST_RESTORE_TR

	/* Enable Host interrupts. */
	stgi

	/* Restore the Host GPRs. */
	HOST_RESTORE_GPRS

	xorq	%rax,%rax
	retq
END(svm_vmrun)
