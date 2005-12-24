/*	$NetBSD: cache_mipsNN.c,v 1.10 2005/12/24 20:07:19 perry Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: cache_mipsNN.c,v 1.10 2005/12/24 20:07:19 perry Exp $");

#include <sys/param.h>

#include <mips/cache.h>
#include <mips/cache_r4k.h>
#include <mips/cache_mipsNN.h>
#include <mips/mipsNN.h>

#include <uvm/uvm_extern.h>

#define	round_line16(x)		(((x) + 15) & ~15)
#define	trunc_line16(x)		((x) & ~15)

#define	round_line32(x)		(((x) + 31) & ~31)
#define	trunc_line32(x)		((x) & ~31)


#ifdef SB1250_PASS1
#define	SYNC	__asm volatile("sync; sync")
#else
#define	SYNC	__asm volatile("sync")
#endif

__asm(".set mips32");

static int picache_stride;
static int picache_loopcount;
static int pdcache_stride;
static int pdcache_loopcount;

void
mipsNN_cache_init(uint32_t config, uint32_t config1)
{
	int flush_multiple_lines_per_way;

	flush_multiple_lines_per_way = mips_picache_way_size > PAGE_SIZE;
	if (config & MIPSNN_CFG_VI) {
		/*
		 * With a virtual Icache we don't need to flush
		 * multiples of the page size with index ops; we just
		 * need to flush one pages' worth.
		 */
		flush_multiple_lines_per_way = 0;
	}

	if (flush_multiple_lines_per_way) {
		picache_stride = PAGE_SIZE;
		picache_loopcount = (mips_picache_way_size / PAGE_SIZE) *
		    mips_picache_ways;
	} else {
		picache_stride = mips_picache_way_size;
		picache_loopcount = mips_picache_ways;
	}

	if (mips_pdcache_way_size < PAGE_SIZE) {
		pdcache_stride = mips_pdcache_way_size;
		pdcache_loopcount = mips_pdcache_ways;
	} else {
		pdcache_stride = PAGE_SIZE;
		pdcache_loopcount = (mips_pdcache_way_size / PAGE_SIZE) *
		    mips_pdcache_ways;
	}
#define CACHE_DEBUG
#ifdef CACHE_DEBUG
	if (config & MIPSNN_CFG_VI)
		printf("  icache is virtual\n");
	printf("  picache_stride    = %d\n", picache_stride);
	printf("  picache_loopcount = %d\n", picache_loopcount);
	printf("  pdcache_stride    = %d\n", pdcache_stride);
	printf("  pdcache_loopcount = %d\n", pdcache_loopcount);
#endif
}

void
mipsNN_icache_sync_all_16(void)
{
	vaddr_t va, eva;

	va = MIPS_PHYS_TO_KSEG0(0);
	eva = va + mips_picache_size;

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
	vaddr_t va, eva;

	va = MIPS_PHYS_TO_KSEG0(0);
	eva = va + mips_picache_size;

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
	unsigned int eva, tmpva;
	int i, stride, loopcount;

	/*
	 * Since we're doing Index ops, we expect to not be able
	 * to access the address we've been given.  So, get the
	 * bits that determine the cache index, and make a KSEG0
	 * address out of them.
	 */
	va = MIPS_PHYS_TO_KSEG0(va & mips_picache_way_mask);

	eva = round_line16(va + size);
	va = trunc_line16(va);

	/*
	 * GCC generates better code in the loops if we reference local
	 * copies of these global variables.
	 */
	stride = picache_stride;
	loopcount = picache_loopcount;

	mips_intern_dcache_wbinv_range_index(va, (eva - va));

	while ((eva - va) >= (8 * 16)) {
		tmpva = va;
		for (i = 0; i < loopcount; i++, tmpva += stride)
			cache_r4k_op_8lines_16(tmpva,
			    CACHE_R4K_I|CACHEOP_R4K_INDEX_INV);
		va += 8 * 16;
	}

	while (va < eva) {
		tmpva = va;
		for (i = 0; i < loopcount; i++, tmpva += stride)
			cache_op_r4k_line(tmpva,
			    CACHE_R4K_I|CACHEOP_R4K_INDEX_INV);
		va += 16;
	}
}

