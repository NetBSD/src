/*	$NetBSD: cpufunc.c,v 1.1.2.2 2018/04/07 04:12:10 pgoyette Exp $	*/

/*
 * Copyright (c) 2017 Ryo Shimizu <ryo@nerv.org>
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
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: cpufunc.c,v 1.1.2.2 2018/04/07 04:12:10 pgoyette Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <aarch64/armreg.h>
#include <aarch64/cpufunc.h>

u_int cputype;	/* compat arm */

/* L1-L8 cache info */
struct aarch64_cache_info aarch64_cache_info[MAX_CACHE_LEVEL];
u_int aarch64_cache_vindexsize;
u_int aarch64_cache_prefer_mask;

u_int arm_dcache_minline;
u_int arm_dcache_align;
u_int arm_dcache_align_mask;

static void
extract_cacheunit(int level, bool insn, int cachetype)
{
	struct aarch64_cache_unit *cunit;
	uint32_t ccsidr;

	/* select and extract level N data cache */
	reg_csselr_el1_write(__SHIFTIN(level, CSSELR_LEVEL) |
	    __SHIFTIN(insn ? 1 : 0, CSSELR_IND));
	__asm __volatile ("isb");

	ccsidr = reg_ccsidr_el1_read();

	if (insn)
		cunit = &aarch64_cache_info[level].icache;
	else
		cunit = &aarch64_cache_info[level].dcache;

	cunit->cache_type = cachetype;

	cunit->cache_line_size = 1 << (__SHIFTOUT(ccsidr, CCSIDR_LINESIZE) + 4);
	cunit->cache_ways = __SHIFTOUT(ccsidr, CCSIDR_ASSOC) + 1;
	cunit->cache_sets = __SHIFTOUT(ccsidr, CCSIDR_NUMSET) + 1;

	/* calc waysize and whole size */
	cunit->cache_way_size = cunit->cache_line_size * cunit->cache_sets;
	cunit->cache_size = cunit->cache_way_size * cunit->cache_ways;

	/* cache purging */
	cunit->cache_purging = (ccsidr & CCSIDR_WT) ? CACHE_PURGING_WT : 0;
	cunit->cache_purging |= (ccsidr & CCSIDR_WB) ? CACHE_PURGING_WB : 0;
	cunit->cache_purging |= (ccsidr & CCSIDR_RA) ? CACHE_PURGING_RA : 0;
	cunit->cache_purging |= (ccsidr & CCSIDR_WA) ? CACHE_PURGING_WA : 0;
}

int
aarch64_getcacheinfo(void)
{
	uint32_t clidr, ctr;
	int level, cachetype;

	cputype = aarch64_cpuid();

	/*
	 * CTR - Cache Type Register
	 */
	ctr = reg_ctr_el0_read();
	switch (__SHIFTOUT(ctr, CTR_EL0_L1IP_MASK)) {
	case CTR_EL0_L1IP_AIVIVT:
		cachetype = CACHE_TYPE_VIVT;
		break;
	case CTR_EL0_L1IP_VIPT:
		cachetype = CACHE_TYPE_VIPT;
		break;
	case CTR_EL0_L1IP_PIPT:
		cachetype = CACHE_TYPE_PIPT;
		break;
	default:
		cachetype = 0;
		break;
	}

	arm_dcache_minline = __SHIFTOUT(ctr, CTR_EL0_DMIN_LINE);
	arm_dcache_align = sizeof(int) << arm_dcache_minline;
	arm_dcache_align_mask = arm_dcache_align - 1;

	/*
	 * CLIDR -  Cache Level ID Register
	 * CSSELR - Cache Size Selection Register
	 * CCSIDR - CurrentCache Size ID Register (selected by CSSELR)
	 */

	/* L1, L2, L3, ..., L8 cache */
	for (level = 0, clidr = reg_clidr_el1_read();
	    level < MAX_CACHE_LEVEL; level++, clidr >>= 3) {

		int cacheable;

		switch (clidr & 7) {
		case CLIDR_TYPE_NOCACHE:
			cacheable = CACHE_CACHEABLE_NONE;
			break;
		case CLIDR_TYPE_ICACHE:
			cacheable = CACHE_CACHEABLE_ICACHE;
			extract_cacheunit(level, true, cachetype);
			break;
		case CLIDR_TYPE_DCACHE:
			cacheable = CACHE_CACHEABLE_DCACHE;
			extract_cacheunit(level, false, CACHE_TYPE_PIPT);
			break;
		case CLIDR_TYPE_IDCACHE:
			cacheable = CACHE_CACHEABLE_IDCACHE;
			extract_cacheunit(level, true, cachetype);
			extract_cacheunit(level, false, CACHE_TYPE_PIPT);
			break;
		case CLIDR_TYPE_UNIFIEDCACHE:
			cacheable = CACHE_CACHEABLE_UNIFIED;
			extract_cacheunit(level, false, CACHE_TYPE_PIPT);
			break;
		}

		aarch64_cache_info[level].cacheable = cacheable;
		if (cacheable == CACHE_CACHEABLE_NONE) {
			/* no more level */
			break;
		}

		/*
		 * L1 insn cachetype is CTR_EL0:L1IP,
		 * all other cachetype is PIPT.
		 */
		cachetype = CACHE_TYPE_PIPT;
	}

	/* calculate L1 icache virtual index size */
	if (((aarch64_cache_info[0].icache.cache_type == CACHE_TYPE_VIVT) ||
	     (aarch64_cache_info[0].icache.cache_type == CACHE_TYPE_VIPT)) &&
	    ((aarch64_cache_info[0].cacheable == CACHE_CACHEABLE_ICACHE) ||
	     (aarch64_cache_info[0].cacheable == CACHE_CACHEABLE_IDCACHE))) {

		aarch64_cache_vindexsize =
		    aarch64_cache_info[0].icache.cache_size /
		    aarch64_cache_info[0].icache.cache_ways;

		KASSERT(aarch64_cache_vindexsize != 0);
		aarch64_cache_prefer_mask = aarch64_cache_vindexsize - 1;
	} else {
		aarch64_cache_vindexsize = 0;
	}

	return 0;
}

