/*	$NetBSD: _context_u.S,v 1.1.2.1 2001/07/13 21:53:40 nathanw Exp $	*/

/*-
 * Copyright (c) 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Nathan J. Williams.
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
	
#define GETC \
	movl	%gs,	%eax	; \
	movl	%eax,  36(%ecx) ; \
	movl	%fs,	%eax	; \
	movl	%eax,  40(%ecx)	; \
	movl	%es,	%eax	; \
	movl	%eax,  44(%ecx)	; \
	movl	%ds,	%eax	; \
	movl	%eax,  48(%ecx)	; \
	movl	%edi,  52(%ecx)	; \
	movl	%esi,  56(%ecx)	; \
	movl	%ebp,  60(%ecx)	; \
	movl	$0,    64(%ecx) ; \
	movl	%ebx,  68(%ecx)	; \
	movl    $0,    72(%ecx) ; \
	movl    $0,    76(%ecx) ; \
	movl    $0,    80(%ecx) ; \
	movl    $0,    84(%ecx) ; \
	movl    $0,    88(%ecx) ; \
	movl	%edx,  92(%ecx)	; \
	movl	%cs,	%eax	; \
	movl	%eax,  96(%ecx) ; \
	pushfl			; \
	popl	%eax		; \
	movl	%eax, 100(%ecx)	; \
	movl	%ss,	%eax	; \
	movl	%eax, 108(%ecx)	; \
	movl	$4,	%eax	; \
 	movl	%eax,	0(%ecx)	

#define SETC \
	movl	36(%ecx), %gs	; \
	movl	40(%ecx), %fs	; \
	movl	44(%ecx), %es	; \
	movl	52(%ecx), %edi	; \
	movl	56(%ecx), %esi	; \
	movl	60(%ecx), %ebp	; \
	movl	68(%ecx), %ebx	; \
	movl	72(%ecx), %edx	; \
	movl	80(%ecx), %eax	; \
	pushl	100(%ecx)	; \
	popfl			; \
	movl	108(%ecx), %ss	; \
	movl	104(%ecx), %esp	; \
	push	96(%ecx)	; \
	pushl	92(%ecx)	; \
	push	48(%ecx)	; \
	movl	76(%ecx), %ecx	; \
	pop	%ds		; \
	lret

		
		
ENTRY(_getcontext_u)
	movl	4(%esp), %ecx
	movl	0(%esp), %edx
	GETC
	movl	%esp, %eax
	addl	$4, %eax
	movl	%eax, 104(%ecx)
	xorl	%eax,  %eax
	ret
	
ENTRY(_setcontext_u)					
	movl	4(%esp), %ecx
	SETC
	
ENTRY(_swapcontext_u)
	movl	4(%esp), %ecx
	movl	0(%esp), %edx
	GETC
	movl	%esp, %eax
	addl	$4, %eax
	movl	%eax, 104(%ecx)
	movl	8(%esp), %ecx
	SETC

