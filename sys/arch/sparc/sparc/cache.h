/*	$NetBSD: cache.h,v 1.31 2004/04/17 23:45:40 pk Exp $ */

/*
 * Copyright (c) 1996
 * 	The President and Fellows of Harvard College. All rights reserved.
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This software was developed by the Computer Systems Engineering group
 * at Lawrence Berkeley Laboratory under DARPA contract BG 91-66 and
 * contributed to Berkeley.
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
 *	This product includes software developed by Aaron Brown and
 *	Harvard University.
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)cache.h	8.1 (Berkeley) 6/11/93
 */

#ifndef SPARC_CACHE_H
#define SPARC_CACHE_H

#if defined(_KERNEL_OPT)
#include "opt_sparc_arch.h"
#endif

/*
 * Sun-4 and Sun-4c virtual address cache.
 *
 * Sun-4 virtual caches come in two flavors, write-through (Sun-4c)
 * and write-back (Sun-4).  The write-back caches are much faster
 * but require a bit more care.
 *
 */
enum vactype { VAC_UNKNOWN, VAC_NONE, VAC_WRITETHROUGH, VAC_WRITEBACK };

/*
 * Cache tags can be written in control space, and must be set to 0
 * (or invalid anyway) before turning on the cache.  The tags are
 * addressed as an array of 32-bit structures of the form:
 *
 *	struct cache_tag {
 *		u_int	:7,		(unused; must be zero)
 *			ct_cid:3,	(context ID)
 *			ct_w:1,		(write flag from PTE)
 *			ct_s:1,		(supervisor flag from PTE)
 *			ct_v:1,		(set => cache entry is valid)
 *			:3,		(unused; must be zero)
 *			ct_tid:14,	(cache tag ID)
 *			:2;		(unused; must be zero)
 *	};
 *
 * (The SS2 has 16 MMU contexts, which makes `ct_cid' one bit wider.)
 *
 * The SPARCstation 1 cache sees virtual addresses as:
 *
 *	struct cache_va {
 *		u_int	:2,		(unused; probably copies of va_tid<13>)
 *			cva_tid:14,	(tag ID)
 *			cva_line:12,	(cache line number)
 *			cva_byte:4;	(byte in cache line)
 *	};
 *
 * (The SS2 cache is similar but has half as many lines, each twice as long.)
 *
 * Note that, because the 12-bit line ID is `wider' than the page offset,
 * it is possible to have one page map to two different cache lines.
 * This can happen whenever two different physical pages have the same bits
 * in the part of the virtual address that overlaps the cache line ID, i.e.,
 * bits <15:12>.  In order to prevent cache duplication, we have to
 * make sure that no one page has more than one virtual address where
 * (va1 & 0xf000) != (va2 & 0xf000).  (The cache hardware turns off ct_v
 * when a cache miss occurs on a write, i.e., if va1 is in the cache and
 * va2 is not, and you write to va2, va1 goes out of the cache.  If va1
 * is in the cache and va2 is not, reading va2 also causes va1 to become
 * uncached, and the [same] data is then read from main memory into the
 * cache.)
 *
 * The other alternative, of course, is to disable caching of aliased
 * pages.  (In a few cases this might be faster anyway, but we do it
 * only when forced.)
 *
 * The Sun4, since it has an 8K pagesize instead of 4K, needs to check
 * bits that are one position higher.
 */

/* Some more well-known values: */

#define	CACHE_ALIAS_DIST_SUN4	0x20000
#define	CACHE_ALIAS_DIST_SUN4C	0x10000

#define	CACHE_ALIAS_BITS_SUN4	0x1e000
#define	CACHE_ALIAS_BITS_SUN4C	0xf000

#define CACHE_ALIAS_DIST_HS128k		0x20000
#define CACHE_ALIAS_BITS_HS128k		0x1f000
#define CACHE_ALIAS_DIST_HS256k		0x40000
#define CACHE_ALIAS_BITS_HS256k		0x3f000

