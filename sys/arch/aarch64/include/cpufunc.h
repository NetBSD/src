/*	$NetBSD: cpufunc.h,v 1.1 2018/04/01 04:35:03 ryo Exp $	*/

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

#ifndef _AARCH64_CPUFUNC_H_
#define _AARCH64_CPUFUNC_H_

#ifdef _KERNEL

#include <arm/armreg.h>

static inline int
set_cpufuncs(void)
{
	return 0;
}

struct aarch64_cache_unit {
	u_int cache_type;
#define CACHE_TYPE_UNKNOWN	0
#define CACHE_TYPE_VIVT		1	/* ASID-tagged VIVT */
#define CACHE_TYPE_VIPT		2
#define CACHE_TYPE_PIPT		3
	u_int cache_line_size;
	u_int cache_ways;
	u_int cache_sets;
	u_int cache_way_size;
	u_int cache_size;
	u_int cache_purging;
#define CACHE_PURGING_WB	0x01
#define CACHE_PURGING_WT	0x02
#define CACHE_PURGING_RA	0x04
#define CACHE_PURGING_WA	0x08
};

struct aarch64_cache_info {
	u_int cacheable;
#define CACHE_CACHEABLE_NONE	0
#define CACHE_CACHEABLE_ICACHE	1	/* instruction cache only */
#define CACHE_CACHEABLE_DCACHE	2	/* data cache only */
#define CACHE_CACHEABLE_IDCACHE	3	/* instruction and data caches */
#define CACHE_CACHEABLE_UNIFIED	4	/* unified cache */
	struct aarch64_cache_unit icache;
	struct aarch64_cache_unit dcache;
};

#define MAX_CACHE_LEVEL	8		/* ARMv8 has maximum 8 level cache */
extern struct aarch64_cache_info aarch64_cache_info[MAX_CACHE_LEVEL];
extern u_int aarch64_cache_vindexsize;	/* cachesize/way (VIVT/VIPT) */
extern u_int aarch64_cache_prefer_mask;
extern u_int cputype;			/* compat arm */

int aarch64_getcacheinfo(void);

void aarch64_dcache_wbinv_all(void);
void aarch64_dcache_inv_all(void);
void aarch64_dcache_wb_all(void);
void aarch64_icache_inv_all(void);

/* cache op in cpufunc_asm_armv8.S */
void aarch64_nullop(void);
uint32_t aarch64_cpuid(void);
void aarch64_icache_sync_range(vaddr_t, vsize_t);
void aarch64_idcache_wbinv_range(vaddr_t, vsize_t);
void aarch64_dcache_wbinv_range(vaddr_t, vsize_t);
void aarch64_dcache_inv_range(vaddr_t, vsize_t);
void aarch64_dcache_wb_range(vaddr_t, vsize_t);
void aarch64_drain_writebuf(void);

/* tlb op in cpufunc_asm_armv8.S */
void aarch64_set_ttbr0(uint64_t);
void aarch64_tlbi_all(void);			/* all ASID, all VA */
void aarch64_tlbi_by_asid(int);			/*  an ASID, all VA */
void aarch64_tlbi_by_va(vaddr_t);		/* all ASID, a VA */
void aarch64_tlbi_by_va_ll(vaddr_t);		/* all ASID, a VA, lastlevel */
void aarch64_tlbi_by_asid_va(int, vaddr_t);	/*  an ASID, a VA */
void aarch64_tlbi_by_asid_va_ll(int, vaddr_t);	/*  an ASID, a VA, lastlevel */


/* misc */
#define cpu_idnum()			aarch64_cpuid()

/* cache op */

#define cpu_dcache_wbinv_all()		aarch64_dcache_wbinv_all()
#define cpu_dcache_inv_all()		aarch64_dcache_inv_all()
#define cpu_dcache_wb_all()		aarch64_dcache_wb_all()
#define cpu_idcache_wbinv_all()		\
	(aarch64_dcache_wbinv_all(), aarch64_icache_inv_all())
#define cpu_icache_sync_all()		\
	(aarch64_dcache_wb_all(), aarch64_icache_inv_all())

#define cpu_dcache_wbinv_range(v,s)	aarch64_dcache_wbinv_range((v),(s))
#define cpu_dcache_inv_range(v,s)	aarch64_dcache_inv_range((v),(s))
#define cpu_dcache_wb_range(v,s)	aarch64_dcache_wb_range((v),(s))
#define cpu_idcache_wbinv_range(v,s)	aarch64_idcache_wbinv_range((v),(s))
#define cpu_icache_sync_range(v,s)	aarch64_icache_sync_range((v),(s))

#define cpu_sdcache_wbinv_range(v,p,s)	((void)0)
#define cpu_sdcache_inv_range(v,p,s)	((void)0)
#define cpu_sdcache_wb_range(v,p,s)	((void)0)

/* others */
#define cpu_drain_writebuf()		aarch64_drain_writebuf()

extern u_int arm_dcache_align;
extern u_int arm_dcache_align_mask;

static inline bool
cpu_gtmr_exists_p(void)
{

	return true;
}

static inline u_int
cpu_clusterid(void)
{

	return __SHIFTOUT(reg_mpidr_el1_read(), MPIDR_AFF1);
}

static inline bool
cpu_earlydevice_va_p(void)
{

	return false;
}

#endif /* _KERNEL */

#endif /* _AARCH64_CPUFUNC_H_ */
