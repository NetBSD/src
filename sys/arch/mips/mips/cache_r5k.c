/*	$NetBSD: cache_r5k.c,v 1.13 2010/11/13 09:22:10 uebayasi Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: cache_r5k.c,v 1.13 2010/11/13 09:22:10 uebayasi Exp $");

#include <sys/param.h>

#include <mips/cache.h>
#include <mips/cache_r4k.h>
#include <mips/cache_r5k.h>
#include <mips/locore.h>

/*
 * Cache operations for R5000-style caches:
 *
 *	- 2-way set-associative
 *	- Write-back
 *	- Virtually indexed, physically tagged
 *
 * Since the R4600 is so similar (2-way set-associative, 32b/l),
 * we handle that here, too.  Note for R4600, we have to work
 * around some chip bugs.  From the v1.7 errata:
 *
 *  18. The CACHE instructions Hit_Writeback_Invalidate_D, Hit_Writeback_D,
 *      Hit_Invalidate_D and Create_Dirty_Excl_D should only be
 *      executed if there is no other dcache activity. If the dcache is
 *      accessed for another instruction immeidately preceding when these
 *      cache instructions are executing, it is possible that the dcache
 *      tag match outputs used by these cache instructions will be
 *      incorrect. These cache instructions should be preceded by at least
 *      four instructions that are not any kind of load or store
 *      instruction.
 *
 * ...and from the v2.0 errata:
 *
 *   The CACHE instructions Hit_Writeback_Inv_D, Hit_Writeback_D,
 *   Hit_Invalidate_D and Create_Dirty_Exclusive_D will only operate
 *   correctly if the internal data cache refill buffer is empty.  These
 *   CACHE instructions should be separated from any potential data cache
 *   miss by a load instruction to an uncached address to empty the response
 *   buffer.
 *
 * XXX Does not handle split secondary caches.
 */

#define	round_line16(x)		(((x) + 15) & ~15)
#define	trunc_line16(x)		((x) & ~15)
#define	round_line(x)		(((x) + 31) & ~31)
#define	trunc_line(x)		((x) & ~31)

__asm(".set mips3");

void
r5k_icache_sync_all_32(void)
{
	vaddr_t va = MIPS_PHYS_TO_KSEG0(0);
	vaddr_t eva = va + mips_picache_size;

	/*
	 * Since we're hitting the whole thing, we don't have to
	 * worry about the 2 different "ways".
	 */

	mips_dcache_wbinv_all();

	__asm volatile("sync");

	while (va < eva) {
		cache_r4k_op_32lines_32(va, CACHE_R4K_I|CACHEOP_R4K_INDEX_INV);
		va += (32 * 32);
	}
}

void
r5k_icache_sync_range_32(vaddr_t va, vsize_t size)
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
r5k_icache_sync_range_index_32(vaddr_t va, vsize_t size)
{
	vaddr_t w2va, eva, orig_va;

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
	w2va = va + mips_picache_way_size;

	while ((eva - va) >= (16 * 32)) {
		cache_r4k_op_16lines_32_2way(va, w2va,
		    CACHE_R4K_I|CACHEOP_R4K_INDEX_INV);
		va   += (16 * 32);
		w2va += (16 * 32);
	}

	while (va < eva) {
		cache_op_r4k_line(  va, CACHE_R4K_I|CACHEOP_R4K_INDEX_INV);
		cache_op_r4k_line(w2va, CACHE_R4K_I|CACHEOP_R4K_INDEX_INV);
		va   += 32;
		w2va += 32;
	}
}

void
r5k_pdcache_wbinv_all_16(void)
{
	vaddr_t va = MIPS_PHYS_TO_KSEG0(0);
	vaddr_t eva = va + mips_pdcache_size;

	/*
	 * Since we're hitting the whole thing, we don't have to
	 * worry about the 2 different "ways".
	 */

	while (va < eva) {
		cache_r4k_op_32lines_16(va,
		    CACHE_R4K_D|CACHEOP_R4K_INDEX_WB_INV);
		va += (32 * 16);
	}
}

