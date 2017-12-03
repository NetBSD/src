/*	$NetBSD: cache_r10k.c,v 1.5.14.1 2017/12/03 11:36:28 jdolecek Exp $	*/

/*-
 * Copyright (c) 2003 Takao Shinohara.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

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

#include <sys/param.h>

#include <mips/cpuregs.h>
#include <mips/cache.h>
#include <mips/cache_r4k.h>
#include <mips/cache_r10k.h>

/*
 * Cache operations for R10000-style caches:
 *
 *	2-way, write-back
 *	primary cache: virtual index/physical tag
 *	secondary cache: physical index/physical tag
 */

__asm(".set mips3");

#define	round_line(x)	(((x) + 64 - 1) & ~(64 - 1))
#define	trunc_line(x)	((x) & ~(64 - 1))

void
r10k_icache_sync_all(void)
{
	const struct mips_cache_info * const mci = &mips_cache_info;
	vaddr_t va = MIPS_PHYS_TO_KSEG0(0);
	vaddr_t eva = va + mci->mci_picache_way_size;

	mips_dcache_wbinv_all();

	__asm volatile("sync");

	while (va < eva) {
		cache_op_r4k_line(va, CACHE_R4K_I|CACHEOP_R4K_INDEX_INV);
		va++;
		cache_op_r4k_line(va, CACHE_R4K_I|CACHEOP_R4K_INDEX_INV);
		va += 63;
	}
}

void
r10k_icache_sync_range(register_t va, vsize_t size)
{
	vaddr_t eva = round_line(va + size);

	va = trunc_line(va);

	mips_dcache_wb_range(va, (eva - va));

	__asm volatile("sync");

	while (va < eva) {
		cache_op_r4k_line(va, CACHE_R4K_I|CACHEOP_R4K_HIT_INV);
		va += 64;
	}
}

void
r10k_icache_sync_range_index(vaddr_t va, vsize_t size)
{
	const struct mips_cache_info * const mci = &mips_cache_info;
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
	va = MIPS_PHYS_TO_KSEG0(orig_va & mci->mci_picache_way_mask);

	eva = round_line(va + size);
	va = trunc_line(va);

	while (va < eva) {
		cache_op_r4k_line(va, CACHE_R4K_I|CACHEOP_R4K_INDEX_INV);
		va++;
		cache_op_r4k_line(va, CACHE_R4K_I|CACHEOP_R4K_INDEX_INV);
		va += 63;
	}
}

#undef round_line
#undef trunc_line

#define	round_line(x)	(((x) + 32 - 1) & ~(32 - 1))
#define	trunc_line(x)	((x) & ~(32 - 1))

void
r10k_pdcache_wbinv_all(void)
{
	const struct mips_cache_info * const mci = &mips_cache_info;
	vaddr_t va = MIPS_PHYS_TO_KSEG0(0);
	vaddr_t eva = va + mci->mci_pdcache_way_size;

	while (va < eva) {
		cache_op_r4k_line(va, CACHE_R4K_D|CACHEOP_R4K_INDEX_WB_INV);
		va++;
		cache_op_r4k_line(va, CACHE_R4K_D|CACHEOP_R4K_INDEX_WB_INV);
		va += 31;
	}
}

void
r10k_pdcache_wbinv_range(register_t va, vsize_t size)
{
	vaddr_t eva = round_line(va + size);

	va = trunc_line(va);

	while (va < eva) {
		cache_op_r4k_line(va, CACHE_R4K_D|CACHEOP_R4K_HIT_WB_INV);
		va += 32;
	}
}

void
r10k_pdcache_wbinv_range_index(vaddr_t va, vsize_t size)
{
	const struct mips_cache_info * const mci = &mips_cache_info;
	vaddr_t eva;

	/*
	 * Since we're doing Index ops, we expect to not be able
	 * to access the address we've been given.  So, get the
	 * bits that determine the cache index, and make a KSEG0
	 * address out of them.
	 */
	va = MIPS_PHYS_TO_KSEG0(va & mci->mci_pdcache_way_mask);

	eva = round_line(va + size);
	va = trunc_line(va);

	while (va < eva) {
		cache_op_r4k_line(va, CACHE_R4K_D|CACHEOP_R4K_INDEX_WB_INV);
		va++;
		cache_op_r4k_line(va, CACHE_R4K_D|CACHEOP_R4K_INDEX_WB_INV);
		va += 31;
	}
}

