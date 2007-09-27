/*	$NetBSD: acpi_wakecode.S,v 1.1.2.9 2007/09/27 18:27:49 joerg Exp $	*/

/*-
 * Copyright (c) 2002 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Takuya SHIOZAKI.
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
 *	  This product includes software developed by the NetBSD
 *	  Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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


/*
 * This code is derived from FreeBSD.  Original copyrights:
 *
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
 *
 *	FreeBSD: src/sys/i386/acpica/acpi_wakecode.S,v 1.1 2001/07/20 06:07:31 takawata Exp
 */

#define _LOCORE

#include <machine/psl.h>
#include <machine/specialreg.h>
#include <machine/segments.h>

#define	ACPI_WAKEUP_ADDR	0x3000

	.text
	.code16
	.org 0	/* ACPI spec says: cs==(phys>>8), ip==(phys&0x000F) */
	.globl wakeup_16
wakeup_16:
	nop
	cli
	cld

	/* Set up segment registers for real mode */
	movw	%cs,%ax
	movw	%ax,%ds
	movw	%ax,%ss

	/* Small call stack */
	mov	$0x1000,%sp

	/* Clear flags */
	pushl	$0
	popfl

	/* Only beep on reset if machdep.acpi_beep_on_reset=1 */
	cmpb	$1,WAKEUP_beep_on_reset
	jne	1f
	movb	$0xc0,%al
	outb	%al,$0x42
	movb	$0x04,%al
	outb	%al,$0x42
	inb	$0x61,%al
	orb	$0x3,%al
	outb	%al,$0x61
1:

	/* Only reset the VBIOS if machdep.acpi_vbios_reset=1 */
	cmpb	$1,WAKEUP_vbios_reset
	jne	1f

	/* Kick the VBIOS. */
	lcall	$0xc000,$3

	/* Reset registers in case the VBIOS changed them. */
	movw	%cs,%ax
	movw	%ax,%ds
	movw	%ax,%ss
1:

	/* Disable beep again if machdep.acpi_beep_on_reset=1 */
	cmpb	$1,WAKEUP_beep_on_reset
	jne	1f
	inb	$0x61,%al
	andb	$0xfc,%al
	outb	%al,$0x61
1:

	/* Load temporary 32bit GDT */
	data32 addr32 lgdt	tmp_gdt

	/* Enable protected mode w/o paging */
	mov	%cr0,%eax
	orl	$(CR0_PE),%eax
	mov	%eax,%cr0

wakeup_sw32:
	/*
	 * Switch to protected mode by intersegmental jump.
	 * Target and everything else has to compensate for the new origin
	 * as this is using the flat memory model now.
	 */

	ljmpl	$0x8,$wakeup_32 + ACPI_WAKEUP_ADDR

	.code32
	.align	16
wakeup_32:
	/*
	 * Switched to protected mode w/o paging
	 */

	nop
	/* Set up segment registers and initial stack for protected mode */
	movw	$0x10, %ax
	movw	%ax,%ds
	movw	%ax,%ss

	movl	$(ACPI_WAKEUP_ADDR + 4096),%esp

	/* First, reset the PSL. */
	pushl	$PSL_MBO
	popfl

	/* Enable PAE */
	movl	%cr4,%eax
	orl	$(CR4_PAE),%eax
	movl	%eax,%cr4

	/* Enable SYSCALL extension and Long Mode */
	movl	$MSR_EFER,%ecx
	rdmsr
	xorl	%eax,%eax
	orl	$(EFER_LME|EFER_SCE),%eax
	wrmsr

	/* Load temporary PML4, code will switch to full PML4 later */
	movl	WAKEUP_r_cr3 + ACPI_WAKEUP_ADDR,%eax
	movl	%eax,%cr3

	/* Reload cr0 with normal values, activating paging effectively */
	movl	WAKEUP_r_cr0 + ACPI_WAKEUP_ADDR,%eax
        movl	%eax,%cr0
	/* Flush prefetch queue */
	jmp	1f
1:
	/* Switch to temporary 64bit GDT */
	lgdt	tmp_gdt64 + ACPI_WAKEUP_ADDR

	/* Switch to long mode using intersegmental jump. */
	ljmp	$0x8, $wakeup_64 + ACPI_WAKEUP_ADDR

	.code64
wakeup_64:

	/* Reload normal segment registers with correct values */
	movw	$GSEL(GDATA_SEL, SEL_KPL),%ax
	movw	%ax,%ds
	movw	%ax,%es
	movw	%ax,%ss

	movq	WAKEUP_restorecpu + ACPI_WAKEUP_ADDR,%rbx

	/* Switch to the normal kernel GDT */
	lgdt	WAKEUP_r_gdt + ACPI_WAKEUP_ADDR

	/* Continue with wakeup in the high-level wakeup code */
	jmp	*%rbx

	.align	8
tmp_gdt:
	.word	0xffff
	.long	tmp_gdtable + ACPI_WAKEUP_ADDR

	.align	8, 0
tmp_gdtable:
	/* null */
	.word	0, 0
	.byte	0, 0, 0, 0
	/* code */
	.word	0xffff, 0
	.byte	0, 0x9f, 0xcf, 0
	/* data */
	.word	0xffff, 0
	.byte	0, 0x93, 0xcf, 0

tmp_gdt64:
	.word 0xffff
	.long tmp_gdtable64 + ACPI_WAKEUP_ADDR

tmp_gdtable64:
	.quad 0x0000000000000000
	.quad 0x00af9a000000ffff
	.quad 0x00cf92000000ffff

	.align	16, 0
	.global WAKEUP_r_cr0
WAKEUP_r_cr0:		.quad 0
	.global WAKEUP_r_cr3
WAKEUP_r_cr3:		.quad 0

	.global WAKEUP_r_gdt
WAKEUP_r_gdt:		.word 0
			.quad 0
	.global WAKEUP_restorecpu
WAKEUP_restorecpu:	.quad 0

	.global WAKEUP_vbios_reset
WAKEUP_vbios_reset:	.byte 0
	.global WAKEUP_beep_on_reset
WAKEUP_beep_on_reset:	.byte 0
