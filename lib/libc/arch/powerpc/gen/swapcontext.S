/*	$NetBSD: swapcontext.S,v 1.4 2008/04/28 20:22:57 martin Exp $	*/

/*-
 * Copyright (c) 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Klaus Klein.
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

#include "SYS.h"

#if defined(LIBC_SCCS) && !defined(lint)
	.text
	.asciz "$NetBSD: swapcontext.S,v 1.4 2008/04/28 20:22:57 martin Exp $"
#endif /* LIBC_SCCS && !lint */

ENTRY(swapcontext)
	stwu	%r1,-16(%r1)			# set up new stack frame
	mflr	%r0
	stw	%r0,20(%r1)			# save link register
	stw	%r3,8(%r1)			# must save oucp
	stw	%r4,12(%r1)			# must save ucp
	bl	PIC_PLT(_C_LABEL(_getcontext))	# getcontext(oucp)
	cmpwi	%r3,0
	bne	1f
	lwz	%r11,8(%r1)			# load oucp for adjustment
	lwz	%r0,20(%r1)
	stw	%r0,(48 + 34 * 4)(%r11)		# pc <- lr
	la	%r0,16(%r1)
	stw	%r0,(48 +  1 * 4)(%r11)		# adjust sp
	lwz	%r3,12(%r1)			# load ucp
	bl	PIC_PLT(_C_LABEL(setcontext))	# setcontext(ucp)
1:
	lwz	%r0,20(%r1)
	mtlr	%r0
	la	%r1,16(%r1)
	blr
