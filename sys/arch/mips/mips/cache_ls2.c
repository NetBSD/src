/*	$NetBSD: cache_ls2.c,v 1.4.30.1 2016/10/05 20:55:32 skrll Exp $	*/

/*-
 * Copyright (c) 2009 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Matt Thomas <matt@3am-software.com>.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: cache_ls2.c,v 1.4.30.1 2016/10/05 20:55:32 skrll Exp $");

#include <sys/param.h>

#include <mips/cache.h>
#include <mips/cache_ls2.h>
#include <mips/locore.h>

/*
 * Cache operations for Loongson2-style caches:
 *
 *	- 4-way set-associative 32b/l
 *	- Write-back
 *	- Primary is virtually indexed, physically tagged
 *	- Seconadry is physically indexed, physically tagged
 */

#define	round_line(x)		(((x) + 31) & ~31)
#define	trunc_line(x)		((x) & ~31)

__asm(".set mips3");

void
ls2_icache_sync_range(register_t va, vsize_t size)
{
	struct mips_cache_info * const mci = &mips_cache_info;
	const vaddr_t eva = round_line(va + size);

	va = trunc_line(va);

	if (va + mci->mci_picache_size <= eva) {
		ls2_icache_sync_all();
		return;
	}

	for (; va + 8 * 32 <= eva; va += 8 * 32) {
		cache_op_ls2_8line(va, CACHEOP_LS2_D_HIT_WB_INV);
		cache_op_ls2_8line(va, CACHEOP_LS2_I_INDEX_INV);
	}

	for (; va < eva; va += 32) {
		cache_op_ls2_line(va, CACHEOP_LS2_D_HIT_WB_INV);
		cache_op_ls2_line(va, CACHEOP_LS2_I_INDEX_INV);
	}

	__asm volatile("sync");
}

void
ls2_icache_sync_range_index(vaddr_t va, vsize_t size)
{
	vaddr_t eva;
	struct mips_cache_info * const mci = &mips_cache_info;

	/*
	 * Since we're doing Index ops, we expect to not be able
	 * to access the address we've been given.  So, get the
	 * bits that determine the cache index, and make a KSEG0
	 * address out of them.
	 */

	va = MIPS_PHYS_TO_KSEG0(va & mci->mci_picache_way_mask);
	eva = round_line(va + size);
	va = trunc_line(va);

	if (va + mci->mci_picache_way_size < eva) {
		va = MIPS_PHYS_TO_KSEG0(0);
		eva = mci->mci_picache_way_size;
	}

	for (; va + 8 * 32 <= eva; va += 8 * 32) {
		cache_op_ls2_8line_4way(va, CACHEOP_LS2_D_INDEX_WB_INV);
		cache_op_ls2_8line(va, CACHEOP_LS2_I_INDEX_INV);
	}

	for (; va < eva; va += 32) {
		cache_op_ls2_line_4way(va, CACHEOP_LS2_D_INDEX_WB_INV);
		cache_op_ls2_line(va, CACHEOP_LS2_I_INDEX_INV);
	}

	__asm volatile("sync");
}

void
ls2_icache_sync_all(void)
{
	struct mips_cache_info * const mci = &mips_cache_info;
	ls2_icache_sync_range_index(0, mci->mci_picache_way_size);
}

void
ls2_pdcache_inv_range(register_t va, vsize_t size)
{
	const vaddr_t eva = round_line(va + size);

	va = trunc_line(va);

	for (; va + 8 * 32 <= eva; va += 8 * 32) {
		cache_op_ls2_8line(va, CACHEOP_LS2_D_HIT_INV);
	}

	for (; va < eva; va += 32) {
		cache_op_ls2_line(va, CACHEOP_LS2_D_HIT_INV);
	}

	__asm volatile("sync");
}

void
ls2_pdcache_wbinv_range(register_t va, vsize_t size)
{
	const vaddr_t eva = round_line(va + size);

	va = trunc_line(va);

	for (; va + 8 * 32 <= eva; va += 8 * 32) {
		cache_op_ls2_8line(va, CACHEOP_LS2_D_HIT_WB_INV);
	}

	for (; va < eva; va += 32) {
		cache_op_ls2_line(va, CACHEOP_LS2_D_HIT_WB_INV);
	}

	__asm volatile("sync");
}

void
ls2_pdcache_wb_range(register_t va, vsize_t size)
{
	/*
	 * Alas, can't writeback without invalidating...
	 */
	ls2_pdcache_wbinv_range(va, size);
}

