/*	$NetBSD: atomic_cas_32.S,v 1.1.2.5 2007/04/22 05:12:41 thorpej Exp $	*/

/*-
 * Copyright (c) 2006, 2007 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Andrew Doran and Jason R. Thorpe.
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
	.align	64

/*
 * We start out with the i386 version that is NOT MP-safe.  We will
 * hot-patch in the (MP-safe) 486 version when we detect a 486-or-better
 * CPU.
 */
NENTRY(_atomic_cas_32)
	movl	4(%esp), %edx
	movl	8(%esp), %eax
	movl	12(%esp), %ecx

	/* This will not work on multiprocessors. */

	cli
	cmpl	%eax, (%edx)
	jne	1f
	movl	%ecx, (%edx)
	sti
	ret

1:	movl	(%edx), %eax
	sti
	ret
END(_atomic_cas_32_end, 32)
ATOMIC_OP_ALIAS(atomic_cas_32,_atomic_cas_32)
ATOMIC_OP_ALIAS(atomic_cas_uint,_atomic_cas_32)
STRONG_ALIAS(_atomic_cas_uint,_atomic_cas_32)
ATOMIC_OP_ALIAS(atomic_cas_ulong,_atomic_cas_32)
STRONG_ALIAS(_atomic_cas_ulong,_atomic_cas_32)
ATOMIC_OP_ALIAS(atomic_cas_ptr,_atomic_cas_32)
STRONG_ALIAS(_atomic_cas_ptr,_atomic_cas_32)

#if defined(I486_CPU) || defined(I586_CPU) || defined(I686_CPU)
NENTRY(_atomic_cas_32_486)
	movl	4(%esp), %edx
	movl	8(%esp), %eax
	movl	12(%esp), %ecx
	LOCK
	cmpxchgl %ecx, (%edx)
	/* %eax now contains the old value */
	ret
END(_atomic_cas_32_486_end, 32)
#endif /* I486_CPU || I586_CPU || I686_CPU */

#else /* _KERNEL */

/*
 * XXX TODO
 */

#endif /* _KERNEL */
