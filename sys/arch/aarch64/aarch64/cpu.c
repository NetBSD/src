/* $NetBSD: cpu.c,v 1.20.2.1 2019/10/23 19:14:19 martin Exp $ */

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
__KERNEL_RCSID(1, "$NetBSD: cpu.c,v 1.20.2.1 2019/10/23 19:14:19 martin Exp $");

#include "locators.h"
#include "opt_arm_debug.h"
#include "opt_fdt.h"
#include "opt_multiprocessor.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/atomic.h>
#include <sys/device.h>
#include <sys/cpu.h>
#include <sys/kmem.h>
#include <sys/reboot.h>
#include <sys/sysctl.h>

#include <aarch64/armreg.h>
#include <aarch64/cpu.h>
#include <aarch64/cpufunc.h>
#include <aarch64/machdep.h>

#ifdef FDT
#include <arm/fdt/arm_fdtvar.h>
#endif

#ifdef VERBOSE_INIT_ARM
#define VPRINTF(...)	printf(__VA_ARGS__)
#else
#define VPRINTF(...)	__nothing
#endif

void cpu_attach(device_t, cpuid_t);
static void identify_aarch64_model(uint32_t, char *, size_t);
static void cpu_identify(device_t self, struct cpu_info *);
static void cpu_identify1(device_t self, struct cpu_info *);
static void cpu_identify2(device_t self, struct cpu_info *);
static void cpu_setup_id(struct cpu_info *);
static void cpu_setup_sysctl(device_t, struct cpu_info *);

#ifdef MULTIPROCESSOR
uint64_t cpu_mpidr[MAXCPUS];

volatile u_int aarch64_cpu_mbox[howmany(MAXCPUS, sizeof(u_int))] __cacheline_aligned = { 0 };
volatile u_int aarch64_cpu_hatched[howmany(MAXCPUS, sizeof(u_int))] __cacheline_aligned = { 0 };
u_int arm_cpu_max = 1;

static kmutex_t cpu_hatch_lock;
#endif /* MULTIPROCESSOR */

#ifdef MULTIPROCESSOR
#define NCPUINFO	MAXCPUS
#else
#define NCPUINFO	1
#endif /* MULTIPROCESSOR */

/*
 * Our exported CPU info;
 * these will be refered from secondary cpus in the middle of hatching.
 */
struct cpu_info cpu_info_store[NCPUINFO] = {
	[0] = {
		.ci_cpl = IPL_HIGH,
		.ci_curlwp = &lwp0
	}
};

struct cpu_info *cpu_info[NCPUINFO] __read_mostly = {
	[0] = &cpu_info_store[0]
};

void
cpu_attach(device_t dv, cpuid_t id)
{
	struct cpu_info *ci;
	const int unit = device_unit(dv);
	uint64_t mpidr;

	if (unit == 0) {
		ci = curcpu();
		ci->ci_cpuid = id;
		cpu_setup_id(ci);
	} else {
#ifdef MULTIPROCESSOR
		if ((boothowto & RB_MD1) != 0) {
			aprint_naive("\n");
			aprint_normal(": multiprocessor boot disabled\n");
			return;
		}

		KASSERT(unit < MAXCPUS);
		ci = &cpu_info_store[unit];

		ci->ci_cpl = IPL_HIGH;
		ci->ci_cpuid = id;
		// XXX big.LITTLE
		ci->ci_data.cpu_cc_freq = cpu_info_store[0].ci_data.cpu_cc_freq;
		/* ci_id is stored by own cpus when hatching */

		cpu_info[ncpu] = ci;
		if (cpu_hatched_p(unit) == 0) {
			ci->ci_dev = dv;
			dv->dv_private = ci;
			ci->ci_index = -1;

			aprint_naive(": disabled\n");
			aprint_normal(": disabled (unresponsive)\n");
			return;
		}
#else /* MULTIPROCESSOR */
		aprint_naive(": disabled\n");
		aprint_normal(": disabled (uniprocessor kernel)\n");
		return;
#endif /* MULTIPROCESSOR */
	}

	mpidr = ci->ci_id.ac_mpidr;
	if (mpidr & MPIDR_MT) {
		ci->ci_smt_id = __SHIFTOUT(mpidr, MPIDR_AFF0);
		ci->ci_core_id = __SHIFTOUT(mpidr, MPIDR_AFF1);
		ci->ci_package_id = __SHIFTOUT(mpidr, MPIDR_AFF2);
	} else {
		ci->ci_core_id = __SHIFTOUT(mpidr, MPIDR_AFF0);
		ci->ci_package_id = __SHIFTOUT(mpidr, MPIDR_AFF1);
	}

	ci->ci_dev = dv;
	dv->dv_private = ci;

	cpu_identify(ci->ci_dev, ci);
#ifdef MULTIPROCESSOR
	if (unit != 0) {
		mi_cpu_attach(ci);
		return;
	}
#endif /* MULTIPROCESSOR */

	set_cpufuncs();
	fpu_attach(ci);

	cpu_identify1(dv, ci);
	aarch64_getcacheinfo();
	aarch64_printcacheinfo(dv);
	cpu_identify2(dv, ci);

	cpu_setup_sysctl(dv, ci);
}

