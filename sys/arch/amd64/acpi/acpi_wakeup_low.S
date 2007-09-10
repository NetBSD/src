/*	$NetBSD: acpi_wakeup_low.S,v 1.1.2.2 2007/09/10 23:11:23 jmcneill Exp $	*/

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

	.text
	.p2align 2, 0x90
	.globl acpi_restorecpu
acpi_restorecpu:

	movw	r_fs,%ax
	movw	%ax,%fs
	movw	r_gs,%ax
	movw	%ax,%gs

	movq	r_cr8,%rax
	movq	%rax,%cr8
	movq	r_cr4,%rax
	movq	%rax,%cr4
	movq	r_cr3,%rax
	movq	%rax,%cr3
	movq	r_cr2,%rax
	movq	%rax,%cr2
	movq	r_cr0,%rax
	movq	%rax,%cr0


	jmp 1f
1:
	//movq	CPUVAR(GDT),%rax
	//movzwq	r_tr,%rdx
	//andq	$~0x0200,4(%rax,%rdx, 1)
	//ltr	%dx

	movq	r_rsp,%rsp

	movq	r_rbx,%rbx
	movq	r_rcx,%rcx
	movq	r_rdx,%rdx
	movq	r_rbp,%rbp
	movq	r_rsi,%rsi
	movq	r_rdi,%rdi
	movq	r_r8,%r8
	movq	r_r9,%r9
	movq	r_r10,%r10
	movq	r_r11,%r11
	movq	r_r12,%r12
	movq	r_r13,%r13
	movq	r_r14,%r14
	movq	r_r15,%r15

	lidt	r_idt
	//lldt 	r_ldt

	movq ret_addr,%rax
	movq %rax,(%rsp)
	xorq	%rax,%rax

	pushq	r_rfl
	popfq

	ret

	.p2align 2, 0x90
	.type acpi_savecpu, @function
	.globl acpi_savecpu
acpi_savecpu:
	movw	%fs,r_fs
	movw	%gs,r_gs

	movq	%rbx,r_rbx
	movq	%rcx,r_rcx
	movq	%rdx,r_rdx
	movq	%rbp,r_rbp
	movq	%rsi,r_rsi
	movq	%rdi,r_rdi
	movq	%r8,r_r8
	movq	%r9,r_r9
	movq	%r10,r_r10
	movq	%r11,r_r11
	movq	%r12,r_r12
	movq	%r13,r_r13
	movq	%r14,r_r14
	movq	%r15,r_r15

	movq	%cr0,%rax
	movq	%rax,r_cr0
	movq	%cr2,%rax
	movq	%rax,r_cr2
	movq	%cr3,%rax
	movq	%rax,r_cr3
	movq	%cr4,%rax
	movq	%rax,r_cr4
	movq	%cr8,%rax
	movq	%rax,r_cr8

	pushfq
	popq	r_rfl

	movq	%rsp,r_rsp

	sgdt	r_gdt
	sidt	r_idt
	sldt	r_ldt
	str	r_tr

	movq	(%rsp),%rax
	movq	%rax,ret_addr
	movq	$1,%rax
	ret

	.data
	.align 16
	.globl acpi_wakeup_ds
acpi_wakeup_ds:
r_fs:		.word	0
r_gs:		.word	0
r_tr:		.word	0
r_ldt:		.word	0

r_rbx:		.quad	0
r_rcx:		.quad	0
r_rdx:		.quad	0
r_rbp:		.quad	0
r_rsi:		.quad	0
r_rdi:		.quad	0
r_rsp:		.quad	0
r_rfl:		.quad	0
r_r8:		.quad	0
r_r9:		.quad	0
r_r10:		.quad	0
r_r11:		.quad	0
r_r12:		.quad	0
r_r13:		.quad	0
r_r14:		.quad	0
r_r15:		.quad	0

	.globl acpi_wakeup_cr0
acpi_wakeup_cr0:
r_cr0:		.quad	0
r_cr2:		.quad	0
	.globl acpi_wakeup_cr3
acpi_wakeup_cr3:
r_cr3:		.quad	0
r_cr4:		.quad	0
r_cr8:		.quad	0

	.globl acpi_wakeup_gdt
acpi_wakeup_gdt:
r_gdt:		.word	0
		.quad	0
r_idt:		.word	0
		.quad	0

	.globl acpi_wakeup_ret_addr
acpi_wakeup_ret_addr:
ret_addr:	.quad	0
