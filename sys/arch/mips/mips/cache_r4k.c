/*	$NetBSD: cache_r4k.c,v 1.10 2005/12/24 20:07:19 perry Exp $	*/

/*
 * Copyright 2001 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Jason R. Thorpe for Wasabi Systems, Inc.
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
__KERNEL_RCSID(0, "$NetBSD: cache_r4k.c,v 1.10 2005/12/24 20:07:19 perry Exp $");

#include <sys/param.h>

#include <mips/cache.h>
#include <mips/cache_r4k.h>

/*
 * Cache operations for R4000/R4400-style caches:
 *
 *	- Direct-mapped
 *	- Write-back
 *	- Virtually indexed, physically tagged
 *
 * XXX Does not handle split secondary caches.
 */

#define	round_line(x)		(((x) + 15) & ~15)
#define	trunc_line(x)		((x) & ~15)

__asm(".set mips3");

void
r4k_icache_sync_all_16(void)
{
	vaddr_t va = MIPS_PHYS_TO_KSEG0(0);
	vaddr_t eva = va + mips_picache_size;

	mips_dcache_wbinv_all();

	__asm volatile("sync");

	while (va < eva) {
		cache_r4k_op_32lines_16(va, CACHE_R4K_I|CACHEOP_R4K_INDEX_INV);
		va += (32 * 16);
	}
}

void
r4k_icache_sync_range_16(vaddr_t va, vsize_t size)
{
	vaddr_t eva = round_line(va + size);

	va = trunc_line(va);

	mips_dcache_wb_range(va, (eva - va));

	__asm volatile("sync");

	while ((eva - va) >= (32 * 16)) {
		cache_r4k_op_32lines_16(va, CACHE_R4K_I|CACHEOP_R4K_HIT_INV);
		va += (32 * 16);
	}

	while (va < eva) {
		cache_op_r4k_line(va, CACHE_R4K_I|CACHEOP_R4K_HIT_INV);
		va += 16;
	}
}

void
r4k_icache_sync_range_index_16(vaddr_t va, vsize_t size)
{
	vaddr_t eva, orig_va;

	orig_va = va;

	eva = round_line(va + size);
	va = trunc_line(va);

	mips_dcache_wbinv_range_index(va, (eva - va));

	__asm volatile("sync");

	/*
	 * Since we're doing Index ops, we expect to not be able
	 * to access the address we've been given.  So, get the
	 * bits that determine the cache index, and make a KSEG0
	 * address out of them.
	 */
	va = MIPS_PHYS_TO_KSEG0(orig_va & mips_picache_way_mask);

	eva = round_line(va + size);
	va = trunc_line(va);

	while ((eva - va) >= (32 * 16)) {
		cache_r4k_op_32lines_16(va, CACHE_R4K_I|CACHEOP_R4K_INDEX_INV);
		va += (32 * 16);
	}

	while (va < eva) {
		cache_op_r4k_line(va, CACHE_R4K_I|CACHEOP_R4K_INDEX_INV);
		va += 16;
	}
}

void
r4k_pdcache_wbinv_all_16(void)
{
	vaddr_t va = MIPS_PHYS_TO_KSEG0(0);
	vaddr_t eva = va + mips_pdcache_size;

	while (va < eva) {
		cache_r4k_op_32lines_16(va,
		    CACHE_R4K_D|CACHEOP_R4K_INDEX_WB_INV);
		va += (32 * 16);
	}
}

void
r4k_pdcache_wbinv_range_16(vaddr_t va, vsize_t size)
{
	vaddr_t eva = round_line(va + size);

	va = trunc_line(va);

	while ((eva - va) >= (32 * 16)) {
		cache_r4k_op_32lines_16(va,
		    CACHE_R4K_D|CACHEOP_R4K_HIT_WB_INV);
		va += (32 * 16);
	}

	while (va < eva) {
		cache_op_r4k_line(va, CACHE_R4K_D|CACHEOP_R4K_HIT_WB_INV);
		va += 16;
	}
}

