/*	$NetBSD: mbr.S,v 1.5 1999/04/30 15:29:44 christos Exp $	*/

/*
 * Copyright (C) 1998 Wolfgang Solfrank.
 * Copyright (C) 1998 TooLs GmbH.
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by TooLs GmbH.
 * 4. The name of TooLs GmbH may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY TOOLS GMBH ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL TOOLS GMBH BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * i386 master boot code
 */

#include <machine/asm.h>
#define	addr32	.byte 0x67
#define	data32	.byte 0x66

#define	BOOTADDR	0x7c00

/*
 * XXX should be in an include file
 */
/*
 * Register values used in the i386 instruction set
 */
#define AL      0
#define BL      3
#define CL      1
#define DL      2
#define AH      4
#define BH      7
#define CH      5
#define DH      6

#define AX      0
#define BX      3
#define CX      1
#define DX      2
#define SP      4
#define BP      5
#define SI      6
#define DI      7

#define SI_INDEX        4
#define DI_INDEX        5
#define BP_INDEX        6
#define BX_INDEX        7

#define cmpw_reg_iregoff(reg,ireg,off)	\
	.byte 0x39 ; .byte (0x40 + ireg) | (reg << 3) ; .byte off

#define movb_ireg0_reg(ireg,reg) .byte 0x8a ; .byte (reg << 3) | ireg

	.text
/*
 * Move ourselves out of the way first
 */
ENTRY(start)
	xorl	%eax, %eax
	movl	%ax, %ss
	data32
	movl	$BOOTADDR, %esp
	movl	%ax, %es
	movl	%ax, %ds
	movl	%esp, %esi
	data32
	movl	$_C_LABEL(start), %edi
	data32
	movl	$0x100, %ecx
	rep
	movsl
#ifdef	ASSEMBLE_16BIT
	jmp	0:1f
#else
	.byte	0xea
	.word	1f
	.word	0
#endif
1:
/*
 * Now look for the active partition
 */
	data32
	movl	$parttab, %edi
	xorl	%esi, %esi
	data32
	movl	$4, %ecx
1:
	addr32
	cmpb	$0x80,0(%edi)
	jnz	2f
	test	%esi, %esi
	jnz	invpart
	data32
	movl	%edi, %esi
2:
	add	$0x10, %edi
	loop	1b
	test	%esi, %esi
	jnz	boot
/*
 * No active partition, or more than one partition active
 */
invpart:
	data32
	movl	$msinvp, %esi
	jmp out

/*
 * Active partition pointed to by si.
 * Read the first sector.
 *
 * First:
 * - see what the total number of sectors is that we can address through the
 *   normal CHS interface
 * - if this partition needs more, determine whether we have int13-extensions.
 *
 * Why this extra check? Well, some BIOSes apparently report that they do
 * int13 extensions, but fail if you actually use them (oh, great). So we
 * try to avoid them unless really necessary.
 */
boot:
	movb_ireg0_reg(SI_INDEX,DL)
	data32
	xorl	%ecx,%ecx
	data32
	xorl	%eax,%eax
	movb	$8,%ah
	int	$0x13

	shrl	$8,%edx
	incl	%edx			/* dx now has #heads */

	movl	%ecx,%eax
	andl	$0x3f,%eax		/* ax has #sectors */

	movl	%ecx,%ebx
	shrl	$8,%ecx
	andl	$0xc0,%ebx
	shll	$2,%ebx
	orl	%ebx,%ecx
	incl	%ecx			/* cx has #cylinders */

	mull	%edx			/* h * s (will fit in 16 bits) */
	data32
	mull	%ecx			/* %eax = c * h * s */

	movb_ireg0_reg(SI_INDEX,DL)
	data32
	addr32
	movl	8(%esi), %ebx
	data32
	cmpl	%eax, %ebx
	jl	noext			/* total # chs sectors > part. start */

tryext:
	data32
	push	%esi
	push	%edx
	data32
	movl	$0x55aa, %ebx
	movb	$0x41, %ah
	int	$0x13
	pop	%edx
	data32
	pop	%esi
	jc	noext
	data32
	movl	$0xaa55,%eax
	cmpl	%eax, %ebx
	jnz	noext
	testb	$1, %cl
	jz	noext

/*
 * Modify the partition table entry to look like an int13-extension
 * parameter block.
 */
	addr32
	data32
	movl	$0x10010, 0(%esi)
	addr32
	data32
	movl	$BOOTADDR, 4(%esi)
	addr32
	data32
	movl	$0, 12(%esi)
	movb	$5, %dh
1:
	push	%esi
	push	%edx
	movb	$0x42, %ah
	int	$0x13
	pop	%edx
	pop	%esi
	jnc	ok
	dec	%dh
	jnz	1b
rderr:
	data32
	movl	$readerr, %esi
out:
	lodsb
	testb	%al, %al
	jz	1f
	movb	$0xe, %ah
	movb	$7, %bl
	push	%esi
	int	$0x10
	pop	%esi
	jmp	out
1:
	sti
	jmp	1b

/*
 * No int13-extensions available, try old method.
 */
noext:
	addr32
	movb	1(%esi), %dh
	addr32
	movl	2(%esi), %ecx
	data32
	movl	$5, %edi
1:
	data32
	movl	$BOOTADDR, %ebx
	data32
	movl	$0x201, %eax
	push	%edi
	push	%edx
	push	%ecx
	int	$0x13
	pop	%ecx
	pop	%edx
	pop	%edi
	jnc	ok
	dec	%edi
	jnz	1b
	jmp	rderr

/*
 * Check signature for valid bootcode
 */
ok:
	data32
	mov	$noos, %esi
	addr32
	movl	BOOTADDR+0x1fe, %eax
	data32
	movl	$0xaa55, %ebx
	cmpl	%eax, %ebx
	jnz	out
#ifdef	ASSEMBLE_16BIT
	jmp	0:BOOTADDR
#else
	.byte	0xea
	.word	BOOTADDR
	.word	0
#endif

msinvp:
	.asciz	"Invalid partition table"
noos:
	.asciz	"No operating system"
readerr:
	.asciz	"Error loading operating system"
	. = _C_LABEL(start) + 0x1be
parttab:
	. = _C_LABEL(start) + 0x1fe
	.byte	0x55, 0xaa
