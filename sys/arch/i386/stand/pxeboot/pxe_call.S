/*	$NetBSD: pxe_call.S,v 1.3 2003/02/01 14:48:19 dsl Exp $	*/

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
 * Low level PXE BIOS call glue.
 */

#include <machine/asm.h>

ENTRY(pxecall_bangpxe)
	pushl	%ebp
	movl	%esp, %ebp
	pushl	%ebx
	pushl	%ecx
	pushl	%edx

	/* For simplicity, just move all 32 bits. */
	movl	8(%ebp), %ebx

	pushw	_C_LABEL(pxe_command_buf_seg)
	pushw	_C_LABEL(pxe_command_buf_off)
	pushw	%bx

	call	_C_LABEL(prot_to_real)		# enter real mode
	.code16

	/* The encoding is: 0x9a offlo offhi seglo seghi */
	lcall	$0, $0xffff
	.globl	_C_LABEL(bangpxe_off)
_C_LABEL(bangpxe_off) = . - 4
	.globl	_C_LABEL(bangpxe_seg)
_C_LABEL(bangpxe_seg) = . - 2

	calll	_C_LABEL(real_to_prot)		# leave real mode
	.code32

	add	$6, %esp

	popl	%edx
	popl	%ecx
	popl	%ebx
	popl	%ebp
	ret

ENTRY(pxecall_pxenv)
	pushl	%ebp
	movl	%esp, %ebp
	pushl	%ebx
	pushl	%ecx
	pushl	%edx
	pushl	%edi

	/* For simplicity, just move all 32 bits. */
	movl	8(%ebp), %ebx

	call	_C_LABEL(prot_to_real)		# enter real mode
	.code16

	/* prot_to_real() has already set %es to BOOTSEG */
	lea	_C_LABEL(pxe_command_buf), %di

	/* The encoding is: 0x9a offlo offhi seglo seghi */
	lcall	$0, $0xffff
	.globl	_C_LABEL(pxenv_off)
_C_LABEL(pxenv_off) = . - 4
	.globl	_C_LABEL(pxenv_seg)
_C_LABEL(pxenv_seg) = . - 2

	calll	_C_LABEL(real_to_prot)		# leave real mode
	.code32

	popl	%edi
	popl	%edx
	popl	%ecx
	popl	%ebx
	popl	%ebp
	ret
