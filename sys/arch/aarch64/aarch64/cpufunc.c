/*	$NetBSD: cpufunc.c,v 1.33 2022/01/31 09:16:09 ryo Exp $	*/

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

#include "opt_cpuoptions.h"
#include "opt_multiprocessor.h"

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: cpufunc.c,v 1.33 2022/01/31 09:16:09 ryo Exp $");

#include <sys/param.h>
#include <sys/types.h>
#include <sys/kmem.h>
#include <sys/cpu.h>

#include <uvm/uvm.h>
#include <uvm/uvm_page.h>

#include <arm/cpufunc.h>

u_int cputype;			/* compat arm */
u_int arm_dcache_align;		/* compat arm */
u_int arm_dcache_align_mask;	/* compat arm */
u_int arm_dcache_maxline;

u_int aarch64_cache_vindexsize;
u_int aarch64_cache_prefer_mask;

int aarch64_hafdbs_enabled __read_mostly;
int aarch64_pan_enabled __read_mostly;
int aarch64_pac_enabled __read_mostly;

static void __noasan
extract_cacheunit(int level, bool insn, int cachetype,
    struct aarch64_cache_info *cacheinfo)
{
	struct aarch64_cache_unit *cunit;
	uint64_t ccsidr, mmfr2;

	/* select and extract level N data cache */
	reg_csselr_el1_write(__SHIFTIN(level, CSSELR_LEVEL) |
	    __SHIFTIN(insn ? 1 : 0, CSSELR_IND));
	isb();

	ccsidr = reg_ccsidr_el1_read();
	mmfr2 = reg_id_aa64mmfr2_el1_read();

	if (insn)
		cunit = &cacheinfo[level].icache;
	else
		cunit = &cacheinfo[level].dcache;

	cunit->cache_type = cachetype;

	switch (__SHIFTOUT(mmfr2, ID_AA64MMFR2_EL1_CCIDX)) {
	case ID_AA64MMFR2_EL1_CCIDX_32BIT:
		cunit->cache_line_size =
		    1 << (__SHIFTOUT(ccsidr, CCSIDR_LINESIZE) + 4);
		cunit->cache_ways = __SHIFTOUT(ccsidr, CCSIDR_ASSOC) + 1;
		cunit->cache_sets = __SHIFTOUT(ccsidr, CCSIDR_NUMSET) + 1;
		break;
	case ID_AA64MMFR2_EL1_CCIDX_64BIT:
		cunit->cache_line_size =
		    1 << (__SHIFTOUT(ccsidr, CCSIDR64_LINESIZE) + 4);
		cunit->cache_ways = __SHIFTOUT(ccsidr, CCSIDR64_ASSOC) + 1;
		cunit->cache_sets = __SHIFTOUT(ccsidr, CCSIDR64_NUMSET) + 1;
		break;
	}

	/* calc waysize and whole size */
	cunit->cache_way_size = cunit->cache_line_size * cunit->cache_sets;
	cunit->cache_size = cunit->cache_way_size * cunit->cache_ways;
}


/* Must be called on each processor */
void __noasan
aarch64_getcacheinfo(struct cpu_info *ci)
{
	struct aarch64_cache_info * const cinfo = ci->ci_cacheinfo;
	uint32_t clidr, ctr;
	int level, cachetype;

	/*
	 * CTR - Cache Type Register
	 */
	ctr = reg_ctr_el0_read();
	switch (__SHIFTOUT(ctr, CTR_EL0_L1IP_MASK)) {
	case CTR_EL0_L1IP_VPIPT:
		cachetype = CACHE_TYPE_VPIPT;
		break;
	case CTR_EL0_L1IP_AIVIVT:
		cachetype = CACHE_TYPE_VIVT;
		break;
	case CTR_EL0_L1IP_VIPT:
		cachetype = CACHE_TYPE_VIPT;
		break;
	case CTR_EL0_L1IP_PIPT:
		cachetype = CACHE_TYPE_PIPT;
		break;
	}

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
			extract_cacheunit(level, true, cachetype, cinfo);
			break;
		case CLIDR_TYPE_DCACHE:
			cacheable = CACHE_CACHEABLE_DCACHE;
			extract_cacheunit(level, false, CACHE_TYPE_PIPT, cinfo);
			break;
		case CLIDR_TYPE_IDCACHE:
			cacheable = CACHE_CACHEABLE_IDCACHE;
			extract_cacheunit(level, true, cachetype, cinfo);
			extract_cacheunit(level, false, CACHE_TYPE_PIPT, cinfo);
			break;
		case CLIDR_TYPE_UNIFIEDCACHE:
			cacheable = CACHE_CACHEABLE_UNIFIED;
			extract_cacheunit(level, false, CACHE_TYPE_PIPT, cinfo);
			break;
		default:
			cacheable = CACHE_CACHEABLE_NONE;
			break;
		}

		cinfo[level].cacheable = cacheable;
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
}


