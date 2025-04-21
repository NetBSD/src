/*	$NetBSD: contextspfunc.S,v 1.2 2025/04/21 12:06:08 riastradh Exp $	*/

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

#include <machine/asm.h>

RCSID("$NetBSD: contextspfunc.S,v 1.2 2025/04/21 12:06:08 riastradh Exp $")

/*
 * contextspfunc()
 *
 *	makecontext(3) function.  Store the stack pointer on entry at
 *	the global variable contextsp and call contextdone.
 */
LEAF_ENTRY(contextspfunc)
	addil	LT%_C_LABEL(contextdone), %r19	/* r20 := contextdone */
	ldw	RT%_C_LABEL(contextdone)(%r1), %r20
	addil	LT%_C_LABEL(contextsp), %r19	/* r21 := &contextsp */
	ldw	RT%_C_LABEL(contextsp)(%r1), %r21
	bv	%r0(%r20)			/* jump to contextdone */
	 stw	%sp, 0(%r21)			/* contextsp := sp */
EXIT(contextspfunc)