void
r4k_pdcache_wbinv_range_index_16(vaddr_t va, vsize_t size)
{
	vaddr_t eva;

	/*
	 * Since we're doing Index ops, we expect to not be able
	 * to access the address we've been given.  So, get the
	 * bits that determine the cache index, and make a KSEG0
	 * address out of them.
	 */
	va = MIPS_PHYS_TO_KSEG0(va & (mips_pdcache_size - 1));

	eva = round_line(va + size);
	va = trunc_line(va);

	while ((eva - va) >= (32 * 16)) {
		cache_r4k_op_32lines_16(va,
		    CACHE_R4K_D|CACHEOP_R4K_INDEX_WB_INV);
		va += (32 * 16);
	}

	while (va < eva) {
		cache_op_r4k_line(va, CACHE_R4K_D|CACHEOP_R4K_INDEX_WB_INV);
		va += 16;
	}
}

void
r4k_pdcache_inv_range_16(vaddr_t va, vsize_t size)
{
	vaddr_t eva = round_line(va + size);

	va = trunc_line(va);

	while ((eva - va) >= (32 * 16)) {
		cache_r4k_op_32lines_16(va, CACHE_R4K_D|CACHEOP_R4K_HIT_INV);
		va += (32 * 16);
	}

	while (va < eva) {
		cache_op_r4k_line(va, CACHE_R4K_D|CACHEOP_R4K_HIT_INV);
		va += 16;
	}
}

void
r4k_pdcache_wb_range_16(vaddr_t va, vsize_t size)
{
	vaddr_t eva = round_line(va + size);

	va = trunc_line(va);

	while ((eva - va) >= (32 * 16)) {
		cache_r4k_op_32lines_16(va, CACHE_R4K_D|CACHEOP_R4K_HIT_WB);
		va += (32 * 16);
	}

	while (va < eva) {
		cache_op_r4k_line(va, CACHE_R4K_D|CACHEOP_R4K_HIT_WB);
		va += 16;
	}
}

#undef round_line
#undef trunc_line

#define	round_line(x)		(((x) + 31) & ~31)
#define	trunc_line(x)		((x) & ~31)

void
r4k_icache_sync_all_32(void)
{
	vaddr_t va = MIPS_PHYS_TO_KSEG0(0);
	vaddr_t eva = va + mips_picache_size;

	mips_dcache_wbinv_all();

	__asm volatile("sync");

	while (va < eva) {
		cache_r4k_op_32lines_32(va, CACHE_R4K_I|CACHEOP_R4K_INDEX_INV);
		va += (32 * 32);
	}
}

void
r4k_icache_sync_range_32(vaddr_t va, vsize_t size)
{
	vaddr_t eva = round_line(va + size);

	va = trunc_line(va);

	mips_dcache_wb_range(va, (eva - va));

	__asm volatile("sync");

	while ((eva - va) >= (32 * 32)) {
		cache_r4k_op_32lines_32(va, CACHE_R4K_I|CACHEOP_R4K_HIT_INV);
		va += (32 * 32);
	}

	while (va < eva) {
		cache_op_r4k_line(va, CACHE_R4K_I|CACHEOP_R4K_HIT_INV);
		va += 32;
	}
}

void
r4k_icache_sync_range_index_32(vaddr_t va, vsize_t size)
{
	vaddr_t eva;

	eva = round_line(va + size);
	va = trunc_line(va);

	mips_dcache_wbinv_range_index(va, (eva - va));

	__asm volatile("sync");

	/*
	 * Since we're doing Index ops, we expect to not be able
	 * to access the address we've been given.  So, get the
	 * bits that determine the cache index, and make a KSEG0
	 * address out of them.
	 */
	va = MIPS_PHYS_TO_KSEG0(va & mips_picache_way_mask);

	eva = round_line(va + size);
	va = trunc_line(va);

	while ((eva - va) >= (32 * 32)) {
		cache_r4k_op_32lines_32(va, CACHE_R4K_I|CACHEOP_R4K_INDEX_INV);
		va += (32 * 32);
	}

	while (va < eva) {
		cache_op_r4k_line(va, CACHE_R4K_I|CACHEOP_R4K_INDEX_INV);
		va += 32;
	}
}

