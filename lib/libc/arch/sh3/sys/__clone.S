/*	$NetBSD: __clone.S,v 1.1 2001/07/22 17:18:25 tsubai Exp $	*/

/*-
 * Copyright (c) 2001 Tsubai Masanari.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/errno.h>
#include "SYS.h"

#ifdef WEAK_ALIAS
WEAK_ALIAS(clone,__clone)
#endif

/*
 * int __clone(int (*fn)(void *), void *stack, int flags, void *arg);
 */
ENTRY(__clone)
	/*
	 * Sanity checks: func and stack may not be NULL.
	 */
	tst	r4, r4
	bt	inval
	tst	r5, r5
	bt	inval

	mov	r4, r2
	mov	r6, r4
	mov.l	LSYS___clone, r0
	trapa	#0x80
	bf	err

	tst	r0, r0
	bf	1f		/* We're the parent, just return. */

	jsr	@r2		/* Call the clone's entry point. */
	mov	r7, r4		/* arg */

	mov.l	L__exit, r1
	jsr	@r1
	mov	r0, r4

inval:
	mov	#EINVAL, r0
err:
	mov.l	Lcerror, r1
	jmp	@r1
	nop
1:
	rts
	nop

	.align	2
LSYS___clone:
	.long	SYS___clone
L__exit:
	.long	_C_LABEL(_exit)
Lcerror:
	.long	cerror
