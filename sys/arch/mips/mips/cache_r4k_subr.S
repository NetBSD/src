/*-
 * Copyright (c) 2011 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Matt Thomas of 3am Software Foundry.
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

#ifdef __mips_n32
/*
 * Treat N32 as N64
 */
#undef _MIPS_SZPTR
#define _MIPS_SZPTR 64
#endif

#include <mips/asm.h>

RCSID("$NetBSD: cache_r4k_subr.S,v 1.2.4.2 2016/10/05 20:55:32 skrll Exp $")

#include <mips/cache_r4k.h>

#ifndef LINE_SHIFT
#error LINE_SHIFT undefined
#endif

#ifdef __mips_o32
	.set	mips32
#else
	.set	mips3
#endif

#define	LINE_SIZE		(1 << LINE_SHIFT)

#ifdef __STDC__
#define	_CONCAT3(a,b,c)		a ## _ ## b ## _ ## c
#define	_CONCAT4(a,b,c,d)	a ## _ ## b ## _ ## c ## _ ## d
#else
#define	_CONCAT3(a,b,c)		a/**/_/**/b/**/_/**/c
#define	_CONCAT4(a,b,c,d)	a/**/_/**/b/**/_/**/c/**/_/**/d
#endif

#define	_OPNAME(a,b,c)		_CONCAT3(a,b,c)
#define	_OPNAME32(a,b,c)	_CONCAT4(a,b,32lines,c)

#ifndef OPNAME
#define OPNAME is not defined!
#endif
#ifndef OPNAME32
#define OPNAME32 is not defined!
#endif

STATIC_LEAF_NOPROFILE(OPNAME(doop))
	.set	push
	.set	noreorder
	/*
	 * Jump in the table to do from 0 to 32 cache ops
	 * t9 = first op @ 31*LINE_SIZE(a0)
	 * a0 = starting va (may be unaligned)
	 * a1 = size in bytes (may be unaligned)
	 */
	and	v1, a0, LINE_SIZE - 1		# get offset in cache line
	xor	a0, v1				# align start to cache line
	PTR_ADDU a1, v1				# add offset to size
	PTR_ADDU a1, LINE_SIZE - 1		# size is now rouned
	PTR_SRL	t0, a1, LINE_SHIFT		# get # of cache lines

	beqz	t0, 3f				# bail if t0 is 0 (no lines)
	 PTR_SRL v0, t0, 5			# v0 = # of (32x lines)
	beqz	v0, 2f				# must have < 32 sets
	 nop					# delay slot
	/*
	 * We have 1 more sets of 32 cache lines to deal with.  The number of
	 * sets is in v0.  Keep calling the 32op routines until we run out of
	 * sets.  We have to save ra since the jalr will overwrite it.
	 */
	move	ta0, ra				# save return address
1:
	jalr	t9				# do 32 cache ops (31..0)
	 PTR_ADDU v0, -1			# decrement 32x set count
	bnez	v0, 1b				# do next 32x set if > 0
	 PTR_ADDU a0, 32*LINE_SIZE		# adjust address by 32 lines
	/*
	 * Now we have < 32 sets so need to remove the number of 32x line sets
	 * from the of lines we have left to do.
	 */
	andi	t0, 31				# t0 = # of lines left
	beqz	t0, 3f				# no more to do?
	 move	ra, ta0				# restore return address
2:
	PTR_SLL	t0, INT_SCALESHIFT		# shift to instruction index
	PTR_ADDU t9, 32 << INT_SCALESHIFT	# point to end of table
	PTR_SUBU t9, t0				# backup N cacheops
	jr	t9				# and do them.
	 nop
3:
	jr	ra
	 nop
	.set	pop
END(OPNAME(doop))

#ifdef WANT_ICACHE_OPS
LEAF_NOPROFILE(OPNAME(icache_index_inv))
	PTR_LA	t9, OPNAME32(icache_index_inv)	# get address of first op
	b	OPNAME(doop)
END(OPNAME(icache_index_inv))

LEAF_NOPROFILE(OPNAME(icache_hit_inv))
	PTR_LA	t9, OPNAME32(icache_hit_inv)	# get address of first op
	b	OPNAME(doop)
END(OPNAME(icache_hit_inv))
#endif /* WANT_ICACHE_OPS */