void
aarch64_parsecacheinfo(struct cpu_info *ci)
{
	struct aarch64_cache_info * const cinfo = ci->ci_cacheinfo;
	struct aarch64_sysctl_cpu_id *id = &ci->ci_id;
	const uint32_t ctr = id->ac_ctr;
	u_int vindexsize;

	/* remember maximum alignment */
	if (arm_dcache_maxline < __SHIFTOUT(ctr, CTR_EL0_DMIN_LINE)) {
		arm_dcache_maxline = __SHIFTOUT(ctr, CTR_EL0_DMIN_LINE);
		arm_dcache_align = sizeof(int) << arm_dcache_maxline;
		arm_dcache_align_mask = arm_dcache_align - 1;
	}

#ifdef MULTIPROCESSOR
	if (coherency_unit < arm_dcache_align)
		panic("coherency_unit %ld < %d; increase COHERENCY_UNIT",
		    coherency_unit, arm_dcache_align);
#endif

	/* calculate L1 icache virtual index size */
	if ((cinfo[0].icache.cache_type == CACHE_TYPE_VIVT ||
	     cinfo[0].icache.cache_type == CACHE_TYPE_VIPT) &&
	    (cinfo[0].cacheable == CACHE_CACHEABLE_ICACHE ||
	     cinfo[0].cacheable == CACHE_CACHEABLE_IDCACHE)) {

		vindexsize =
		    cinfo[0].icache.cache_size /
		    cinfo[0].icache.cache_ways;

		KASSERT(vindexsize != 0);
	} else {
		vindexsize = 0;
	}

	if (vindexsize > aarch64_cache_vindexsize) {
		aarch64_cache_vindexsize = vindexsize;
		aarch64_cache_prefer_mask = vindexsize - 1;

		if (uvm.page_init_done)
			uvm_page_recolor(vindexsize / PAGE_SIZE);
	}
}

static int
prt_cache(device_t self, struct aarch64_cache_info *cinfo, int level)
{
	struct aarch64_cache_unit *cunit;
	int i;
	const char *cacheable, *cachetype;

	if (cinfo[level].cacheable == CACHE_CACHEABLE_NONE)
		return -1;

	for (i = 0; i < 2; i++) {
		switch (cinfo[level].cacheable) {
		case CACHE_CACHEABLE_ICACHE:
			cunit = &cinfo[level].icache;
			cacheable = "Instruction";
			break;
		case CACHE_CACHEABLE_DCACHE:
			cunit = &cinfo[level].dcache;
			cacheable = "Data";
			break;
		case CACHE_CACHEABLE_IDCACHE:
			if (i == 0) {
				cunit = &cinfo[level].icache;
				cacheable = "Instruction";
			} else {
				cunit = &cinfo[level].dcache;
				cacheable = "Data";
			}
			break;
		case CACHE_CACHEABLE_UNIFIED:
			cunit = &cinfo[level].dcache;
			cacheable = "Unified";
			break;
		default:
			cunit = &cinfo[level].dcache;
			cacheable = "*UNK*";
			break;
		}

		switch (cunit->cache_type) {
		case CACHE_TYPE_VPIPT:
			cachetype = "VPIPT";
			break;
		case CACHE_TYPE_VIVT:
			cachetype = "VIVT";
			break;
		case CACHE_TYPE_VIPT:
			cachetype = "VIPT";
			break;
		case CACHE_TYPE_PIPT:
			cachetype = "PIPT";
			break;
		default:
			cachetype = "*UNK*";
			break;
		}

		aprint_verbose_dev(self,
		    "L%d %uKB/%uB %u-way (%u set) %s %s cache\n",
		    level + 1,
		    cunit->cache_size / 1024,
		    cunit->cache_line_size,
		    cunit->cache_ways,
		    cunit->cache_sets,
		    cachetype, cacheable);

		if (cinfo[level].cacheable != CACHE_CACHEABLE_IDCACHE)
			break;
	}

	return 0;
}

