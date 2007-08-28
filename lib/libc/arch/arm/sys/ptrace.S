/*	$NetBSD: ptrace.S,v 1.8.16.1 2007/08/28 17:36:32 matt Exp $ */

/*-
 * Copyright (c) 1990 The Regents of the University of California.
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
 *	from: @(#)ptrace.s	5.1 (Berkeley) 4/23/90
 */

#include "SYS.h"

ENTRY(ptrace)
#ifdef _REENTRANT
	XPUSH	{r0-r3, lr}
	sub	sp, sp, #4		/* align stack */
	bl	PLT_SYM(_C_LABEL(__errno))
	add	sp, sp, #4		/* unalign stack */
	mov	r1, #0
	str	r1, [r0]
	XPOP	{r0-r3}
#else /* _REENTRANT */
	XPUSH	{r0-r1}
	GOT_INIT(r0, .Lgot, .L1)
	GOT_GET(r1, r0, .Lerrno)
	mov	r0, #0
	str	r0, [r1]
	XPOP	{r0-r1}
#endif /* _REENTRANT */

	SYSTRAP(ptrace)
	_SYSCALL_CERROR_CHECK
	XPOPRET

#ifndef _REENTRANT
	.align	0
	GOT_INITSYM(.Lgot, .L1)
.Lerrno:
	.word	GOT_SYM(_C_LABEL(errno))
#endif /* !_REENTRANT */
END(ptrace)