void
mipsNN_icache_sync_range_index_32(vaddr_t va, vsize_t size)
{
	unsigned int eva, tmpva;
	int i, stride, loopcount;

	/*
	 * Since we're doing Index ops, we expect to not be able
	 * to access the address we've been given.  So, get the
	 * bits that determine the cache index, and make a KSEG0
	 * address out of them.
	 */
	va = MIPS_PHYS_TO_KSEG0(va & mips_picache_way_mask);

	eva = round_line32(va + size);
	va = trunc_line32(va);

	/*
	 * GCC generates better code in the loops if we reference local
	 * copies of these global variables.
	 */
	stride = picache_stride;
	loopcount = picache_loopcount;

	mips_intern_dcache_wbinv_range_index(va, (eva - va));

	while ((eva - va) >= (8 * 32)) {
		tmpva = va;
		for (i = 0; i < loopcount; i++, tmpva += stride)
			cache_r4k_op_8lines_32(tmpva,
			    CACHE_R4K_I|CACHEOP_R4K_INDEX_INV);
		va += 8 * 32;
	}

	while (va < eva) {
		tmpva = va;
		for (i = 0; i < loopcount; i++, tmpva += stride)
			cache_op_r4k_line(tmpva,
			    CACHE_R4K_I|CACHEOP_R4K_INDEX_INV);
		va += 32;
	}
}

void
mipsNN_pdcache_wbinv_all_16(void)
{
	vaddr_t va, eva;

	va = MIPS_PHYS_TO_KSEG0(0);
	eva = va + mips_pdcache_size;

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
	vaddr_t va, eva;

	va = MIPS_PHYS_TO_KSEG0(0);
	eva = va + mips_pdcache_size;

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

void
mipsNN_pdcache_wbinv_range_index_16(vaddr_t va, vsize_t size)
{
	unsigned int eva, tmpva;
	int i, stride, loopcount;

	/*
	 * Since we're doing Index ops, we expect to not be able
	 * to access the address we've been given.  So, get the
	 * bits that determine the cache index, and make a KSEG0
	 * address out of them.
	 */
	va = MIPS_PHYS_TO_KSEG0(va & mips_pdcache_way_mask);

	eva = round_line16(va + size);
	va = trunc_line16(va);

	/*
	 * GCC generates better code in the loops if we reference local
	 * copies of these global variables.
	 */
	stride = pdcache_stride;
	loopcount = pdcache_loopcount;

	while ((eva - va) >= (8 * 16)) {
		tmpva = va;
		for (i = 0; i < loopcount; i++, tmpva += stride)
			cache_r4k_op_8lines_16(tmpva,
			    CACHE_R4K_D|CACHEOP_R4K_INDEX_WB_INV);
		va += 8 * 16;
	}

	while (va < eva) {
		tmpva = va;
		for (i = 0; i < loopcount; i++, tmpva += stride)
			cache_op_r4k_line(tmpva,
			    CACHE_R4K_D|CACHEOP_R4K_INDEX_WB_INV);
		va += 16;
	}
}

void
mipsNN_pdcache_wbinv_range_index_32(vaddr_t va, vsize_t size)
{
	unsigned int eva, tmpva;
	int i, stride, loopcount;

	/*
	 * Since we're doing Index ops, we expect to not be able
	 * to access the address we've been given.  So, get the
	 * bits that determine the cache index, and make a KSEG0
	 * address out of them.
	 */
	va = MIPS_PHYS_TO_KSEG0(va & mips_pdcache_way_mask);

	eva = round_line32(va + size);
	va = trunc_line32(va);

	/*
	 * GCC generates better code in the loops if we reference local
	 * copies of these global variables.
	 */
	stride = pdcache_stride;
	loopcount = pdcache_loopcount;

	while ((eva - va) >= (8 * 32)) {
		tmpva = va;
		for (i = 0; i < loopcount; i++, tmpva += stride)
			cache_r4k_op_8lines_32(tmpva,
			    CACHE_R4K_D|CACHEOP_R4K_INDEX_WB_INV);
		va += 8 * 32;
	}

	while (va < eva) {
		tmpva = va;
		for (i = 0; i < loopcount; i++, tmpva += stride)
			cache_op_r4k_line(tmpva,
			    CACHE_R4K_D|CACHEOP_R4K_INDEX_WB_INV);
		va += 32;
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
