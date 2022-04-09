/*	$NetBSD: membar_ops.S,v 1.3 2022/04/09 23:32:51 riastradh Exp $	*/

/*-
 * Copyright (c) 2007 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe, and by Andrew Doran.
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

RCSID("$NetBSD: membar_ops.S,v 1.3 2022/04/09 23:32:51 riastradh Exp $")

	.text

LEAF_ENTRY(_membar_sync)
	sync
	nop
	nop
	nop
	nop
	nop
	bv	%r0(%rp)
	 nop
EXIT(_membar_sync)
ATOMIC_OP_ALIAS(membar_sync,_membar_sync)

ATOMIC_OP_ALIAS(membar_producer,_membar_sync)
STRONG_ALIAS(_membar_producer,_membar_sync)
ATOMIC_OP_ALIAS(membar_consumer,_membar_sync)
STRONG_ALIAS(_membar_consumer,_membar_sync)
ATOMIC_OP_ALIAS(membar_acquire,_membar_sync)
STRONG_ALIAS(_membar_acquire,_membar_sync)
ATOMIC_OP_ALIAS(membar_release,_membar_sync)
STRONG_ALIAS(_membar_release,_membar_sync)
ATOMIC_OP_ALIAS(membar_enter,_membar_sync)
STRONG_ALIAS(_membar_enter,_membar_sync)
ATOMIC_OP_ALIAS(membar_exit,_membar_sync)
STRONG_ALIAS(_membar_exit,_membar_sync)