void
r10k_pdcache_inv_range(register_t va, vsize_t size)
{
	vaddr_t eva = round_line(va + size);

	va = trunc_line(va);

	while (va < eva) {
		cache_op_r4k_line(va, CACHE_R4K_D|CACHEOP_R4K_HIT_INV);
		va += 32;
	}
}

void
r10k_pdcache_wb_range(register_t va, vsize_t size)
{
	vaddr_t eva = round_line(va + size);

	va = trunc_line(va);

	while (va < eva) {
		/* R10000 does not support HitWriteBack operation */
		cache_op_r4k_line(va, CACHE_R4K_D|CACHEOP_R4K_HIT_WB_INV);
		va += 32;
	}
}

#undef round_line
#undef trunc_line

#define	round_line(x)	(((x) + mci->mci_sdcache_line_size - 1) & ~(mci->mci_sdcache_line_size - 1))
#define	trunc_line(x)	((x) & ~(mci->mci_sdcache_line_size - 1))

void
r10k_sdcache_wbinv_all(void)
{
	const struct mips_cache_info * const mci = &mips_cache_info;
	vaddr_t va = MIPS_PHYS_TO_KSEG0(0);
	vaddr_t eva = va + mci->mci_sdcache_way_size;
	vsize_t line_size = mci->mci_sdcache_line_size;

	while (va < eva) {
		cache_op_r4k_line(va, CACHE_R4K_SD|CACHEOP_R4K_INDEX_WB_INV);
		va++;
		cache_op_r4k_line(va, CACHE_R4K_SD|CACHEOP_R4K_INDEX_WB_INV);
		va += line_size - 1;
	}
}

void
r10k_sdcache_wbinv_range(register_t va, vsize_t size)
{
	const struct mips_cache_info * const mci = &mips_cache_info;
	vaddr_t eva = round_line(va + size);
	vsize_t line_size = mci->mci_sdcache_line_size;

	va = trunc_line(va);

	while (va < eva) {
		cache_op_r4k_line(va, CACHE_R4K_SD|CACHEOP_R4K_HIT_WB_INV);
		va += line_size;
	}
}

void
r10k_sdcache_wbinv_range_index(vaddr_t va, vsize_t size)
{
	const struct mips_cache_info * const mci = &mips_cache_info;
	vaddr_t eva;
	vsize_t line_size = mci->mci_sdcache_line_size;

	/*
	 * Since we're doing Index ops, we expect to not be able
	 * to access the address we've been given.  So, get the
	 * bits that determine the cache index, and make a KSEG0
	 * address out of them.
	 */
	va = MIPS_PHYS_TO_KSEG0(va & mci->mci_sdcache_way_mask);

	eva = round_line(va + size);
	va = trunc_line(va);

	while (va < eva) {
		cache_op_r4k_line(va, CACHE_R4K_SD|CACHEOP_R4K_INDEX_WB_INV);
		va++;
		cache_op_r4k_line(va, CACHE_R4K_SD|CACHEOP_R4K_INDEX_WB_INV);
		va += line_size - 1;
	}
}

void
r10k_sdcache_inv_range(register_t va, vsize_t size)
{
	const struct mips_cache_info * const mci = &mips_cache_info;
	vaddr_t eva = round_line(va + size);
	vsize_t line_size = mci->mci_sdcache_line_size;

	va = trunc_line(va);

	while (va < eva) {
		cache_op_r4k_line(va, CACHE_R4K_SD|CACHEOP_R4K_HIT_INV);
		va += line_size;
	}
}

void
r10k_sdcache_wb_range(register_t va, vsize_t size)
{
	const struct mips_cache_info * const mci = &mips_cache_info;
	vaddr_t eva = round_line(va + size);
	vsize_t line_size = mci->mci_sdcache_line_size;

	va = trunc_line(va);

	while (va < eva) {
		/* R10000 does not support HitWriteBack operation */
		cache_op_r4k_line(va, CACHE_R4K_SD|CACHEOP_R4K_HIT_WB_INV);
		va += line_size;
	}
}

#undef round_line
#undef trunc_line
