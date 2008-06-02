/*	$NetBSD: cache_tx39_subr.S,v 1.2.122.1 2008/06/02 13:22:24 mjf Exp $	*/

/*-
 * Copyright (c) 2000, 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by UCHIYAMA Yasushi; and by Jason R. Thorpe.
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

/*
 * XXX THIS IS NOT YET COMPLETE.
 */

#include <mips/asm.h>
#include <mips/cpuregs.h>
#include <mips/r3900regs.h>
#include <mips/cache_tx39.h>

	.set noreorder

/*
 * tx3900_icache_do_inv_index_16:
 *
 *	Do an Index Invalidate of the I-cache for the specified
 *	range.
 *
 *	Arguments:	a0	starting address
 *			a1	ending address
 *
 *	Addresses must already be cache-line aligned.
 */
LEAF(tx3900_icache_do_inv_index_16)
	mfc0	t0, MIPS_COP_0_STATUS		# Save SR.
	nop
	mtc0	zero, MIPS_COP_0_STATUS		# Disable interrupts.
	nop

	# Disable the I-cache.
	li	t1, ~R3900_CONFIG_ICE
	mfc0	t2, R3900_COP_0_CONFIG
	and	t1, t1, t2
	nop
	mtc0	t1, R3900_COP_0_CONFIG

	# Break the I-stream.
	j	1f
	nop

	.set push
	.set mips3
1:
	cache	CACHE_TX39_I|CACHEOP_TX3900_INDEX_INV, 0(a0)
	addiu	a0, a0, 16
	bltu	a0, a1, 1b
	nop
	.set pop

	# Re-enable I-cache.
	nop
	mtc0	t2, R3900_COP_0_CONFIG
	nop
	mtc0	t0, MIPS_COP_0_STATUS		# Restore SR.

	j	ra
	nop
END(tx3900_icache_do_inv_index_16)

/*
 * tx3920_icache_do_inv_16:
 *
 *	Do a Hit Invalidate of the I-cache for the specified
 *	range.
 *
 *	Arguments:	a0	starting address
 *			a1	ending address
 *
 *	Addresses must already be cache-line aligned.
 */
LEAF(tx3920_icache_do_inv_16)
	mfc0	t0, MIPS_COP_0_STATUS		# Save SR.
	nop
	mtc0	zero, MIPS_COP_0_STATUS		# Disable interrupts.
	nop

	# Disable the I-cache.
	li	t1, ~R3900_CONFIG_ICE
	mfc0	t2, R3900_COP_0_CONFIG
	and	t1, t1, t2
	nop
	mtc0	t1, R3900_COP_0_CONFIG

	# Break the I-stream.
	j	1f
	nop

	.set push
	.set mips3
1:
	cache	CACHE_TX39_I|CACHEOP_TX3920_HIT_INV, 0(a0)
	addiu	a0, a0, 16
	bltu	a0, a1, 1b
	nop
	.set pop

	# Re-enable I-cache.
	nop
	mtc0	t2, R3900_COP_0_CONFIG
	nop
	mtc0	t0, MIPS_COP_0_STATUS		# Restore SR.

	j	ra
	nop
END(tx3920_icache_do_inv_16)
