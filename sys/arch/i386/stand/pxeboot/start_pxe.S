/*	$NetBSD: start_pxe.S,v 1.4.94.2 2011/03/28 23:04:47 jym Exp $	*/

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
#include <sys/bootblock.h>

	.text
ENTRY(start)
	.code16
	/* Boot parameter area in same format as boot and bootxx */
	jmp	1f
	.balign 4
	.long	X86_BOOT_MAGIC_PXE
	.globl	_C_LABEL(boot_params)
_C_LABEL(boot_params):
	.long	1f - _C_LABEL(boot_params)
#include <boot_params.S>
	.space	4 * 4			/* some spare */
1:
	# start is loaded at 0x0:0x7c00 but we want 0x7c0:0x0
	# ljmp to the next instruction to adjust %cs
	ljmp	$0x7c0, $2f
2:
	# set up %ds
	mov	%cs, %ax
	mov	%ax, %ds

	# set up %ss and %sp
	movl	$_end, %eax		/* top of bss */
	shrl	$4, %eax		/* as a segment */
	addw	$0x1001, %ax		/* and + 64k */
	movw	%ax, %ss		/* for stack */
	movw	$0xfffc, %sp		/* %sp at top of it */

	call	gdt_fixup

	/* change to protected mode */
	calll	_C_LABEL(real_to_prot)
	.code32

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

ENTRY(_rtt)
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
10:
	cli
	hlt
	jmp	10b
#endif

efail:	.asciz		"Boot fail\r\n"
