/*	$NetBSD: membar_ops.S,v 1.13 2022/04/21 12:06:31 riastradh Exp $	*/

/*-
 * Copyright (c) 2006, 2007 The NetBSD Foundation, Inc.
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

	.text
	.set noreorder

LEAF(_membar_sync)
	j	ra
	 BDSYNC
END(_membar_sync)
ATOMIC_OP_ALIAS(membar_sync,_membar_sync)

STRONG_ALIAS(_membar_enter,_membar_sync)
ATOMIC_OP_ALIAS(membar_enter,_membar_sync)

#ifdef __OCTEON__

/*
 * cnMIPS guarantees load-before-load/store ordering without any
 * barriers.  So the only barriers we need are store-before-load (sync)
 * and store-before-store (syncw, i.e., sync 4).  See Table 2-32
 * `Execution Ordering Rules' on p. 104 of Cavium OCTEON III CN78XX
 * Hardware Reference Manual, CN78XX-HM-0.99E, September 2014:
 *
 *	First Operation		DLD [load instruction to a physical
 *				address that is L2/DRAM]
 *	Second Operation	Any
 *	Execution Ordering Comments
 *
 *		The second operation cannot appear to execute before
 *		the first (DLD) operation, regardless of the presence
 *		or absence of SYNC* instructions.
 *
 * Note: I'm not sure if this applies to earlier cnMIPS -- can't find
 * it in the Cavium Networks OCTEON Plus CN50XX Hardware Reference
 * Manual CN50XX-HM-0.99E, July 2008.  Experimentally, on an erlite3
 * (Cavium Octeon CN5020-500), I can easily detect reordering of
 * store-before-store and store-before-load, but I haven't been able to
 * detect any reordering of load-before-load or load-before-store.
 *
 * Note: On early cnMIPS (CN3xxx), there is an erratum which sometimes
 * requires issuing two syncw's in a row.  I don't know the details --
 * don't have documentation -- and in Linux it is only used for I/O
 * purposes.
 *
 * Currently we don't build kernels that work on both Octeon and
 * non-Octeon MIPS CPUs, so none of this is done with binary patching.
 * For userlands we could use a separate shared library on Octeon with
 * ld.so.conf to override the symbols with cheaper definitions, but we
 * don't do that now.
 */

LEAF(_membar_acquire)
	j	ra
	 nop
END(_membar_acquire)
ATOMIC_OP_ALIAS(membar_acquire,_membar_acquire)

STRONG_ALIAS(_membar_consumer,_membar_acquire)
ATOMIC_OP_ALIAS(membar_consumer,_membar_acquire)

LEAF(_membar_release)
	j	ra
	 syncw
END(_membar_release)
ATOMIC_OP_ALIAS(membar_release,_membar_release)

STRONG_ALIAS(_membar_exit,_membar_release)
ATOMIC_OP_ALIAS(membar_exit,_membar_release)

STRONG_ALIAS(_membar_producer,_membar_release)
ATOMIC_OP_ALIAS(membar_producer,_membar_release)

#else  /* !__OCTEON__ */

STRONG_ALIAS(_membar_acquire,_membar_sync)
ATOMIC_OP_ALIAS(membar_acquire,_membar_sync)
STRONG_ALIAS(_membar_release,_membar_sync)
ATOMIC_OP_ALIAS(membar_release,_membar_sync)
STRONG_ALIAS(_membar_exit,_membar_sync)
ATOMIC_OP_ALIAS(membar_exit,_membar_sync)
STRONG_ALIAS(_membar_consumer,_membar_sync)
ATOMIC_OP_ALIAS(membar_consumer,_membar_sync)
STRONG_ALIAS(_membar_producer,_membar_sync)
ATOMIC_OP_ALIAS(membar_producer,_membar_sync)

#endif
