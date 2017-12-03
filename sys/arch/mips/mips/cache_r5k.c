/*	$NetBSD: cache_r5k.c,v 1.15.14.1 2017/12/03 11:36:28 jdolecek Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: cache_r5k.c,v 1.15.14.1 2017/12/03 11:36:28 jdolecek Exp $");

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

#define	round_line16(x)		round_line(x, 16)
#define	trunc_line16(x)		trunc_line(x, 16)
#define	round_line32(x)		round_line(x, 32)
#define	trunc_line32(x)		trunc_line(x, 32)
#define	round_line(x,n)		(((x) + (register_t)(n) - 1) & -(register_t)(n))
#define	trunc_line(x,n)		((x) & -(register_t)(n))

__asm(".set mips3");

void
r5k_picache_sync_all(void)
{
        struct mips_cache_info * const mci = &mips_cache_info;

        /*
         * Since we're hitting the whole thing, we don't have to
         * worry about the N different "ways".
         */
        mips_intern_dcache_sync_all();
	__asm volatile("sync");
        mips_intern_icache_sync_range_index(MIPS_KSEG0_START,
            mci->mci_picache_size);
}

void
r5k_picache_sync_range(register_t va, vsize_t size)
{

	mips_intern_dcache_sync_range(va, size);
	mips_intern_icache_sync_range(va, size);
}

void
r5k_picache_sync_range_index(vaddr_t va, vsize_t size)
{
	struct mips_cache_info * const mci = &mips_cache_info;
	const size_t ways = mci->mci_picache_ways;
	const size_t line_size = mci->mci_picache_line_size;
	const size_t way_size = mci->mci_picache_way_size;
	const size_t way_mask = way_size - 1;
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
	size = eva - va;

	/*
	 * If we are going to flush more than is in a way (or the stride
	 * needed for that way), we are flushing everything.
	 */
	if (size >= way_size) {
		r5k_picache_sync_all();
		return;
	}

	for (size_t way = 0; way < ways; way++) {
		mips_intern_dcache_sync_range_index(va, size);
		mips_intern_icache_sync_range_index(va, size);
		va += way_size;
		eva += way_size;
	}
}

void
r5k_pdcache_wbinv_all(void)
{
	struct mips_cache_info * const mci = &mips_cache_info;

	/*
	 * Since we're hitting the whole thing, we don't have to
	 * worry about the N different "ways".
	 */
	mips_intern_pdcache_wbinv_range_index(MIPS_KSEG0_START,
	    mci->mci_pdcache_size);
}

void
r5k_pdcache_wbinv_range_index(vaddr_t va, vsize_t size)
{
	struct mips_cache_info * const mci = &mips_cache_info;
	const size_t ways = mci->mci_pdcache_ways;
	const size_t line_size = mci->mci_pdcache_line_size;
	const vaddr_t way_size = mci->mci_pdcache_way_size;
	const vaddr_t way_mask = way_size - 1;
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
	size = eva - va;

	/*
	 * If we are going to flush more than is in a way, we are flushing
	 * everything.
	 */
	if (size >= way_size) {
		mips_intern_pdcache_wbinv_range_index(MIPS_KSEG0_START,
		    mci->mci_pdcache_size);
		return;
	}

	/*
	 * Invalidate each way.  If the address range wraps past the end of
	 * the way, we will be invalidating in two ways but eventually things
	 * work out since the last way will wrap into the first way.
	 */
	for (size_t way = 0; way < ways; way++) {
		mips_intern_pdcache_wbinv_range_index(va, size);
		va += way_size;
		eva += way_size;
	}
}

void
r4600v1_pdcache_wbinv_range_32(register_t va, vsize_t size)
{
	const register_t eva = round_line32(va + size);

	/*
	 * This is pathetically slow, but the chip bug is pretty
	 * nasty, and we hope that not too many v1.x R4600s are
	 * around.
	 */

	va = trunc_line32(va);

	/*
	 * To make this a little less painful, just hit the entire
	 * cache if we have a range >= the cache size.
	 */
	if (eva - va >= mips_cache_info.mci_pdcache_size) {
		r5k_pdcache_wbinv_all();
		return;
	}

	const uint32_t ostatus = mips_cp0_status_read();

	mips_cp0_status_write(ostatus & ~MIPS_SR_INT_IE);

	while (va < eva) {
		__asm volatile("nop; nop; nop; nop");
		cache_op_r4k_line(va, CACHE_R4K_D|CACHEOP_R4K_HIT_WB_INV);
		va += 32;
	}

	mips_cp0_status_write(ostatus);
}

void
r4600v2_pdcache_wbinv_range_32(register_t va, vsize_t size)
{
	const register_t eva = round_line32(va + size);

	va = trunc_line32(va);

	const uint32_t ostatus = mips_cp0_status_read();

	mips_cp0_status_write(ostatus & ~MIPS_SR_INT_IE);

	for (; (eva - va) >= (32 * 32); va += (32 * 32)) {
		(void) *(volatile int *)MIPS_PHYS_TO_KSEG1(0);
		cache_r4k_op_32lines_32(va, CACHE_R4K_D|CACHEOP_R4K_HIT_WB_INV);
	}

	(void) *(volatile int *)MIPS_PHYS_TO_KSEG1(0);
	for (; va < eva; va += 32) {
		cache_op_r4k_line(va, CACHE_R4K_D|CACHEOP_R4K_HIT_WB_INV);

	}

	mips_cp0_status_write(ostatus);
}

