/*	$NetBSD: cache_mipsNN.c,v 1.11.78.6 2011/12/24 09:52:44 matt Exp $	*/

/*
 * Copyright 2001 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Jason R. Thorpe and Simon Burge for Wasabi Systems, Inc.
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
 *	This product includes software developed for the NetBSD Project by
 *	Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: cache_mipsNN.c,v 1.11.78.6 2011/12/24 09:52:44 matt Exp $");

#include <sys/param.h>

#include <mips/cache.h>
#include <mips/cache_r4k.h>
#include <mips/cache_mipsNN.h>
#include <mips/mipsNN.h>

#include <uvm/uvm_extern.h>

#define	round_line(x,n)		(((x) + (n) - 1) & -(n))
#define	trunc_line(x,n)		((x) & -(n))

#ifdef SB1250_PASS1
#define	SYNC	__asm volatile("sync; sync")
#else
#define	SYNC	__asm volatile("sync")
#endif

#ifdef __mips_o32
__asm(".set mips32");
#else
__asm(".set mips64");
#endif

static inline void
mipsNN_cache_op_intern(size_t line_size, u_int op, vaddr_t va, vsize_t size)
{
	vaddr_t eva;

	eva = round_line(va + size, line_size);
	va = trunc_line(va, line_size);

	for (; va + 32 * line_size <= eva; va += 32 * line_size) {
		cache_r4k_op_32lines_NN(line_size, va, op);
	}

	for (; va + 8 * line_size <= eva; va += 8 * line_size) {
		cache_r4k_op_8lines_NN(line_size, va, op);
	}

	for (;va < eva; va += line_size) {
		cache_op_r4k_line(va, op);
	}

	SYNC;
}

static void
mipsNN_intern_icache_index_inv_16(vaddr_t va, vaddr_t eva)
{
	const u_int op = CACHE_R4K_I|CACHEOP_R4K_INDEX_INV;
	const size_t line_size = 16;

	mipsNN_cache_op_intern(line_size, op, va, eva - va);
}

static void
mipsNN_intern_icache_index_inv_32(vaddr_t va, vaddr_t eva)
{
	const u_int op = CACHE_R4K_I|CACHEOP_R4K_INDEX_INV;
	const size_t line_size = 32;

	mipsNN_cache_op_intern(line_size, op, va, eva - va);
}

void
mipsNN_cache_init(uint32_t config, uint32_t config1)
{
	/* nothing to do */
}

void
mipsNN_icache_sync_all_16(void)
{
	struct mips_cache_info * const mci = &mips_cache_info;
	vaddr_t va = MIPS_KSEG0_START;
	const vaddr_t eva = va + mci->mci_picache_size;

	/*
	 * Since we're hitting the whole thing, we don't have to
	 * worry about the N different "ways".
	 */
	mips_intern_dcache_wbinv_all();

	mipsNN_intern_icache_index_inv_16(va, eva);
}

void
mipsNN_icache_sync_all_32(void)
{
	struct mips_cache_info * const mci = &mips_cache_info;
	vaddr_t va = MIPS_KSEG0_START;
	const vaddr_t eva = va + mci->mci_picache_size;

	/*
	 * Since we're hitting the whole thing, we don't have to
	 * worry about the N different "ways".
	 */
	mips_intern_dcache_wbinv_all();

	mipsNN_intern_icache_index_inv_32(va, eva);
}

void
mipsNN_icache_sync_range_16(vaddr_t va, vsize_t size)
{
	const u_int op = CACHE_R4K_I|CACHEOP_R4K_HIT_INV;
	const size_t line_size = 16;
	vaddr_t eva;

	eva = round_line(va + size, line_size);
	va = trunc_line(va, line_size);

	mips_intern_dcache_wb_range(va, eva - va);

	mipsNN_cache_op_intern(line_size, op, va, eva - va);
}

void
mipsNN_icache_sync_range_32(vaddr_t va, vsize_t size)
{
	const u_int op = CACHE_R4K_I|CACHEOP_R4K_HIT_INV;
	const size_t line_size = 32;
	vaddr_t eva;

	eva = round_line(va + size, line_size);
	va = trunc_line(va, line_size);

	mips_intern_dcache_wb_range(va, eva - va);

	mipsNN_cache_op_intern(line_size, op, va, eva - va);
}

