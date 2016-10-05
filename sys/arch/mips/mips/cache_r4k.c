/*	$NetBSD: cache_r4k.c,v 1.11.32.2 2016/10/05 20:55:32 skrll Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: cache_r4k.c,v 1.11.32.2 2016/10/05 20:55:32 skrll Exp $");

#include <sys/param.h>

#include <mips/cpuregs.h>
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

void
r4k_icache_sync_all_generic(void)
{
	mips_dcache_wbinv_all();

	mips_intern_icache_sync_range_index(MIPS_PHYS_TO_KSEG0(0),
	    mips_cache_info.mci_picache_size);
}

void
r4k_icache_sync_range_generic(register_t va, vsize_t size)
{
	mips_dcache_wb_range(va, size);

	mips_intern_icache_sync_range_index(va, size);
}

void
r4k_icache_sync_range_index_generic(vaddr_t va, vsize_t size)
{
	mips_dcache_wbinv_range_index(va, size);

	/*
	 * Since we're doing Index ops, we expect to not be able
	 * to access the address we've been given.  So, get the
	 * bits that determine the cache index, and make a KSEG0
	 * address out of them.
	 */
	va = MIPS_PHYS_TO_KSEG0(va & mips_cache_info.mci_picache_way_mask);
	size &= mips_cache_info.mci_picache_way_mask;

	mips_intern_icache_sync_range_index(va, size);
}

void
r4k_pdcache_wbinv_all_generic(void)
{
	mips_intern_pdcache_wbinv_range_index(MIPS_PHYS_TO_KSEG0(0),
	     mips_cache_info.mci_pdcache_size);
}

void
r4k_sdcache_wbinv_all_generic(void)
{
	mips_intern_sdcache_wbinv_range_index(MIPS_PHYS_TO_KSEG0(0),
	     mips_cache_info.mci_sdcache_size);
}
