/*	$NetBSD: cache_mipsNN.c,v 1.14.32.2 2016/10/05 20:55:32 skrll Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: cache_mipsNN.c,v 1.14.32.2 2016/10/05 20:55:32 skrll Exp $");

#include <sys/param.h>

#include <mips/locore.h>
#include <mips/cache.h>
#include <mips/cache_r4k.h>
#include <mips/cache_mipsNN.h>
#include <mips/mipsNN.h>

#include <uvm/uvm_extern.h>

#define	round_line(x,n)		(((x) + (n) - 1) & -(n))
#define	trunc_line(x,n)		((x) & -(n))

void
mipsNN_cache_init(uint32_t config, uint32_t config1)
{
	/* nothing to do */
}

void
mipsNN_picache_sync_all(void)
{
	struct mips_cache_info * const mci = &mips_cache_info;

	/*
	 * Since we're hitting the whole thing, we don't have to
	 * worry about the N different "ways".
	 */
	mips_intern_dcache_sync_all();
	mips_intern_icache_sync_range_index(MIPS_KSEG0_START,
	    mci->mci_picache_size);
}

void
mipsNN_pdcache_wbinv_all(void)
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
mipsNN_sdcache_wbinv_all(void)
{
	struct mips_cache_info * const mci = &mips_cache_info;

	/*
	 * Since we're hitting the whole thing, we don't have to
	 * worry about the N different "ways".
	 */
	mips_intern_sdcache_wbinv_range_index(MIPS_KSEG0_START,
	    mci->mci_sdcache_size);
}

void
mipsNN_picache_sync_range(register_t va, vsize_t size)
{

	mips_intern_dcache_sync_range(va, size);
	mips_intern_icache_sync_range(va, size);
}

void
mipsNN_picache_sync_range_index(vaddr_t va, vsize_t size)
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
	 * need for that way), we are flushing everything.
	 */
	if (size >= way_size) {
		mipsNN_picache_sync_all();
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
mipsNN_pdcache_wbinv_range_index(vaddr_t va, vsize_t size)
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
mipsNN_sdcache_wbinv_range_index(vaddr_t va, vsize_t size)
{
	struct mips_cache_info * const mci = &mips_cache_info;
	const size_t ways = mci->mci_sdcache_ways;
	const size_t line_size = mci->mci_sdcache_line_size;
	const vaddr_t way_size = mci->mci_sdcache_way_size;
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
		mips_intern_sdcache_wbinv_range_index(MIPS_KSEG0_START,
		    mci->mci_sdcache_size);
		return;
	}

	/*
	 * Invalidate each way.  If the address range wraps past the end of
	 * the way, we will be invalidating in two ways but eventually things
	 * work out since the last way will wrap into the first way.
	 */
	for (size_t way = 0; way < ways; way++) {
		mips_intern_sdcache_wbinv_range_index(va, size);
		va += way_size;
		eva += way_size;
	}
}
