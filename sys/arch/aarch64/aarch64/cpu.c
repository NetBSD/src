/* $NetBSD: cpu.c,v 1.1.2.2 2018/04/07 04:12:10 pgoyette Exp $ */

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
__KERNEL_RCSID(1, "$NetBSD: cpu.c,v 1.1.2.2 2018/04/07 04:12:10 pgoyette Exp $");

#include "locators.h"
#include "opt_multiprocessor.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/cpu.h>
#include <sys/kmem.h>

#include <aarch64/armreg.h>
#include <aarch64/cpu.h>
#include <aarch64/cpufunc.h>
#include <aarch64/machdep.h>

void cpu_attach(device_t, cpuid_t);
static void cpu_identify(device_t self, struct cpu_info *);
static void cpu_identify2(device_t self, struct cpu_info *);

void
cpu_attach(device_t dv, cpuid_t id)
{
	struct cpu_info *ci;

	if (id == 0) {
		ci = curcpu();

	} else {
#ifdef MULTIPROCESSOR
		//XXXAARCH64: notyet?

		uint64_t mpidr = reg_mpidr_el1_read();

		KASSERT(cpu_info[id] == NULL);
		ci = kmem_zalloc(sizeof(*ci), KM_SLEEP);
		ci->ci_cpl = IPL_HIGH;
		ci->ci_cpuid = id;
		if (mpidr & MPIDR_MT) {
			ci->ci_data.cpu_smt_id = mpidr & MPIDR_AFF0;
			ci->ci_data.cpu_core_id = mpidr & MPIDR_AFF1;
			ci->ci_data.cpu_package_id = mpidr & MPIDR_AFF2;
		} else {
			ci->ci_data.cpu_core_id = mpidr & MPIDR_AFF0;
			ci->ci_data.cpu_package_id = mpidr & MPIDR_AFF1;
		}
		ci->ci_data.cpu_cc_freq = cpu_info_store.ci_data.cpu_cc_freq;
		cpu_info[ci->ci_cpuid] = ci;
		if ((arm_cpu_hatched & (1 << id)) == 0) {
			ci->ci_dev = dv;
			dv->dv_private = ci;

			aprint_naive(": disabled\n");
			aprint_normal(": disabled (uniprocessor kernel)\n");
			return;
		}
#else
		aprint_naive(": disabled\n");
		aprint_normal(": disabled (uniprocessor kernel)\n");
		return;
#endif
	}

	ci->ci_dev = dv;
	dv->dv_private = ci;

#ifdef MULTIPROCESSOR
	if (caa->caa_cpucore != 0) {
		aprint_naive("\n");
		aprint_normal(": %s\n", cpu_getmodel());
		mi_cpu_attach(ci);

		// XXXAARCH64
		//pmap_tlb_info_attach();
		panic("notyet");
	}
#endif

	fpu_attach(ci);

	cpu_identify(dv, ci);
	cpu_identify2(dv, ci);
}

struct cpuidtab {
	uint32_t cpu_partnum;
	const char *cpu_name;
	const char *cpu_class;
	const char *cpu_architecture;
};

#define CPU_PARTMASK	(CPU_ID_IMPLEMENTOR_MASK | CPU_ID_PARTNO_MASK)

const struct cpuidtab cpuids[] = {
	{ CPU_ID_CORTEXA53R0 & CPU_PARTMASK, "Cortex-A53", "Cortex", "V8-A" },
	{ CPU_ID_CORTEXA57R0 & CPU_PARTMASK, "Cortex-A57", "Cortex", "V8-A" },
	{ CPU_ID_CORTEXA72R0 & CPU_PARTMASK, "Cortex-A72", "Cortex", "V8-A" },
	{ CPU_ID_CORTEXA73R0 & CPU_PARTMASK, "Cortex-A73", "Cortex", "V8-A" },
	{ CPU_ID_CORTEXA55R1 & CPU_PARTMASK, "Cortex-A55", "Cortex", "V8.2-A" },
	{ CPU_ID_CORTEXA75R2 & CPU_PARTMASK, "Cortex-A75", "Cortex", "V8.2-A" }
};

