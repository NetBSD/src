/*	$NetBSD: profile.h,v 1.6 2006/02/03 07:27:05 skrll Exp $	*/

/*
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
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
 *	@(#)profile.h	8.1 (Berkeley) 6/11/93
 */

#ifdef _KERNEL_OPT
#include "opt_multiprocessor.h"
#endif

#include <machine/cpufunc.h>

#define	_MCOUNT_DECL void _mcount

#define EPROL_EXPORT	__asm(".globl _eprol")

#ifdef PIC
#define __MCPLT	"@PLT"
#else
#define __MCPLT
#endif

#define	MCOUNT						\
__weak_alias(mcount, __mcount)				\
__asm(" .globl __mcount		\n"			\
"	.type __mcount,@function\n"			\
"__mcount:			\n"			\
"	pushq	%rbp		\n"			\
"	movq	%rsp,%rbp	\n"			\
"	subq	$56,%rsp	\n"			\
"	movq	%rdi,0(%rsp)	\n"			\
"	movq	%rsi,8(%rsp)	\n"			\
"	movq	%rdx,16(%rsp)	\n"			\
"	movq	%rcx,24(%rsp)	\n"			\
"	movq	%r8,32(%rsp)	\n"			\
"	movq	%r9,40(%rsp)	\n"			\
"	movq	%rax,48(%rsp)	\n"			\
"	movq	0(%rbp),%r11	\n"			\
"	movq	8(%r11),%rdi	\n"			\
"	movq	8(%rbp),%rsi	\n"			\
"	call	_mcount"__MCPLT "	\n"			\
"	movq	0(%rsp),%rdi	\n"			\
"	movq	8(%rsp),%rsi	\n"			\
"	movq	16(%rsp),%rdx	\n"			\
"	movq	24(%rsp),%rcx	\n"			\
"	movq	32(%rsp),%r8	\n"			\
"	movq	40(%rsp),%r9	\n"			\
"	movq	48(%rsp),%rax	\n"			\
"	leave			\n"			\
"	ret			\n"			\
"	.size __mcount,.-__mcount");


#ifdef _KERNEL

#ifdef MULTIPROCESSOR
__cpu_simple_lock_t __mcount_lock;

#define	MCOUNT_ENTER_MP							\
	__cpu_simple_lock(&__mcount_lock);
#define	MCOUNT_EXIT_MP							\
	__cpu_simple_unlock(&__mcount_lock);

#else
#define MCOUNT_ENTER_MP
#define MCOUNT_EXIT_MP
#endif

#define	MCOUNT_ENTER							\
	s = (int)read_psl();						\
	disable_intr();							\
	MCOUNT_ENTER_MP

#define	MCOUNT_EXIT							\
	MCOUNT_EXIT_MP							\
	write_psl(s);

#endif /* _KERNEL */
