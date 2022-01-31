/*	$NetBSD: cpufunc.h,v 1.23 2022/01/31 09:16:09 ryo Exp $	*/

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
#include <sys/device_if.h>


extern u_int aarch64_cache_vindexsize;	/* cachesize/way (VIVT/VIPT) */
extern u_int aarch64_cache_prefer_mask;
extern u_int cputype;			/* compat arm */

extern int aarch64_bti_enabled;
extern int aarch64_hafdbs_enabled;
extern int aarch64_pan_enabled;
extern int aarch64_pac_enabled;

void aarch64_hafdbs_init(int);
void aarch64_pan_init(int);
int aarch64_pac_init(int);

int set_cpufuncs(void);
int aarch64_setcpufuncs(struct cpu_info *);
void aarch64_getcacheinfo(struct cpu_info *);
void aarch64_parsecacheinfo(struct cpu_info *);
void aarch64_printcacheinfo(device_t, struct cpu_info *);

void aarch64_dcache_wbinv_all(void);
void aarch64_dcache_inv_all(void);
void aarch64_dcache_wb_all(void);
void aarch64_icache_inv_all(void);

/* cache op in cpufunc_asm_armv8.S */
void aarch64_nullop(void);
uint32_t aarch64_cpuid(void);
void aarch64_icache_sync_range(vaddr_t, vsize_t);
void aarch64_icache_inv_range(vaddr_t, vsize_t);
void aarch64_icache_barrier_range(vaddr_t, vsize_t);
void aarch64_idcache_wbinv_range(vaddr_t, vsize_t);
void aarch64_dcache_wbinv_range(vaddr_t, vsize_t);
void aarch64_dcache_inv_range(vaddr_t, vsize_t);
void aarch64_dcache_wb_range(vaddr_t, vsize_t);
void aarch64_icache_inv_all(void);
void aarch64_drain_writebuf(void);

/* tlb op in cpufunc_asm_armv8.S */
#define cpu_set_ttbr0(t)		curcpu()->ci_cpufuncs.cf_set_ttbr0((t))
void aarch64_set_ttbr0(uint64_t);
void aarch64_set_ttbr0_thunderx(uint64_t);
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
#define cpu_icache_inv_all()		aarch64_icache_inv_all()

#define cpu_dcache_wbinv_range(v,s)	aarch64_dcache_wbinv_range((v),(s))
#define cpu_dcache_inv_range(v,s)	aarch64_dcache_inv_range((v),(s))
#define cpu_dcache_wb_range(v,s)	aarch64_dcache_wb_range((v),(s))
#define cpu_idcache_wbinv_range(v,s)	aarch64_idcache_wbinv_range((v),(s))
#define cpu_icache_sync_range(v,s)	\
	curcpu()->ci_cpufuncs.cf_icache_sync_range((v),(s))

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
	extern bool pmap_devmap_bootstrap_done;	/* in pmap.c */

	/* This function may be called before enabling MMU, or mapping KVA */
	if ((reg_sctlr_el1_read() & SCTLR_M) == 0)
		return false;

	/* device mapping will be availabled after pmap_devmap_bootstrap() */
	if (!pmap_devmap_bootstrap_done)
		return false;

	return true;
}

#endif /* _KERNEL */

/* definitions of TAG and PAC in pointers */
#define AARCH64_ADDRTOP_TAG_BIT		55
#define AARCH64_ADDRTOP_TAG		__BIT(55)	/* ECR_EL1.TBI[01]=1 */
#define AARCH64_ADDRTOP_MSB		__BIT(63)	/* ECR_EL1.TBI[01]=0 */
#define AARCH64_ADDRESS_TAG_MASK	__BITS(63,56)	/* if TCR.TBI[01]=1 */
#define AARCH64_ADDRESS_PAC_MASK	__BITS(54,48)	/* depend on VIRT_BIT */
#define AARCH64_ADDRESS_TAGPAC_MASK	\
			(AARCH64_ADDRESS_TAG_MASK|AARCH64_ADDRESS_PAC_MASK)

#ifdef _KERNEL
/*
 * Which is the address space of this VA?
 * return the space considering TBI. (PAC is not yet)
 *
 * return value: AARCH64_ADDRSPACE_{LOWER,UPPER}{_OUTOFRANGE}?
 */
#define AARCH64_ADDRSPACE_LOWER			0	/* -> TTBR0 */
#define AARCH64_ADDRSPACE_UPPER			1	/* -> TTBR1 */
#define AARCH64_ADDRSPACE_LOWER_OUTOFRANGE	-1	/* certainly fault */
#define AARCH64_ADDRSPACE_UPPER_OUTOFRANGE	-2	/* certainly fault */
static inline int
aarch64_addressspace(vaddr_t va)
{
	uint64_t addrtop, tbi;

	addrtop = va & AARCH64_ADDRTOP_TAG;
	tbi = addrtop ? TCR_TBI1 : TCR_TBI0;
	if (reg_tcr_el1_read() & tbi) {
		if (addrtop == 0) {
			/* lower address, and TBI0 enabled */
			if ((va & AARCH64_ADDRESS_PAC_MASK) != 0)
				return AARCH64_ADDRSPACE_LOWER_OUTOFRANGE;
			return AARCH64_ADDRSPACE_LOWER;
		}
		/* upper address, and TBI1 enabled */
		if ((va & AARCH64_ADDRESS_PAC_MASK) != AARCH64_ADDRESS_PAC_MASK)
			return AARCH64_ADDRSPACE_UPPER_OUTOFRANGE;
		return AARCH64_ADDRSPACE_UPPER;
	}

	addrtop = va & AARCH64_ADDRTOP_MSB;
	if (addrtop == 0) {
		/* lower address, and TBI0 disabled */
		if ((va & AARCH64_ADDRESS_TAGPAC_MASK) != 0)
			return AARCH64_ADDRSPACE_LOWER_OUTOFRANGE;
		return AARCH64_ADDRSPACE_LOWER;
	}
	/* upper address, and TBI1 disabled */
	if ((va & AARCH64_ADDRESS_TAGPAC_MASK) != AARCH64_ADDRESS_TAGPAC_MASK)
		return AARCH64_ADDRSPACE_UPPER_OUTOFRANGE;
	return AARCH64_ADDRSPACE_UPPER;
}

static inline vaddr_t
aarch64_untag_address(vaddr_t va)
{
	uint64_t addrtop, tbi;

	addrtop = va & AARCH64_ADDRTOP_TAG;
	tbi = addrtop ? TCR_TBI1 : TCR_TBI0;
	if (reg_tcr_el1_read() & tbi) {
		if (addrtop == 0) {
			/* lower address, and TBI0 enabled */
			return va & ~AARCH64_ADDRESS_TAG_MASK;
		}
		/* upper address, and TBI1 enabled */
		return va | AARCH64_ADDRESS_TAG_MASK;
	}

	/* TBI[01] is disabled, nothing to do */
	return va;
}

#endif /* _KERNEL */

static __inline uint64_t
aarch64_strip_pac(uint64_t __val)
{
	if (__val & AARCH64_ADDRTOP_TAG)
		return __val | AARCH64_ADDRESS_TAGPAC_MASK;
	return __val & ~AARCH64_ADDRESS_TAGPAC_MASK;
}

#endif /* _AARCH64_CPUFUNC_H_ */
