/*	$NetBSD: acpi_wakecode.S,v 1.1.2.5 2007/09/10 20:38:59 joerg Exp $	*/

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
1:

	/* Disable beep again if machdep.acpi_beep_on_reset=1 */
	cmpb	$1,WAKEUP_beep_on_reset
	jne	1f
	inb	$0x61,%al
	andb	$0xfc,%al
	outb	%al,$0x61
1:

	/* Get physical address of the code */
	xorl	%esi,%esi
	movw	%cs,%si
	shll	$4,%esi

	/* Prepare ljmpl into 32bit mode */
	movl	%esi,%eax
	addl	$wakeup_32,%eax
	movl	%eax,wakeup_sw32+2

	/* Flush prefetch queue
	jmp	1f
1:	jmp	1f
1:

	/* Prepare ljmpl into 64bit mode */
	addl	%esi,wakeup_64_ptr
	/* Prepare GDT64 */
	movl	%esi,%eax
	addl	$tmp_gdtable64,%eax
	movl	%eax,tmp_gdt64+2
	/* Prepare GDT */
	movl	%esi,%eax
	addl	$tmp_gdtable,%eax
	movl	%eax,tmp_gdt+2
	/* Load 32bit GDT */
	data32 addr32 lgdt	tmp_gdt

	/* Enable protected mode */
	mov	%cr0,%eax
	orl	$(CR0_PE),%eax
	mov	%eax,%cr0


wakeup_sw32:
	/* Switch to protected mode by intersegmental jump */
	ljmpl	$0x8,$0x12345678	/* Code location, to be replaced */


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

	movl	%esi,%esp
	addl	4096,%esp

	/* First, reset the PSL. */
	pushl	$PSL_MBO
	popfl

	movl	%cr4,%eax
	orl	$(CR4_PAE|CR4_OSFXSR|CR4_OSXMMEXCPT),%eax
	movl	%eax,%cr4

	movl	$MSR_EFER,%ecx
	rdmsr
	xorl	%eax,%eax
	orl	$(EFER_LME|EFER_SCE),%eax
	wrmsr

	movl	%esi,%edx
	addl	$wakeup_64_ptr,%edx
	movl	%esi,%ecx
	addl	$tmp_gdt64,%ecx
	movl	WAKEUP_r_cr3(%esi),%eax
	movl	%eax,%cr3

	movl	WAKEUP_r_cr0(%esi),%eax
        movl	%eax,%cr0
	jmp	1f
1:
	lgdt	(%ecx)

	ljmp	*(%edx)

wakeup_64_ptr:
	.long   wakeup_64
	.word   GSEL(GCODE_SEL, SEL_KPL)

	.code64
wakeup_64:

	movw	$GSEL(GDATA_SEL, SEL_KPL),%ax 	#switch to new segment
	movw	%ax,%ds

	xorq	%rdi,%rdi
	movl	%esi,%edi

	movq	WAKEUP_restorecpu(%rdi),%rbx

	lgdt	WAKEUP_r_gdt(%rdi)

	jmp	*%rbx

	.align	8
tmp_gdt:
	.word	0xffff
	.long	0

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
	.long 0

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