static void
identify_aarch64_model(uint32_t cpuid, char *buf, size_t len)
{
	int i;
	uint32_t cpupart, variant, revision;

	cpupart = cpuid & CPU_PARTMASK;
	variant = __SHIFTOUT(cpuid, CPU_ID_VARIANT_MASK);
	revision = __SHIFTOUT(cpuid, CPU_ID_REVISION_MASK);

	for (i = 0; i < __arraycount(cpuids); i++) {
		if (cpupart == cpuids[i].cpu_partnum) {
			snprintf(buf, len, "%s r%dp%d (%s %s core)",
			    cpuids[i].cpu_name, variant, revision,
			    cpuids[i].cpu_class,
			    cpuids[i].cpu_architecture);
			return;
		}
	}

	snprintf(buf, len, "unknown CPU (ID = 0x%08x)", cpuid);
}

static int
prt_cache(device_t self, int level)
{
	struct aarch64_cache_info *cinfo;
	struct aarch64_cache_unit *cunit;
	u_int purging;
	int i;
	const char *cacheable, *cachetype;

	cinfo = &aarch64_cache_info[level];

	if (cinfo->cacheable == CACHE_CACHEABLE_NONE)
		return -1;

	for (i = 0; i < 2; i++) {
		switch (cinfo->cacheable) {
		case CACHE_CACHEABLE_ICACHE:
			cunit = &cinfo->icache;
			cacheable = "Instruction";
			break;
		case CACHE_CACHEABLE_DCACHE:
			cunit = &cinfo->dcache;
			cacheable = "Data";
			break;
		case CACHE_CACHEABLE_IDCACHE:
			if (i == 0) {
				cunit = &cinfo->icache;
				cacheable = "Instruction";
			} else {
				cunit = &cinfo->dcache;
				cacheable = "Data";
			}
			break;
		case CACHE_CACHEABLE_UNIFIED:
			cunit = &cinfo->dcache;
			cacheable = "Unified";
			break;
		}

		switch (cunit->cache_type) {
		case CACHE_TYPE_VIVT:
			cachetype = "VIVT";
			break;
		case CACHE_TYPE_VIPT:
			cachetype = "VIPT";
			break;
		case CACHE_TYPE_PIPT:
			cachetype = "PIPT";
			break;
		}

		purging = cunit->cache_purging;
		aprint_normal_dev(self,
		    "L%d %dKB/%dB %d-way%s%s%s%s %s %s cache\n",
		    level + 1,
		    cunit->cache_size / 1024,
		    cunit->cache_line_size,
		    cunit->cache_ways,
		    (purging & CACHE_PURGING_WT) ? " write-through" : "",
		    (purging & CACHE_PURGING_WB) ? " write-back" : "",
		    (purging & CACHE_PURGING_RA) ? " read-allocate" : "",
		    (purging & CACHE_PURGING_WA) ? " write-allocate" : "",
		    cachetype, cacheable);

		if (cinfo->cacheable != CACHE_CACHEABLE_IDCACHE)
			break;
	}

	return 0;
}

