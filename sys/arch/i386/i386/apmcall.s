/*	$NetBSD: apmcall.s,v 1.2.2.3 1997/10/15 20:59:24 thorpej Exp $ */
/*
 *  Copyright (c) 1997 John T. Kohl
 *  All rights reserved.
 * 
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *  1. Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *  3. The name of the author may not be used to endorse or promote products
 *     derived from this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR `AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 * 
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
apmstatus:	.long 0
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
	movb	%cs:8(%ebp),%al
	movb	$0x53,%ah
	movl	%cs:12(%ebp),%ebx
	movw	%cs:BIOSCALLREG_CX(%ebx),%cx
	movw	%cs:BIOSCALLREG_DX(%ebx),%dx
	movw	%cs:BIOSCALLREG_BX(%ebx),%bx
	pushfl
	cli
	pushl	%ds
	/* Now call the 32-bit code segment entry point */
	lcall	%cs:(_apminfo+APM_ENTRY)
	popl	%ds
	setc	apmstatus
	popfl
#if defined(DEBUG) || defined(DIAGNOSTIC)
	popl	%gs
	popl	%fs
	popl	%es
	popl	%ds		# see above
#endif
	movl	12(%ebp),%esi
	movw	%ax,BIOSCALLREG_AX(%esi)
	movw	%bx,BIOSCALLREG_BX(%esi)
	movw	%cx,BIOSCALLREG_CX(%esi)
	movw	%dx,BIOSCALLREG_DX(%esi)
/* todo: do something with %edi? */
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
