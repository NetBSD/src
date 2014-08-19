/*	$NetBSD: cache.h,v 1.22.12.1 2014/08/20 00:03:25 tls Exp $ */

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
 * Copyright (C) 1996-1999 Eduardo Horvath.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR  ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR  BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

/*
 * The spitfire has a 16K two-way set-associative L1 I$ and a separate
 * 16K L2 D$.  The I$ can be invalidated using the FLUSH instructions,
 * so we don't really need to worry about it much.  The D$ is a 16K
 * write-through, direct mapped virtually-addressed cache with two 16-byte
 * sub-blocks per line.  The E$ is a 512KB to 4MB direct mapped
 * physically-indexed physically-tagged cache.  Since the L1 caches
 * are write-through, they don't need flushing and can be invalidated directly.
 *
 * The spitfire sees virtual addresses as:
 *
 *	struct cache_va {
 *		uint64_t	:22,		(unused; VAs are only 40 bits)
 *				cva_tag:28,	(tag ID)
 *				cva_line:9,	(cache line number)
 *				cva_byte:5;	(byte within line)
 *	};
 *
 * Since there is one bit of overlap between the page offset and the line index,
 * all we need to do is make sure that bit 14 of the va remains constant
 * and we have no aliasing problems.
 *
 * Let me try again...
 * Page size is 8K, cache size is 16K so if (va1 & 0x3fff != va2 & 0x3fff)
 * then we have a problem.  Bit 14 *must* be the same for all mappings
 * of a page to be cacheable in the D$.  (The I$ is 16K 2-way
 * set-associative -- each bank is 8K.  No conflict there.)
 */

#include <machine/psl.h>
#ifdef SUN4V
#include <machine/hypervisor.h>
#endif

/* Various cache size/line sizes */
extern	int	ecache_min_line_size;
extern	int	dcache_line_size;
extern	int	dcache_size;
extern	int	icache_line_size;
extern	int	icache_size;

/* The following are for I$ and D$ flushes and are in locore.s */
void 	dcache_flush_page_us(paddr_t);	/* flush page from D$ */
void 	dcache_flush_page_usiii(paddr_t); /* flush page from D$ */
void 	sp_blast_dcache(int, int);	/* Clear entire D$ */
void 	blast_icache_us(void);		/* Clear entire I$ */
void 	blast_icache_usiii(void);	/* Clear entire I$ */

/* The following flush a range from the D$ and I$ but not E$. */
void	cache_flush_phys_us(paddr_t, psize_t, int);
void	cache_flush_phys_usiii(paddr_t, psize_t, int);

static __inline__ void
cache_flush_phys(paddr_t pa, psize_t size, int ecache)
{
	if (CPU_IS_USIII_UP() || CPU_IS_SPARC64_V_UP())
		cache_flush_phys_usiii(pa, size, ecache);
	else
		cache_flush_phys_us(pa, size, ecache);
}

/* SPARC64 specific */
/* Assembly routines to flush TLB mappings */
void sp_tlb_flush_pte_us(vaddr_t, int);
void sp_tlb_flush_pte_usiii(vaddr_t, int);
void sp_tlb_flush_all_us(void);
void sp_tlb_flush_all_usiii(void);

#ifdef SUN4V
static __inline__ void
sp_tlb_flush_pte_sun4v(vaddr_t va, int ctx)
{
	int64_t hv_rc;
	hv_rc = hv_mmu_demap_page(va, ctx, MAP_DTLB|MAP_ITLB);
	if ( hv_rc != H_EOK )
		panic("hv_mmu_demap_page(%p,%d) failed - rc = %" PRIx64 "\n", (void*)va, ctx, hv_rc);
}
#endif

static __inline__ void
sp_tlb_flush_pte(vaddr_t va, int ctx)
{
	if (CPU_ISSUN4U || CPU_ISSUN4US) {
		if (CPU_IS_USIII_UP() || CPU_IS_SPARC64_V_UP())
			sp_tlb_flush_pte_usiii(va, ctx);
		else
			sp_tlb_flush_pte_us(va, ctx);
	}
#ifdef SUN4V	
	else
		sp_tlb_flush_pte_sun4v(va, ctx);
#endif
}

static __inline__ void
sp_tlb_flush_all(void)
{
	if (CPU_IS_USIII_UP() || CPU_IS_SPARC64_V_UP())
		sp_tlb_flush_all_usiii();
	else
		sp_tlb_flush_all_us();
}

extern	void	(*dcache_flush_page)(paddr_t);
extern	void	(*dcache_flush_page_cpuset)(paddr_t, sparc64_cpuset_t);
extern	void	(*blast_dcache)(void);
extern	void	(*blast_icache)(void);

void cache_setup_funcs(void);

#ifdef MULTIPROCESSOR
extern	void	(*sp_dcache_flush_page)(paddr_t);

void smp_tlb_flush_pte(vaddr_t, struct pmap *);
void smp_dcache_flush_page_cpuset(paddr_t pa, sparc64_cpuset_t);
void smp_dcache_flush_page_allcpu(paddr_t pa);
void smp_blast_dcache(void);
#define	tlb_flush_pte(va,pm)		smp_tlb_flush_pte(va, pm)
#define	dcache_flush_page_all(pa)	smp_dcache_flush_page_cpuset(pa, cpus_active)
#define	dcache_flush_page_cpuset(pa,cs)	smp_dcache_flush_page_cpuset(pa, cs)
#else
#define	tlb_flush_pte(va,pm)		sp_tlb_flush_pte(va, (pm)->pm_ctx[0])
#define	dcache_flush_page_all(pa)	dcache_flush_page(pa)
#define	dcache_flush_page_cpuset(pa,cs)	dcache_flush_page(pa)
#endif
