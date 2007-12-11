/*	$NetBSD: acpi_wakeup_low.S,v 1.3 2007/12/11 23:06:11 lukem Exp $	*/

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

#include <machine/asm.h>
__KERNEL_RCSID(0, "$NetBSD: acpi_wakeup_low.S,v 1.3 2007/12/11 23:06:11 lukem Exp $");

#include "assym.h"

	.text
	.p2align 2, 0x90
	.globl acpi_md_sleep_exit
acpi_md_sleep_exit:
	movw	r_es,%ax
	movw	%ax,%es
	movw	r_fs,%ax
	movw	%ax,%fs
	movw	r_gs,%ax
	movw	%ax,%gs
	movw	r_ss,%ax
	movw	%ax,%ss

	movl	r_cr2,%eax
	movl	%eax,%cr2
	movl	r_cr4,%eax
	movl	%eax,%cr4
	movl	r_cr3,%eax
	movl	%eax,%cr3

	jmp 1f
1:

	lidt	r_idt
	lldt	r_ldt

	movl	CPUVAR(GDT),%eax
	movzwl	r_tr,%edx
	andl	$~0x0200,4(%eax,%edx, 1)
	ltr	%dx

	movl	r_esp,%esp

	movl	r_ebx,%ebx
	movl	r_ecx,%ecx
	movl	r_edx,%edx
	movl	r_ebp,%ebp
	movl	r_esi,%esi
	movl	r_edi,%edi

	pushl	r_efl
	popfl

	movl	ret_addr,%eax
	movl	%eax,(%esp)
	xorl	%eax,%eax

	ret

	.p2align 2, 0x90
	.type acpi_md_sleep_prepare, @function
	.globl acpi_md_sleep_prepare
acpi_md_sleep_prepare:
	movw	%ds,r_ds
	movw	%es,r_es
	movw	%fs,r_fs
	movw	%gs,r_gs
	movw	%ss,r_ss

	movl	%ebx,r_ebx
	movl	%ecx,r_ecx
	movl	%edx,r_edx
	movl	%ebp,r_ebp
	movl	%esi,r_esi
	movl	%edi,r_edi

	movl	%cr0,%eax
	movl	%eax,r_cr0
	movl	%cr2,%eax
	movl	%eax,r_cr2
	movl	%cr3,%eax
	movl	%eax,r_cr3
	movl	%cr4,%eax
	movl	%eax,r_cr4

	pushfl
	popl	r_efl

	movl	%esp,r_esp

	sgdt	r_gdt
	sidt	r_idt
	sldt	r_ldt
	str	r_tr

	movl	(%esp),%eax
	movl	%eax,ret_addr
	movl	4(%esp),%eax
	pushl	%eax
	call	acpi_md_sleep_enter
	/* acpi_md_sleep_enter only returns on failure. */
	popl	%eax
	movl	$-1,%eax
	ret

	.data
	.align 16
	.globl acpi_wakeup_ds
acpi_wakeup_ds:
r_ds:		.word	0
r_es:		.word	0
r_fs:		.word	0
r_gs:		.word	0
r_ss:		.word	0
r_tr:		.word	0
r_ldt:		.word	0

r_ebx:		.long	0
r_ecx:		.long	0
r_edx:		.long	0
r_ebp:		.long	0
r_esi:		.long	0
r_edi:		.long	0
r_esp:		.long	0
r_efl:		.long	0

	.globl acpi_wakeup_cr0
acpi_wakeup_cr0:
r_cr0:		.long	0
r_cr2:		.long	0
r_cr3:		.long	0
	.globl acpi_wakeup_cr4
acpi_wakeup_cr4:
r_cr4:		.long	0

	.globl acpi_wakeup_gdt
acpi_wakeup_gdt:
r_gdt:		.word	0
		.long	0
r_idt:		.word	0
		.long	0

ret_addr:	.long	0
