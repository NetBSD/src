/* $NetBSD: linux_support.S,v 1.4.6.2 2009/01/17 13:28:43 mjf Exp $ */

/*-
 * Copyright (c) 2006,2007 Konstantin Belousov
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
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * $FreeBSD: src/sys/i386/linux/linux_support.s,v 1.1 2007/05/23 08:33:06 kib Exp $
 */
#include <machine/asm.h>

#include "assym.h"

#include <sys/errno.h>
#include <machine/frameasm.h>

#define GET_CURPCB(reg)	\
	movl	CPUVAR(CURLWP), reg;	\
	movl	L_ADDR(reg), reg

futex_fault_decx:
	GET_CURPCB(%ecx)
futex_fault:
	movl	$0,PCB_ONFAULT(%ecx)
	movl	$-EFAULT,%eax
	ret

ENTRY(futex_xchgl)
	GET_CURPCB(%ecx)
	movl	$futex_fault,PCB_ONFAULT(%ecx)
	movl	4(%esp),%eax
	movl	8(%esp),%edx
	cmpl    $VM_MAXUSER_ADDRESS-4,%edx
	ja     	futex_fault
	xchgl	%eax,(%edx)
	movl	12(%esp),%edx
	movl	%eax,(%edx)
	xorl	%eax,%eax
	movl	%eax,PCB_ONFAULT(%ecx)
	ret

ENTRY(futex_addl)
	GET_CURPCB(%ecx)
	movl	$futex_fault,PCB_ONFAULT(%ecx)
	movl	4(%esp),%eax
	movl	8(%esp),%edx
	cmpl    $VM_MAXUSER_ADDRESS-4,%edx
	ja     	futex_fault
	lock
	xaddl	%eax,(%edx)
	movl	12(%esp),%edx
	movl	%eax,(%edx)
	xorl	%eax,%eax
	movl	%eax,PCB_ONFAULT(%ecx)
	ret

ENTRY(futex_orl)
	GET_CURPCB(%ecx)
	movl	$futex_fault_decx,PCB_ONFAULT(%ecx)
	movl	8(%esp),%edx
	cmpl    $VM_MAXUSER_ADDRESS-4,%edx
	ja     	futex_fault
	movl	(%edx),%eax
1:	movl	%eax,%ecx
	orl	4(%esp),%ecx
	lock
	cmpxchgl %ecx,(%edx)
	jnz	1b
futex_tail:	
	movl	12(%esp),%edx
	movl	%eax,(%edx)
	xorl	%eax,%eax
	GET_CURPCB(%ecx)
	movl	%eax,PCB_ONFAULT(%ecx)
	ret

ENTRY(futex_andl)
	GET_CURPCB(%ecx)
	movl	$futex_fault_decx,PCB_ONFAULT(%ecx)
	movl	8(%esp),%edx
	cmpl    $VM_MAXUSER_ADDRESS-4,%edx
	ja     	futex_fault
	movl	(%edx),%eax
1:	movl	%eax,%ecx
	andl	4(%esp),%ecx
	lock
	cmpxchgl %ecx,(%edx)
	jnz	1b
	jmp	futex_tail

ENTRY(futex_xorl)
	GET_CURPCB(%ecx)
	movl	$futex_fault_decx,PCB_ONFAULT(%ecx)
	movl	8(%esp),%edx
	cmpl    $VM_MAXUSER_ADDRESS-4,%edx
	ja     	futex_fault
	movl	(%edx),%eax
1:	movl	%eax,%ecx
	xorl	4(%esp),%ecx
	lock
	cmpxchgl %ecx,(%edx)
	jnz	1b
	jmp	futex_tail