void
aarch64_printcacheinfo(device_t dev, struct cpu_info *ci)
{
	struct aarch64_cache_info * const cinfo = ci->ci_cacheinfo;
	int level;

	for (level = 0; level < MAX_CACHE_LEVEL; level++)
		if (prt_cache(dev, cinfo, level) < 0)
			break;
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
	struct cpu_info * const ci = curcpu();
	struct aarch64_cache_info * const cinfo = ci->ci_cacheinfo;
	int level;

	for (level = 0; level < MAX_CACHE_LEVEL; level++) {
		if (cinfo[level].cacheable == CACHE_CACHEABLE_NONE)
			break;

		dsb(ish);
		ln_dcache_wbinv_all(level, &cinfo[level].dcache);
	}
	dsb(ish);
}

void
aarch64_dcache_inv_all(void)
{
	struct cpu_info * const ci = curcpu();
	struct aarch64_cache_info * const cinfo = ci->ci_cacheinfo;
	int level;

	for (level = 0; level < MAX_CACHE_LEVEL; level++) {
		if (cinfo[level].cacheable == CACHE_CACHEABLE_NONE)
			break;

		dsb(ish);
		ln_dcache_inv_all(level, &cinfo[level].dcache);
	}
	dsb(ish);
}

void
aarch64_dcache_wb_all(void)
{
	struct cpu_info * const ci = curcpu();
	struct aarch64_cache_info * const cinfo = ci->ci_cacheinfo;
	int level;

	for (level = 0; level < MAX_CACHE_LEVEL; level++) {
		if (cinfo[level].cacheable == CACHE_CACHEABLE_NONE)
			break;

		dsb(ish);
		ln_dcache_wb_all(level, &cinfo[level].dcache);
	}
	dsb(ish);
}

int
set_cpufuncs(void)
{
	// This is only called from the BP

	return aarch64_setcpufuncs(&cpu_info_store[0]);
}


int
aarch64_setcpufuncs(struct cpu_info *ci)
{
	const uint64_t ctr = reg_ctr_el0_read();
	const uint64_t clidr = reg_clidr_el1_read();

	/* install default functions */
	ci->ci_cpufuncs.cf_set_ttbr0 = aarch64_set_ttbr0;
	ci->ci_cpufuncs.cf_icache_sync_range = aarch64_icache_sync_range;

	/*
	 * install core/cluster specific functions
	 */

	/* Icache sync op */
	if (__SHIFTOUT(ctr, CTR_EL0_DIC) == 1) {
		/* Icache invalidation to the PoU is not required */
		ci->ci_cpufuncs.cf_icache_sync_range =
		    aarch64_icache_barrier_range;
	} else if (__SHIFTOUT(ctr, CTR_EL0_IDC) == 1 ||
	    __SHIFTOUT(clidr, CLIDR_LOC) == 0 ||
	    (__SHIFTOUT(clidr, CLIDR_LOUIS) == 0 && __SHIFTOUT(clidr, CLIDR_LOUU) == 0)) {
		/* Dcache clean to the PoU is not required for Icache */
		ci->ci_cpufuncs.cf_icache_sync_range =
		    aarch64_icache_inv_range;
	}

#ifdef CPU_THUNDERX
	const uint32_t midr = reg_midr_el1_read();

	/* Cavium erratum 27456 */
	if ((midr == CPU_ID_THUNDERXP1d0) ||
	    (midr == CPU_ID_THUNDERXP1d1) ||
	    (midr == CPU_ID_THUNDERXP2d1) ||
	    (midr == CPU_ID_THUNDERX81XXRX)) {
		ci->ci_cpufuncs.cf_set_ttbr0 = aarch64_set_ttbr0_thunderx;
	}
#endif

	return 0;
}

