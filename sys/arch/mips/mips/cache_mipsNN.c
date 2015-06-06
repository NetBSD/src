/*	$NetBSD: cache_mipsNN.c,v 1.14.32.1 2015/06/06 14:40:02 skrll Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: cache_mipsNN.c,v 1.14.32.1 2015/06/06 14:40:02 skrll Exp $");

#include <sys/param.h>

#include <mips/cache.h>
#include <mips/cache_r4k.h>
#include <mips/cache_mipsNN.h>
#include <mips/mipsNN.h>

#include <uvm/uvm_extern.h>

#define	round_line16(x)		(((x) + 15L) & -16L)
#define	trunc_line16(x)		((x) & -16L)

#define	round_line32(x)		(((x) + 31L) & -32L)
#define	trunc_line32(x)		((x) & -32L)


#ifdef SB1250_PASS1
#define	SYNC	__asm volatile("sync; sync")
#else
#define	SYNC	__asm volatile("sync")
#endif

#ifdef __mips_o32
__asm(".set mips32");
#elif !defined(__mips64)
__asm(".set mips64");
#endif

static int picache_stride;
static int picache_loopcount;

void
mipsNN_cache_init(uint32_t config, uint32_t config1)
{
	struct mips_cache_info * const mci = &mips_cache_info;
	bool flush_multiple_lines_per_way;

	flush_multiple_lines_per_way = mci->mci_picache_way_size > PAGE_SIZE;
	if (config & MIPSNN_CFG_VI) {
		/*
		 * With a virtual Icache we don't need to flush
		 * multiples of the page size with index ops; we just
		 * need to flush one pages' worth.
		 */
		flush_multiple_lines_per_way = false;
	}

	if (flush_multiple_lines_per_way) {
		picache_stride = PAGE_SIZE;
		picache_loopcount = (mci->mci_picache_way_size / PAGE_SIZE) *
		    mci->mci_picache_ways;
	} else {
		picache_stride = mci->mci_picache_way_size;
		picache_loopcount = mci->mci_picache_ways;
	}

#define CACHE_DEBUG
#ifdef CACHE_DEBUG
	if (config & MIPSNN_CFG_VI)
		printf("  icache is virtual\n");
	printf("  picache_stride    = %d\n", picache_stride);
	printf("  picache_loopcount = %d\n", picache_loopcount);
#endif
}

void
mipsNN_icache_sync_all_16(void)
{
	struct mips_cache_info * const mci = &mips_cache_info;
	vaddr_t va, eva;

	va = MIPS_PHYS_TO_KSEG0(0);
	eva = va + mci->mci_picache_size;

	/*
	 * Since we're hitting the whole thing, we don't have to
	 * worry about the N different "ways".
	 */

	mips_intern_dcache_wbinv_all();

	while (va < eva) {
		cache_r4k_op_32lines_16(va, CACHE_R4K_I|CACHEOP_R4K_INDEX_INV);
		va += (32 * 16);
	}

	SYNC;
}

void
mipsNN_icache_sync_all_32(void)
{
	struct mips_cache_info * const mci = &mips_cache_info;
	vaddr_t va, eva;

	va = MIPS_PHYS_TO_KSEG0(0);
	eva = va + mci->mci_picache_size;

	/*
	 * Since we're hitting the whole thing, we don't have to
	 * worry about the N different "ways".
	 */

	mips_intern_dcache_wbinv_all();

	while (va < eva) {
		cache_r4k_op_32lines_32(va, CACHE_R4K_I|CACHEOP_R4K_INDEX_INV);
		va += (32 * 32);
	}

	SYNC;
}

void
mipsNN_icache_sync_range_16(vaddr_t va, vsize_t size)
{
	vaddr_t eva;

	eva = round_line16(va + size);
	va = trunc_line16(va);

	mips_intern_dcache_wb_range(va, (eva - va));

	while ((eva - va) >= (32 * 16)) {
		cache_r4k_op_32lines_16(va, CACHE_R4K_I|CACHEOP_R4K_HIT_INV);
		va += (32 * 16);
	}

	while (va < eva) {
		cache_op_r4k_line(va, CACHE_R4K_I|CACHEOP_R4K_HIT_INV);
		va += 16;
	}

	SYNC;
}

