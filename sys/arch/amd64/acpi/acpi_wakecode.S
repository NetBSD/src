/* $NetBSD: acpi_wakecode.S,v 1.1.2.3 2007/09/07 21:01:03 jmcneill Exp $ */

/*-
 * Copyright (c) 2002, 2007 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Takuya SHIOZAKI, and by Andrew Doran.
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

#define WAKECODE_VERBOSE

#include <machine/asm.h>
#include <machine/specialreg.h>
#include <machine/param.h>
#include <machine/segments.h>
#include <machine/psl.h>

	.text
	.code16
	.org	0
	.globl wakeup_16
wakeup_16:
	nop
	cli
	cld

	/* Set up segment registers for real mode */
	movw	%cs, %ax
	movw	%ax, %ds
	movw	%ax, %ss

	/* Small call stack XXXAD Where is this? Is it in the ACPI spec? */
	mov	$0x1000, %sp

	/* Clear flags */
	pushl	$0
	popfl

	/* Only beep on reset if machdep.acpi_beep_on_reset=1 */
	cmpb	$1, beep_on_reset
	jne	nobeepon
	call	beepon

nobeepon:

	/* Only reset the VBIOS if machdep.acpi_vbios_reset=1 */
	cmpb	$1, vbios_reset
	jne	novbiosreset

	/* Kick the VBIOS. */
	lcall	$0xc000, $3

	/* Paranoid, restore segment registers. */
	movw	%cs, %ax
	movw	%ax, %ds
	movw	%ax, %ss

novbiosreset:

	/* Only beep on reset if machdep.acpi_beep_on_reset=1 */
	cmpb	$1, beep_on_reset
	jne	nobeepoff
	call	beepoff

nobeepoff:

#ifdef WAKECODE_VERBOSE
	movw	$0xb800, %ax
	movw	%ax, %fs
	movw	$0x0200 + '0', %fs:(0x00)
#endif

	/* Get physical address of the code */
	mov	%cs, %ax
	movzx	%ax, %esi
	shll	$4, %esi

	/* Fill 16->32 address */
	addl	%esi, wakeup_sw32+2	/* 1b opcode, 1b prefix */
	addl	%esi, wakeup_sw64+1	/* 1b opcode */
	addl	%esi, tmp_gdt+2
	addl	%esi, tmp_gdt64+2
	wbinvd					/* flush uop/trace cache */
	jmp	1f
1:	jmp	1f
1:


#ifdef WAKECODE_VERBOSE
	movw	$0xb800, %ax
	movw	%ax, %fs
	movw	$0x0200 + '1', %fs:(0x02)
#endif

	/* Load GDT, enable protected mode */
	lgdt	tmp_gdt

	/* Enable protected mode */
	movl	%eax, %cr0
	orb	$(CR0_PE), %al
	movl	%eax, %cr0

wakeup_sw32:

	/* Switch to protected mode by intersegmental jump */
	ljmpl	$GSEL(GCODE_SEL,SEL_KPL), $wakeup_32

	.align	16
	.code32
wakeup_32:

	nop
	movl	$GSEL(GDATA_SEL,SEL_KPL), %eax
	movw	%ax, %ds
	movw	%ax, %ss
	movw	%ax, %es
	movw	%ax, %fs
	movw	%ax, %gs

	movl	$0x1000, %esp	/* XXXJDM is this needed? */

#ifdef WAKECODE_VERBOSE
	movw	$0x0200 + '2', (0xb8004)
#endif

	pushl	$PSL_MBO
	popfl

	movl	%cr4, %eax
	orl	$(CR4_PAE|CR4_OSFXSR|CR4_OSXMMEXCPT), %eax
	movl	%eax, %cr4

	/* Set long mode enable in EFER and enable syscall extensions */
	movl	$MSR_EFER, %ecx
	rdmsr
	xorl	%eax, %eax
	orl	$(EFER_LME|EFER_SCE), %eax
	wrmsr

#ifdef WAKECODE_VERBOSE
	movw	$0x0200 + '3', (0xb8006)
#endif

	movl	tmp_pml4(%esi), %ecx
	movl	%ecx, %cr3

#ifdef WAKECODE_VERBOSE
	movw	$0x0200 + '4', (0xb8008)
#endif

	movl	%cr0, %eax
	orl	$(CR0_PE|CR0_PG|CR0_NE|CR0_TS|CR0_MP|CR0_WP), %eax
	movl	%eax, %cr0
	jmp	compat
compat:

	movl	$GSEL(GDATA_SEL,SEL_KPL), %eax
	movl	%eax, %ds
	movl	%eax, %es
	movl	%eax, %ss

	movw	$0x0200 + '5', (0xb800a)

	/* Load 64-bit GDT. */
	lgdt	tmp_gdt64(%esi)

	movw	$0x0200 + '6', (0xb800c)

wakeup_sw64:

	/* Jump to long mode segment. */
	ljmpl	$GSEL(GCODE_SEL,SEL_KPL), $wakeup_64

	.align	16
	.code64
wakeup_64:

	nop
	nop

	/* Re-initialize segment regsisters to be on the safe side. */
	movl	$GSEL(GDATA_SEL,SEL_KPL), %eax
	movl	%eax, %ds
	movl	%eax, %es
	movl	%eax, %gs
	movl	%eax, %ss
	movl	%eax, %fs