void
vr4131v1_pdcache_wbinv_range_16(register_t va, vsize_t size)
{
	register_t eva = round_line16(va + size);

	va = trunc_line16(va);

	for (; (eva - va) >= (32 * 16); va += (32 * 16)) {
		cache_r4k_op_32lines_16(va, CACHE_R4K_D|CACHEOP_R4K_HIT_WB);
		cache_r4k_op_32lines_16(va, CACHE_R4K_D|CACHEOP_R4K_HIT_INV);

	}

	for (; va < eva; va += 16) {
		cache_op_r4k_line(va, CACHE_R4K_D|CACHEOP_R4K_HIT_WB);
		cache_op_r4k_line(va, CACHE_R4K_D|CACHEOP_R4K_HIT_INV);
	}
}

void
r4600v1_pdcache_inv_range_32(register_t va, vsize_t size)
{
	const register_t eva = round_line32(va + size);

	/*
	 * This is pathetically slow, but the chip bug is pretty
	 * nasty, and we hope that not too many v1.x R4600s are
	 * around.
	 */

	va = trunc_line32(va);

	const uint32_t ostatus = mips_cp0_status_read();

	mips_cp0_status_write(ostatus & ~MIPS_SR_INT_IE);

	for (; va < eva; va += 32) {
		__asm volatile("nop; nop; nop; nop;");
		cache_op_r4k_line(va, CACHE_R4K_D|CACHEOP_R4K_HIT_INV);

	}

	mips_cp0_status_write(ostatus);
}

void
r4600v2_pdcache_inv_range_32(register_t va, vsize_t size)
{
	const register_t eva = round_line32(va + size);

	va = trunc_line32(va);

	const uint32_t ostatus = mips_cp0_status_read();

	mips_cp0_status_write(ostatus & ~MIPS_SR_INT_IE);

	/*
	 * Between blasts of big cache chunks, give interrupts
	 * a chance to get though.
	 */
	for (; (eva - va) >= (32 * 32); va += (32 * 32)) {
		(void) *(volatile int *)MIPS_PHYS_TO_KSEG1(0);
		cache_r4k_op_32lines_32(va, CACHE_R4K_D|CACHEOP_R4K_HIT_INV);

	}

	(void) *(volatile int *)MIPS_PHYS_TO_KSEG1(0);
	for (; va < eva; va += 32) {
		cache_op_r4k_line(va, CACHE_R4K_D|CACHEOP_R4K_HIT_INV);

	}

	mips_cp0_status_write(ostatus);
}

void
r4600v1_pdcache_wb_range_32(register_t va, vsize_t size)
{
	const register_t eva = round_line32(va + size);

	/*
	 * This is pathetically slow, but the chip bug is pretty
	 * nasty, and we hope that not too many v1.x R4600s are
	 * around.
	 */

	va = trunc_line32(va);

	const uint32_t ostatus = mips_cp0_status_read();

	mips_cp0_status_write(ostatus & ~MIPS_SR_INT_IE);

	for (; va < eva; va += 32) {
		__asm volatile("nop; nop; nop; nop;");
		cache_op_r4k_line(va, CACHE_R4K_D|CACHEOP_R4K_HIT_WB);

	}

	mips_cp0_status_write(ostatus);
}

void
r4600v2_pdcache_wb_range_32(register_t va, vsize_t size)
{
	const register_t eva = round_line32(va + size);

	va = trunc_line32(va);

	const uint32_t ostatus = mips_cp0_status_read();

	mips_cp0_status_write(ostatus & ~MIPS_SR_INT_IE);

	/*
	 * Between blasts of big cache chunks, give interrupts
	 * a chance to get though.
	 */
	for (; (eva - va) >= (32 * 32); va += (32 * 32)) {
		(void) *(volatile int *)MIPS_PHYS_TO_KSEG1(0);
		cache_r4k_op_32lines_32(va, CACHE_R4K_D|CACHEOP_R4K_HIT_WB);

	}

	(void) *(volatile int *)MIPS_PHYS_TO_KSEG1(0);
	for (; va < eva; va += 32) {
		cache_op_r4k_line(va, CACHE_R4K_D|CACHEOP_R4K_HIT_WB);
	}

	mips_cp0_status_write(ostatus);
}

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
CTASSERT(R5K_Page_Invalidate_S == (CACHEOP_R4K_HIT_WB_INV|CACHE_R4K_SD));

void
r5k_sdcache_wbinv_all(void)
{

	r5k_sdcache_wbinv_range(MIPS_PHYS_TO_KSEG0(0), mips_cache_info.mci_sdcache_size);
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
	va = MIPS_PHYS_TO_KSEG0(va & (mips_cache_info.mci_sdcache_size - 1));
	r5k_sdcache_wbinv_range((intptr_t)va, size);
}

#define	mips_r5k_round_page(x)	round_line(x, PAGE_SIZE)
#define	mips_r5k_trunc_page(x)	trunc_line(x, PAGE_SIZE)

void
r5k_sdcache_wbinv_range(register_t va, vsize_t size)
{
	uint32_t ostatus, taglo;
	register_t eva = mips_r5k_round_page(va + size);

	va = mips_r5k_trunc_page(va);

	ostatus = mips_cp0_status_read();
	mips_cp0_status_write(ostatus & ~MIPS_SR_INT_IE);

	__asm volatile("mfc0 %0, $28" : "=r"(taglo));
	__asm volatile("mtc0 $0, $28");

	for (; va < eva; va += (128 * 32)) {
		cache_op_r4k_line(va, CACHEOP_R4K_HIT_WB_INV|CACHE_R4K_SD);
	}

	mips_cp0_status_write(ostatus);
	__asm volatile("mtc0 %0, $28; nop" :: "r"(taglo));
}
