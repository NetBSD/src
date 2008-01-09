/* $NetBSD: pnpbioscall.S,v 1.1.86.1 2008/01/09 01:46:47 matt Exp $ */
/*
 * Copyright (c) 1999
 * 	Matthias Drochner.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions, and the following disclaimer.
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
 */

#include <machine/asm.h>
__KERNEL_RCSID(0, "$NetBSD: pnpbioscall.S,v 1.1.86.1 2008/01/09 01:46:47 matt Exp $");

#include "assym.h"
#include <machine/segments.h>

	.data
saveesp:
	.space	4

NENTRY(pnpbioscall)
	pushl	%ebp
	movl	%esp, %ebp
	/*
	 * disable interrupts - our handlers can't deal with frames of the
	 * segment we are calling, and the BIOS can't use our IDT anyway
	 */
	pushfl
	cli

	/* call pnpbiostramp() */
	lcall	$GSEL(GPNPBIOSTRAMP_SEL, SEL_KPL), $0

	popfl
	popl	%ebp
	ret

NENTRY(pnpbiostramp)
	pushl	%ebx

	/* save current stack pointer */
	movl	%esp, %eax
	movl	%eax, saveesp

	/* switch to new stack */
	movl	8(%ebp), %esp /* call argument */
	movl	$GSEL(GPNPBIOSSCRATCH_SEL, SEL_KPL), %eax
	mov	%ax, %ss

	/* fake the call return address (16-bit frame) */
	mov	%cs, %ax
	push	%ax
	movl	$(1f - _C_LABEL(pnpbiostramp)), %eax
	push	%ax

	/* do a long jump */
	pushl	$GSEL(GPNPBIOSCODE_SEL, SEL_KPL)
	pushl	(_C_LABEL(pnpbios_entry))
#if 1
	/* paranoia: don't pass our ds */
	movl	$GSEL(GPNPBIOSSCRATCH_SEL, SEL_KPL), %eax
	mov	%ax, %ds
#endif
	lret

1:	/* returned from BIOS call */
	movl	$GSEL(GDATA_SEL, SEL_KPL), %ebx
#if 1
	mov	%bx, %ds
#endif
	/* back to old stack */
	mov	%bx, %ss
	movl	saveesp, %ebx
	movl	%ebx, %esp

	popl	%ebx
	lret
	.globl _C_LABEL(epnpbiostramp)
_C_LABEL(epnpbiostramp):