void
mipsNN_icache_sync_range_index_16(vaddr_t va, vsize_t size)
{
	struct mips_cache_info * const mci = &mips_cache_info;
	const size_t way_size = mci->mci_picache_way_size;
	const size_t way_mask = way_size - 1;
	const size_t ways = mci->mci_picache_ways;
	const size_t line_size = 16;
	vaddr_t eva;

	/*
	 * Since we're doing Index ops, we expect to not be able
	 * to access the address we've been given.  So, get the
	 * bits that determine the cache index, and make a KSEG0
	 * address out of them.
	 */
	va = MIPS_PHYS_TO_KSEG0(va & way_mask);

	eva = round_line(va + size, line_size);
	va = trunc_line(va, line_size);

	/*
	 * If we are going to flush more than is in a way (or the stride
	 * need for that way), we are flushing everything.
	 */
	if (va + way_size >= eva) {
		mipsNN_icache_sync_all_16();
		return;
	}

	mips_intern_dcache_wbinv_range_index(va, eva - va);

	for (size_t way = 0; way < ways; way++) {
		mipsNN_intern_icache_index_inv_16(va, eva);
		va += way_size;
		eva += way_size;
	}
}

void
mipsNN_icache_sync_range_index_32(vaddr_t va, vsize_t size)
{
	struct mips_cache_info * const mci = &mips_cache_info;
	const size_t way_size = mci->mci_picache_way_size;
	const size_t way_mask = way_size - 1;
	const size_t ways = mci->mci_picache_ways;
	const size_t line_size = 32;
	vaddr_t eva;

	/*
	 * Since we're doing Index ops, we expect to not be able
	 * to access the address we've been given.  So, get the
	 * bits that determine the cache index, and make a KSEG0
	 * address out of them.
	 */
	va = MIPS_PHYS_TO_KSEG0(va & way_mask);

	eva = round_line(va + size, line_size);
	va = trunc_line(va, line_size);

	/*
	 * If we are going to flush more than is in a way (or the stride
	 * need for that way), we are flushing everything.
	 */
	if (va + way_size >= eva) {
		mipsNN_icache_sync_all_32();
		return;
	}

	mips_intern_dcache_wbinv_range_index(va, eva - va);

	for (size_t way = 0; way < ways; way++) {
		mipsNN_intern_icache_index_inv_32(va, eva);
		va += way_size;
		eva += way_size;
	}
}

void
mipsNN_pdcache_wbinv_all_16(void)
{
	struct mips_cache_info * const mci = &mips_cache_info;
	const u_int op = CACHE_R4K_D|CACHEOP_R4K_INDEX_WB_INV;
	const size_t line_size = 16;

	vaddr_t va = MIPS_KSEG0_START;
	const vaddr_t eva = va + mci->mci_pdcache_size;

	/*
	 * Since we're hitting the whole thing, we don't have to
	 * worry about the N different "ways".
	 */
	for (; va < eva; va += 32 * line_size) {
		cache_r4k_op_32lines_NN(line_size, va, op);
	}

	SYNC;
}

void
mipsNN_pdcache_wbinv_all_32(void)
{
	struct mips_cache_info * const mci = &mips_cache_info;
	const u_int op = CACHE_R4K_D|CACHEOP_R4K_INDEX_WB_INV;
	const size_t line_size = 32;

	vaddr_t va = MIPS_KSEG0_START;
	const vaddr_t eva = va + mci->mci_pdcache_size;

	/*
	 * Since we're hitting the whole thing, we don't have to
	 * worry about the N different "ways".
	 */
	for (; va < eva; va += 32 * line_size) {
		cache_r4k_op_32lines_NN(line_size, va, op);
	}

	SYNC;
}

static void
mipsNN_pdcache_wbinv_range_index_16_intern(vaddr_t va, vaddr_t eva)
{
	const u_int op = CACHE_R4K_D|CACHEOP_R4K_INDEX_WB_INV;
	const size_t line_size = 16;

	/*
	 * Since we're doing Index ops, we expect to not be able
	 * to access the address we've been given.  So, get the
	 * bits that determine the cache index, and make a KSEG0
	 * address out of them.
	 */
	va = MIPS_PHYS_TO_KSEG0(va);
	eva = MIPS_PHYS_TO_KSEG0(eva);

	mipsNN_cache_op_intern(line_size, op, va, eva - va);
}

