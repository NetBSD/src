/*	$NetBSD: acpi_wakecode.S,v 1.6 2006/06/15 18:07:58 jmcneill Exp $	*/

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

#include <machine/asm.h>
#include <machine/specialreg.h>
#include <machine/param.h>
#include <machine/segments.h>

	.code16
	.org 0	/* ACPI spec says: cs==(phys>>8), ip==(phys&0x000F) */
wakeup_16:
	nop
	cli
	cld

	/* Set up segment registers for real mode */
	movw	%cs,%ax
	movw	%ax,%ds
	movw	%ax,%ss

	/* The following needs to be configurable */
#ifdef notyet
	/* Kick the VBIOS. */
	movw	$0x1000, %sp /* Small call stack. */
	lcall	$0xc000,$3

	movw	%cs,%ax
	movw	%ax,%ds
	movw	%ax,%ss
#endif

	/* Get physical address of the code */
	xorl	%esi,%esi
	movw	%cs,%si
	shll	$4,%esi

	/* Fill 16->32 address */
	movl	%esi,%eax
	addl	$wakeup_32,%eax
	movl	%eax,wakeup_sw32+2
	jmp	1f			/* flush prefetch queue */
1:	jmp	1f
1:

	/* Load GDT while non-paging */
	movl	%esi,%eax
	addl	$tmp_gdtable,%eax
	movl	%eax,tmp_gdt+2
	lgdt	tmp_gdt

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
	movw	%ax,%es
	movw	%ax,%gs
	movw	%ax,%ss
	movw	%ax,%fs

	/* Fixup TSS type field; 386 busy TSS (11) -> 386 available TSS (9) */
#define TSS_TYPEFIX_MASK	0xf9
	movl	physical_gdt+2(%esi),%ebx
	movzxw	previous_tr(%esi),%ecx
	leal	(%ebx,%ecx),%eax	/* get TSS segment descriptor */
	andb	$TSS_TYPEFIX_MASK,5(%eax)

	/* Enable paging (assumes identical mapping) */
        movl	previous_cr3(%esi),%eax
	movl	%eax,%cr3
	movl	previous_cr0(%esi),%eax
	movl	%eax,%cr0

	/* Flush the prefetch queue */
	jmp	1f
1:	jmp	1f
1:

        /* Restore registers */
	lgdt	previous_gdt(%esi)
	lidt	previous_idt(%esi)
        lldt	previous_ldt(%esi)
#if 0
        ltr     previous_tr(%esi)
#endif

	mov	previous_cr2(%esi),%eax
	mov	%eax,%cr2
	mov	previous_cr4(%esi),%eax
	mov	%eax,%cr4

        movw	previous_es(%esi),%ax
        movw	%ax,%es
        movw	previous_fs(%esi),%ax
        movw	%ax,%fs
        movw	previous_gs(%esi),%ax
        movw	%ax,%gs
        movw	previous_ss(%esi),%ax
        movw	%ax,%ss
        movl	where_to_recover(%esi),%ebx
        movw	previous_ds(%esi),%ax
        movw	%ax,%ds
        jmp	*%ebx


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

	.align	16, 0
physical_gdt:		.word 0
			.long 0
previous_cr2:		.long 0
previous_cr3:		.long 0
previous_cr4:		.long 0
previous_cr0:		.long 0
previous_tr:		.word 0
previous_gdt:		.word 0
			.long 0
previous_ldt:		.word 0
previous_idt:		.word 0
			.long 0
previous_ds:		.word 0
previous_es:		.word 0
previous_fs:		.word 0
previous_gs:		.word 0
previous_ss:		.word 0
where_to_recover:	.long 0
