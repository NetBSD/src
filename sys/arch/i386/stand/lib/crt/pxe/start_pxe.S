/*	$NetBSD: start_pxe.S,v 1.3 2003/02/01 14:48:19 dsl Exp $	*/

/*
 * Copyright 2001 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Jason R. Thorpe for Wasabi Systems, Inc.
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
 *	This product includes software developed for the NetBSD Project by
 *	Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
	
/*
 * PXE startup
 * parts from sys/arch/i386/stand/lib/crt/bootsect/start_bootsect.S
 *
 * See PXE SPEC 4.4.5 (pdf page 88)
 */

#include <machine/asm.h>

	.text
ENTRY(start)
	.code16
	# start is loaded at 0x0:0x7c00 but we want 0x7c0:0x0
	# ljmp to the next instruction to adjust %cs
	ljmp	$0x7c0, $start1

	# Our stack		XXX probably not!
	.space	1024

start1:
	# set up %ds
	xorl	%eax, %eax
	mov	%cs, %ax
	mov	%ax, %ds

	# set up %ss and %esp
	mov	%ax, %ss
	movl	$0xfffc, %esp		/* stack at top of 64k segment */

	movl	%eax, _C_LABEL(ourseg)
	shll	$4, %eax

	/* fix up GDT entries for bootstrap */
#define	FIXUP(gdt_index) \
	movw	%ax, gdt+gdt_index+2;	\
	movb	%bl, gdt+gdt_index+4

	shldl	$16, %eax, %ebx

	FIXUP(bootcodeseg)
	FIXUP(bootrealseg)
	FIXUP(bootdataseg)

	/* fix up GDT pointer */
	addl	$gdt, %eax
	movl	%eax, gdtarg+2

	/* change to protected mode */
	calll	_C_LABEL(real_to_prot)
	.code16

	/* clear bss */
	xorl	%eax, %eax
	movl	$_C_LABEL(edata), %edi
	movl	$_C_LABEL(end), %ecx
	subl	%edi, %ecx
	cld
	rep
	stosb

	/* ...and call main()! */
	call	_C_LABEL(main)

	.globl	_C_LABEL(exit)
_C_LABEL(exit):
	call	_C_LABEL(prot_to_real)
	.code16
	movw	$efail, %si
	call	message

#ifdef notyet
	/* sleep for 3s = 0x2dc6c0 us */
	movb	$0x86, %ah
	mov	$0x002d, %cx
	mov	$0xc6c0, %dx
	int	$0x15

	/* call ROM BASIC */
	int	$0x18
#else
	cli
	hlt
#endif

efail:	.asciz		"Boot fail\r\n"

ENTRY(ourseg)
	.long	0

/**************************************************************************
GLOBAL DESCRIPTOR TABLE
**************************************************************************/
#ifdef __ELF__
	.align	16
#else
	.align	4
#endif
gdt:
	.word	0, 0
	.byte	0, 0x00, 0x00, 0

#ifdef SUPPORT_LINUX	/* additional dummy */
	.word	0, 0
	.byte	0, 0x00, 0x00, 0
#endif

	/* kernel code segment */
	.globl flatcodeseg
flatcodeseg = . - gdt         
	.word	0xffff, 0
	.byte	0, 0x9f, 0xcf, 0

	/* kernel data segment */
	.globl flatdataseg
flatdataseg = . - gdt
	.word	0xffff, 0
	.byte	0, 0x93, 0xcf, 0

	/* boot code segment, base will be patched */
bootcodeseg = . - gdt
	.word	0xffff, 0
	.byte	0, 0x9e, 0x4f, 0

	/* boot data segment, base will be patched */
bootdataseg = . - gdt
	.word	0xffff, 0
	.byte	0, 0x92, 0x4f, 0

	/* 16 bit real mode, base will be patched */
bootrealseg = . - gdt
	.word	0xffff, 0
	.byte	0, 0x9e, 0x00, 0
gdtlen = . - gdt

#ifdef __ELF__
	.align	16
#else
	.align	4
#endif
gdtarg:
	.word	gdtlen-1		/* limit */
	.long	0			/* addr, will be inserted */

CR0_PE		=	0x1

/*
 * real_to_prot()
 *	transfer from real mode to protected mode.
 */