/*
 * Assuming a tag format where the least significant bits are the byte offset
 * into the cache line, and the next-most significant bits are the line id,
 * we can calculate the appropriate aliasing constants. We also assume that
 * the linesize and total cache size are powers of 2.
 */
#define GUESS_CACHE_ALIAS_BITS		((cpuinfo.cacheinfo.c_totalsize - 1) & ~PGOFSET)
#define GUESS_CACHE_ALIAS_DIST		(cpuinfo.cacheinfo.c_totalsize)

extern int cache_alias_dist;		/* */
extern int cache_alias_bits;
extern u_long dvma_cachealign;

/* Optimize cache alias macros on single architecture kernels */
#if defined(SUN4) && !defined(SUN4C) && !defined(SUN4M) && !defined(SUN4D)
#define	CACHE_ALIAS_DIST	CACHE_ALIAS_DIST_SUN4
#define	CACHE_ALIAS_BITS	CACHE_ALIAS_BITS_SUN4
#elif !defined(SUN4) && defined(SUN4C) && !defined(SUN4M) && !defined(SUN4D)
#define	CACHE_ALIAS_DIST	CACHE_ALIAS_DIST_SUN4C
#define	CACHE_ALIAS_BITS	CACHE_ALIAS_BITS_SUN4C
#else
#define	CACHE_ALIAS_DIST	cache_alias_dist
#define	CACHE_ALIAS_BITS	cache_alias_bits
#endif

/*
 * True iff a1 and a2 are `bad' aliases (will cause cache duplication).
 */
#define	BADALIAS(a1, a2) (((int)(a1) ^ (int)(a2)) & CACHE_ALIAS_BITS)

/*
 * Routines for dealing with the cache.
 */
void	sun4_cache_enable(void);
void	ms1_cache_enable(void);	
void	viking_cache_enable(void);
void	hypersparc_cache_enable(void);
void	swift_cache_enable(void);
void	cypress_cache_enable(void);
void	turbosparc_cache_enable(void);

void	sun4_vcache_flush_context(int);		/* flush current context */
void	sun4_vcache_flush_region(int, int);	/* flush region in cur ctx */
void	sun4_vcache_flush_segment(int, int, int);/* flush seg in cur ctx */
void	sun4_vcache_flush_page(int va, int);	/* flush page in cur ctx */
void	sun4_vcache_flush_page_hw(int va, int);	/* flush page in cur ctx */
void	sun4_cache_flush(caddr_t, u_int);	/* flush range */

void	srmmu_vcache_flush_context(int);	/* flush current context */
void	srmmu_vcache_flush_region(int, int);	/* flush region in cur ctx */
void	srmmu_vcache_flush_segment(int, int, int);/* flush seg in cur ctx */
void	srmmu_vcache_flush_page(int va, int);	/* flush page in cur ctx */
void	srmmu_vcache_flush_range(int, int, int);
void	srmmu_cache_flush(caddr_t, u_int);	/* flush range */

/* `Fast trap' versions for use in cross-call cache flushes on MP systems */
#if defined(MULTIPROCESSOR)
void	ft_srmmu_vcache_flush_context(int);	/* flush current context */
void	ft_srmmu_vcache_flush_region(int, int);	/* flush region in cur ctx */
void	ft_srmmu_vcache_flush_segment(int, int, int);/* flush seg in cur ctx */
void	ft_srmmu_vcache_flush_page(int va, int);/* flush page in cur ctx */
void	ft_srmmu_vcache_flush_range(int, int, int);/* flush range in cur ctx */
#else
#define ft_srmmu_vcache_flush_context	0
#define ft_srmmu_vcache_flush_region	0
#define ft_srmmu_vcache_flush_segment	0
#define ft_srmmu_vcache_flush_page	0
#define ft_srmmu_vcache_flush_range	0
#endif /* MULTIPROCESSOR */