static inline void
ln_dcache_wb_all(int level, struct aarch64_cache_unit *cunit)
{
	uint64_t x;
	unsigned int set, way, setshift, wayshift;

	setshift = ffs(cunit->cache_line_size) - 1;
	wayshift = 32 - (ffs(cunit->cache_ways) - 1);

	for (way = 0; way < cunit->cache_ways; way++) {
		for (set = 0; set < cunit->cache_sets; set++) {
			x = (way << wayshift) | (set << setshift) |
			    (level << 1);
			__asm __volatile ("dc csw, %0; dsb sy" :: "r"(x));
		}
	}
}

static inline void
ln_dcache_wbinv_all(int level, struct aarch64_cache_unit *cunit)
{
	uint64_t x;
	unsigned int set, way, setshift, wayshift;

	setshift = ffs(cunit->cache_line_size) - 1;
	wayshift = 32 - (ffs(cunit->cache_ways) - 1);

	for (way = 0; way < cunit->cache_ways; way++) {
		for (set = 0; set < cunit->cache_sets; set++) {
			x = (way << wayshift) | (set << setshift) |
			    (level << 1);
			__asm __volatile ("dc cisw, %0; dsb sy" :: "r"(x));
		}
	}
}

static inline void
ln_dcache_inv_all(int level, struct aarch64_cache_unit *cunit)
{
	uint64_t x;
	unsigned int set, way, setshift, wayshift;

	setshift = ffs(cunit->cache_line_size) - 1;
	wayshift = 32 - (ffs(cunit->cache_ways) - 1);

	for (way = 0; way < cunit->cache_ways; way++) {
		for (set = 0; set < cunit->cache_sets; set++) {
			x = (way << wayshift) | (set << setshift) |
			    (level << 1);
			__asm __volatile ("dc isw, %0; dsb sy" :: "r"(x));
		}
	}
}

void
aarch64_dcache_wbinv_all(void)
{
	int level;

	for (level = 0; level < MAX_CACHE_LEVEL; level++) {
		if (aarch64_cache_info[level].cacheable == CACHE_CACHEABLE_NONE)
			break;

		__asm __volatile ("dsb ish");
		ln_dcache_wbinv_all(level, &aarch64_cache_info[level].dcache);
	}
	__asm __volatile ("dsb ish");
}

void
aarch64_dcache_inv_all(void)
{
	int level;

	for (level = 0; level < MAX_CACHE_LEVEL; level++) {
		if (aarch64_cache_info[level].cacheable == CACHE_CACHEABLE_NONE)
			break;

		__asm __volatile ("dsb ish");
		ln_dcache_inv_all(level, &aarch64_cache_info[level].dcache);
	}
	__asm __volatile ("dsb ish");
}

void
aarch64_dcache_wb_all(void)
{
	int level;

	for (level = 0; level < MAX_CACHE_LEVEL; level++) {
		if (aarch64_cache_info[level].cacheable == CACHE_CACHEABLE_NONE)
			break;

		__asm __volatile ("dsb ish");
		ln_dcache_wb_all(level, &aarch64_cache_info[level].dcache);
	}
	__asm __volatile ("dsb ish");
}