static void
cpu_identify(device_t self, struct cpu_info *ci)
{
	uint64_t mpidr;
	int level;
	uint32_t cpuid;
	uint32_t ctr, sctlr;	/* for cache */
	char model[128];

	cpuid = reg_midr_el1_read();
	identify_aarch64_model(cpuid, model, sizeof(model));
	if (ci->ci_cpuid == 0)
		cpu_setmodel("%s", model);

	aprint_naive("\n");
	aprint_normal(": %s\n", model);


	mpidr = reg_mpidr_el1_read();
	aprint_normal_dev(self, "CPU Affinity %llu-%llu-%llu-%llu\n",
	    __SHIFTOUT(mpidr, MPIDR_AFF3),
	    __SHIFTOUT(mpidr, MPIDR_AFF2),
	    __SHIFTOUT(mpidr, MPIDR_AFF1),
	    __SHIFTOUT(mpidr, MPIDR_AFF0));


	/* SCTLR - System Control Register */
	sctlr = reg_sctlr_el1_read();
	if (sctlr & SCTLR_I)
		aprint_normal_dev(self, "IC enabled");
	else
		aprint_normal_dev(self, "IC disabled");

	if (sctlr & SCTLR_C)
		aprint_normal(", DC enabled");
	else
		aprint_normal(", DC disabled");

	if (sctlr & SCTLR_A)
		aprint_normal(", Alignment check enabled\n");
	else {
		switch (sctlr & (SCTLR_SA | SCTLR_SA0)) {
		case SCTLR_SA | SCTLR_SA0:
			aprint_normal(
			    ", EL0/EL1 stack Alignment check enabled\n");
			break;
		case SCTLR_SA:
			aprint_normal(", EL1 stack Alignment check enabled\n");
			break;
		case SCTLR_SA0:
			aprint_normal(", EL0 stack Alignment check enabled\n");
			break;
		case 0:
			aprint_normal(", Alignment check disabled\n");
			break;
		}
	}

	/*
	 * CTR - Cache Type Register
	 */
	ctr = reg_ctr_el0_read();
	aprint_normal_dev(self, "Cache Writeback Granule %lluB,"
	    " Exclusives Reservation Granule %lluB\n",
	    __SHIFTOUT(ctr, CTR_EL0_CWG_LINE) * 4,
	    __SHIFTOUT(ctr, CTR_EL0_ERG_LINE) * 4);

	aprint_normal_dev(self, "Dcache line %ld, Icache line %ld\n",
	    sizeof(int) << __SHIFTOUT(ctr, CTR_EL0_DMIN_LINE),
	    sizeof(int) << __SHIFTOUT(ctr, CTR_EL0_IMIN_LINE));

	for (level = 0; level < MAX_CACHE_LEVEL; level++) {
		if (prt_cache(self, level) < 0)
			break;
	}
}


/*
 * identify vfp, etc.
 */