struct cpuidtab {
	uint32_t cpu_partnum;
	const char *cpu_name;
	const char *cpu_class;
	const char *cpu_architecture;
};

#define CPU_PARTMASK	(CPU_ID_IMPLEMENTOR_MASK | CPU_ID_PARTNO_MASK)

const struct cpuidtab cpuids[] = {
	{ CPU_ID_CORTEXA35R0 & CPU_PARTMASK, "Cortex-A35", "Cortex", "V8-A" },
	{ CPU_ID_CORTEXA53R0 & CPU_PARTMASK, "Cortex-A53", "Cortex", "V8-A" },
	{ CPU_ID_CORTEXA57R0 & CPU_PARTMASK, "Cortex-A57", "Cortex", "V8-A" },
	{ CPU_ID_CORTEXA55R1 & CPU_PARTMASK, "Cortex-A55", "Cortex", "V8.2-A+" },
	{ CPU_ID_CORTEXA65R0 & CPU_PARTMASK, "Cortex-A65", "Cortex", "V8.2-A+" },
	{ CPU_ID_CORTEXA72R0 & CPU_PARTMASK, "Cortex-A72", "Cortex", "V8-A" },
	{ CPU_ID_CORTEXA73R0 & CPU_PARTMASK, "Cortex-A73", "Cortex", "V8-A" },
	{ CPU_ID_CORTEXA75R2 & CPU_PARTMASK, "Cortex-A75", "Cortex", "V8.2-A+" },
	{ CPU_ID_CORTEXA76R3 & CPU_PARTMASK, "Cortex-A76", "Cortex", "V8.2-A+" },
	{ CPU_ID_CORTEXA76AER1 & CPU_PARTMASK, "Cortex-A76AE", "Cortex", "V8.2-A+" },
	{ CPU_ID_CORTEXA77R0 & CPU_PARTMASK, "Cortex-A77", "Cortex", "V8.2-A+" },
	{ CPU_ID_EMAG8180 & CPU_PARTMASK, "Ampere eMAG", "Skylark", "V8-A" },
	{ CPU_ID_THUNDERXRX, "Cavium ThunderX", "Cavium", "V8-A" },
	{ CPU_ID_THUNDERX81XXRX, "Cavium ThunderX CN81XX", "Cavium", "V8-A" },
	{ CPU_ID_THUNDERX83XXRX, "Cavium ThunderX CN83XX", "Cavium", "V8-A" },
	{ CPU_ID_THUNDERX2RX, "Cavium ThunderX2", "Cavium", "V8.1-A" },
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

static void
cpu_identify(device_t self, struct cpu_info *ci)
{
	char model[128];

	identify_aarch64_model(ci->ci_id.ac_midr, model, sizeof(model));
	if (ci->ci_index == 0)
		cpu_setmodel("%s", model);

	aprint_naive("\n");
	aprint_normal(": %s\n", model);
	aprint_normal_dev(ci->ci_dev, "package %lu, core %lu, smt %lu\n",
	    ci->ci_package_id, ci->ci_core_id, ci->ci_smt_id);
}

static void
cpu_identify1(device_t self, struct cpu_info *ci)
{
	uint32_t ctr, sctlr;	/* for cache */

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
	aprint_normal_dev(self, "Cache Writeback Granule %" PRIu64 "B,"
	    " Exclusives Reservation Granule %" PRIu64 "B\n",
	    __SHIFTOUT(ctr, CTR_EL0_CWG_LINE) * 4,
	    __SHIFTOUT(ctr, CTR_EL0_ERG_LINE) * 4);

	aprint_normal_dev(self, "Dcache line %ld, Icache line %ld\n",
	    sizeof(int) << __SHIFTOUT(ctr, CTR_EL0_DMIN_LINE),
	    sizeof(int) << __SHIFTOUT(ctr, CTR_EL0_IMIN_LINE));
}


/*
 * identify vfp, etc.
 */
static void
cpu_identify2(device_t self, struct cpu_info *ci)
{
	struct aarch64_sysctl_cpu_id *id = &ci->ci_id;
	uint64_t dfr0;

	if (!CPU_IS_PRIMARY(ci)) {
		cpu_setup_id(ci);
		cpu_setup_sysctl(self, ci);
	}

	dfr0 = reg_id_aa64dfr0_el1_read();

	aprint_normal_dev(self, "revID=0x%" PRIx64, id->ac_revidr);

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
	switch (__SHIFTOUT(id->ac_aa64mmfr0, ID_AA64MMFR0_EL1_TGRAN4)) {
	case ID_AA64MMFR0_EL1_TGRAN4_4KB:
		aprint_normal(", 4k table");
		break;
	}
	switch (__SHIFTOUT(id->ac_aa64mmfr0, ID_AA64MMFR0_EL1_TGRAN16)) {
	case ID_AA64MMFR0_EL1_TGRAN16_16KB:
		aprint_normal(", 16k table");
		break;
	}
	switch (__SHIFTOUT(id->ac_aa64mmfr0, ID_AA64MMFR0_EL1_TGRAN64)) {
	case ID_AA64MMFR0_EL1_TGRAN64_64KB:
		aprint_normal(", 64k table");
		break;
	}

	switch (__SHIFTOUT(id->ac_aa64mmfr0, ID_AA64MMFR0_EL1_ASIDBITS)) {
	case ID_AA64MMFR0_EL1_ASIDBITS_8BIT:
		aprint_normal(", 8bit ASID");
		break;
	case ID_AA64MMFR0_EL1_ASIDBITS_16BIT:
		aprint_normal(", 16bit ASID");
		break;
	}
	aprint_normal("\n");



	aprint_normal_dev(self, "auxID=0x%" PRIx64, ci->ci_id.ac_aa64isar0);

	/* PFR0 */
	switch (__SHIFTOUT(id->ac_aa64pfr0, ID_AA64PFR0_EL1_GIC)) {
	case ID_AA64PFR0_EL1_GIC_CPUIF_EN:
		aprint_normal(", GICv3");
		break;
	}
	switch (__SHIFTOUT(id->ac_aa64pfr0, ID_AA64PFR0_EL1_FP)) {
	case ID_AA64PFR0_EL1_FP_IMPL:
		aprint_normal(", FP");
		break;
	}

	/* ISAR0 */
	switch (__SHIFTOUT(id->ac_aa64isar0, ID_AA64ISAR0_EL1_CRC32)) {
	case ID_AA64ISAR0_EL1_CRC32_CRC32X:
		aprint_normal(", CRC32");
		break;
	}
	switch (__SHIFTOUT(id->ac_aa64isar0, ID_AA64ISAR0_EL1_SHA1)) {
	case ID_AA64ISAR0_EL1_SHA1_SHA1CPMHSU:
		aprint_normal(", SHA1");
		break;
	}
	switch (__SHIFTOUT(id->ac_aa64isar0, ID_AA64ISAR0_EL1_SHA2)) {
	case ID_AA64ISAR0_EL1_SHA2_SHA256HSU:
		aprint_normal(", SHA256");
		break;
	}
	switch (__SHIFTOUT(id->ac_aa64isar0, ID_AA64ISAR0_EL1_AES)) {
	case ID_AA64ISAR0_EL1_AES_AES:
		aprint_normal(", AES");
		break;
	case ID_AA64ISAR0_EL1_AES_PMUL:
		aprint_normal(", AES+PMULL");
		break;
	}


	/* PFR0:AdvSIMD */
	switch (__SHIFTOUT(id->ac_aa64pfr0, ID_AA64PFR0_EL1_ADVSIMD)) {
	case ID_AA64PFR0_EL1_ADV_SIMD_IMPL:
		aprint_normal(", NEON");
		break;
	}

	/* MVFR0/MVFR1 */
	switch (__SHIFTOUT(id->ac_mvfr0, MVFR0_FPROUND)) {
	case MVFR0_FPROUND_ALL:
		aprint_normal(", rounding");
		break;
	}
	switch (__SHIFTOUT(id->ac_mvfr0, MVFR0_FPTRAP)) {
	case MVFR0_FPTRAP_TRAP:
		aprint_normal(", exceptions");
		break;
	}
	switch (__SHIFTOUT(id->ac_mvfr1, MVFR1_FPDNAN)) {
	case MVFR1_FPDNAN_NAN:
		aprint_normal(", NaN propagation");
		break;
	}
	switch (__SHIFTOUT(id->ac_mvfr1, MVFR1_FPFTZ)) {
	case MVFR1_FPFTZ_DENORMAL:
		aprint_normal(", denormals");
		break;
	}
	switch (__SHIFTOUT(id->ac_mvfr0, MVFR0_SIMDREG)) {
	case MVFR0_SIMDREG_16x64:
		aprint_normal(", 16x64bitRegs");
		break;
	case MVFR0_SIMDREG_32x64:
		aprint_normal(", 32x64bitRegs");
		break;
	}
	switch (__SHIFTOUT(id->ac_mvfr1, MVFR1_SIMDFMAC)) {
	case MVFR1_SIMDFMAC_FMAC:
		aprint_normal(", Fused Multiply-Add");
		break;
	}

	aprint_normal("\n");
}

/*
 * Fill in this CPUs id data.  Must be called from hatched cpus.
 */
static void
cpu_setup_id(struct cpu_info *ci)
{
	struct aarch64_sysctl_cpu_id *id = &ci->ci_id;

	memset(id, 0, sizeof *id);

	id->ac_midr      = reg_midr_el1_read();
	id->ac_revidr    = reg_revidr_el1_read();
	id->ac_mpidr     = reg_mpidr_el1_read();

	id->ac_aa64dfr0  = reg_id_aa64dfr0_el1_read();
	id->ac_aa64dfr1  = reg_id_aa64dfr1_el1_read();

	id->ac_aa64isar0 = reg_id_aa64isar0_el1_read();
	id->ac_aa64isar1 = reg_id_aa64isar1_el1_read();

	id->ac_aa64mmfr0 = reg_id_aa64mmfr0_el1_read();
	id->ac_aa64mmfr1 = reg_id_aa64mmfr1_el1_read();
	/* Only in ARMv8.2. */
	id->ac_aa64mmfr2 = 0 /* reg_id_aa64mmfr2_el1_read() */;

	id->ac_mvfr0     = reg_mvfr0_el1_read();
	id->ac_mvfr1     = reg_mvfr1_el1_read();
	id->ac_mvfr2     = reg_mvfr2_el1_read();

	/* Only in ARMv8.2. */
	id->ac_aa64zfr0  = 0 /* reg_id_aa64zfr0_el1_read() */;

	id->ac_aa64pfr0  = reg_id_aa64pfr0_el1_read();
	id->ac_aa64pfr1  = reg_id_aa64pfr1_el1_read();
}

/*
 * setup the per-cpu sysctl tree.
 */
static void
cpu_setup_sysctl(device_t dv, struct cpu_info *ci)
{
	const struct sysctlnode *cpunode = NULL;

	sysctl_createv(NULL, 0, NULL, &cpunode,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_NODE, device_xname(dv), NULL,
		       NULL, 0, NULL, 0,
		       CTL_MACHDEP,
		       CTL_CREATE, CTL_EOL);

	if (cpunode == NULL)
		return;

	sysctl_createv(NULL, 0, &cpunode, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_STRUCT, "cpu_id", NULL,
		       NULL, 0, &ci->ci_id, sizeof(ci->ci_id),
		       CTL_CREATE, CTL_EOL);
}

#ifdef MULTIPROCESSOR
void
cpu_boot_secondary_processors(void)
{
	u_int n, bit;

	if ((boothowto & RB_MD1) != 0)
		return;

	mutex_init(&cpu_hatch_lock, MUTEX_DEFAULT, IPL_NONE);

	VPRINTF("%s: starting secondary processors\n", __func__);

	/* send mbox to have secondary processors do cpu_hatch() */
	for (n = 0; n < __arraycount(aarch64_cpu_mbox); n++)
		atomic_or_uint(&aarch64_cpu_mbox[n], aarch64_cpu_hatched[n]);
	__asm __volatile ("sev; sev; sev");

	/* wait all cpus have done cpu_hatch() */
	for (n = 0; n < __arraycount(aarch64_cpu_mbox); n++) {
		while (membar_consumer(), aarch64_cpu_mbox[n] & aarch64_cpu_hatched[n]) {
			__asm __volatile ("wfe");
		}
		/* Add processors to kcpuset */
		for (bit = 0; bit < 32; bit++) {
			if (aarch64_cpu_hatched[n] & __BIT(bit))
				kcpuset_set(kcpuset_attached, n * 32 + bit);
		}
	}

	VPRINTF("%s: secondary processors hatched\n", __func__);
}

void
cpu_hatch(struct cpu_info *ci)
{
	KASSERT(curcpu() == ci);

	mutex_enter(&cpu_hatch_lock);

	set_cpufuncs();
	fpu_attach(ci);

	cpu_identify1(ci->ci_dev, ci);
	aarch64_getcacheinfo();
	aarch64_printcacheinfo(ci->ci_dev);
	cpu_identify2(ci->ci_dev, ci);

	mutex_exit(&cpu_hatch_lock);

	intr_cpu_init(ci);

#ifdef FDT
	arm_fdt_cpu_hatch(ci);
#endif
#ifdef MD_CPU_HATCH
	MD_CPU_HATCH(ci);	/* for non-fdt arch? */
#endif

	/*
	 * clear my bit of aarch64_cpu_mbox to tell cpu_boot_secondary_processors().
	 * there are cpu0,1,2,3, and if cpu2 is unresponsive,
	 * ci_index are each cpu0=0, cpu1=1, cpu2=undef, cpu3=2.
	 * therefore we have to use device_unit instead of ci_index for mbox.
	 */
	const u_int off = device_unit(ci->ci_dev) / 32;
	const u_int bit = device_unit(ci->ci_dev) % 32;
	atomic_and_uint(&aarch64_cpu_mbox[off], ~__BIT(bit));
	__asm __volatile ("sev; sev; sev");
}

bool
cpu_hatched_p(u_int cpuindex)
{
	const u_int off = cpuindex / 32;
	const u_int bit = cpuindex % 32;
	membar_consumer();
	return (aarch64_cpu_hatched[off] & __BIT(bit)) != 0;
}
#endif /* MULTIPROCESSOR */
