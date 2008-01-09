/*	$NetBSD: _context_u.S,v 1.2.24.2 2008/01/09 01:36:41 matt Exp $	*/

/*-
 * Copyright (c) 2001, 2007 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Nathan J. Williams, and by Andrew Doran.
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
#include "assym.h"

#define	STUB(n)		ENTRY(n); .hidden _C_LABEL(n)

#define FPSAVE(reg)	fnsave UC_FPREGS(reg)
#define FPLOAD(reg)	frstor UC_FPREGS(reg)
#include "_getsetc.S"
	
STUB(_getcontext_u_s87)
	movl	4(%esp), %ecx
	movl	0(%esp), %edx
	GETC
	movl	%esp, %eax
	addl	$4, %eax
	movl	%eax, (UC_REGS + _REG_UESP * 4)(%ecx)
	xorl	%eax,  %eax
	ret

STUB(_setcontext_u_s87)					
	movl	4(%esp), %ecx
	SETC

STUB(_swapcontext_u_s87)
	movl	4(%esp), %ecx
	movl	0(%esp), %edx
	GETC
	movl	%esp, %eax
	addl	$4, %eax
	movl	%eax, (UC_REGS + _REG_UESP * 4)(%ecx)
	movl	8(%esp), %ecx
	SETC


#undef FPSAVE
#undef FPLOAD
		
#define FPSAVE(reg)	fxsave UC_FXSAVEREGS(reg)
#define FPLOAD(reg)	fxrstor UC_FXSAVEREGS(reg)
#include "_getsetc.S"

STUB(_getcontext_u_xmm)
	movl	4(%esp), %ecx
	movl	0(%esp), %edx
	GETC
	movl	%esp, %eax
	addl	$4, %eax
	movl	%eax, (UC_REGS + _REG_UESP * 4)(%ecx)
	xorl	%eax,  %eax
	ret

STUB(_setcontext_u_xmm)					
	movl	4(%esp), %ecx
	SETC

STUB(_swapcontext_u_xmm)
	movl	4(%esp), %ecx
	movl	0(%esp), %edx
	GETC
	movl	%esp, %eax
	addl	$4, %eax
	movl	%eax, (UC_REGS + _REG_UESP * 4)(%ecx)
	movl	8(%esp), %ecx
	SETC

STUB(pthread__atomic_swap_ptr)
	movl	4(%esp), %ecx
	movl	8(%esp), %eax
	xchgl	%eax, (%ecx)
	ret

STUB(pthread__atomic_or_ulong)
	movl	4(%esp), %ecx
	movl	8(%esp), %eax
	lock
	orl	%eax, (%ecx)
	ret

STUB(pthread__membar_full)
	lock
	addl	$0, -4(%esp)
	ret

STUB(pthread__membar_producer)
	/* A store is enough */
	addl	$0, -4(%esp)
	ret

STUB(pthread__membar_consumer)
	lock
	addl	$0, -4(%esp)
	ret