void
r4k_pdcache_wbinv_all_32(void)
{
	vaddr_t va = MIPS_PHYS_TO_KSEG0(0);
	vaddr_t eva = va + mips_pdcache_size;

	while (va < eva) {
		cache_r4k_op_32lines_32(va,
		    CACHE_R4K_D|CACHEOP_R4K_INDEX_WB_INV);
		va += (32 * 32);
	}
}

void
r4k_pdcache_wbinv_range_32(vaddr_t va, vsize_t size)
{
	vaddr_t eva = round_line(va + size);

	va = trunc_line(va);

	while ((eva - va) >= (32 * 32)) {
		cache_r4k_op_32lines_32(va,
		    CACHE_R4K_D|CACHEOP_R4K_HIT_WB_INV);
		va += (32 * 32);
	}

	while (va < eva) {
		cache_op_r4k_line(va, CACHE_R4K_D|CACHEOP_R4K_HIT_WB_INV);
		va += 32;
	}
}

void
r4k_pdcache_wbinv_range_index_32(vaddr_t va, vsize_t size)
{
	vaddr_t eva;

	/*
	 * Since we're doing Index ops, we expect to not be able
	 * to access the address we've been given.  So, get the
	 * bits that determine the cache index, and make a KSEG0
	 * address out of them.
	 */
	va = MIPS_PHYS_TO_KSEG0(va & (mips_pdcache_size - 1));

	eva = round_line(va + size);
	va = trunc_line(va);

	while ((eva - va) >= (32 * 32)) {
		cache_r4k_op_32lines_32(va,
		    CACHE_R4K_D|CACHEOP_R4K_INDEX_WB_INV);
		va += (32 * 32);
	}

	while (va < eva) {
		cache_op_r4k_line(va, CACHE_R4K_D|CACHEOP_R4K_INDEX_WB_INV);
		va += 32;
	}
}

void
r4k_pdcache_inv_range_32(vaddr_t va, vsize_t size)
{
	vaddr_t eva = round_line(va + size);

	va = trunc_line(va);

	while ((eva - va) >= (32 * 32)) {
		cache_r4k_op_32lines_32(va, CACHE_R4K_D|CACHEOP_R4K_HIT_INV);
		va += (32 * 32);
	}

	while (va < eva) {
		cache_op_r4k_line(va, CACHE_R4K_D|CACHEOP_R4K_HIT_INV);
		va += 32;
	}
}

void
r4k_pdcache_wb_range_32(vaddr_t va, vsize_t size)
{
	vaddr_t eva = round_line(va + size);

	va = trunc_line(va);

	while ((eva - va) >= (32 * 32)) {
		cache_r4k_op_32lines_32(va, CACHE_R4K_D|CACHEOP_R4K_HIT_WB);
		va += (32 * 32);
	}

	while (va < eva) {
		cache_op_r4k_line(va, CACHE_R4K_D|CACHEOP_R4K_HIT_WB);
		va += 32;
	}
}

void
r4k_sdcache_wbinv_all_32(void)
{
	vaddr_t va = MIPS_PHYS_TO_KSEG0(0);
	vaddr_t eva = va + mips_sdcache_size;

	while (va < eva) {
		cache_r4k_op_32lines_32(va,
		    CACHE_R4K_SD|CACHEOP_R4K_INDEX_WB_INV);
		va += (32 * 32);
	}
}

void
r4k_sdcache_wbinv_range_32(vaddr_t va, vsize_t size)
{
	vaddr_t eva = round_line(va + size);

	va = trunc_line(va);

	while ((eva - va) >= (32 * 32)) {
		cache_r4k_op_32lines_32(va,
		    CACHE_R4K_SD|CACHEOP_R4K_HIT_WB_INV);
		va += (32 * 32);
	}

	while (va < eva) {
		cache_op_r4k_line(va, CACHE_R4K_SD|CACHEOP_R4K_HIT_WB_INV);
		va += 32;
	}
}