void
r5k_pdcache_wbinv_all_32(void)
{
	vaddr_t va = MIPS_PHYS_TO_KSEG0(0);
	vaddr_t eva = va + mips_pdcache_size;

	/*
	 * Since we're hitting the whole thing, we don't have to
	 * worry about the 2 different "ways".
	 */

	while (va < eva) {
		cache_r4k_op_32lines_32(va,
		    CACHE_R4K_D|CACHEOP_R4K_INDEX_WB_INV);
		va += (32 * 32);
	}
}

void
r4600v1_pdcache_wbinv_range_32(vaddr_t va, vsize_t size)
{
	vaddr_t eva = round_line(va + size);
	uint32_t ostatus;

	/*
	 * This is pathetically slow, but the chip bug is pretty
	 * nasty, and we hope that not too many v1.x R4600s are
	 * around.
	 */

	va = trunc_line(va);

	/*
	 * To make this a little less painful, just hit the entire
	 * cache if we have a range >= the cache size.
	 */
	if ((eva - va) >= mips_pdcache_size) {
		r5k_pdcache_wbinv_all_32();
		return;
	}

	ostatus = mips_cp0_status_read();

	mips_cp0_status_write(ostatus & ~MIPS_SR_INT_IE);

	while (va < eva) {
		__asm volatile("nop; nop; nop; nop;");
		cache_op_r4k_line(va, CACHE_R4K_D|CACHEOP_R4K_HIT_WB_INV);
		va += 32;
	}

	mips_cp0_status_write(ostatus);
}

void
r4600v2_pdcache_wbinv_range_32(vaddr_t va, vsize_t size)
{
	vaddr_t eva = round_line(va + size);
	uint32_t ostatus;

	va = trunc_line(va);

	ostatus = mips_cp0_status_read();

	mips_cp0_status_write(ostatus & ~MIPS_SR_INT_IE);

	while ((eva - va) >= (32 * 32)) {
		(void) *(volatile int *)MIPS_PHYS_TO_KSEG1(0);
		cache_r4k_op_32lines_32(va,
		    CACHE_R4K_D|CACHEOP_R4K_HIT_WB_INV);
		va += (32 * 32);
	}

	(void) *(volatile int *)MIPS_PHYS_TO_KSEG1(0);
	while (va < eva) {
		cache_op_r4k_line(va, CACHE_R4K_D|CACHEOP_R4K_HIT_WB_INV);
		va += 32;
	}

	mips_cp0_status_write(ostatus);
}

void
vr4131v1_pdcache_wbinv_range_16(vaddr_t va, vsize_t size)
{
	vaddr_t eva = round_line16(va + size);

	va = trunc_line16(va);

	while ((eva - va) >= (32 * 16)) {
		cache_r4k_op_32lines_16(va,
		    CACHE_R4K_D|CACHEOP_R4K_HIT_WB);
		cache_r4k_op_32lines_16(va,
		    CACHE_R4K_D|CACHEOP_R4K_HIT_INV);
		va += (32 * 16);
	}

	while (va < eva) {
		cache_op_r4k_line(va, CACHE_R4K_D|CACHEOP_R4K_HIT_WB);
		cache_op_r4k_line(va, CACHE_R4K_D|CACHEOP_R4K_HIT_INV);
		va += 16;
	}
}

void
r5k_pdcache_wbinv_range_16(vaddr_t va, vsize_t size)
{
	vaddr_t eva = round_line16(va + size);

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
}

void
r5k_pdcache_wbinv_range_32(vaddr_t va, vsize_t size)
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
r5k_pdcache_wbinv_range_index_16(vaddr_t va, vsize_t size)
{
	vaddr_t w2va, eva;

	/*
	 * Since we're doing Index ops, we expect to not be able
	 * to access the address we've been given.  So, get the
	 * bits that determine the cache index, and make a KSEG0
	 * address out of them.
	 */
	va = MIPS_PHYS_TO_KSEG0(va & mips_pdcache_way_mask);

	eva = round_line16(va + size);
	va = trunc_line16(va);
	w2va = va + mips_pdcache_way_size;

	while ((eva - va) >= (16 * 16)) {
		cache_r4k_op_16lines_16_2way(va, w2va,
		    CACHE_R4K_D|CACHEOP_R4K_INDEX_WB_INV);
		va   += (16 * 16);
		w2va += (16 * 16);
	}

	while (va < eva) {
		cache_op_r4k_line(  va, CACHE_R4K_D|CACHEOP_R4K_INDEX_WB_INV);
		cache_op_r4k_line(w2va, CACHE_R4K_D|CACHEOP_R4K_INDEX_WB_INV);
		va   += 16;
		w2va += 16;
	}
}

