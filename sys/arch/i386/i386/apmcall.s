/*	$NetBSD: apmcall.s,v 1.5.4.1 1999/11/09 22:01:22 he Exp $ */

/*-
 * Copyright (c) 1997 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by John Kohl.
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

#include "assym.h"
#include <machine/asm.h>
#include <machine/apmvar.h>
#include <machine/bioscall.h>

/*
 * int apmcall(int function, struct bioscallregs *regs):
 * 	call the APM protected mode bios function FUNCTION for BIOS selection
 * 	WHICHBIOS.
 *	Fills in *regs with registers as returned by APM.
 *	returns nonzero if error returned by APM.
 */
	.data
	.globl  _C_LABEL(apm_disable_interrupts)
apmstatus:	.long 0
#ifndef APM_DISABLE_INTERRUPTS
#define APM_DISABLE_INTERRUPTS 1
#endif
_C_LABEL(apm_disable_interrupts):	.long APM_DISABLE_INTERRUPTS
	.text
NENTRY(apmcall)
	pushl	%ebp
	movl	%esp,%ebp
	pushl	%esi
	pushl	%edi
	pushl	%ebx
	
#if defined(DEBUG) || defined(DIAGNOSTIC)
	pushl	%ds		
	pushl	%es
	pushl	%fs
	pushl	%gs
	xorl	%ax,%ax
/*	movl	%ax,%ds		# can't toss %ds, we need it for apmstatus*/
	movl	%ax,%es
	movl	%ax,%fs
	movl	%ax,%gs
#endif
	xorl	%eax,%eax
	movb	%cs:8(%ebp),%al
	movb	$0x53,%ah
	movl	%cs:12(%ebp),%ebx
	movl	%cs:BIOSCALLREG_ECX(%ebx),%ecx
	movl	%cs:BIOSCALLREG_EDX(%ebx),%edx
	movl	%cs:BIOSCALLREG_EBX(%ebx),%ebx
	pushfl
	movl	$0,apmstatus	/* zero out just in case %ds is hosed? */
	cmp	$0, _C_LABEL(apm_disable_interrupts)
	je	nocli
	cli
nocli:
	clc /* clear carry in case BIOS doesn't do it for us on no-error */
	pushl	%ds
	/* Now call the 32-bit code segment entry point */
	lcall	%cs:(_C_LABEL(apminfo)+APM_ENTRY)
	popl	%ds
	setc	apmstatus
	popfl
#if defined(DEBUG) || defined(DIAGNOSTIC)
	popl	%gs
	popl	%fs
	popl	%es
	popl	%ds		# see above
#endif
	movl	12(%ebp),%edi
	/* XXX other addressing mode? */
	movl	%eax,BIOSCALLREG_EAX(%edi)
	movl	%ebx,BIOSCALLREG_EBX(%edi)
	movl	%ecx,BIOSCALLREG_ECX(%edi)
	movl	%edx,BIOSCALLREG_EDX(%edi)
	movl	%esi,BIOSCALLREG_ESI(%edi)
	movl	$1,%eax
	cmpl	$0,apmstatus
	jne	1f
	xorl	%eax,%eax
1:	
	popl	%ebx
	popl	%edi
	popl	%esi
	popl	%ebp
	ret
