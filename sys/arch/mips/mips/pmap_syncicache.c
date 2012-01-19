/*	$NetBSD: pmap_syncicache.c,v 1.1.2.3 2012/01/19 08:28:50 matt Exp $	*/

/*-
 * Copyright (c) 2010 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Matt Thomas at 3am Software Foundry.
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

__KERNEL_RCSID(0, "$NetBSD: pmap_syncicache.c,v 1.1.2.3 2012/01/19 08:28:50 matt Exp $");

/*
 *
 */

#include "opt_multiprocessor.h"
#include "opt_sysv.h"
#include "opt_cputype.h"
#include "opt_mips_cache.h"
#include "opt_multiprocessor.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/mutex.h>
#include <sys/atomic.h>
#include <sys/kernel.h>			/* for cold */
#include <sys/cpu.h>

#include <uvm/uvm.h>

#include <mips/cache.h>
#include <mips/cpuregs.h>
#include <mips/locore.h>
#include <mips/pte.h>

u_int pmap_syncicache_page_mask;
u_int pmap_syncicache_map_mask;

void
pmap_syncicache_page(struct vm_page *pg, uint32_t colors)
{
	if (!MIPS_HAS_R4K_MMU) {
		mips_icache_sync_range(MIPS_PHYS_TO_KSEG0(VM_PAGE_TO_PHYS(pg)),
		    PAGE_SIZE);
		return;
	}

	struct vm_page_md * const md = VM_PAGE_TO_MD(pg);
	if (!PG_MD_CACHED_P(md))
		return;

	/*
	 * The page may not be mapped so we can't use one of its
	 * virtual addresses.  But if the cache is not vivt (meaning
	 * it's physically tagged), we can use its XKPHYS cached or
	 * KSEG0 (if it lies within) address to invalid it.
	 */
	if (__predict_true(!mips_cache_info.mci_picache_vivt)) {
		const paddr_t pa = VM_PAGE_TO_PHYS(pg);
#ifndef __mips_o32
		mips_icache_sync_range(MIPS_PHYS_TO_XKPHYS_CACHED(pa),
		    PAGE_SIZE);
		return;
#else
		if (MIPS_KSEG0_P(pa)) {
			mips_icache_sync_range(MIPS_PHYS_TO_KSEG0(pa),
			    PAGE_SIZE);
			return;
		}
#endif
	}
	colors >>= PG_MD_EXECPAGE_SHIFT;
	/*
	 * If not all the colors are in use, just flush the
	 * ones that are.
	 */
	for (vaddr_t va = MIPS_KSEG0_START;
	     colors != 0;
	     colors >>= 1, va += PAGE_SIZE) {
		if (colors & 1) {
			mips_icache_sync_range_index(va, PAGE_SIZE);
		}
	}
}