void
r4k_sdcache_wbinv_range_index_32(vaddr_t va, vsize_t size)
{
	vaddr_t eva;

	/*
	 * Since we're doing Index ops, we expect to not be able
	 * to access the address we've been given.  So, get the
	 * bits that determine the cache index, and make a KSEG0
	 * address out of them.
	 */
	va = MIPS_PHYS_TO_KSEG0(va & (mips_sdcache_size - 1));

	eva = round_line(va + size);
	va = trunc_line(va);

	while ((eva - va) >= (32 * 32)) {
		cache_r4k_op_32lines_32(va,
		    CACHE_R4K_SD|CACHEOP_R4K_INDEX_WB_INV);
		va += (32 * 32);
	}

	while (va < eva) {
		cache_op_r4k_line(va, CACHE_R4K_SD|CACHEOP_R4K_INDEX_WB_INV);
		va += 32;
	}
}

void
r4k_sdcache_inv_range_32(vaddr_t va, vsize_t size)
{
	vaddr_t eva = round_line(va + size);

	va = trunc_line(va);

	while ((eva - va) >= (32 * 32)) {
		cache_r4k_op_32lines_32(va, CACHE_R4K_SD|CACHEOP_R4K_HIT_INV);
		va += (32 * 32);
	}

	while (va < eva) {
		cache_op_r4k_line(va, CACHE_R4K_SD|CACHEOP_R4K_HIT_INV);
		va += 32;
	}
}

void
r4k_sdcache_wb_range_32(vaddr_t va, vsize_t size)
{
	vaddr_t eva = round_line(va + size);

	va = trunc_line(va);

	while ((eva - va) >= (32 * 32)) {
		cache_r4k_op_32lines_32(va, CACHE_R4K_SD|CACHEOP_R4K_HIT_WB);
		va += (32 * 32);
	}

	while (va < eva) {
		cache_op_r4k_line(va, CACHE_R4K_SD|CACHEOP_R4K_HIT_WB);
		va += 32;
	}
}

#undef round_line
#undef trunc_line

#define	round_line(x)		(((x) + 127) & ~127)
#define	trunc_line(x)		((x) & ~127)

void
r4k_sdcache_wbinv_all_128(void)
{
	vaddr_t va = MIPS_PHYS_TO_KSEG0(0);
	vaddr_t eva = va + mips_sdcache_size;

	while (va < eva) {
		cache_r4k_op_32lines_128(va,
		    CACHE_R4K_SD|CACHEOP_R4K_INDEX_WB_INV);
		va += (32 * 128);
	}
}

void
r4k_sdcache_wbinv_range_128(vaddr_t va, vsize_t size)
{
	vaddr_t eva = round_line(va + size);

	va = trunc_line(va);

	while ((eva - va) >= (32 * 128)) {
		cache_r4k_op_32lines_128(va,
		    CACHE_R4K_SD|CACHEOP_R4K_HIT_WB_INV);
		va += (32 * 128);
	}

	while (va < eva) {
		cache_op_r4k_line(va, CACHE_R4K_SD|CACHEOP_R4K_HIT_WB_INV);
		va += 128;
	}
}

void
r4k_sdcache_wbinv_range_index_128(vaddr_t va, vsize_t size)
{
	vaddr_t eva;

	/*
	 * Since we're doing Index ops, we expect to not be able
	 * to access the address we've been given.  So, get the
	 * bits that determine the cache index, and make a KSEG0
	 * address out of them.
	 */
	va = MIPS_PHYS_TO_KSEG0(va & (mips_sdcache_size - 1));

	eva = round_line(va + size);
	va = trunc_line(va);

	while ((eva - va) >= (32 * 128)) {
		cache_r4k_op_32lines_128(va,
		    CACHE_R4K_SD|CACHEOP_R4K_INDEX_WB_INV);
		va += (32 * 128);
	}

	while (va < eva) {
		cache_op_r4k_line(va, CACHE_R4K_SD|CACHEOP_R4K_INDEX_WB_INV);
		va += 128;
	}
}