#ifdef WAKECODE_VERBOSE
	movw	$0x0200 + '7', (0xb800e)
#endif

	movq	previous_rsp(%rsi), %rsp

#ifdef WAKECODE_VERBOSE
	movw	$0x0200 + '8', (0xb8010)
#endif

#if 1	/* XXXAD ?? */
	/* Fixup TSS type field; 386 busy TSS (11) -> 386 available TSS (9) */
#define TSS_TYPEFIX_MASK	0xf9
	movq	physical_gdt+2(%rsi), %rbx
	movzxw	previous_tr(%rsi), %rcx
	leaq	(%rbx,%rcx),%rax	/* get TSS segment descriptor */
	andb	$TSS_TYPEFIX_MASK, 5(%rax)
#endif

#ifdef WAKECODE_VERBOSE
	movw	$0x0200 + '9', (0xb8012)
#endif

        /* Restore registers */
	lgdt	previous_gdt(%rsi)
	lidt	previous_idt(%rsi)
        lldt	previous_ldt(%rsi)
#if 0
        ltr     previous_tr(%rsi)
#endif

#ifdef WAKECODE_VERBOSE
	movw	$0x0200 + 'a', (0xb8014)
#endif

	movq	previous_cr2(%rsi), %rax
	movq	%rax, %cr2
	movq	previous_cr3(%rsi), %rdx
	movq	previous_cr4(%rsi), %rax
	movq	%rax, %cr4

#ifdef WAKECODE_VERBOSE
	movw	$0x0200 + 'b', (0xb8016)
#endif

        movw	previous_es(%rsi), %ax
        movw	%ax, %es
        movw	previous_fs(%rsi), %ax
        movw	%ax, %fs
        movw	previous_gs(%rsi), %ax
        movw	%ax, %gs
        movw	previous_ss(%rsi), %ax
        movw	%ax, %ss
        movq	where_to_recover(%rsi), %rbx
        movw	previous_ds(%rsi), %ax

#ifdef WAKECODE_VERBOSE
	movw	$0x0200 + 'c', (0xb8018)
#endif

	/* Re-enable paging, %rdx loaded above */
	movq	%rdx, %cr3
	movq	previous_cr0(%rsi), %rcx
	movq	%rcx, %cr0
	jmp	1f
1:	jmp	1f
1:

	/* New address space active - reload %ds */
        movw	%ax, %ds
        jmp	*%rbx

beepon:
	movb	$0xc0, %al
	outb	%al, $0x42
	movb	$0x04, %al
	outb	%al, $0x42
	inb	$0x61, %al
	orb	$0x3, %al
	outb	%al, $0x61
	ret

beepoff:
	inb	$0x61, %al
	andb	$0xfc, %al
	outb	%al, $0x61
	ret

tmp_gdt:
	.word	0xffff
	.long	tmp_gdtable

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
	.word	0xffff
	.long	tmp_gdtable64

tmp_gdtable64:
	/* null */
	.word	0, 0
	.byte	0, 0, 0, 0
	/* code */
	.word	0xffff, 0
	.byte	0, 0x9a, 0xaf, 0
	/* data */
	.word	0xffff, 0
	.byte	0, 0x92, 0xcf, 0
	
	.align	16, 0
	.global	WAKEUP_physical_gdt
WAKEUP_physical_gdt:
physical_gdt:		.word 0
			.quad 0
	.global WAKEUP_previous_cr2
WAKEUP_previous_cr2:
previous_cr2:		.quad 0
	.global	WAKEUP_previous_cr3
WAKEUP_previous_cr3:
previous_cr3:		.quad 0
	.global	WAKEUP_previous_cr4
WAKEUP_previous_cr4:
previous_cr4:		.quad 0
	.global	WAKEUP_previous_cr0
WAKEUP_previous_cr0:
previous_cr0:		.quad 0
	.global	WAKEUP_previous_tr
WAKEUP_previous_tr:
previous_tr:		.word 0
	.global	WAKEUP_previous_gdt
WAKEUP_previous_gdt:
previous_gdt:		.word 0
			.quad 0
	.global	WAKEUP_previous_ldt
WAKEUP_previous_ldt:
previous_ldt:		.word 0
	.global WAKEUP_previous_idt
WAKEUP_previous_idt:
previous_idt:		.word 0
			.quad 0
	.global WAKEUP_previous_ds
WAKEUP_previous_ds:
previous_ds:		.word 0
	.global	WAKEUP_previous_es
WAKEUP_previous_es:
previous_es:		.word 0
	.global	WAKEUP_previous_fs
WAKEUP_previous_fs:
previous_fs:		.word 0
	.global	WAKEUP_previous_gs
WAKEUP_previous_gs:
previous_gs:		.word 0
	.global	WAKEUP_previous_ss
WAKEUP_previous_ss:
previous_ss:		.word 0
	.global	WAKEUP_previous_rsp
WAKEUP_previous_rsp:
previous_rsp:		.quad 0
	.global WAKEUP_where_to_recover
WAKEUP_where_to_recover:
where_to_recover:	.quad 0
	.global WAKEUP_tmp_pml4
WAKEUP_tmp_pml4:
tmp_pml4:		.quad 0
	.global WAKEUP_vbios_reset
WAKEUP_vbios_reset:
vbios_reset:		.byte 0
	.global	WAKEUP_beep_on_reset
WAKEUP_beep_on_reset:
beep_on_reset:		.byte 0
