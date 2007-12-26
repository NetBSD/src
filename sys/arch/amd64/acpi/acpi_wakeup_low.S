/*	$NetBSD: acpi_wakeup_low.S,v 1.1.10.1 2007/12/26 19:17:16 ad Exp $	*/

/*-
 * Copyright (c) 2007 Joerg Sonnenberger <joerg@netbsd.org>
 * Copyright (c) 2001 Takanori Watanabe <takawata@jp.freebsd.org>
 * Copyright (c) 2001 Mitsuru IWASAKI <iwasaki@jp.freebsd.org>
 * All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include "assym.h"
#include <machine/asm.h>
#include <machine/segments.h>
#include <machine/specialreg.h>

	.text
	.p2align 2, 0x90
	.globl acpi_md_sleep_exit
acpi_md_sleep_exit:
	lgdt	ACPI_SUSPEND_GDT(%r8)

	/* Reload fixed descriptors for new GDT */
	movw	$GSEL(GDATA_SEL, SEL_KPL),%ax
	movw	%ax,%ds
	movw	%ax,%es
	movw	%ax,%ss

	/* FS and GS are driven by MSRs, so use NULL for them */
	xorw	%ax,%ax
	movw	%ax,%fs
	movw	%ax,%gs

	movl	$MSR_EFER,%ecx
	movl	ACPI_SUSPEND_MSR_EFER(%r8),%eax
	wrmsr

	movl	$MSR_FSBASE,%ecx
	movl	ACPI_SUSPEND_FS_BASE_L(%r8),%eax
	movl	ACPI_SUSPEND_FS_BASE_H(%r8),%edx
	wrmsr

	movl	$MSR_GSBASE,%ecx
	movl	ACPI_SUSPEND_GS_BASE_L(%r8),%eax
	movl	ACPI_SUSPEND_GS_BASE_H(%r8),%edx
	wrmsr

	movl	$MSR_KERNELGSBASE,%ecx
	movl	ACPI_SUSPEND_GS_KERNELBASE_L(%r8),%eax
	movl	ACPI_SUSPEND_GS_KERNELBASE_H(%r8),%edx
	wrmsr

	movq	ACPI_SUSPEND_CR8(%r8),%rax
	movq	%rax,%cr8
	movq	ACPI_SUSPEND_CR4(%r8),%rax
	movq	%rax,%cr4
	movq	ACPI_SUSPEND_CR3(%r8),%rax
	movq	%rax,%cr3
	movq	ACPI_SUSPEND_CR2(%r8),%rax
	movq	%rax,%cr2
	movq	ACPI_SUSPEND_CR0(%r8),%rax
	movq	%rax,%cr0

	jmp 1f
1:

	movq	CPUVAR(GDT),%rax
	movzwq	ACPI_SUSPEND_TR(%r8),%rdx
	andq	$~0x0200,4(%rax,%rdx, 1)

	ltr	%dx
	lldt 	ACPI_SUSPEND_LDT(%r8)
	lidt	ACPI_SUSPEND_IDT(%r8)

	movq	ACPI_SUSPEND_RSP(%r8),%rsp

	movq	ACPI_SUSPEND_RBX(%r8),%rbx
	movq	ACPI_SUSPEND_RBP(%r8),%rbp
	movq	ACPI_SUSPEND_R12(%r8),%r12
	movq	ACPI_SUSPEND_R13(%r8),%r13
	movq	ACPI_SUSPEND_R14(%r8),%r14
	movq	ACPI_SUSPEND_R15(%r8),%r15

	xorq	%rax,%rax

	pushq	ACPI_SUSPEND_RFL(%r8)
	popfq
	ret

	.p2align 2, 0x90
	.type acpi_md_sleep_prepare, @function
	.globl acpi_md_sleep_prepare
acpi_md_sleep_prepare:
	movq	CPUVAR(SELF),%r8
	movq	%rbx,ACPI_SUSPEND_RBX(%r8)
	movq	%rbp,ACPI_SUSPEND_RBP(%r8)
	movq	%r12,ACPI_SUSPEND_R12(%r8)
	movq	%r13,ACPI_SUSPEND_R13(%r8)
	movq	%r14,ACPI_SUSPEND_R14(%r8)
	movq	%r15,ACPI_SUSPEND_R15(%r8)

	movq	%cr0,%rax
	movq	%rax,ACPI_SUSPEND_CR0(%r8)
	movq	%cr2,%rax
	movq	%rax,ACPI_SUSPEND_CR2(%r8)
	movq	%cr3,%rax
	movq	%rax,ACPI_SUSPEND_CR3(%r8)
	movq	%cr4,%rax
	movq	%rax,ACPI_SUSPEND_CR4(%r8)
	movq	%cr8,%rax
	movq	%rax,ACPI_SUSPEND_CR8(%r8)

	pushfq
	popq	ACPI_SUSPEND_RFL(%r8)

	movq	%rsp,ACPI_SUSPEND_RSP(%r8)

	movl	$MSR_FSBASE,%ecx
	rdmsr
	movl	%eax,ACPI_SUSPEND_FS_BASE_L(%r8)
	movl	%edx,ACPI_SUSPEND_FS_BASE_H(%r8)

	movl	$MSR_GSBASE,%ecx
	rdmsr
	movl	%eax,ACPI_SUSPEND_GS_BASE_L(%r8)
	movl	%edx,ACPI_SUSPEND_GS_BASE_H(%r8)

	movl	$MSR_KERNELGSBASE,%ecx
	rdmsr
	movl	%eax,ACPI_SUSPEND_GS_KERNELBASE_L(%r8)
	movl	%edx,ACPI_SUSPEND_GS_KERNELBASE_H(%r8)

	movl	$MSR_EFER,%ecx
	rdmsr
	movl	%eax,ACPI_SUSPEND_MSR_EFER(%r8)

	sgdt	ACPI_SUSPEND_GDT(%r8)
	sidt	ACPI_SUSPEND_IDT(%r8)
	sldt	ACPI_SUSPEND_LDT(%r8)
	str	ACPI_SUSPEND_TR(%r8)

	call	acpi_md_sleep_enter
	/* acpi_md_sleep_enter only returns on failure. */
	movl	$-1,%eax
	ret