void
r4k_sdcache_inv_range_128(vaddr_t va, vsize_t size)
{
	vaddr_t eva = round_line(va + size);

	va = trunc_line(va);

	while ((eva - va) >= (32 * 128)) {
		cache_r4k_op_32lines_128(va, CACHE_R4K_SD|CACHEOP_R4K_HIT_INV);
		va += (32 * 128);
	}

	while (va < eva) {
		cache_op_r4k_line(va, CACHE_R4K_SD|CACHEOP_R4K_HIT_INV);
		va += 128;
	}
}

void
r4k_sdcache_wb_range_128(vaddr_t va, vsize_t size)
{
	vaddr_t eva = round_line(va + size);

	va = trunc_line(va);

	while ((eva - va) >= (32 * 128)) {
		cache_r4k_op_32lines_128(va, CACHE_R4K_SD|CACHEOP_R4K_HIT_WB);
		va += (32 * 128);
	}

	while (va < eva) {
		cache_op_r4k_line(va, CACHE_R4K_SD|CACHEOP_R4K_HIT_WB);
		va += 128;
	}
}

#undef round_line
#undef trunc_line

#define	round_line(x)		(((x) + mips_sdcache_line_size - 1) & ~(mips_sdcache_line_size - 1))
#define	trunc_line(x)		((x) & ~(mips_sdcache_line_size - 1))

void
r4k_sdcache_wbinv_all_generic(void)
{
	vaddr_t va = MIPS_PHYS_TO_KSEG0(0);
	vaddr_t eva = va + mips_sdcache_size;
	int line_size = mips_sdcache_line_size;

	while (va < eva) {
		cache_op_r4k_line(va, CACHE_R4K_SD|CACHEOP_R4K_INDEX_WB_INV);
		va += line_size;
	}
}

void
r4k_sdcache_wbinv_range_generic(vaddr_t va, vsize_t size)
{
	vaddr_t eva = round_line(va + size);
	int line_size = mips_sdcache_line_size;

	va = trunc_line(va);

	while (va < eva) {
		cache_op_r4k_line(va, CACHE_R4K_SD|CACHEOP_R4K_HIT_WB_INV);
		va += line_size;
	}
}

void
r4k_sdcache_wbinv_range_index_generic(vaddr_t va, vsize_t size)
{
	vaddr_t eva;
	int line_size = mips_sdcache_line_size;

	/*
	 * Since we're doing Index ops, we expect to not be able
	 * to access the address we've been given.  So, get the
	 * bits that determine the cache index, and make a KSEG0
	 * address out of them.
	 */
	va = MIPS_PHYS_TO_KSEG0(va & (mips_sdcache_size - 1));

	eva = round_line(va + size);
	va = trunc_line(va);

	while (va < eva) {
		cache_op_r4k_line(va, CACHE_R4K_SD|CACHEOP_R4K_INDEX_WB_INV);
		va += line_size;
	}
}

void
r4k_sdcache_inv_range_generic(vaddr_t va, vsize_t size)
{
	vaddr_t eva = round_line(va + size);
	int line_size = mips_sdcache_line_size;

	va = trunc_line(va);

	while (va < eva) {
		cache_op_r4k_line(va, CACHE_R4K_SD|CACHEOP_R4K_HIT_INV);
		va += line_size;
	}
}

void
r4k_sdcache_wb_range_generic(vaddr_t va, vsize_t size)
{
	vaddr_t eva = round_line(va + size);
	int line_size = mips_sdcache_line_size;

	va = trunc_line(va);

	while (va < eva) {
		cache_op_r4k_line(va, CACHE_R4K_SD|CACHEOP_R4K_HIT_WB);
		va += line_size;
	}
}

#undef round_line
#undef trunc_line