void
mipsNN_icache_sync_range_32(vaddr_t va, vsize_t size)
{
	vaddr_t eva;

	eva = round_line32(va + size);
	va = trunc_line32(va);

	mips_intern_dcache_wb_range(va, (eva - va));

	while ((eva - va) >= (32 * 32)) {
		cache_r4k_op_32lines_32(va, CACHE_R4K_I|CACHEOP_R4K_HIT_INV);
		va += (32 * 32);
	}

	while (va < eva) {
		cache_op_r4k_line(va, CACHE_R4K_I|CACHEOP_R4K_HIT_INV);
		va += 32;
	}

	SYNC;
}

void
mipsNN_icache_sync_range_index_16(vaddr_t va, vsize_t size)
{
	struct mips_cache_info * const mci = &mips_cache_info;
	vaddr_t eva, tmpva;
	int i, stride, loopcount;

	/*
	 * Since we're doing Index ops, we expect to not be able
	 * to access the address we've been given.  So, get the
	 * bits that determine the cache index, and make a KSEG0
	 * address out of them.
	 */
	va = MIPS_PHYS_TO_KSEG0(va & mci->mci_picache_way_mask);

	eva = round_line16(va + size);
	va = trunc_line16(va);

	/*
	 * If we are going to flush more than is in a way, we are flushing
	 * everything.
	 */
	if (eva - va >= mci->mci_picache_way_size) {
		mipsNN_icache_sync_all_16();
		return;
	}

	/*
	 * GCC generates better code in the loops if we reference local
	 * copies of these global variables.
	 */
	stride = picache_stride;
	loopcount = picache_loopcount;

	mips_intern_dcache_wbinv_range_index(va, (eva - va));

	while ((eva - va) >= (8 * 16)) {
		tmpva = va;
		for (i = 0; i < loopcount; i++, tmpva += stride) {
			cache_r4k_op_8lines_16(tmpva,
			    CACHE_R4K_I|CACHEOP_R4K_INDEX_INV);
		}
		va += 8 * 16;
	}

	while (va < eva) {
		tmpva = va;
		for (i = 0; i < loopcount; i++, tmpva += stride) {
			cache_op_r4k_line(tmpva,
			    CACHE_R4K_I|CACHEOP_R4K_INDEX_INV);
		}
		va += 16;
	}
}

void
mipsNN_icache_sync_range_index_32(vaddr_t va, vsize_t size)
{
	struct mips_cache_info * const mci = &mips_cache_info;
	vaddr_t eva, tmpva;
	int i, stride, loopcount;

	/*
	 * Since we're doing Index ops, we expect to not be able
	 * to access the address we've been given.  So, get the
	 * bits that determine the cache index, and make a KSEG0
	 * address out of them.
	 */
	va = MIPS_PHYS_TO_KSEG0(va & mci->mci_picache_way_mask);

	eva = round_line32(va + size);
	va = trunc_line32(va);

	/*
	 * If we are going to flush more than is in a way, we are flushing
	 * everything.
	 */
	if (eva - va >= mci->mci_picache_way_size) {
		mipsNN_icache_sync_all_32();
		return;
	}

	/*
	 * GCC generates better code in the loops if we reference local
	 * copies of these global variables.
	 */
	stride = picache_stride;
	loopcount = picache_loopcount;

	mips_intern_dcache_wbinv_range_index(va, (eva - va));

	while ((eva - va) >= (8 * 32)) {
		tmpva = va;
		for (i = 0; i < loopcount; i++, tmpva += stride) {
			cache_r4k_op_8lines_32(tmpva,
			    CACHE_R4K_I|CACHEOP_R4K_INDEX_INV);
		}
		va += 8 * 32;
	}

	while (va < eva) {
		tmpva = va;
		for (i = 0; i < loopcount; i++, tmpva += stride) {
			cache_op_r4k_line(tmpva,
			    CACHE_R4K_I|CACHEOP_R4K_INDEX_INV);
		}
		va += 32;
	}
}

