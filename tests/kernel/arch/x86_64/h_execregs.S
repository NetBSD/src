/*	$NetBSD: h_execregs.S,v 1.1 2025/02/27 00:55:32 riastradh Exp $	*/

/*-
 * Copyright (c) 2025 The NetBSD Foundation, Inc.
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

#define	_LOCORE

#include <sys/syscall.h>

#include <machine/asm.h>

#include "execregs.h"

ENTRY(execregs_start)
	andq	$-0x10,%rsp		/* align stack to 16-byte boundary */

	/* store registers to a buffer on stack */
	subq	$(NEXECREGS*8),%rsp	/* space for NEXECREGS registers */
	movq	%rdi,0*8(%rsp)		/* order matches execregs.h */
	movq	%rsi,1*8(%rsp)
	movq	%rdx,2*8(%rsp)
	movq	%r10,3*8(%rsp)
	movq	%r8,4*8(%rsp)
	movq	%r9,5*8(%rsp)
	movq	%rcx,6*8(%rsp)
	movq	%r11,7*8(%rsp)
	movq	%r12,8*8(%rsp)
	movq	%r13,9*8(%rsp)
	movq	%r14,10*8(%rsp)
	movq	%r15,11*8(%rsp)
	movq	%rbp,12*8(%rsp)
	movq	%rax,13*8(%rsp)

	/* call write(STDOUT_FILENO, regs, sizeof(regs)) */
	movl	$0x1,%edi		/* arg0 := STDOUT_FILENO */
	movq	%rsp,%rsi		/* arg1 := regs */
	movl	$(NEXECREGS*8),%edx	/* arg2 := sizeof(regs) */
	movl	$SYS_write,%eax		/* syscall number */
	syscall

	jb	2f			/* bail if write failed */
	cmpq	$(NEXECREGS*8),%rax	/* bail if wrote wrong # of bytes */
	jne	2f

	/* call exit(0) */
	xorl	%edi,%edi		/* arg0 := 0 */
1:	movl	$SYS_exit,%eax		/* syscall number */
	syscall
	hlt				/* paranoia */

2:	/* call exit(127) */
	movl	$127,%edi		/* arg0 := 127 */
	jmp	1b
END(execregs_start)

/* main stub to simplify linking */
ENTRY(main)
	hlt
END(main)
