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

#include	"asm.h"

/* Conventional GDT indexes. */
#define BOOT_CS_INDEX		3
#define BOOT_CS16_INDEX		5
#define BOOT_DS_INDEX		4

/* Vector numbers. */
#define BREAKPOINT_VECTOR	3
#define DEBUG_VECTOR		1

/*
 * boot2() -- second stage boot
 */

.globl EXT(ouraddr)

ENTRY(boot2)
	data32
	subl	%eax, %eax
	movl	%cs, %ax
	movl	%ax, %ds
	movl	%ax, %es
	data32
	shll	$4, %eax
	addr32
	data32
	movl	%eax, EXT(ouraddr)

	/* fix up GDT entries for bootstrap */
#define FIXUP(gdt_index) \
	addr32; \
	movl	%eax, EXT(Gdt)+(8*gdt_index)+2;	/* actually movw %ax */ \
	addr32; \
	movb	%bl, EXT(Gdt)+(8*gdt_index)+4

	data32
	shldl	$16, %eax, %ebx
	FIXUP(BOOT_CS_INDEX)
	FIXUP(BOOT_CS16_INDEX)
	FIXUP(BOOT_DS_INDEX)

	/* fix up GDT pointer */
	data32
	movl	%eax, %ecx
	data32
	addl	$ EXT(Gdt), %eax
	addr32
	data32
	movl	%eax, EXT(Gdtr)+2

	/* change to protected mode */
	data32
	call	EXT(real_to_prot)

	/* clear the bss */
	movl	$ EXT(edata), %edi	/* no EXT(_edata) - krufty ld */
	movl	$ EXT(end), %ecx	/* or EXT(_end) */
	subl	%edi, %ecx
	subb	%al, %al
	rep
	stosb

	movzbl	%dl, %edx	/* discard head (%dh) and random high bits */
	pushl	%edx
	call	EXT(boot)
oops:
	cli
	hlt

	.data
        .align 2
#if 0 /* XXX this would give losing "_ouraddr :".  Better declared in C */
EXT(ouraddr):
#else
_ouraddr:
#endif
        .long 0