void
r5k_pdcache_wbinv_range_index_32(vaddr_t va, vsize_t size)
{
	vaddr_t w2va, eva;

	/*
	 * Since we're doing Index ops, we expect to not be able
	 * to access the address we've been given.  So, get the
	 * bits that determine the cache index, and make a KSEG0
	 * address out of them.
	 */
	va = MIPS_PHYS_TO_KSEG0(va & mips_pdcache_way_mask);

	eva = round_line(va + size);
	va = trunc_line(va);
	w2va = va + mips_pdcache_way_size;

	while ((eva - va) >= (16 * 32)) {
		cache_r4k_op_16lines_32_2way(va, w2va,
		    CACHE_R4K_D|CACHEOP_R4K_INDEX_WB_INV);
		va   += (16 * 32);
		w2va += (16 * 32);
	}

	while (va < eva) {
		cache_op_r4k_line(  va, CACHE_R4K_D|CACHEOP_R4K_INDEX_WB_INV);
		cache_op_r4k_line(w2va, CACHE_R4K_D|CACHEOP_R4K_INDEX_WB_INV);
		va   += 32;
		w2va += 32;
	}
}

void
r4600v1_pdcache_inv_range_32(vaddr_t va, vsize_t size)
{
	vaddr_t eva = round_line(va + size);
	uint32_t ostatus;

	/*
	 * This is pathetically slow, but the chip bug is pretty
	 * nasty, and we hope that not too many v1.x R4600s are
	 * around.
	 */

	va = trunc_line(va);

	ostatus = mips_cp0_status_read();

	mips_cp0_status_write(ostatus & ~MIPS_SR_INT_IE);

	while (va < eva) {
		__asm volatile("nop; nop; nop; nop;");
		cache_op_r4k_line(va, CACHE_R4K_D|CACHEOP_R4K_HIT_INV);
		va += 32;
	}

	mips_cp0_status_write(ostatus);
}

void
r4600v2_pdcache_inv_range_32(vaddr_t va, vsize_t size)
{
	vaddr_t eva = round_line(va + size);
	uint32_t ostatus;

	va = trunc_line(va);

	ostatus = mips_cp0_status_read();

	mips_cp0_status_write(ostatus & ~MIPS_SR_INT_IE);

	/*
	 * Between blasts of big cache chunks, give interrupts
	 * a chance to get though.
	 */
	while ((eva - va) >= (32 * 32)) {
		(void) *(volatile int *)MIPS_PHYS_TO_KSEG1(0);
		cache_r4k_op_32lines_32(va, CACHE_R4K_D|CACHEOP_R4K_HIT_INV);
		va += (32 * 32);
	}

	(void) *(volatile int *)MIPS_PHYS_TO_KSEG1(0);
	while (va < eva) {
		cache_op_r4k_line(va, CACHE_R4K_D|CACHEOP_R4K_HIT_INV);
		va += 32;
	}

	mips_cp0_status_write(ostatus);
}

