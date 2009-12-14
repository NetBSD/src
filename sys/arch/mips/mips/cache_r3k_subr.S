/*	$NetBSD: cache_r3k_subr.S,v 1.5 2009/12/14 00:46:05 matt Exp $	*/

/*
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Digital Equipment Corporation and Ralph Campbell.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permited provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * Copyright (C) 1989 Digital Equipment Corporation.
 * Permission to use, copy, modify, and distribute this software and
 * its documentation for any purpose and without fee is hereby granted,
 * provided that the above copyright notice appears in all copies.
 * Digital Equipment Corporation makes no representations about the
 * suitability of this software for any purpose.  It is provided "as is"
 * without express or implied warranty.
 *
 * from: Header: /sprite/src/kernel/mach/ds3100.md/RCS/loMem.s,
 *	v 1.1 89/07/11 17:55:04 nelson Exp  SPRITE (DECWRL)
 * from: Header: /sprite/src/kernel/mach/ds3100.md/RCS/machAsm.s,
 *	v 9.2 90/01/29 18:00:39 shirriff Exp  SPRITE (DECWRL)
 * from: Header: /sprite/src/kernel/vm/ds3100.md/vmPmaxAsm.s,
 *	v 1.1 89/07/10 14:27:41 nelson Exp  SPRITE (DECWRL)
 *
 *	@(#)locore.s	8.5 (Berkeley) 1/4/94
 */

#include <mips/asm.h>
#include <mips/cpuregs.h>

#include "assym.h"

	.set noreorder

/*
 * r3k_size_cache:
 *
 *	This works by writing marker values repeatedly at possible
 *	cache boundaries (256K to 512).  Since the cache is direct-mapped
 *	and the address ranges have aliased with given cache size,
 *	whatever value is found at cache location zero indicates the size.
 */
LEAF_NOPROFILE(r3k_size_cache)
	la	v0, 1f
	or	v0, MIPS_KSEG1_START
	j	v0				# run uncached
	mfc0	v1, MIPS_COP_0_STATUS
1:
	mtc0	a0, MIPS_COP_0_STATUS		# disable interrupts,
	nop					# isolating cache
	nop
	nop
	nop

	li	t1, 512				# minimum cache size
	li	t2, 256*1024			# maximum cache size

2:	sw	t2, MIPS_KSEG0_START(t2)
	bne	t2, t1, 2b
	srl	t2, 1				# BDSLOT
	lw	v0, MIPS_KSEG0_START(zero)	# magic! cache size is there

	li	t1, MIPS_KSEG0_START
	addu	t2, t1, v0

3:	sw	zero, 0(t1)			# clear parity
	sw	zero, 4(t1)
	sw	zero, 8(t1)
	sw	zero, 12(t1)
	sb	zero, 0(t1)			# invalidate
	sb	zero, 4(t1)
	sb	zero, 8(t1)
	addu	t1, 16
	bne	t1, t2, 3b
	sb	zero, -4(t1)			# BDSLOT

	# 4 cycles to pipeline to drain.
	nop
	nop
	nop
	nop

	mtc0	v1, MIPS_COP_0_STATUS
	nop
	nop
	nop
	nop

	j	ra
	nop
END(r3k_size_cache)

/*
 * r3k_picache_size:
 *
 *	Determine the size of the R3000 I-cache.
 */
LEAF_NOPROFILE(r3k_picache_size)
	li	a0, MIPS1_ISOL_CACHES|MIPS1_SWAP_CACHES
	j	_C_LABEL(r3k_size_cache)
	nop
END(r3k_picache_size)

/*
 * r3k_pdcache_size:
 *
 *	Determine the size of the R3000 D-cache.
 */
LEAF_NOPROFILE(r3k_pdcache_size)
	li	a0, MIPS1_ISOL_CACHES
	j	_C_LABEL(r3k_size_cache)
	nop
END(r3k_pdcache_size)

/*
 * r3k_picache_do_inv:
 *
 *	Invalidate a range from the R3000 I-cache.
 *
 *	Arguments:	a0	starting address
 *			a1	ending address
 *
 *	Addresses must already be cache-line aligned.
 */
LEAF(r3k_picache_do_inv)
	mfc0	t0, MIPS_COP_0_STATUS		# Save SR.
	mtc0	zero, MIPS_COP_0_STATUS		# Disable interrupts.

	la	v1, 1f
	or	v1, MIPS_KSEG1_START		# Run uncached.
	j	v1
	nop

1:
	# Swap the caches (so we store to I$) and isolate them from memory.
	li	v1, MIPS_SR_ISOL_CACHES | MIPS_SR_SWAP_CACHES
	mtc0	v1, MIPS_COP_0_STATUS
	nop
	nop
1:
	addu	a0, a0, 4
	bne	a0, a1, 1b
	sb	zero, -4(a0)

	# 4 cycles to wait for pipeline to drain.

	nop
	nop

	mtc0	t0, MIPS_COP_0_STATUS		# Restore SR.
	nop
	nop
	j	ra				# Jumps back to cached address.
	nop
END(r3k_picache_do_inv)

/*
 * r3k_pdcache_do_inv:
 *
 *	Invalidate a range from the R3000 D-cache.
 *
 *	Arguments:	a0	starting address
 *			a1	ending address
 *
 *	Addresses must already be cache-line aligned.
 */
LEAF(r3k_pdcache_do_inv)
	mfc0	t0, MIPS_COP_0_STATUS		# Save SR.
	mtc0	zero, MIPS_COP_0_STATUS		# Disable interrupts.
	nop

	# Isolate the caches from memory.
	li	v1, MIPS_SR_ISOL_CACHES
	mtc0	v1, MIPS_COP_0_STATUS
	nop
	nop
1:
	sb	zero, 0(a0)
	sb	zero, 4(a0)
	sb	zero, 8(a0)
	sb	zero, 12(a0)
	sb	zero, 16(a0)
	sb	zero, 20(a0)
	sb	zero, 24(a0)
	addu	a0, 32
	bltu	a0, a1, 1b
	sb	zero, -4(a0)

	# 4 cycles to wait for pipeline to drain.

	nop
	nop

	mtc0	t0, MIPS_COP_0_STATUS		# Restore SR.
	nop
	nop
	j	ra				# Jumps back to cached address.
	nop
END(r3k_pdcache_do_inv)
