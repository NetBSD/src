/*	$NetBSD: _context_u.S,v 1.2 2003/07/26 19:25:07 salo Exp $	*/

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
 *
 * Adapted for x86_64 by fvdl@NetBSD.org
 */

#include <machine/asm.h>
#include "assym.h"

#define GETC \
	movq	(%rsp), %r11					; \
	movq	%r11, (UC_REGS + _REG_RIP * 8)(%rdi)		; \
	movq	%rbx, (UC_REGS + _REG_RBX * 8)(%rdi)		; \
	movq	%rbp, (UC_REGS + _REG_RBP * 8)(%rdi)		; \
	movq	%r12, (UC_REGS + _REG_R12 * 8)(%rdi)		; \
	movq	%r13, (UC_REGS + _REG_R13 * 8)(%rdi)		; \
	movq	%r14, (UC_REGS + _REG_R14 * 8)(%rdi)		; \
	movq	%r15, (UC_REGS + _REG_R15 * 8)(%rdi)		; \
	fxsave	UC_FPREGS(%rdi)					; \
	movl	$(_UC_USER | _UC_CPU | _UC_FPU),UC_FLAGS(%rdi)

#define SETC \
	movl	UC_FLAGS(%rdi), %eax				; \
	btl	$_UC_USER_BIT, %eax				; \
	jnc	1f						; \
	fxrstor	UC_FPREGS(%rdi)					; \
	movq	(UC_REGS + _REG_RBX * 8)(%rdi),%rbx		; \
	movq	(UC_REGS + _REG_RBP * 8)(%rdi),%rbp		; \
	movq	(UC_REGS + _REG_R12 * 8)(%rdi),%r12		; \
	movq	(UC_REGS + _REG_R13 * 8)(%rdi),%r13		; \
	movq	(UC_REGS + _REG_R14 * 8)(%rdi),%r14		; \
	movq	(UC_REGS + _REG_R15 * 8)(%rdi),%r15		; \
	movq	(UC_REGS + _REG_URSP * 8)(%rdi),%r11		; \
	movq	(UC_REGS + _REG_RIP * 8)(%rdi),%rax		; \
	leaq	-8(%r11),%rsp					; \
	movq	%rax, (%rsp)					; \
	ret							; \
1: 	andl	$_UC_FPU, %eax					; \
	jz	2f						; \
	fxrstor	UC_FPREGS(%rdi)					; \
2: 	xorq	%rax,%rax					; \
	movw	(UC_REGS + _REG_GS * 8)(%rdi), %gs		; \
	movw	(UC_REGS + _REG_FS * 8)(%rdi), %fs		; \
	movw	(UC_REGS + _REG_ES * 8)(%rdi), %es		; \
	movq	(UC_REGS + _REG_URSP * 8)(%rdi), %r11		; \
	movq	(UC_REGS + _REG_RBX * 8)(%rdi), %rbx		; \
	movq	(UC_REGS + _REG_RBP * 8)(%rdi), %rbp		; \
	movq	(UC_REGS + _REG_R12 * 8)(%rdi), %r12		; \
	movq	(UC_REGS + _REG_R13 * 8)(%rdi), %r13		; \
	movq	(UC_REGS + _REG_R14 * 8)(%rdi), %r14		; \
	movq	(UC_REGS + _REG_R15 * 8)(%rdi), %r15		; \
	movq	(UC_REGS + _REG_RBX * 8)(%rdi), %rbx		; \
	movq	(UC_REGS + _REG_R10 * 8)(%rdi), %r10		; \
	movq	(UC_REGS + _REG_R9  * 8)(%rdi), %r9		; \
	movq	(UC_REGS + _REG_R8  * 8)(%rdi), %r8		; \
	movq	(UC_REGS + _REG_RCX * 8)(%rdi), %rcx		; \
	movq	(UC_REGS + _REG_RDX * 8)(%rdi), %rdx		; \
	movq	(UC_REGS + _REG_RSI * 8)(%rdi), %rsi		; \
	movw	(UC_REGS + _REG_CS * 8)(%rdi), %ax		; \
	movq	%rax, -8(%r11)					; \
	movw	(UC_REGS + _REG_DS * 8)(%rdi), %ax		; \
	movq	%rax, -32(%r11)					; \
	movq	(UC_REGS + _REG_RIP * 8)(%rdi), %rax		; \
	movq	%rax, -16(%r11)					; \
	movq	(UC_REGS + _REG_RAX)(%rdi), %rax		; \
	movq	%rax, -24(%r11)					; \
	movq	(UC_REGS + _REG_R11 * 8)(%rdi), %rax		; \
	movq	%rax, -40(%r11)					; \
	movq	(UC_REGS + _REG_RFL * 8)(%rdi), %rax		; \
	movq	%rax, -48(%r11)					; \
								; \
	movw	(UC_REGS + _REG_SS * 8)(%rdi), %ss		; \
	movq	(UC_REGS + _REG_RDI)(%rdi), %rdi		; \
	leaq	-48(%r11), %rsp					; \
								; \
	popfq							; \
	popq	%r11						; \
	popq	%rax						; \
	movl	%eax,%ds					; \
	popq	%rax						; \
	lretq

ENTRY(_getcontext_u)
	GETC
	leaq	8(%rsp), %r11
	movq	%r11, (UC_REGS + _REG_URSP * 8)(%rdi)
	xorl	%eax, %eax
	ret

ENTRY(_setcontext_u)					
	SETC

ENTRY(_swapcontext_u)
	GETC
	leaq	8(%rsp),%rax
	movq	%rax, (UC_REGS + _REG_URSP * 8)(%rdi)
	movq	%rsi, %rdi
	SETC
