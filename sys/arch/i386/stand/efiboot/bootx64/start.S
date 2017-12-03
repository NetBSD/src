/*	$NetBSD: start.S,v 1.2.8.2 2017/12/03 11:36:19 jdolecek Exp $	*/

/*-
 * Copyright (C) 1999 Hewlett-Packard Co.
 *     Contributed by David Mosberger <davidm@hpl.hp.com>.
 * Copyright (C) 2005 Intel Co.
 *     Contributed by Fenghua Yu <fenghua.yu@intel.com>.
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
 * 3. Neither the name of Hewlett-Packard Co. nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
 * TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
 * THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * crt0-efi-x86_64.S - x86_64 EFI startup code.
 * $FreeBSD: head/sys/boot/efi/loader/arch/amd64/start.S 282727 2015-05-10 13:24:26Z ian $
 */

#include <machine/asm.h>
#include <sys/bootblock.h>

	.text
	.align 16
	.globl _start
_start:
	subq	$8, %rsp
	pushq	%rcx
	pushq	%rdx

0:
	lea	ImageBase(%rip), %rdi
	lea	_DYNAMIC(%rip), %rsi

	popq	%rcx
	popq	%rdx
	pushq	%rcx
	pushq	%rdx
	call	_C_LABEL(self_reloc)

	popq	%rdi
	popq	%rsi

	call	_C_LABEL(efi_main)
	addq	$8, %rsp

.Lexit:
	ret

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