static void
mipsNN_pdcache_wbinv_range_index_32_intern(vaddr_t va, vaddr_t eva)
{
	const u_int op = CACHE_R4K_D|CACHEOP_R4K_INDEX_WB_INV;
	const size_t line_size = 32;

	/*
	 * Since we're doing Index ops, we expect to not be able
	 * to access the address we've been given.  So, get the
	 * bits that determine the cache index, and make a KSEG0
	 * address out of them.
	 */
	va = MIPS_PHYS_TO_KSEG0(va);
	eva = MIPS_PHYS_TO_KSEG0(eva);

	mipsNN_cache_op_intern(line_size, op, va, eva - va);
}

void
mipsNN_pdcache_wbinv_range_index_16(vaddr_t va, vsize_t size)
{
	struct mips_cache_info * const mci = &mips_cache_info;
	const size_t ways = mci->mci_pdcache_ways;
	const vaddr_t way_size = mci->mci_pdcache_way_size;
	const vaddr_t way_mask = way_size - 1;
	const size_t line_size = 16;
	vaddr_t eva;

	va &= way_mask;
	eva = round_line(va + size, line_size);
	va = trunc_line(va, line_size);

	/*
	 * If we are going to flush more than is in a way, we are flushing
	 * everything.
	 */
	if (va + way_size >= eva) {
		mipsNN_pdcache_wbinv_all_16();
		return;
	}

	/*
	 * Invalidate each way.  If the address range wraps past the end of
	 * the way, we will be invalidating in two ways but eventually things
	 * work out since the last way will wrap into the first way.
	 */
	for (size_t way = 0; way < ways; way++) {
		mipsNN_pdcache_wbinv_range_index_16_intern(va, eva);
		va += way_size;
		eva += way_size;
	}
}
 
void
mipsNN_pdcache_wbinv_range_index_32(vaddr_t va, vsize_t size)
{
	struct mips_cache_info * const mci = &mips_cache_info;
	const size_t ways = mci->mci_pdcache_ways;
	const vaddr_t way_size = mci->mci_pdcache_way_size;
	const vaddr_t way_mask = way_size - 1;
	const size_t line_size = 32;
	vaddr_t eva;

	va &= way_mask;
	eva = round_line(va + size, line_size);
	va = trunc_line(va, line_size);

	/*
	 * If we are going to flush more than is in a way, we are flushing
	 * everything.
	 */
	if (va + way_size >= eva) {
		mipsNN_pdcache_wbinv_all_32();
		return;
	}

	/*
	 * Invalidate each way.  If the address range wraps past the end of
	 * the way, we will be invalidating in two ways but eventually things
	 * work out since the last way will wrap into the first way.
	 */
	for (size_t way = 0; way < ways; way++) {
		mipsNN_pdcache_wbinv_range_index_32_intern(va, eva);
		va += way_size;
		eva += way_size;
	}
}
 
void
mipsNN_pdcache_wbinv_range_16(vaddr_t va, vsize_t size)
{
	const u_int op = CACHE_R4K_D|CACHEOP_R4K_HIT_WB_INV;
	const size_t line_size = 16;

	mipsNN_cache_op_intern(line_size, op, va, size);
}

void
mipsNN_pdcache_wbinv_range_32(vaddr_t va, vsize_t size)
{
	const u_int op = CACHE_R4K_D|CACHEOP_R4K_HIT_WB_INV;
	const size_t line_size = 32;

	mipsNN_cache_op_intern(line_size, op, va, size);
}

void
mipsNN_pdcache_inv_range_16(vaddr_t va, vsize_t size)
{
	const u_int op = CACHE_R4K_D|CACHEOP_R4K_HIT_INV;
	const size_t line_size = 16;

	mipsNN_cache_op_intern(line_size, op, va, size);
}

void
mipsNN_pdcache_inv_range_32(vaddr_t va, vsize_t size)
{
	const u_int op = CACHE_R4K_D|CACHEOP_R4K_HIT_INV;
	const size_t line_size = 32;

	mipsNN_cache_op_intern(line_size, op, va, size);
}

void
mipsNN_pdcache_wb_range_16(vaddr_t va, vsize_t size)
{
	const u_int op = CACHE_R4K_D|CACHEOP_R4K_HIT_WB;
	const size_t line_size = 16;

	mipsNN_cache_op_intern(line_size, op, va, size);
}

void
mipsNN_pdcache_wb_range_32(vaddr_t va, vsize_t size)
{
	const u_int op = CACHE_R4K_D|CACHEOP_R4K_HIT_WB;
	const size_t line_size = 32;

	mipsNN_cache_op_intern(line_size, op, va, size);
}
