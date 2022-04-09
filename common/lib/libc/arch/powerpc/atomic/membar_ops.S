/*	$NetBSD: membar_ops.S,v 1.6 2022/04/09 23:32:52 riastradh Exp $	*/

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

__RCSID("$NetBSD: membar_ops.S,v 1.6 2022/04/09 23:32:52 riastradh Exp $")

	.text

ENTRY(_membar_acquire)
	/*
	 * It is tempting to use isync to order load-before-load/store.
	 * However, isync orders prior loads only if their value flows
	 * into a control-flow dependency prior to the isync:
	 *
	 *	`[I]f an isync follows a conditional Branch instruction
	 *	 that depends on the value returned by a preceding Load
	 *	 instruction, the load on which the Branch depends is
	 *	 performed before any loads caused by instructions
	 *	 following the isync. This applies even if the effects
	 *	 of the ``dependency'' are independent of the value
	 *	 loaded (e.g., the value is compared to itself and the
	 *	 Branch tests the EQ bit in the selected CR field), and
	 *	 even if the branch target is the sequentially next
	 *	 instruction.'
	 *
	 *	--PowerPC Virtual Environment Architecture, Book II,
	 *	  Version 2.01, December 2003, 1.7.1 `Storage Access
	 *	  Ordering', p. 7.
	 *
	 * We are required here, however, to order _all_ prior loads,
	 * even if they do not flow into any control flow dependency.
	 * For example:
	 *
	 *	x = *p;
	 *	membar_acquire();
	 *	if (x) goto foo;
	 *
	 * This can't be implemented by:
	 *
	 *	lwz	x, p
	 *	isync
	 *	cmpwi	x, 0
	 *	bne	foo
	 *
	 * isync doesn't work here because there's no conditional
	 * dependency on x between the lwz x, p and the isync.
	 *
	 * isync would only work if it followed the branch:
	 *
	 *	lwz	x, p
	 *	isync
	 *	cmpwi	x, 0
	 *	bne	foo
	 *	...
	 * foo:	isync
	 *	...
	 *
	 * lwsync orders everything except store-before-load, so it
	 * serves here -- see below in membar_release in lwsync.
	 * Except we can't use it on booke, so use sync for now.
	 */
	sync
	blr
END(_membar_acquire)
ATOMIC_OP_ALIAS(membar_acquire,_membar_acquire)

ENTRY(_membar_release)
	/*
	 *	`The memory barrier provides an ordering function for
	 *	 the storage accesses caused by Load, Store, and dcbz
	 *	 instructions that are executed by the processor
	 *	 executing the [lwsync] instruction and for which the
	 *	 specified storage location is in storage that is
	 *	 Memory Coherence Required and is neither Write Through
	 *	 Required nor Caching Inhibited.  The applicable pairs
	 *	 are all pairs a_i, b_j of such accesses except those
	 *	 in which a_i is an access caused by a Store or dcbz
	 *	 instruction and b_j is an access caused by a Load
	 *	 instruction.'
	 *
	 *	--PowerPC Virtual Environment Architecture, Book II,
	 *	  Version 2.01, December 2003, 3.3.3 `Memory Barrier
	 *	  Instructions', p. 25.
	 *
	 * In brief, lwsync is an acquire-release barrier -- it orders
	 * load-before-load/store and load/store-before-store, but not
	 * store-before-load.  Except we can't use it on booke, so use
	 * sync for now.
	 */
	sync
	blr
END(_membar_release)
ATOMIC_OP_ALIAS(membar_release,_membar_release)

ENTRY(_membar_sync)
	/*
	 * sync, or `heavyweight sync', is a full sequential
	 * consistency barrier.
	 */
	sync
	blr
END(_membar_sync)
ATOMIC_OP_ALIAS(membar_sync,_membar_sync)

ATOMIC_OP_ALIAS(membar_producer,_membar_release)
STRONG_ALIAS(_membar_producer,_membar_release)
ATOMIC_OP_ALIAS(membar_consumer,_membar_acquire)
STRONG_ALIAS(_membar_consumer,_membar_acquire)
ATOMIC_OP_ALIAS(membar_enter,_membar_sync)
STRONG_ALIAS(_membar_enter,_membar_sync)
ATOMIC_OP_ALIAS(membar_exit,_membar_release)
STRONG_ALIAS(_membar_exit,_membar_release)