void
r5k_pdcache_inv_range_16(vaddr_t va, vsize_t size)
{
	vaddr_t eva = round_line16(va + size);

	va = trunc_line16(va);

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
r5k_pdcache_inv_range_32(vaddr_t va, vsize_t size)
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
r4600v1_pdcache_wb_range_32(vaddr_t va, vsize_t size)
{
	vaddr_t eva = round_line(va + size);
	uint32_t ostatus;

	/*
	 * This is pathetically slow, but the chip bug is pretty
	 * nasty, and we hope that not too many v1.x R4600s are
	 * around.
	 */

	va = trunc_line(va);

	ostatus = mips_cp0_status_read();

	mips_cp0_status_write(ostatus & ~MIPS_SR_INT_IE);

	while (va < eva) {
		__asm volatile("nop; nop; nop; nop;");
		cache_op_r4k_line(va, CACHE_R4K_D|CACHEOP_R4K_HIT_WB);
		va += 32;
	}

	mips_cp0_status_write(ostatus);
}

void
r4600v2_pdcache_wb_range_32(vaddr_t va, vsize_t size)
{
	vaddr_t eva = round_line(va + size);
	uint32_t ostatus;

	va = trunc_line(va);

	ostatus = mips_cp0_status_read();

	mips_cp0_status_write(ostatus & ~MIPS_SR_INT_IE);

	/*
	 * Between blasts of big cache chunks, give interrupts
	 * a chance to get though.
	 */
	while ((eva - va) >= (32 * 32)) {
		(void) *(volatile int *)MIPS_PHYS_TO_KSEG1(0);
		cache_r4k_op_32lines_32(va, CACHE_R4K_D|CACHEOP_R4K_HIT_WB);
		va += (32 * 32);
	}

	(void) *(volatile int *)MIPS_PHYS_TO_KSEG1(0);
	while (va < eva) {
		cache_op_r4k_line(va, CACHE_R4K_D|CACHEOP_R4K_HIT_WB);
		va += 32;
	}

	mips_cp0_status_write(ostatus);
}

void
r5k_pdcache_wb_range_16(vaddr_t va, vsize_t size)
{
	vaddr_t eva = round_line16(va + size);

	va = trunc_line16(va);

	while ((eva - va) >= (32 * 16)) {
		cache_r4k_op_32lines_16(va, CACHE_R4K_D|CACHEOP_R4K_HIT_WB);
		va += (32 * 16);
	}

	while (va < eva) {
		cache_op_r4k_line(va, CACHE_R4K_D|CACHEOP_R4K_HIT_WB);
		va += 16;
	}
}

void
r5k_pdcache_wb_range_32(vaddr_t va, vsize_t size)
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

#undef round_line16
#undef trunc_line16
#undef round_line
#undef trunc_line

/*
 * Cache operations for R5000-style secondary caches:
 *
 *	- Direct-mapped
 *	- Write-through
 *	- Physically indexed, physically tagged
 *
 */


__asm(".set mips3");

#define R5K_Page_Invalidate_S   0x17

void
r5k_sdcache_wbinv_all(void)
{

	r5k_sdcache_wbinv_range(MIPS_PHYS_TO_KSEG0(0), mips_sdcache_size);
}

void
r5k_sdcache_wbinv_range_index(vaddr_t va, vsize_t size)
{

	/*
	 * Since we're doing Index ops, we expect to not be able
	 * to access the address we've been given.  So, get the
	 * bits that determine the cache index, and make a KSEG0
	 * address out of them.
	 */
	va = MIPS_PHYS_TO_KSEG0(va & (mips_sdcache_size - 1));
	r5k_sdcache_wbinv_range(va, size);
}

#define	mips_r5k_round_page(x)		(((x) + (128 * 32 - 1)) & ~(128 * 32 - 1))
#define	mips_r5k_trunc_page(x)		((x) & ~(128 * 32 - 1))

void
r5k_sdcache_wbinv_range(vaddr_t va, vsize_t size)
{
	uint32_t ostatus, taglo;
	vaddr_t eva = mips_r5k_round_page(va + size);

	va = mips_r5k_trunc_page(va);

	__asm volatile(
		".set noreorder		\n\t"
		".set noat		\n\t"
		"mfc0 %0, $12		\n\t"
		"mtc0 $0, $12		\n\t"
		".set reorder		\n\t"
		".set at"
		: "=r"(ostatus));

	__asm volatile("mfc0 %0, $28" : "=r"(taglo));
	__asm volatile("mtc0 $0, $28");

	while (va < eva) {
		cache_op_r4k_line(va, R5K_Page_Invalidate_S);
		va += (128 * 32);
	}

	__asm volatile("mtc0 %0, $12; nop" :: "r"(ostatus));
	__asm volatile("mtc0 %0, $28; nop" :: "r"(taglo));
}

void
r5k_sdcache_wb_range(vaddr_t va, vsize_t size)
{
	/* Write-through cache, no need to WB */
}