void
ls2_pdcache_wbinv_range_index(vaddr_t va, vsize_t size)
{
	vaddr_t eva;
	struct mips_cache_info * const mci = &mips_cache_info;

	/*
	 * Since we're doing Index ops, we expect to not be able
	 * to access the address we've been given.  So, get the
	 * bits that determine the cache index, and make a KSEG0
	 * address out of them.
	 */
	va = MIPS_PHYS_TO_KSEG0(va & mci->mci_pdcache_way_mask);

	eva = round_line(va + size);
	va = trunc_line(va);

	if (va + mci->mci_pdcache_way_size > eva) {
		va = MIPS_PHYS_TO_KSEG0(0);
		eva = mci->mci_pdcache_way_size;
	}

	for (; va + 8 * 32 <= eva; va += 8 * 32) {
		cache_op_ls2_8line_4way(va, CACHEOP_LS2_D_INDEX_WB_INV);
	}

	for (; va < eva; va += 32) {
		cache_op_ls2_line_4way(va, CACHEOP_LS2_D_INDEX_WB_INV);
	}

	__asm volatile("sync");
}

void
ls2_pdcache_wbinv_all(void)
{
	struct mips_cache_info * const mci = &mips_cache_info;
	ls2_pdcache_wbinv_range_index(0, mci->mci_pdcache_way_size);
}

/*
 * Cache operations for secondary caches:
 *
 *	- Direct-mapped
 *	- Write-back
 *	- Physically indexed, physically tagged
 *
 */

void
ls2_sdcache_inv_range(register_t va, vsize_t size)
{
	const vaddr_t eva = round_line(va + size);

	va = trunc_line(va);

	for (; va + 8 * 32 <= eva; va += 8 * 32) {
		cache_op_ls2_8line(va, CACHEOP_LS2_D_HIT_INV);
		cache_op_ls2_8line(va, CACHEOP_LS2_S_HIT_INV);
	}

	for (; va < eva; va += 32) {
		cache_op_ls2_line(va, CACHEOP_LS2_D_HIT_INV);
		cache_op_ls2_line(va, CACHEOP_LS2_S_HIT_INV);
	}

	__asm volatile("sync");
}

void
ls2_sdcache_wbinv_range(register_t va, vsize_t size)
{
	const vaddr_t eva = round_line(va + size);

	va = trunc_line(va);

	for (; va + 8 * 32 <= eva; va += 8 * 32) {
		cache_op_ls2_8line(va, CACHEOP_LS2_D_HIT_WB_INV);
		cache_op_ls2_8line(va, CACHEOP_LS2_S_HIT_WB_INV);
	}

	for (; va < eva; va += 32) {
		cache_op_ls2_line(va, CACHEOP_LS2_D_HIT_WB_INV);
		cache_op_ls2_line(va, CACHEOP_LS2_S_HIT_WB_INV);
	}

	__asm volatile("sync");
}

void
ls2_sdcache_wb_range(register_t va, vsize_t size)
{
	/*
	 * Alas, can't writeback without invalidating...
	 */
	ls2_sdcache_wbinv_range(va, size);
}

void
ls2_sdcache_wbinv_range_index(vaddr_t va, vsize_t size)
{
	vaddr_t eva;
	struct mips_cache_info * const mci = &mips_cache_info;

	/*
	 * Since we're doing Index ops, we expect to not be able
	 * to access the address we've been given.  So, get the
	 * bits that determine the cache index, and make a KSEG0
	 * address out of them.
	 */
	va = MIPS_PHYS_TO_KSEG0(va & mci->mci_sdcache_way_mask);

	eva = round_line(va + size);
	va = trunc_line(va);

	if (va + mci->mci_sdcache_way_size > eva) {
		va = MIPS_PHYS_TO_KSEG0(0);
		eva = va + mci->mci_sdcache_way_size;
	}

	for (; va + 8 * 32 <= eva; va += 8 * 32) {
		cache_op_ls2_8line_4way(va, CACHEOP_LS2_D_INDEX_WB_INV);
		cache_op_ls2_8line_4way(va, CACHEOP_LS2_S_INDEX_WB_INV);
	}

	for (; va < eva; va += 32) {
		cache_op_ls2_line_4way(va, CACHEOP_LS2_D_INDEX_WB_INV);
		cache_op_ls2_line_4way(va, CACHEOP_LS2_S_INDEX_WB_INV);
	}

	__asm volatile("sync");
}

void
ls2_sdcache_wbinv_all(void)
{
	struct mips_cache_info * const mci = &mips_cache_info;
	ls2_sdcache_wbinv_range_index(0, mci->mci_sdcache_way_size);
}
