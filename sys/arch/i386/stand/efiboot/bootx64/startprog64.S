/*	$NetBSD: startprog64.S,v 1.2.2.2 2017/02/05 13:40:13 skrll Exp $	*/
/*	NetBSD: startprog.S,v 1.3 2003/02/01 14:48:18 dsl Exp	*/

/* starts program in protected mode / flat space
 with given stackframe
 needs global variables flatcodeseg and flatdataseg
 (gdt offsets)
  derived from: NetBSD:sys/arch/i386/boot/asm.S
 */

/*
 * Ported to boot 386BSD by Julian Elischer (julian@tfs.com) Sept 1992
 *
 * Mach Operating System
 * Copyright (c) 1992, 1991 Carnegie Mellon University
 * All Rights Reserved.
 *
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 *
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND FOR
 * ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 *
 * Carnegie Mellon requests users of this software to return to
 *
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 *
 * any improvements or extensions that they make and grant Carnegie Mellon
 * the rights to redistribute these changes.
 */

/*
  Copyright 1988, 1989, 1990, 1991, 1992
   by Intel Corporation, Santa Clara, California.

                All Rights Reserved

Permission to use, copy, modify, and distribute this software and
its documentation for any purpose and without fee is hereby
granted, provided that the above copyright notice appears in all
copies and that both the copyright notice and this permission notice
appear in supporting documentation, and that the name of Intel
not be used in advertising or publicity pertaining to distribution
of the software without specific, written prior permission.

INTEL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE
INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS,
IN NO EVENT SHALL INTEL BE LIABLE FOR ANY SPECIAL, INDIRECT, OR
CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
LOSS OF USE, DATA OR PROFITS, WHETHER IN ACTION OF CONTRACT,
NEGLIGENCE, OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION
WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/

#include <machine/asm.h>
#include <machine/specialreg.h>

#define	CODE_SEGMENT	0x08
#define	DATA_SEGMENT	0x10

	.align	16
	.globl _C_LABEL(startprog64)
_C_LABEL(startprog64):
	.quad 0

	.globl _C_LABEL(startprog64_size)
_C_LABEL(startprog64_size):
	.long startprog64_end - _C_LABEL(startprog64_start)

	.text
	.p2align 4,,15

/*
 * startprog64(loadddr,entry,stack)
 */
ENTRY(startprog64_start)
start:
	/*
	 * This function is to call the loaded kernel's start() with
	 * 32bit segment mode from x64 mode.
	 * %rdi: loaded start address
	 * %rsi: kernel entry address
	 * %rdx: stack address
	 */

	cld		# LynxOS depends on it

	/* Prepare jump address */
	lea	(start32a - start)(%rdi), %rax
	movl	%eax, (start32r - start)(%rdi)

	cli

	/* Setup GDT */
	lea	(gdt - start)(%rdi), %rax
	mov	%rax, (gdtrr - start)(%rdi)
	lgdt	(gdtr - start)(%rdi)

	/* Jump to set %cs */
	ljmp	*(start32r - start)(%rdi)

	.align	4
	.code32
start32a:
	movl	$DATA_SEGMENT, %eax
	movw	%eax, %ds
	movw	%eax, %es
	movw	%eax, %fs
	movw	%eax, %gs
	movw	%eax, %ss

	movl	%edx, %esp

	/* Disable Paging in CR0 */
	movl	%cr0, %eax
	andl	$(~CR0_PG), %eax
	movl	%eax, %cr0

	/* Disable PAE in CR4 */
	movl	%cr4, %eax
	andl	$(~CR4_PAE), %eax
	movl	%eax, %cr4

	jmp	start32b

	.align	4
start32b:
	xor	%eax, %eax
	call	*%esi

	.align	16
start32r:
	.long	0
	.long	CODE_SEGMENT
	.align	16
gdt:
	.long	0, 0
	.byte	0xff, 0xff, 0x00, 0x00, 0x00, 0x9f, 0xcf, 0x00
	.byte	0xff, 0xff, 0x00, 0x00, 0x00, 0x93, 0xcf, 0x00
gdtr:
	.word	gdtr - gdt
gdtrr:
	.quad
start32end:
	/* Space for the stack */
	.align	16
	.space	8192
startprog64_end:
