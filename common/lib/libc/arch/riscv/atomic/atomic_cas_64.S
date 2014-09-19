/*	$NetBSD: atomic_cas_64.S,v 1.1 2014/09/19 17:36:24 matt Exp $	*/
/*-
 * Copyright (c) 2014 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Matt Thomas <matt@3am-software.com>
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

#ifdef _LP64
#include "atomic_op_asm.h"

ENTRY_NP(_atomic_cas_64)
1:	lr.d	v0, 0(a0)
	bne	a1, v0, 2f
	sc.d	v1, a2, 0(a0)
	bnez	v1, 1b
2:
	ret
END(_atomic_cas_64)

ATOMIC_OP_ALIAS(atomic_cas_64,_atomic_cas_64)
ATOMIC_OP_ALIAS(atomic_cas_ulong,_atomic_cas_64)
STRONG_ALIAS(_atomic_cas_ulong,_atomic_cas_64)
ATOMIC_OP_ALIAS(atomic_cas_ptr,_atomic_cas_64)
STRONG_ALIAS(_atomic_cas_ptr,_atomic_cas_64)
ATOMIC_OP_ALIAS(atomic_cas_ulong_ni,_atomic_cas_64)
STRONG_ALIAS(_atomic_cas_ulong_ni,_atomic_cas_64)
ATOMIC_OP_ALIAS(atomic_cas_ptr_ni,_atomic_cas_64)
STRONG_ALIAS(_atomic_cas_ptr_ni,_atomic_cas_64)

CRT_ALIAS(__sync_val_compare_and_swap_8,_atomic_cas_64)

ENTRY_NP(_atomic_bool_cas_64)
	li	v0, 0
1:	lr.d	v1, 0(a0)
	bne	a1, v1, 2f
	sc.d	v1, a2, 0(a0)
	bnez	v1, 1b
	li	v0, 1
2:
	ret
END(_atomic_bool_cas_64)

ATOMIC_OP_ALIAS(atomic_bool_cas_64,_atomic_bool_cas_64)
CRT_ALIAS(__bool_val_compare_and_swap_8,_atomic_bool_cas_64)
#endif /* _LP64 */