void	ms1_cache_flush(caddr_t, u_int);
void	viking_cache_flush(caddr_t, u_int);
void	viking_pcache_flush_page(paddr_t, int);
void	srmmu_pcache_flush_line(int, int);
void	hypersparc_pure_vcache_flush(void);

void	ms1_cache_flush_all(void);
void	srmmu_cache_flush_all(void);
void	cypress_cache_flush_all(void);
void	hypersparc_cache_flush_all(void);

extern void sparc_noop(void);

#define noop_vcache_flush_context	(void (*)(int))sparc_noop
#define noop_vcache_flush_region	(void (*)(int,int))sparc_noop
#define noop_vcache_flush_segment	(void (*)(int,int,int))sparc_noop
#define noop_vcache_flush_page		(void (*)(int,int))sparc_noop
#define noop_vcache_flush_range		(void (*)(int,int,int))sparc_noop
#define noop_cache_flush		(void (*)(caddr_t,u_int))sparc_noop
#define noop_pcache_flush_page		(void (*)(paddr_t,int))sparc_noop
#define noop_pure_vcache_flush		(void (*)(void))sparc_noop
#define noop_cache_flush_all		(void (*)(void))sparc_noop

/*
 * The SMP versions of the cache flush functions. These functions
 * send a "cache flush" message to each processor.
 */
void	smp_vcache_flush_context(int);		/* flush current context */
void	smp_vcache_flush_region(int,int);	/* flush region in cur ctx */
void	smp_vcache_flush_segment(int, int, int);/* flush seg in cur ctx */
void	smp_vcache_flush_page(int va,int);	/* flush page in cur ctx */


#define cache_flush_page(va,ctx)	cpuinfo.vcache_flush_page(va,ctx)
#define cache_flush_segment(vr,vs,ctx)	cpuinfo.vcache_flush_segment(vr,vs,ctx)
#define cache_flush_region(vr,ctx)	cpuinfo.vcache_flush_region(vr,ctx)
#define cache_flush_context(ctx)	cpuinfo.vcache_flush_context(ctx)
#define cache_flush(va,len)		cpuinfo.cache_flush(va,len)

#define pcache_flush_page(pa,flag)	cpuinfo.pcache_flush_page(pa,flag)

/*
 * Cache control information.
 */
struct cacheinfo {
	int	c_totalsize;		/* total size, in bytes */
					/* if split, MAX(icache,dcache) */
	int	c_enabled;		/* true => cache is enabled */
	int	c_hwflush;		/* true => have hardware flush */
	int	c_linesize;		/* line size, in bytes */
	int	c_l2linesize;		/* log2(linesize) */
	int	c_nlines;		/* number of cache lines */
	int	c_physical;		/* true => cache has physical
						   address tags */
	int 	c_associativity;	/* # of "buckets" in cache line */
	int 	c_split;		/* true => cache is split */

	int 	ic_totalsize;		/* instruction cache */
	int 	ic_enabled;
	int 	ic_linesize;
	int 	ic_l2linesize;
	int 	ic_nlines;
	int 	ic_associativity;

	int 	dc_totalsize;		/* data cache */
	int 	dc_enabled;
	int 	dc_linesize;
	int 	dc_l2linesize;
	int 	dc_nlines;
	int 	dc_associativity;

	int	ec_totalsize;		/* external cache info */
	int 	ec_enabled;
	int	ec_linesize;
	int	ec_l2linesize;
	int 	ec_nlines;
	int 	ec_associativity;

	enum vactype	c_vactype;
};

#define CACHEINFO cpuinfo.cacheinfo

/*
 * Cache control statistics.
 */
struct cachestats {
	int	cs_npgflush;		/* # page flushes */
	int	cs_nsgflush;		/* # seg flushes */
	int	cs_nrgflush;		/* # seg flushes */
	int	cs_ncxflush;		/* # context flushes */
	int	cs_nraflush;		/* # range flushes */
#ifdef notyet
	int	cs_ra[65];		/* pages/range */
#endif
};
#endif /* SPARC_CACHE_H */
