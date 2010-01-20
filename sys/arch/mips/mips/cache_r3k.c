/*	$NetBSD: cache_r3k.c,v 1.4.96.1 2010/01/20 09:04:34 matt Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: cache_r3k.c,v 1.4.96.1 2010/01/20 09:04:34 matt Exp $");

#include <sys/param.h>

#include <mips/cache.h>
#include <mips/cache_r3k.h>

/*
 * Cache operations for R3000-style caches:
 *
 *	- Direct-mapped
 *	- Write-through
 *	- Physically indexed, physically tagged
 */

#define	round_line(x)		(((x) + 31) & ~31)
#define	trunc_line(x)		((x) & ~31)

void
r3k_icache_sync_all(void)
{
	vaddr_t va = MIPS_PHYS_TO_KSEG0(0);
	vaddr_t eva = va + mips_cache_info.mci_picache_size;

	r3k_picache_do_inv(va, eva);
}

void
r3k_icache_sync_range(vaddr_t va, vsize_t size)
{
	vaddr_t eva = round_line(va + size);

	va = trunc_line(va);

	if ((eva - va) >= mips_cache_info.mci_picache_size) {
		r3k_icache_sync_all();
		return;
	}

	r3k_picache_do_inv(va, eva);
}

void
r3k_pdcache_wbinv_all(void)
{
	vaddr_t va = MIPS_PHYS_TO_KSEG0(0);
	vaddr_t eva = va + mips_cache_info.mci_pdcache_size;

	/* Cache is write-through. */

	r3k_pdcache_do_inv(va, eva);
}

void
r3k_pdcache_inv_range(vaddr_t va, vsize_t size)
{
	vaddr_t eva = round_line(va + size);

	va = trunc_line(va);

	if ((eva - va) >= mips_cache_info.mci_pdcache_size) {
		r3k_pdcache_wbinv_all();
		return;
	}

	r3k_pdcache_do_inv(va, eva);
}

void
r3k_pdcache_wb_range(vaddr_t va, vsize_t size)
{

	/* Cache is write-though. */
}

#undef round_line
#undef trunc_line
