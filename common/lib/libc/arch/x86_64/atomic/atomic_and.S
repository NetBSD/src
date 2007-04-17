/*	$NetBSD: atomic_and.S,v 1.1.2.1 2007/04/17 04:28:20 thorpej Exp $	*/

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

#include "atomic_op_asm.h"

	.text
	.align	64

NENTRY(_atomic_and_8)
	LOCK
	andb	%sil, (%rdi)
	ret
ATOMIC_OP_ALIAS(atomic_and_8,_atomic_and_8)
ATOMIC_OP_ALIAS(atomic_and_uchar,_atomic_and_8)

NENTRY(_atomic_and_16)
	LOCK
	andw	%si, (%rdi)
	ret
ATOMIC_OP_ALIAS(atomic_and_16,_atomic_and_16)
ATOMIC_OP_ALIAS(atomic_and_ushort,_atomic_and_16)

NENTRY(_atomic_and_32)
	LOCK
	andl	%esi, (%rdi)
	ret
ATOMIC_OP_ALIAS(atomic_and_32,_atomic_and_32)
ATOMIC_OP_ALIAS(atomic_and_uint,_atomic_and_32)

NENTRY(_atomic_and_64)
	LOCK
	andq	%rsi, (%rdi)
	ret
ATOMIC_OP_ALIAS(atomic_and_64,_atomic_and_64)
ATOMIC_OP_ALIAS(atomic_and_ulong,_atomic_and_64)
