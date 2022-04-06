/*	$NetBSD: atomic_or.S,v 1.9 2022/04/06 22:47:57 riastradh Exp $	*/

/*-
 * Copyright (c) 2007 The NetBSD Foundation, Inc.
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

#include "atomic_op_asm.h"

	.text

ENTRY(_atomic_or_32)
	ld	[%o0], %o2
1:	or	%o1, %o2, %o3
	cas	[%o0], %o2, %o3
	cmp	%o2, %o3
	bne,a,pn %icc, 1b
	 mov	%o3, %o2
	retl
	 or	%o1, %o2, %o0
ATOMIC_OP_ALIAS(atomic_or_32,_atomic_or_32)
ATOMIC_OP_ALIAS(atomic_or_uint,_atomic_or_32)
STRONG_ALIAS(_atomic_or_uint,_atomic_or_32)

ATOMIC_OP_ALIAS(atomic_or_32_nv,_atomic_or_32)
STRONG_ALIAS(_atomic_or_32_nv,_atomic_or_32)
ATOMIC_OP_ALIAS(atomic_or_uint_nv,_atomic_or_32)
STRONG_ALIAS(_atomic_or_uint_nv,_atomic_or_32)

ENTRY(_atomic_or_64)
#ifndef __arch64__
	COMBINE(%o1, %o2, %o1)
#endif
	ldx	[%o0], %o2
1:	or	%o1, %o2, %o3
	casx	[%o0], %o2, %o3
	cmp	%o2, %o3
	bne,a,pn %xcc, 1b
	 mov	%o3, %o2
#ifdef __arch64__
	retl
	 or	%o1, %o2, %o0
#else
	or	%o1, %o2, %o0
	SPLIT_RETL(%o0, %o0, %o1)
#endif
ATOMIC_OP_ALIAS(atomic_or_64,_atomic_or_64)

ATOMIC_OP_ALIAS(atomic_or_64_nv,_atomic_or_64)
STRONG_ALIAS(_atomic_or_64_nv,_atomic_or_64)

ATOMIC_OP_ALIAS_SIZE(atomic_or_ulong,_atomic_or)
STRONG_ALIAS_SIZE(_atomic_or_ulong,_atomic_or)

ATOMIC_OP_ALIAS_SIZE(atomic_or_ulong_nv,_atomic_or)
STRONG_ALIAS_SIZE(_atomic_or_ulong_nv,_atomic_or)

#if !defined(__arch64__) && defined(CRT_ALIAS)
ENTRY(__sync_fetch_and_or_8)
	COMBINE(%o1, %o2, %o1)
	ldx	[%o0], %o2
1:	or	%o1, %o2, %o3
	casx	[%o0], %o2, %o3
	cmp	%o2, %o3
	bne,a,pn %xcc, 1b
	 mov	%o3, %o2
	SPLIT_RETL(%o2, %o0, %o1)
CRT_ALIAS(__sync_or_and_fetch_8,_atomic_or_64_nv)

CRT_ALIAS(__atomic_fetch_or_8,__sync_fetch_and_or_8)
CRT_ALIAS(__atomic_or_fetch_8,__sync_or_and_fetch_8)
#endif