ENTRY(real_to_prot)
	.code16
	# guarantee that interrupt is disabled when in prot mode 
	cli
	pushl	%eax
	push	%cs			/* ensure %ds is correct */
	pop	%ds

	# load the gdtr
	lgdt	gdtarg

	# set the PE bit of CR0
	movl	%cr0, %eax

	orl	$CR0_PE, %eax
	movl	%eax, %cr0

	# make intrasegment jump to flush the processor pipeline and
	# reload CS register   
	ljmp	$bootcodeseg, $xprot

xprot:
	.code32
	# we are in USE32 mode now
	# set up the protected mode segment registers : DS, SS, ES
	mov	$bootdataseg, %ax
	mov	%ax, %ds
	mov	%ax, %ss
	mov	%ax, %es

	popl	%eax
	ret

/*
 * prot_to_real()
 *	transfer from protected mode to real mode
 */
ENTRY(prot_to_real)
	.code32
	pushl	%eax
	# set up a dummy stack frame for the second seg change.
	# Adjust the intersegment jump instruction following
	# the clearing of protected mode bit.
	# This is self-modifying code, but we need a writable
	# code segment, and an intersegment return does not give us that.

	movl	_C_LABEL(ourseg), %eax
	movw	%ax, xreal-2

	# Change to use16 mode.
	ljmp	 $bootrealseg, $x16

x16:
	.code16
	# clear the PE bit of CR0
	movl	%cr0, %eax     
	andl	$~CR0_PE, %eax
	movl	%eax, %cr0
	ljmp	$0, $xreal		/* segment overwritten above */

xreal:
	.code16
	# we are in real mode now
	# set up the real mode segment registers : DS, SS, ES
	mov	%cs, %ax
	mov	%ax, %ds
	mov	%ax, %ss
	mov	%ax, %es

	sti
	popl	%eax
	retl

/*
 * pbzero(dst, cnt)
 *	where dst is a physical address and cnt is the length
 */
ENTRY(pbzero)
	.code32
	pushl	%ebp
	movl	%esp, %ebp
	pushl	%es
	pushl	%edi

	cld

	# set %es to point at the flat segment
	movl	$flatdataseg, %eax
	mov	%ax, %es

	movl	8(%ebp), %edi		# destination
	movl	12(%ebp), %ecx		# count
	xorl	%eax, %eax		# value

	rep
	stosb

	popl	%edi
	popl	%es
	popl	%ebp
	ret

/*
 * vpbcopy(src, dst, cnt)
 *	where src is a virtual address and dst is a physical address
 */
ENTRY(vpbcopy)
	.code32
	pushl	%ebp
	movl	%esp, %ebp
	pushl	%es
	pushl	%esi
	pushl	%edi

	cld

	# set %es to point at the flat segment
	movl	$flatdataseg, %eax
	mov	%ax, %es

	movl	8(%ebp), %esi		# source
	movl	12(%ebp), %edi		# destination
	movl	16(%ebp), %ecx		# count

	rep
	movsb

	popl	%edi
	popl	%esi
	popl	%es
	popl	%ebp
	ret

/*
 * pvbcopy(src, dst, cnt)
 *	where src is a physical address and dst is a virtual address
 */
ENTRY(pvbcopy)
	.code32
	pushl	%ebp
	movl	%esp, %ebp
	pushl	%ds
	pushl	%esi
	pushl	%edi

	cld

	# set %ds to point at the flat segment
	movl	$flatdataseg, %eax
	mov	%ax, %ds

	movl	8(%ebp), %esi		# source
	movl	12(%ebp), %edi		# destination
	movl	16(%ebp), %ecx		# count

	rep
	movsb

	popl	%edi
	popl	%esi
	popl	%ds
	popl	%ebp
	ret

ENTRY(vtophys)
	movl	_C_LABEL(ourseg), %eax
	shll	$4, %eax
	addl	4(%esp), %eax
	ret

/*
 * message: write the error message in %ds:%esi to the console
 */
message:
	.code16
/*
 * BIOS call "INT 10H Function 0Eh" to write character to console
 *	Call with	%ah = 0x0e
 *			%al = character
 *			%bh = page
 *			%bl = foreground color
 */
	pushl	%eax
	pushl	%ebx
	pushl	%edx

nextb:
	cld
	lodsb			# load a byte into %al
	testb	%al, %al
	jz	done

	movb	$0x0e, %ah
	movw	$0x0001, %bx
	int	$0x10

	jmp	nextb

done:
	popl	%edx
	popl	%ebx
	popl	%eax
	ret
