/*	$NetBSD: pthread_md.S,v 1.1 2009/05/18 13:03:35 njoly Exp $	*/

/*-
 * Copyright (c) 2007, 2008, 2009 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Nick Hudson.

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

#include <machine/asm.h>

	.align 2
LEAF(pthread__ras_simple_lock_init, 1)
	stl	zero, 0(a0)
	nop
	RET
END(pthread__ras_simple_lock_init)

	.align 2
LEAF(pthread__ras_simple_lock_try, 1)
	ldiq	t0, 1
pthread__lock_ras_start:	.globl	pthread__lock_ras_start
	ldl	v0, 0(a0)
	stl	t0, 0(a0)
pthread__lock_ras_end:		.globl	pthread__lock_ras_end
	cmpeq	v0, 0, v0
	RET	
END(pthread__ras_simple_lock_try)

	.align 2
LEAF(pthread__ras_simple_unlock, 1)
	stl	zero, 0(a0)
	nop
	RET
END(pthread__ras_simple_unlock)
