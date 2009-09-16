/*	$NetBSD: acpi_wakecode.S,v 1.11.10.2 2009/09/16 13:37:39 yamt Exp $	*/

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

	movw	%cs,%ax
	movw	%ax,%ds
	movw	%ax,%ss

	/* If we need to restore a VESA VBE mode, do it now */
	cmpb	$0,WAKEUP_vesa_modenum
	je	1f
	movw	WAKEUP_vesa_modenum,%bx
	orw	$0x4000,%bx
	movw	$0x4f02,%ax
	int	$0x10

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

	/* Load GDT while non-paging */
	lgdt	tmp_gdt

	/* Enable protected mode */
	mov	%cr0,%eax
	orl	$(CR0_PE),%eax
	mov	%eax,%cr0

	/* Switch to protected mode by intersegmental jump */
	ljmpl	$0x8,$wakeup_32	+ ACPI_WAKEUP_ADDR


	.code32
	.align	16
wakeup_32:
	/*
	 * Switched to protected mode w/o paging
	 */

	nop
	/* Set up segment registers for protected mode */
	movw	$GSEL(GDATA_SEL,SEL_KPL),%ax
	movw	%ax,%ds

	/* Restore PSE and other settings before enabling paging. */
	movl	WAKEUP_r_cr4 + ACPI_WAKEUP_ADDR,%eax
	movl	%eax,%cr4

	/* Enable paging (assumes identical mapping) */
	movl	WAKEUP_r_cr3 + ACPI_WAKEUP_ADDR,%eax
	movl	%eax,%cr3
	movl	%cr0,%eax
	orl	$(CR0_PE|CR0_PG|CR0_NE|CR0_TS|CR0_EM|CR0_MP|CR0_WP),%eax
	movl	%eax,%cr0

	/* Flush the prefetch queue */
	jmp	1f
1:	jmp	1f
1:

	nop

	/* Restore registers */
	movl	WAKEUP_curcpu + ACPI_WAKEUP_ADDR,%edx
	movl	WAKEUP_restorecpu + ACPI_WAKEUP_ADDR,%ebx

	jmp	*%ebx

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

	.align	16, 0
	.global WAKEUP_r_cr3
WAKEUP_r_cr3:		.long 0
	.global WAKEUP_r_cr4
WAKEUP_r_cr4:		.long 0

	.global WAKEUP_curcpu
WAKEUP_curcpu:		.long 0
	.global WAKEUP_restorecpu
WAKEUP_restorecpu:	.long 0

	.global WAKEUP_vbios_reset
WAKEUP_vbios_reset:	.byte 0
	.global WAKEUP_vesa_modenum
WAKEUP_vesa_modenum:	.word 0
	.global WAKEUP_beep_on_reset
WAKEUP_beep_on_reset:	.byte 0
