/*	$NetBSD: compat_sigprocmask.S,v 1.1.10.1 2007/11/06 18:39:12 matt Exp $	*/

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
 *	from: @(#)sigprocmask.s	5.2 (Berkeley) 12/17/90
 */

#include "SYS.h"

WARN_REFERENCES(sigprocmask, \
    "warning: reference to compatibility sigprocmask(); include <signal.h> for correct reference")

ENTRY(sigprocmask)
#ifdef __thumb__
	cmp	r1, #0
	bne	1f
	mov	r0, #1
	b	2f
1:	ldr	r1, [r1]
2:
#else
	teq	r1, #0
	moveq	r0, #1
	moveq	r1, #0
	ldrne	r1, [r1]
#endif
	SYSTRAP(compat_13_sigprocmask13)
	_SYSCALL_CERROR_CHECK
#ifdef __thumb__
	cmp	r2, #0
	beq	3f
	str	r0, [r2]
3:
#else
	teq	r2, #0
	strne	r0, [r2]
#endif
	mov	r0, #0
	RET
