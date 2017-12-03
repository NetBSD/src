/*	$NetBSD: cache.c,v 1.8.14.1 2017/12/03 11:36:45 jdolecek Exp $	*/

/*
 * Copyright (c) 2011 Matthew R. Green
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * Handle picking the right types of the different cache call.
 *
 * This module could take on a larger role.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: cache.c,v 1.8.14.1 2017/12/03 11:36:45 jdolecek Exp $");

#include "opt_multiprocessor.h"

#include <sys/param.h>
#include <sys/reboot.h>

#include <machine/cpu.h>

#include <sparc64/sparc64/cache.h>

static void
cache_nop(void)
{
}

static void
blast_dcache_real(void)
{

	sp_blast_dcache(dcache_size, dcache_line_size);
}

static void
sp_dcache_flush_page_cpuset(paddr_t pa, sparc64_cpuset_t cs)
{

	dcache_flush_page(pa);
}

void    (*dcache_flush_page)(paddr_t) =	dcache_flush_page_us;
void	(*dcache_flush_page_cpuset)(paddr_t, sparc64_cpuset_t) =
					sp_dcache_flush_page_cpuset;
void	(*blast_dcache)(void) =		blast_dcache_real;
void	(*blast_icache)(void) =		blast_icache_us;

#ifdef MULTIPROCESSOR
void    (*sp_dcache_flush_page)(paddr_t) = dcache_flush_page_us;
#endif

void	(*sp_tlb_flush_pte)(vaddr_t, int)	= sp_tlb_flush_pte_us;
void	(*sp_tlb_flush_all)(void)		= sp_tlb_flush_all_us;

void	(*cache_flush_phys)(paddr_t, psize_t, int) = cache_flush_phys_us;

static void
sp_tlb_flush_pte_sun4v(vaddr_t va, int ctx)
{
	int64_t hv_rc;
	hv_rc = hv_mmu_demap_page(va, ctx, MAP_DTLB|MAP_ITLB);
	if ( hv_rc != H_EOK )
		panic("hv_mmu_demap_page(%p,%d) failed - rc = %" PRIx64 "\n", (void*)va, ctx, hv_rc);
}

static void
sp_tlb_flush_all_sun4v(void)
{
	panic("sp_tlb_flush_all_sun4v() not implemented yet");
}


static void
cache_flush_phys_sun4v(paddr_t pa, psize_t size, int ecache)
{
	panic("cache_flush_phys_sun4v() not implemented yet");
}

void
cache_setup_funcs(void)
{

	if (CPU_ISSUN4US || CPU_ISSUN4V) {
		dcache_flush_page = (void (*)(paddr_t)) cache_nop;
#ifdef MULTIPROCESSOR
		/* XXXMRG shouldn't be necessary -- only caller is nop'ed out */
		sp_dcache_flush_page = (void (*)(paddr_t)) cache_nop;
#endif
		blast_dcache = cache_nop;
		blast_icache = cache_nop;
	} else {
		if (CPU_IS_USIII_UP()) {
			dcache_flush_page = dcache_flush_page_usiii;
#ifdef MULTIPROCESSOR
			sp_dcache_flush_page = dcache_flush_page_usiii;
#endif
			blast_icache = blast_icache_usiii;
		}
#ifdef MULTIPROCESSOR
		if (sparc_ncpus > 1 && (boothowto & RB_MD1) == 0) {
			dcache_flush_page = smp_dcache_flush_page_allcpu;
			dcache_flush_page_cpuset = smp_dcache_flush_page_cpuset;
			blast_dcache = smp_blast_dcache;
		}
#endif
	}

	/* Prepare sp_tlb_flush_* and cache_flush_phys() functions */
	if (CPU_ISSUN4V) {
		sp_tlb_flush_pte = sp_tlb_flush_pte_sun4v;
		sp_tlb_flush_all = sp_tlb_flush_all_sun4v;
		cache_flush_phys = cache_flush_phys_sun4v;
	} else {
		if (CPU_IS_USIII_UP() || CPU_IS_SPARC64_V_UP()) {
			sp_tlb_flush_pte = sp_tlb_flush_pte_usiii;
			sp_tlb_flush_all = sp_tlb_flush_all_usiii;
			cache_flush_phys = cache_flush_phys_usiii;
		}
	}

}