#ifdef WANT_PDCACHE_OPS
LEAF_NOPROFILE(OPNAME(pdcache_index_wb_inv))
	PTR_LA	t9, OPNAME32(pdcache_index_wb_inv) # get address of first op
	b	OPNAME(doop)
END(OPNAME(pdcache_index_wb_inv))

LEAF_NOPROFILE(OPNAME(pdcache_hit_inv))
	PTR_LA	t9, OPNAME32(pdcache_hit_inv)	# get address of first op
	b	OPNAME(doop)
END(OPNAME(pdcache_hit_inv))

LEAF_NOPROFILE(OPNAME(pdcache_hit_wb_inv))
	PTR_LA	t9, OPNAME32(pdcache_hit_wb_inv) # get address of first op
	b	OPNAME(doop)
END(OPNAME(pdcache_hit_wb_inv))

LEAF_NOPROFILE(OPNAME(pdcache_hit_wb))
	PTR_LA	t9, OPNAME32(pdcache_hit_wb)	# get address of first op
	b	OPNAME(doop)
END(OPNAME(pdcache_hit_wb))
#endif /* WANT_PDCACHE_OPS */

#ifdef WANT_SDCACHE_OPS
LEAF_NOPROFILE(OPNAME(sdcache_index_wb_inv))
	PTR_LA	t9, OPNAME32(sdcache_index_wb_inv) # get address of first op
	b	OPNAME(doop)
END(OPNAME(sdcache_index_wb_inv))

LEAF_NOPROFILE(OPNAME(sdcache_hit_inv))
	PTR_LA	t9, OPNAME32(sdcache_hit_inv)	# get address of first op
	b	OPNAME(doop)
END(OPNAME(sdcache_hit_inv))

LEAF_NOPROFILE(OPNAME(sdcache_hit_wb_inv))
	PTR_LA	t9, OPNAME32(sdcache_hit_wb_inv) # get address of first op
	b	OPNAME(doop)
END(OPNAME(sdcache_hit_wb_inv))

LEAF_NOPROFILE(OPNAME(sdcache_hit_wb))
	PTR_LA	t9, OPNAME32(sdcache_hit_wb)	# get address of first op
	b	OPNAME(doop)
END(OPNAME(sdcache_hit_wb))
#endif /* WANT_SDCACHE_OPS */

/*
 * Macro to emit 32 cache instruction with the right op and offset in the
 * proper order.
 */
	.macro	emitops	name, op, start=31
	.ifeq	\start - 31
	.p2align 4
STATIC_LEAF_NOPROFILE(\name)
	.endif
	cache	\op, (\start*LINE_SIZE)(a0)
	.ifgt	\start
	emitops	\name, \op, "(\start-1)"
	.else
	j	ra
	 sync
END(\name)
	.endif
	.endm

	.set	noreorder
#ifdef WANT_ICACHE_OPS
	emitops	OPNAME32(icache_index_inv), CACHE_R4K_I|CACHEOP_R4K_INDEX_INV
	emitops	OPNAME32(icache_hit_inv), CACHE_R4K_I|CACHEOP_R4K_HIT_INV
#endif

#ifdef WANT_PDCACHE_OPS
	emitops	OPNAME32(pdcache_index_wb_inv), CACHE_R4K_D|CACHEOP_R4K_INDEX_WB_INV
	emitops	OPNAME32(pdcache_hit_inv), CACHE_R4K_D|CACHEOP_R4K_HIT_INV
	emitops	OPNAME32(pdcache_hit_wb_inv), CACHE_R4K_D|CACHEOP_R4K_HIT_WB_INV
	emitops	OPNAME32(pdcache_hit_wb), CACHE_R4K_D|CACHEOP_R4K_HIT_WB
#endif

#ifdef WANT_SDCACHE_OPS
	emitops	OPNAME32(sdcache_index_wb_inv), CACHE_R4K_SD|CACHEOP_R4K_INDEX_WB_INV
	emitops	OPNAME32(sdcache_hit_inv), CACHE_R4K_SD|CACHEOP_R4K_HIT_INV
	emitops	OPNAME32(sdcache_hit_wb_inv), CACHE_R4K_SD|CACHEOP_R4K_HIT_WB_INV
	emitops	OPNAME32(sdcache_hit_wb), CACHE_R4K_SD|CACHEOP_R4K_HIT_WB
#endif