void
mipsNN_pdcache_wbinv_all_16(void)
{
	struct mips_cache_info * const mci = &mips_cache_info;
	vaddr_t va, eva;

	va = MIPS_PHYS_TO_KSEG0(0);
	eva = va + mci->mci_pdcache_size;

	/*
	 * Since we're hitting the whole thing, we don't have to
	 * worry about the N different "ways".
	 */

	while (va < eva) {
		cache_r4k_op_32lines_16(va,
		    CACHE_R4K_D|CACHEOP_R4K_INDEX_WB_INV);
		va += (32 * 16);
	}

	SYNC;
}

void
mipsNN_pdcache_wbinv_all_32(void)
{
	struct mips_cache_info * const mci = &mips_cache_info;
	vaddr_t va, eva;

	va = MIPS_PHYS_TO_KSEG0(0);
	eva = va + mci->mci_pdcache_size;

	/*
	 * Since we're hitting the whole thing, we don't have to
	 * worry about the N different "ways".
	 */

	while (va < eva) {
		cache_r4k_op_32lines_32(va,
		    CACHE_R4K_D|CACHEOP_R4K_INDEX_WB_INV);
		va += (32 * 32);
	}

	SYNC;
}

void
mipsNN_pdcache_wbinv_range_16(vaddr_t va, vsize_t size)
{
	vaddr_t eva;

	eva = round_line16(va + size);
	va = trunc_line16(va);

	while ((eva - va) >= (32 * 16)) {
		cache_r4k_op_32lines_16(va,
		    CACHE_R4K_D|CACHEOP_R4K_HIT_WB_INV);
		va += (32 * 16);
	}

	while (va < eva) {
		cache_op_r4k_line(va, CACHE_R4K_D|CACHEOP_R4K_HIT_WB_INV);
		va += 16;
	}

	SYNC;
}

void
mipsNN_pdcache_wbinv_range_32(vaddr_t va, vsize_t size)
{
	vaddr_t eva;

	eva = round_line32(va + size);
	va = trunc_line32(va);

	while ((eva - va) >= (32 * 32)) {
		cache_r4k_op_32lines_32(va,
		    CACHE_R4K_D|CACHEOP_R4K_HIT_WB_INV);
		va += (32 * 32);
	}

	while (va < eva) {
		cache_op_r4k_line(va, CACHE_R4K_D|CACHEOP_R4K_HIT_WB_INV);
		va += 32;
	}

	SYNC;
}

static void
mipsNN_pdcache_wbinv_range_index_16_intern(vaddr_t va, vaddr_t eva)
{
	/*
	 * Since we're doing Index ops, we expect to not be able
	 * to access the address we've been given.  So, get the
	 * bits that determine the cache index, and make a KSEG0
	 * address out of them.
	 */
	va = MIPS_PHYS_TO_KSEG0(va);
	eva = MIPS_PHYS_TO_KSEG0(eva);

	for (; (eva - va) >= (8 * 16); va += 8 * 16) {
		cache_r4k_op_8lines_16(va,
		    CACHE_R4K_D|CACHEOP_R4K_INDEX_WB_INV);
	}

	for (; va < eva; va += 16) {
		cache_op_r4k_line(va, CACHE_R4K_D|CACHEOP_R4K_INDEX_WB_INV);
	}
}

static void
mipsNN_pdcache_wbinv_range_index_32_intern(vaddr_t va, vaddr_t eva)
{
	/*
	 * Since we're doing Index ops, we expect to not be able
	 * to access the address we've been given.  So, get the
	 * bits that determine the cache index, and make a KSEG0
	 * address out of them.
	 */
	va = MIPS_PHYS_TO_KSEG0(va);
	eva = MIPS_PHYS_TO_KSEG0(eva);

	for (; (eva - va) >= (8 * 32); va += 8 * 32) {
		cache_r4k_op_8lines_32(va,
		    CACHE_R4K_D|CACHEOP_R4K_INDEX_WB_INV);
	}

	for (; va < eva; va += 32) {
		cache_op_r4k_line(va, CACHE_R4K_D|CACHEOP_R4K_INDEX_WB_INV);
	}
}

