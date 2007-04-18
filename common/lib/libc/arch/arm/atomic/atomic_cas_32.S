/*	$NetBSD: atomic_cas_32.S,v 1.1.2.1 2007/04/18 07:32:30 thorpej Exp $	*/

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

#if defined(_KERNEL)

	.text
	.align	0

/*
 * ARM doesn't have a compare-and-swap, so this is implemented
 * as a restartable atomic sequence.  See irq_dispatch.S.
 */
ENTRY_NP(_atomic_cas_32)
	mov	r3, r0			/* r3 = address */

	.globl	_C_LABEL(_atomic_cas_32_ras_start)
_C_LABEL(_atomic_cas_32_ras_start):

	ldr	r0, [r3]
	teq	r0, r1
	bne	1f
	str	r2, [r3]

	.globl	_C_LABEL(_atomic_cas_32_ras_end)
_C_LABEL(_atomic_cas_32_ras_end):

1:	RET				/* r0 contains old *addr */

ATOMIC_OP_ALIAS(atomic_cas_32,_atomic_cas_32)
ATOMIC_OP_ALIAS(atomic_cas_uint,_atomic_cas_32)
ATOMIC_OP_ALIAS(atomic_cas_ulong,_atomic_cas_32)
ATOMIC_OP_ALIAS(atomic_cas_ptr,_atomic_cas_32)

#else /* _KERNEL */

#endif /* _KERNEL */
