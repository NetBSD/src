/*	$NetBSD: membar_ops.S,v 1.1.2.2 2007/04/22 05:12:42 thorpej Exp $	*/

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

#include "opt_multiprocessor.h"
#include "opt_cputype.h"

#define	END(name,a)	.align a; LABEL(name)

	.text
	.align	64

/*
 * We start out with no-op versions that do nothing.  We hot-patch when
 * we detect a MP system with the appropriate flavor.
 */
NENTRY(_membar_producer)
	ret
END(_membar_producer_end, 8)
ATOMIC_OP_ALIAS(membar_producer,_membar_producer)
ATOMIC_OP_ALIAS(membar_write,_membar_producer)
STRONG_ALIAS(_membar_write,_membar_producer)

NENTRY(_membar_consumer)
	ret
END(_membar_consumer_end, 8)
ATOMIC_OP_ALIAS(membar_consumer,_membar_consumer)
ATOMIC_OP_ALIAS(membar_read,_membar_consumer)
STRONG_ALIAS(_membar_read,_membar_consumer)

NENTRY(_membar_sync)
	ret
END(_membar_sync_end, 8)
ATOMIC_OP_ALIAS(membar_sync,_membar_sync)
ATOMIC_OP_ALIAS(membar_enter,_membar_sync)
STRONG_ALIAS(_membar_enter,_membar_sync)
ATOMIC_OP_ALIAS(membar_exit,_membar_sync)
STRONG_ALIAS(_membar_exit,_membar_sync)

#if defined(MULTIPROCESSOR)
NENTRY(_membar_sync_486)
	lock
	addl	$0, 0(%esp)
	ret
END(_membar_sync_486_end, 8)

#if defined(I686_CPU)
NENTRY(_membar_producer_sse2)
	sfence
	ret
END(_membar_producer_sse2_end, 8)

NENTRY(_membar_consumer_sse2)
	lfence
	ret
END(_membar_consumer_sse2_end, 8)

NENTRY(_membar_sync_sse2)
	mfence
	ret
END(_membar_sync_sse2_end, 8)
#endif /* I686_CPU */

#endif /* MULTIPROCESSOR */

#else /* _KERNEL */

#endif /* _KERNEL */