static void
cpu_identify2(device_t self, struct cpu_info *ci)
{
	uint64_t aidr, revidr;
	uint64_t dfr0, mmfr0;
	uint64_t isar0, pfr0, mvfr0, mvfr1;

	aidr = reg_id_aa64isar0_el1_read();
	revidr = reg_revidr_el1_read();

	dfr0 = reg_id_aa64dfr0_el1_read();
	mmfr0 = reg_id_aa64mmfr0_el1_read();

	isar0 = reg_id_aa64isar0_el1_read();
	pfr0 = reg_id_aa64pfr0_el1_read();
	mvfr0 = reg_mvfr0_el1_read();
	mvfr1 = reg_mvfr1_el1_read();


	aprint_normal_dev(self, "revID=0x%llx", revidr);

	/* ID_AA64DFR0_EL1 */
	switch (__SHIFTOUT(dfr0, ID_AA64DFR0_EL1_PMUVER)) {
	case ID_AA64DFR0_EL1_PMUVER_V3:
		aprint_normal(", PMCv3");
		break;
	case ID_AA64DFR0_EL1_PMUVER_NOV3:
		aprint_normal(", PMC");
		break;
	}

	/* ID_AA64MMFR0_EL1 */
	switch (__SHIFTOUT(mmfr0, ID_AA64MMFR0_EL1_TGRAN4)) {
	case ID_AA64MMFR0_EL1_TGRAN4_4KB:
		aprint_normal(", 4k table");
		break;
	}
	switch (__SHIFTOUT(mmfr0, ID_AA64MMFR0_EL1_TGRAN16)) {
	case ID_AA64MMFR0_EL1_TGRAN16_16KB:
		aprint_normal(", 16k table");
		break;
	}
	switch (__SHIFTOUT(mmfr0, ID_AA64MMFR0_EL1_TGRAN64)) {
	case ID_AA64MMFR0_EL1_TGRAN64_64KB:
		aprint_normal(", 64k table");
		break;
	}

	switch (__SHIFTOUT(mmfr0, ID_AA64MMFR0_EL1_ASIDBITS)) {
	case ID_AA64MMFR0_EL1_ASIDBITS_8BIT:
		aprint_normal(", 8bit ASID");
		break;
	case ID_AA64MMFR0_EL1_ASIDBITS_16BIT:
		aprint_normal(", 16bit ASID");
		break;
	}
	aprint_normal("\n");



	aprint_normal_dev(self, "auxID=0x%llx", aidr);

	/* PFR0 */
	switch (__SHIFTOUT(pfr0, ID_AA64PFR0_EL1_GIC)) {
	case ID_AA64PFR0_EL1_GIC_CPUIF_EN:
		aprint_normal(", GICv3");
		break;
	}
	switch (__SHIFTOUT(pfr0, ID_AA64PFR0_EL1_FP)) {
	case ID_AA64PFR0_EL1_FP_IMPL:
		aprint_normal(", FP");
		break;
	}

	/* ISAR0 */
	switch (__SHIFTOUT(isar0, ID_AA64ISAR0_EL1_CRC32)) {
	case ID_AA64ISAR0_EL1_CRC32_CRC32X:
		aprint_normal(", CRC32");
		break;
	}
	switch (__SHIFTOUT(isar0, ID_AA64ISAR0_EL1_SHA1)) {
	case ID_AA64ISAR0_EL1_SHA1_SHA1CPMHSU:
		aprint_normal(", SHA1");
		break;
	}
	switch (__SHIFTOUT(isar0, ID_AA64ISAR0_EL1_SHA2)) {
	case ID_AA64ISAR0_EL1_SHA2_SHA256HSU:
		aprint_normal(", SHA256");
		break;
	}
	switch (__SHIFTOUT(isar0, ID_AA64ISAR0_EL1_AES)) {
	case ID_AA64ISAR0_EL1_AES_AES:
		aprint_normal(", AES");
		break;
	case ID_AA64ISAR0_EL1_AES_PMUL:
		aprint_normal(", AES+PMULL");
		break;
	}


	/* PFR0:AdvSIMD */
	switch (__SHIFTOUT(pfr0, ID_AA64PFR0_EL1_ADVSIMD)) {
	case ID_AA64PFR0_EL1_ADV_SIMD_IMPL:
		aprint_normal(", NEON");
		break;
	}

	/* MVFR0/MVFR1 */
	switch (__SHIFTOUT(mvfr0, MVFR0_FPROUND)) {
	case MVFR0_FPROUND_ALL:
		aprint_normal(", rounding");
		break;
	}
	switch (__SHIFTOUT(mvfr0, MVFR0_FPTRAP)) {
	case MVFR0_FPTRAP_TRAP:
		aprint_normal(", exceptions");
		break;
	}
	switch (__SHIFTOUT(mvfr1, MVFR1_FPDNAN)) {
	case MVFR1_FPDNAN_NAN:
		aprint_normal(", NaN propagation");
		break;
	}
	switch (__SHIFTOUT(mvfr1, MVFR1_FPFTZ)) {
	case MVFR1_FPFTZ_DENORMAL:
		aprint_normal(", denormals");
		break;
	}
	switch (__SHIFTOUT(mvfr0, MVFR0_SIMDREG)) {
	case MVFR0_SIMDREG_16x64:
		aprint_normal(", 16x64bitRegs");
		break;
	case MVFR0_SIMDREG_32x64:
		aprint_normal(", 32x64bitRegs");
		break;
	}
	switch (__SHIFTOUT(mvfr1, MVFR1_SIMDFMAC)) {
	case MVFR1_SIMDFMAC_FMAC:
		aprint_normal(", Fused Multiply-Add");
		break;
	}

	aprint_normal("\n");
}
