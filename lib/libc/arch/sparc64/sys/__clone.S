/*	$NetBSD: __clone.S,v 1.1.2.3 2002/06/21 18:18:03 nathanw Exp $	*/

/*-     
 * Copyright (c) 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe.
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
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
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

#include <sys/errno.h>

#include "SYS.h"

#ifdef WEAK_ALIAS
WEAK_ALIAS(clone, __clone)
#endif

/*
 * int __clone(int (*fn)(void *), void *stack, int flags, void *arg);
 */
ENTRY(__clone)
	save	%sp, -176, %sp

	/*
	 * Sanity checks: func and stack may not be NULL.
	 */
	tst	%i0
	be	8f		! func == NULL, bail
	 orcc	%i1, %g0, %o1	! setup stack arg for syscall, test
	be	8f		! stack == NULL, bail
	 mov	%i2, %o0	! setup flags arg for syscall

	/*
	 * Store the function and the argument on our child's stack
	 */
	sub	%o1, 16, %o1	! make space on the child's stack
	stx	%i0, [%o1]	! save func
	stx	%i3, [%o1+8]	! save arg

	/*
	 * Args are now set up for system call as (flags, stack).
	 */
	mov	SYS___clone, %g1
	t	ST_SYSCALL
	be	9f
	 tst	%o1		! %o1 (rv[1]) == 0 in parent

1:	bz	2f		! yes, parent
	 ldx	[%sp], %l0	! grab the function
	ldx	[%sp+8], %o0	! and the argument from the stack
	add	%sp, 16, %sp	! restore the stack
	call	%l0 		! Call the clone's entry point.
	 nop

	JUMP(_exit)
	/* NOTREACHED */

2:	ret
	 restore %g0, %o0, %o0

8:	mov	EINVAL, %o0
9:	ERROR()
	mov	-1, %i0
	ret
	 restore %g0, -1, %o0
