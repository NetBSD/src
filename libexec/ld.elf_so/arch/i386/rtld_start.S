/*	$NetBSD: rtld_start.S,v 1.2 1999/02/12 17:15:52 christos Exp $	*/

/*-
 * Copyright (c) 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Christos Zoulas.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
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

#include <machine/asm.h>

	.text
	.align	4
	.globl	.rtld_start
	.type	.rtld_start@function
.rtld_start:
	subl	$8,%esp			# make room of obj_main and exit proc
	movl	%esp,%eax		# save stack pointer for _rtld
	pushl	%ebx			# save ps_strings
	movl	%esp,%ebx		# cleanup
	movl	%esp,%ecx		# obj_main
	addl	$4,%ecx

	pushl	%ecx
	pushl	%ebx
	pushl	%eax
	call	_rtld@PLT		# _rtld(sp, exitproc, obj_main)
	addl	$12,%esp		# pop args

	popl	%ebx			# %ebx = ps_strings
	popl	%edx			# %edx = cleanup
	popl	%ecx			# %ecx = obj_main
	jmp	*%eax

	.align	4
	.globl	_rtld_bind_start
	.type	_rtld_bind_start,@function
_rtld_bind_start:	# (obj, reloff)
	pushf				# save registers
	pushl	%eax
	pushl	%ecx
	pushl	%edx
	pushl	%ebx
	pushl	%ebp
	pushl	%esi
	pushl	%edi
	pushl	%ds
	pushl	%es

	pushl	44(%esp)		# Copy of reloff
	pushl	44(%esp)		# Copy of obj
	call	_rtld_bind@PLT		# Call the binder
	addl	$8,%esp			# pop binder args
	movl	%eax,44(%esp)		# Store function to be called in obj

	popl	%es			# restore registers
	popl	%ds
	popl	%edi
	popl	%esi
	popl	%ebp
	popl	%ebx
	popl	%edx
	popl	%ecx
	popl	%eax
	popf

	leal	4(%esp),%esp		# Discard reloff, do not change eflags
	ret
