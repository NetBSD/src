/*	$NetBSD: acpi_wakeup_low.S,v 1.5.62.1 2016/10/05 20:55:28 skrll Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: acpi_wakeup_low.S,v 1.5.62.1 2016/10/05 20:55:28 skrll Exp $");

#include "assym.h"
#include <machine/segments.h>
#include <machine/specialreg.h>

	.text
	.p2align 2, 0x90
	.globl acpi_md_sleep_exit
acpi_md_sleep_exit:
	lgdt	ACPI_SUSPEND_GDT(%edx)

	/* Reload fixed descriptors for new GDT */
	movw	$GSEL(GDATA_SEL, SEL_KPL),%ax
	movw	%ax,%ds
	movw	%ax,%es
	movw	%ax,%ss

	movw	ACPI_SUSPEND_FS(%edx),%ax
	movw	%ax,%fs
	movw	ACPI_SUSPEND_GS(%edx),%ax
	movw	%ax,%gs

	movl	$MSR_EFER,%ecx
	movl	ACPI_SUSPEND_EFER(%edx),%eax
	movl	%edx,%esi
	movl	$0,%edx
	wrmsr
	movl	%esi,%edx

	movl	ACPI_SUSPEND_CR2(%edx),%eax
	movl	%eax,%cr2
	movl	ACPI_SUSPEND_CR4(%edx),%eax
	movl	%eax,%cr4
	movl	ACPI_SUSPEND_CR3(%edx),%eax
	movl	%eax,%cr3

	jmp	1f
1:

	lidt	ACPI_SUSPEND_IDT(%edx)
	lldt	ACPI_SUSPEND_LDT(%edx)

	movl	CPUVAR(GDT),%eax
	movzwl	ACPI_SUSPEND_TR(%edx),%ecx
	andl	$~0x0200,4(%eax,%ecx, 1)
	ltr	%cx

	movl	ACPI_SUSPEND_REG+(0*4)(%edx),%ebx
	movl	ACPI_SUSPEND_REG+(1*4)(%edx),%esi
	movl	ACPI_SUSPEND_REG+(2*4)(%edx),%edi
	movl	ACPI_SUSPEND_REG+(3*4)(%edx),%ebp
	movl	ACPI_SUSPEND_REG+(4*4)(%edx),%esp

	pushl	ACPI_SUSPEND_REG+(5*4)(%edx)
	popfl

	xorl	%eax,%eax

	ret

	.p2align 2, 0x90
	.type acpi_md_sleep_prepare, @function
	.globl acpi_md_sleep_prepare
acpi_md_sleep_prepare:
	movl	CPUVAR(SELF),%edx
	movw	%fs,ACPI_SUSPEND_FS(%edx)
	movw	%gs,ACPI_SUSPEND_GS(%edx)

	movl	%ebx,ACPI_SUSPEND_REG+(0*4)(%edx)
	movl	%esi,ACPI_SUSPEND_REG+(1*4)(%edx)
	movl	%edi,ACPI_SUSPEND_REG+(2*4)(%edx)
	movl	%ebp,ACPI_SUSPEND_REG+(3*4)(%edx)
	movl	%esp,ACPI_SUSPEND_REG+(4*4)(%edx)

	movl	$MSR_EFER,%ecx
	pushl	%edx
	rdmsr	/* overwrites %edx */
	popl	%edx
	movl	%eax,ACPI_SUSPEND_EFER(%edx)

	movl	%cr0,%eax
	movl	%eax,ACPI_SUSPEND_CR0(%edx)
	movl	%cr2,%eax
	movl	%eax,ACPI_SUSPEND_CR2(%edx)
	movl	%cr3,%eax
	movl	%eax,ACPI_SUSPEND_CR3(%edx)
	movl	%cr4,%eax
	movl	%eax,ACPI_SUSPEND_CR4(%edx)

	pushfl
	popl	ACPI_SUSPEND_REG+(5*4)(%edx)

	sgdt	ACPI_SUSPEND_GDT(%edx)
	sidt	ACPI_SUSPEND_IDT(%edx)
	sldt	ACPI_SUSPEND_LDT(%edx)
	str	ACPI_SUSPEND_TR(%edx)

	movl	4(%esp),%eax
	pushl	%eax
	call	acpi_md_sleep_enter
	/* acpi_md_sleep_enter only returns on failure. */
	popl	%eax
	movl	$-1,%eax
	ret
