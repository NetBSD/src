/*	$NetBSD: membar_ops.S,v 1.14 2026/05/21 08:07:39 kbowling Exp $	*/

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
 * The CN50XX HRM (CN50XX-HM-0.99E, July 2008) does not document this
 * guarantee explicitly, but Section 4.8 (p. 161) only describes
 * stores as reorderable, the cores are in-order with write-through L1,
 * and the Cavium SDK provides no read/acquire barrier either.
 * Experimentally, on an erlite3 (CN5020-500), store-before-store and
 * store-before-load reordering are easily detected, but no
 * load-before-load or load-before-store reordering has been observed.
 *
 * On CN3xxx/CN5xxx (Octeon I/Plus), errata Core-401 can cause a
 * single syncw to fail to enforce store ordering under rare
 * conditions.  Two syncw instructions in a row are needed as a
 * workaround.  This erratum was fixed in Octeon II (CN6xxx), so a
 * single syncw suffices there.
 *
 * Note: on all cnMIPS, the write buffer aggressively merges stores
 * and a releasing store (e.g. lock release) can linger for hundreds
 * of thousands of cycles before becoming visible to other cores.
 * A separate syncw "plunger" is needed _after_ the releasing store
 * to drain the write buffer promptly (CN50XX-HRM p. 943, CN78XX-HRM
 * p. 2168).  That concern is handled by SYNC_PLUNGER in asm.h at
 * the lock release sites, not here.  membar_release runs _before_
 * the releasing store, so it cannot drain a store that hasn't
 * happened yet.
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
#if defined(_MIPS_ARCH_OCTEON2)
	j	ra
	 syncw
#else
	/* Two syncw for errata Core-401 (CN3xxx/CN5xxx) + write buffer drain */
	syncw
	j	ra
	 syncw
#endif
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