void
aarch64_hafdbs_init(int primary)
{
#ifdef ARMV81_HAFDBS
	uint64_t tcr;
	int hafdbs;

	hafdbs = __SHIFTOUT(reg_id_aa64mmfr1_el1_read(),
	    ID_AA64MMFR1_EL1_HAFDBS);

	/*
	 * hafdbs
	 *   0:HAFDBS_NONE - no support for any hardware flags
	 *   1:HAFDBS_A    - only hardware access flag supported
	 *   2:HAFDBS_AD   - hardware access and modified flags supported.
	 */

	if (primary) {
		/* CPU0 does the detection. */
		switch (hafdbs) {
		case ID_AA64MMFR1_EL1_HAFDBS_NONE:
		default:
			aarch64_hafdbs_enabled = 0;
			break;
		case ID_AA64MMFR1_EL1_HAFDBS_A:
		case ID_AA64MMFR1_EL1_HAFDBS_AD:
			aarch64_hafdbs_enabled = hafdbs;
			break;
		}
	} else {
		/*
		 * The support status of HAFDBS on the primary CPU is different
		 * from that of the application processor.
		 *
		 * XXX:
		 *  The correct way to do this is to disable it on all cores,
		 *  or call pmap_fault_fixup() only on the unsupported cores,
		 *  but for now, do panic().
		 */
		if (aarch64_hafdbs_enabled != hafdbs)
			panic("HAFDBS is supported (%d) on primary cpu, "
			    "but isn't equal (%d) on secondary cpu",
			    aarch64_hafdbs_enabled, hafdbs);
	}

	/* enable Hardware updates to Access flag and Dirty state */
	tcr = reg_tcr_el1_read();
	switch (hafdbs) {
	case ID_AA64MMFR1_EL1_HAFDBS_NONE:
	default:
		break;
	case ID_AA64MMFR1_EL1_HAFDBS_A:
		/* enable only access */
		reg_tcr_el1_write(tcr | TCR_HA);
		isb();
		break;
	case ID_AA64MMFR1_EL1_HAFDBS_AD:
		/* enable both access and dirty */
		reg_tcr_el1_write(tcr | TCR_HD | TCR_HA);
		isb();
		break;
	}
#endif
}

void
aarch64_pan_init(int primary)
{
#ifdef ARMV81_PAN
	uint64_t reg, sctlr;

	/* CPU0 does the detection. */
	if (primary) {
		reg = reg_id_aa64mmfr1_el1_read();
		if (__SHIFTOUT(reg, ID_AA64MMFR1_EL1_PAN) !=
		    ID_AA64MMFR1_EL1_PAN_NONE)
			aarch64_pan_enabled = 1;
	}

	if (!aarch64_pan_enabled)
		return;

	/*
	 * On an exception to EL1, have the CPU set the PAN bit automatically.
	 * This ensures PAN is enabled each time the kernel is entered.
	 */
	sctlr = reg_sctlr_el1_read();
	sctlr &= ~SCTLR_SPAN;
	reg_sctlr_el1_write(sctlr);

	/* Set the PAN bit right now. */
	reg_pan_write(1);
#endif
}

/*
 * In order to avoid inconsistencies with pointer authentication
 * in this function itself, the caller must enable PAC according
 * to the return value.
 */
int
aarch64_pac_init(int primary)
{
#ifdef ARMV83_PAC
	uint64_t reg;

	/* CPU0 does the detection. */
	if (primary) {
		reg = reg_id_aa64isar1_el1_read();
		if (__SHIFTOUT(reg, ID_AA64ISAR1_EL1_APA) !=
		    ID_AA64ISAR1_EL1_APA_NONE)
			aarch64_pac_enabled = 1;
		if (__SHIFTOUT(reg, ID_AA64ISAR1_EL1_API) !=
		    ID_AA64ISAR1_EL1_API_NONE)
			aarch64_pac_enabled = 1;
		if (__SHIFTOUT(reg, ID_AA64ISAR1_EL1_GPA) !=
		    ID_AA64ISAR1_EL1_GPA_NONE)
			aarch64_pac_enabled = 1;
		if (__SHIFTOUT(reg, ID_AA64ISAR1_EL1_GPI) !=
		    ID_AA64ISAR1_EL1_GPI_NONE)
			aarch64_pac_enabled = 1;
	}

	if (!aarch64_pac_enabled)
		return -1;

	/* Set the key. Curlwp here is the CPU's idlelwp. */
	reg_APIAKeyLo_EL1_write(curlwp->l_md.md_ia_kern[0]);
	reg_APIAKeyHi_EL1_write(curlwp->l_md.md_ia_kern[1]);

	return 0;
#else
	return -1;
#endif
}
