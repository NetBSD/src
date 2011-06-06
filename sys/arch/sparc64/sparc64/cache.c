/*	$NetBSD: cache.c,v 1.8 2011/06/06 02:49:39 mrg Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: cache.c,v 1.8 2011/06/06 02:49:39 mrg Exp $");

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
}
