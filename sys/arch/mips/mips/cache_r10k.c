/*	$NetBSD: cache_r10k.c,v 1.1 2003/10/05 11:10:25 tsutsui Exp $	*/

/*
 * Copyright (c) 2003 KIYOHARA Takashi <kiyohara@kk.iij4u.or.jp>
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

#include <sys/cdefs.h>

#include <sys/param.h>

#include <mips/cache.h>
#include <mips/cache_r4k.h>
#include <mips/cache_r5k.h>
#include <mips/cache_r10k.h>
#include <mips/locore.h>

/*
 * Cache operations for R10000-style caches:
 *
 *	- 2-way set-associative
 *	- Write-back
 *	- Virtually indexed, physically tagged
 *
 */

#define	round_line(x)		(((x) + 63) & ~63)
#define	trunc_line(x)		((x) & ~63)

__asm(".set mips3");

void
r10k_icache_sync_all_64(void)
{
	vaddr_t va = MIPS_PHYS_TO_KSEG0(0);
	vaddr_t eva = va + mips_picache_size;

	/*
	 * Since we're hitting the whole thing, we don't have to
	 * worry about the 2 different "ways".
	 */

	mips_dcache_wbinv_all();

	__asm __volatile("sync");

	while (va < eva) {
		cache_r10k_op_32lines_64(va, CACHE_R4K_I|CACHEOP_R4K_INDEX_INV);
		va += (32 * 64);
	}
}

void
r10k_icache_sync_range_64(vaddr_t va, vsize_t size)
{
	vaddr_t eva = round_line(va + size);

	va = trunc_line(va);

	mips_dcache_wb_range(va, (eva - va));

	__asm __volatile("sync");

	while ((eva - va) >= (32 * 64)) {
		cache_r10k_op_32lines_64(va, CACHE_R4K_I|CACHEOP_R4K_HIT_INV);
		va += (32 * 64);
	}

	while (va < eva) {
		cache_op_r4k_line(va, CACHE_R4K_I|CACHEOP_R4K_HIT_INV);
		va += 64;
	}
}

void
r10k_icache_sync_range_index_64(vaddr_t va, vsize_t size)
{
	vaddr_t w2va, eva, orig_va;

	orig_va = va;

	eva = round_line(va + size);
	va = trunc_line(va);

	mips_dcache_wbinv_range_index(va, (eva - va));

	__asm __volatile("sync");

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

	while ((eva - va) >= (16 * 64)) {
		cache_r10k_op_16lines_64_2way(va, w2va,
		    CACHE_R4K_I|CACHEOP_R4K_INDEX_INV);
		va   += (16 * 64);
		w2va += (16 * 64);
	}

	while (va < eva) {
		cache_op_r4k_line(  va, CACHE_R4K_I|CACHEOP_R4K_INDEX_INV);
		cache_op_r4k_line(w2va, CACHE_R4K_I|CACHEOP_R4K_INDEX_INV);
		va   += 64;
		w2va += 64;
	}
}

void
r10k_pdcache_wb_range(vaddr_t va, vsize_t size)
{
	/* R10000 processor does not support */
}