void
mipsNN_pdcache_wbinv_range_index_16(vaddr_t va, vsize_t size)
{
	struct mips_cache_info * const mci = &mips_cache_info;
	const vaddr_t way_size = mci->mci_pdcache_way_size;
	const vaddr_t way_mask = way_size - 1;
	const u_int ways = mci->mci_pdcache_ways;
	vaddr_t eva;

	va &= way_mask;
	eva = round_line16(va + size);
	va = trunc_line16(va);

	/*
	 * If we are going to flush more than is in a way, we are flushing
	 * everything.
	 */
	if (eva - va >= way_size) {
		mipsNN_pdcache_wbinv_all_16();
		return;
	}

	/*
	 * Invalidate each way.  If the address range wraps past the end of
	 * the way, we will be invalidating in two ways but eventually things
	 * work out since the last way will wrap into the first way.
	 */
	for (u_int way = 0; way < ways; way++) {
		mipsNN_pdcache_wbinv_range_index_16_intern(va, eva);
		va += way_size;
		eva += way_size;
	}
}
 
void
mipsNN_pdcache_wbinv_range_index_32(vaddr_t va, vsize_t size)
{
	struct mips_cache_info * const mci = &mips_cache_info;
	const vaddr_t way_size = mci->mci_pdcache_way_size;
	const vaddr_t way_mask = way_size - 1;
	const u_int ways = mci->mci_pdcache_ways;
	vaddr_t eva;

	va &= way_mask;
	eva = round_line32(va + size);
	va = trunc_line32(va);

	/*
	 * If we are going to flush more than is in a way, we are flushing
	 * everything.
	 */
	if (eva - va >= way_size) {
		mipsNN_pdcache_wbinv_all_32();
		return;
	}

	/*
	 * Invalidate each way.  If the address range wraps past the end of
	 * the way, we will be invalidating in two ways but eventually things
	 * work out since the last way will wrap into the first way.
	 */
	for (u_int way = 0; way < ways; way++) {
		mipsNN_pdcache_wbinv_range_index_32_intern(va, eva);
		va += way_size;
		eva += way_size;
	}
}
 
void
mipsNN_pdcache_inv_range_16(vaddr_t va, vsize_t size)
{
	vaddr_t eva;

	eva = round_line16(va + size);
	va = trunc_line16(va);

	while ((eva - va) >= (32 * 16)) {
		cache_r4k_op_32lines_16(va, CACHE_R4K_D|CACHEOP_R4K_HIT_INV);
		va += (32 * 16);
	}

	while (va < eva) {
		cache_op_r4k_line(va, CACHE_R4K_D|CACHEOP_R4K_HIT_INV);
		va += 16;
	}

	SYNC;
}

void
mipsNN_pdcache_inv_range_32(vaddr_t va, vsize_t size)
{
	vaddr_t eva;

	eva = round_line32(va + size);
	va = trunc_line32(va);

	while ((eva - va) >= (32 * 32)) {
		cache_r4k_op_32lines_32(va, CACHE_R4K_D|CACHEOP_R4K_HIT_INV);
		va += (32 * 32);
	}

	while (va < eva) {
		cache_op_r4k_line(va, CACHE_R4K_D|CACHEOP_R4K_HIT_INV);
		va += 32;
	}

	SYNC;
}

void
mipsNN_pdcache_wb_range_16(vaddr_t va, vsize_t size)
{
	vaddr_t eva;

	eva = round_line16(va + size);
	va = trunc_line16(va);

	while ((eva - va) >= (32 * 16)) {
		cache_r4k_op_32lines_16(va, CACHE_R4K_D|CACHEOP_R4K_HIT_WB);
		va += (32 * 16);
	}

	while (va < eva) {
		cache_op_r4k_line(va, CACHE_R4K_D|CACHEOP_R4K_HIT_WB);
		va += 16;
	}

	SYNC;
}

void
mipsNN_pdcache_wb_range_32(vaddr_t va, vsize_t size)
{
	vaddr_t eva;

	eva = round_line32(va + size);
	va = trunc_line32(va);

	while ((eva - va) >= (32 * 32)) {
		cache_r4k_op_32lines_32(va, CACHE_R4K_D|CACHEOP_R4K_HIT_WB);
		va += (32 * 32);
	}

	while (va < eva) {
		cache_op_r4k_line(va, CACHE_R4K_D|CACHEOP_R4K_HIT_WB);
		va += 32;
	}

	SYNC;
}
