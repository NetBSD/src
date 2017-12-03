/*	$NetBSD: start.S,v 1.2.8.2 2017/12/03 11:36:19 jdolecek Exp $	*/

/*-
 * Copyright (c) 2008-2010 Rui Paulo <rpaulo@FreeBSD.org>
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
 *	$FreeBSD: head/sys/boot/efi/loader/arch/i386/start.S 282728 2015-05-10 13:30:21Z ian $
 */

#include <machine/asm.h>
#include <sys/bootblock.h>

	.text
	.align	16

/*
 * EFI entry point.
 * _start(EFI_IMAGE image_handle, EFI_SYSTEM_TABLE *system_table);
 *
 * We calculate the base address along with _DYNAMIC, relocate us and finally
 * pass control to efi_main.
 */

ENTRY(_start)
	pushl	%ebp
	movl	%esp, %ebp

	pushl	12(%ebp)	/* image_handle */
	pushl	8(%ebp)		/* system_table */
	call	0f
0:	popl	%eax
	movl	%eax, %ebx
	addl	$ImageBase-0b, %eax
	addl	$_DYNAMIC-0b, %ebx
	pushl	%ebx		/* dynamic */
	pushl	%eax		/* ImageBase */
	call	_C_LABEL(self_reloc)
	popl	%ebx		/* remove ImageBase from the stack */
	popl	%ebx		/* remove dynamic from the stack */
	call	_C_LABEL(efi_main)
1:	leave
	ret
END(_start)

	/*
	 * hand-craft a dummy .reloc section so EFI knows it's a relocatable
	 * executable:
	 */

	.data
	.section .reloc, "a"
	.long   0
	.long   10
	.word   0

	/* boot parameters */
	.text
	.code16
	.align 512
.Lfake_bootxx:
	jmp	1f
	.balign 4
	.long	X86_BOOT_MAGIC_EFI	/* checked by installboot */
	.globl _C_LABEL(boot_params)
_C_LABEL(boot_params):			/* space for patchable variables */
	.long	1f - boot_params	/* length of this data area */
#include <boot_params.S>
	. = .Lfake_bootxx + 0x80	/* Space for patching unknown params */
1:
