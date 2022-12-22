/* $NetBSD: cpu.c,v 1.72 2022/12/22 06:58:47 ryo Exp $ */

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
__KERNEL_RCSID(1, "$NetBSD: cpu.c,v 1.72 2022/12/22 06:58:47 ryo Exp $");

#include "locators.h"
#include "opt_arm_debug.h"
#include "opt_ddb.h"
#include "opt_fdt.h"
#include "opt_multiprocessor.h"

#include <sys/param.h>
#include <sys/atomic.h>
#include <sys/cpu.h>
#include <sys/device.h>
#include <sys/kmem.h>
#include <sys/reboot.h>
#include <sys/rndsource.h>
#include <sys/sysctl.h>
#include <sys/systm.h>

#include <crypto/aes/aes_impl.h>
#include <crypto/aes/arch/arm/aes_armv8.h>
#include <crypto/aes/arch/arm/aes_neon.h>
#include <crypto/chacha/chacha_impl.h>
#include <crypto/chacha/arch/arm/chacha_neon.h>

#include <aarch64/armreg.h>
#include <aarch64/cpu.h>
#include <aarch64/cpu_counter.h>
#ifdef DDB
#include <aarch64/db_machdep.h>
#endif
#include <aarch64/machdep.h>

#include <arm/cpufunc.h>
#include <arm/cpu_topology.h>
#ifdef FDT
#include <arm/fdt/arm_fdtvar.h>
#endif

#ifdef VERBOSE_INIT_ARM
#define VPRINTF(...)	printf(__VA_ARGS__)
#else
#define VPRINTF(...)	__nothing
#endif

void cpu_attach(device_t, cpuid_t);
void cpu_setup_id(struct cpu_info *);

static void identify_aarch64_model(uint32_t, char *, size_t);
static void cpu_identify(device_t self, struct cpu_info *);
static void cpu_identify1(device_t self, struct cpu_info *);
static void cpu_identify2(device_t self, struct cpu_info *);
static void cpu_init_counter(struct cpu_info *);
static void cpu_setup_sysctl(device_t, struct cpu_info *);
static void cpu_setup_rng(device_t, struct cpu_info *);
static void cpu_setup_aes(device_t, struct cpu_info *);
static void cpu_setup_chacha(device_t, struct cpu_info *);

#ifdef MULTIPROCESSOR
#define NCPUINFO	MAXCPUS
#else
#define NCPUINFO	1
#endif /* MULTIPROCESSOR */

/*
 * Our exported cpu_info structs; these will be first used by the
 * secondary cpus as part of cpu_mpstart and the hatching process.
 */
struct cpu_info cpu_info_store[NCPUINFO] = {
	[0] = {
		.ci_cpl = IPL_HIGH,
		.ci_curlwp = &lwp0
	}
};

void
cpu_attach(device_t dv, cpuid_t id)
{
	struct cpu_info *ci;
	const int unit = device_unit(dv);

	if (unit == 0) {
		ci = curcpu();
		ci->ci_cpuid = id;
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
		/* ci_id is stored by own cpus when hatching */

		cpu_info[ncpu] = ci;
		if (cpu_hatched_p(unit) == 0) {
			ci->ci_dev = dv;
			device_set_private(dv, ci);
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

	ci->ci_dev = dv;
	device_set_private(dv, ci);

	ci->ci_kfpu_spl = -1;

	arm_cpu_do_topology(ci);	// XXXNH move this after mi_cpu_attach
	cpu_identify(dv, ci);

	cpu_setup_sysctl(dv, ci);

#ifdef MULTIPROCESSOR
	if (unit != 0) {
		mi_cpu_attach(ci);
		pmap_tlb_info_attach(&pmap_tlb0_info, ci);
		aarch64_parsecacheinfo(ci);
	}
#endif /* MULTIPROCESSOR */

	fpu_attach(ci);

	cpu_identify1(dv, ci);
	aarch64_printcacheinfo(dv, ci);
	cpu_identify2(dv, ci);

	if (unit != 0) {
	    return;
	}

#ifdef DDB
	db_machdep_init(ci);
#endif

	cpu_init_counter(ci);

	/* These currently only check the BP. */
	cpu_setup_rng(dv, ci);
	cpu_setup_aes(dv, ci);
	cpu_setup_chacha(dv, ci);
}

struct cpuidtab {
	uint32_t cpu_partnum;
	const char *cpu_name;
	const char *cpu_vendor;
	const char *cpu_architecture;
};

#define CPU_PARTMASK	(CPU_ID_IMPLEMENTOR_MASK | CPU_ID_PARTNO_MASK)

const struct cpuidtab cpuids[] = {
	{ CPU_ID_CORTEXA35R0 & CPU_PARTMASK, "Cortex-A35", "Arm", "v8-A" },
	{ CPU_ID_CORTEXA53R0 & CPU_PARTMASK, "Cortex-A53", "Arm", "v8-A" },
	{ CPU_ID_CORTEXA57R0 & CPU_PARTMASK, "Cortex-A57", "Arm", "v8-A" },
	{ CPU_ID_CORTEXA55R1 & CPU_PARTMASK, "Cortex-A55", "Arm", "v8.2-A+" },
	{ CPU_ID_CORTEXA65R0 & CPU_PARTMASK, "Cortex-A65", "Arm", "v8.2-A+" },
	{ CPU_ID_CORTEXA72R0 & CPU_PARTMASK, "Cortex-A72", "Arm", "v8-A" },
	{ CPU_ID_CORTEXA73R0 & CPU_PARTMASK, "Cortex-A73", "Arm", "v8-A" },
	{ CPU_ID_CORTEXA75R2 & CPU_PARTMASK, "Cortex-A75", "Arm", "v8.2-A+" },
	{ CPU_ID_CORTEXA76R3 & CPU_PARTMASK, "Cortex-A76", "Arm", "v8.2-A+" },
	{ CPU_ID_CORTEXA76AER1 & CPU_PARTMASK, "Cortex-A76AE", "Arm", "v8.2-A+" },
	{ CPU_ID_CORTEXA77R0 & CPU_PARTMASK, "Cortex-A77", "Arm", "v8.2-A+" },
	{ CPU_ID_NVIDIADENVER2 & CPU_PARTMASK, "Denver2", "NVIDIA", "v8-A" },
	{ CPU_ID_EMAG8180 & CPU_PARTMASK, "eMAG", "Ampere", "v8-A" },
	{ CPU_ID_NEOVERSEE1R1 & CPU_PARTMASK, "Neoverse E1", "Arm", "v8.2-A+" },
	{ CPU_ID_NEOVERSEN1R3 & CPU_PARTMASK, "Neoverse N1", "Arm", "v8.2-A+" },
	{ CPU_ID_THUNDERXRX, "ThunderX", "Cavium", "v8-A" },
	{ CPU_ID_THUNDERX81XXRX, "ThunderX CN81XX", "Cavium", "v8-A" },
	{ CPU_ID_THUNDERX83XXRX, "ThunderX CN83XX", "Cavium", "v8-A" },
	{ CPU_ID_THUNDERX2RX, "ThunderX2", "Marvell", "v8.1-A" },
	{ CPU_ID_APPLE_M1_ICESTORM & CPU_PARTMASK, "M1 Icestorm", "Apple", "Apple Silicon" },
	{ CPU_ID_APPLE_M1_FIRESTORM & CPU_PARTMASK, "M1 Firestorm", "Apple", "Apple Silicon" },
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
			snprintf(buf, len, "%s %s r%dp%d (%s)",
			    cpuids[i].cpu_vendor, cpuids[i].cpu_name,
			    variant, revision,
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
	const char *m;

	identify_aarch64_model(ci->ci_id.ac_midr, model, sizeof(model));

	aprint_naive("\n");
	aprint_normal(": %s, id 0x%lx\n", model, ci->ci_cpuid);
	aprint_normal_dev(ci->ci_dev, "package %u, core %u, smt %u\n",
	    ci->ci_package_id, ci->ci_core_id, ci->ci_smt_id);

	if (ci->ci_index == 0) {
		m = cpu_getmodel();
		if (m == NULL || *m == 0)
			cpu_setmodel("%s", model);

		if (CPU_ID_ERRATA_CAVIUM_THUNDERX_1_1_P(ci->ci_id.ac_midr))
			aprint_normal("WARNING: ThunderX Pass 1.1 detected.\n"
			    "This has known hardware bugs that may cause the "
			    "incorrect operation of atomic operations.\n");
	}
}

static void
cpu_identify1(device_t self, struct cpu_info *ci)
{
	struct aarch64_sysctl_cpu_id *id = &ci->ci_id;
	uint64_t sctlr = ci->ci_sctlr_el1;

	if (sctlr & SCTLR_I)
		aprint_verbose_dev(self, "IC enabled");
	else
		aprint_verbose_dev(self, "IC disabled");

	if (sctlr & SCTLR_C)
		aprint_verbose(", DC enabled");
	else
		aprint_verbose(", DC disabled");

	if (sctlr & SCTLR_A)
		aprint_verbose(", Alignment check enabled\n");
	else {
		switch (sctlr & (SCTLR_SA | SCTLR_SA0)) {
		case SCTLR_SA | SCTLR_SA0:
			aprint_verbose(
			    ", EL0/EL1 stack Alignment check enabled\n");
			break;
		case SCTLR_SA:
			aprint_verbose(", EL1 stack Alignment check enabled\n");
			break;
		case SCTLR_SA0:
			aprint_verbose(", EL0 stack Alignment check enabled\n");
			break;
		case 0:
			aprint_verbose(", Alignment check disabled\n");
			break;
		}
	}

	/*
	 * CTR - Cache Type Register
	 */
	const uint64_t ctr = id->ac_ctr;
	const uint64_t clidr = id->ac_clidr;
	aprint_verbose_dev(self, "Cache Writeback Granule %" PRIu64 "B,"
	    " Exclusives Reservation Granule %" PRIu64 "B\n",
	    __SHIFTOUT(ctr, CTR_EL0_CWG_LINE) * 4,
	    __SHIFTOUT(ctr, CTR_EL0_ERG_LINE) * 4);

	aprint_verbose_dev(self, "Dcache line %ld, Icache line %ld"
	    ", DIC=%lu, IDC=%lu, LoUU=%lu, LoC=%lu, LoUIS=%lu\n",
	    sizeof(int) << __SHIFTOUT(ctr, CTR_EL0_DMIN_LINE),
	    sizeof(int) << __SHIFTOUT(ctr, CTR_EL0_IMIN_LINE),
	    __SHIFTOUT(ctr, CTR_EL0_DIC),
	    __SHIFTOUT(ctr, CTR_EL0_IDC),
	    __SHIFTOUT(clidr, CLIDR_LOUU),
	    __SHIFTOUT(clidr, CLIDR_LOC),
	    __SHIFTOUT(clidr, CLIDR_LOUIS));
}


/*
 * identify vfp, etc.
 */
static void
cpu_identify2(device_t self, struct cpu_info *ci)
{
	struct aarch64_sysctl_cpu_id * const id = &ci->ci_id;

	aprint_debug_dev(self, "midr=0x%" PRIx32 " mpidr=0x%" PRIx32 "\n",
	    (uint32_t)id->ac_midr, (uint32_t)id->ac_mpidr);
	aprint_verbose_dev(self, "revID=0x%" PRIx64, id->ac_revidr);

	/* ID_AA64DFR0_EL1 */
	switch (__SHIFTOUT(id->ac_aa64dfr0, ID_AA64DFR0_EL1_PMUVER)) {
	case ID_AA64DFR0_EL1_PMUVER_V3:
		aprint_verbose(", PMCv3");
		break;
	case ID_AA64DFR0_EL1_PMUVER_NOV3:
		aprint_verbose(", PMC");
		break;
	}

	/* ID_AA64MMFR0_EL1 */
	switch (__SHIFTOUT(id->ac_aa64mmfr0, ID_AA64MMFR0_EL1_TGRAN4)) {
	case ID_AA64MMFR0_EL1_TGRAN4_4KB:
		aprint_verbose(", 4k table");
		break;
	}
	switch (__SHIFTOUT(id->ac_aa64mmfr0, ID_AA64MMFR0_EL1_TGRAN16)) {
	case ID_AA64MMFR0_EL1_TGRAN16_16KB:
		aprint_verbose(", 16k table");
		break;
	}
	switch (__SHIFTOUT(id->ac_aa64mmfr0, ID_AA64MMFR0_EL1_TGRAN64)) {
	case ID_AA64MMFR0_EL1_TGRAN64_64KB:
		aprint_verbose(", 64k table");
		break;
	}

	switch (__SHIFTOUT(id->ac_aa64mmfr0, ID_AA64MMFR0_EL1_ASIDBITS)) {
	case ID_AA64MMFR0_EL1_ASIDBITS_8BIT:
		aprint_verbose(", 8bit ASID");
		break;
	case ID_AA64MMFR0_EL1_ASIDBITS_16BIT:
		aprint_verbose(", 16bit ASID");
		break;
	}
	aprint_verbose("\n");

	aprint_verbose_dev(self, "auxID=0x%" PRIx64, ci->ci_id.ac_aa64isar0);

	/* PFR0 */
	switch (__SHIFTOUT(id->ac_aa64pfr0, ID_AA64PFR0_EL1_CSV3)) {
	case ID_AA64PFR0_EL1_CSV3_IMPL:
		aprint_verbose(", CSV3");
		break;
	}
	switch (__SHIFTOUT(id->ac_aa64pfr0, ID_AA64PFR0_EL1_CSV2)) {
	case ID_AA64PFR0_EL1_CSV2_IMPL:
		aprint_verbose(", CSV2");
		break;
	}
	switch (__SHIFTOUT(id->ac_aa64pfr0, ID_AA64PFR0_EL1_GIC)) {
	case ID_AA64PFR0_EL1_GIC_CPUIF_EN:
		aprint_verbose(", GICv3");
		break;
	}
	switch (__SHIFTOUT(id->ac_aa64pfr0, ID_AA64PFR0_EL1_FP)) {
	case ID_AA64PFR0_EL1_FP_NONE:
		break;
	default:
		aprint_verbose(", FP");
		break;
	}

	/* ISAR0 */
	switch (__SHIFTOUT(id->ac_aa64isar0, ID_AA64ISAR0_EL1_CRC32)) {
	case ID_AA64ISAR0_EL1_CRC32_CRC32X:
		aprint_verbose(", CRC32");
		break;
	}
	switch (__SHIFTOUT(id->ac_aa64isar0, ID_AA64ISAR0_EL1_SHA1)) {
	case ID_AA64ISAR0_EL1_SHA1_SHA1CPMHSU:
		aprint_verbose(", SHA1");
		break;
	}
	switch (__SHIFTOUT(id->ac_aa64isar0, ID_AA64ISAR0_EL1_SHA2)) {
	case ID_AA64ISAR0_EL1_SHA2_SHA256HSU:
		aprint_verbose(", SHA256");
		break;
	}
	switch (__SHIFTOUT(id->ac_aa64isar0, ID_AA64ISAR0_EL1_AES)) {
	case ID_AA64ISAR0_EL1_AES_AES:
		aprint_verbose(", AES");
		break;
	case ID_AA64ISAR0_EL1_AES_PMUL:
		aprint_verbose(", AES+PMULL");
		break;
	}
	switch (__SHIFTOUT(id->ac_aa64isar0, ID_AA64ISAR0_EL1_RNDR)) {
	case ID_AA64ISAR0_EL1_RNDR_RNDRRS:
		aprint_verbose(", RNDRRS");
		break;
	}

	/* PFR0:DIT -- data-independent timing support */
	switch (__SHIFTOUT(id->ac_aa64pfr0, ID_AA64PFR0_EL1_DIT)) {
	case ID_AA64PFR0_EL1_DIT_IMPL:
		aprint_verbose(", DIT");
		break;
	}

	/* PFR0:AdvSIMD */
	switch (__SHIFTOUT(id->ac_aa64pfr0, ID_AA64PFR0_EL1_ADVSIMD)) {
	case ID_AA64PFR0_EL1_ADV_SIMD_NONE:
		break;
	default:
		aprint_verbose(", NEON");
		break;
	}

	/* MVFR0/MVFR1 */
	switch (__SHIFTOUT(id->ac_mvfr0, MVFR0_FPROUND)) {
	case MVFR0_FPROUND_ALL:
		aprint_verbose(", rounding");
		break;
	}
	switch (__SHIFTOUT(id->ac_mvfr0, MVFR0_FPTRAP)) {
	case MVFR0_FPTRAP_TRAP:
		aprint_verbose(", exceptions");
		break;
	}
	switch (__SHIFTOUT(id->ac_mvfr1, MVFR1_FPDNAN)) {
	case MVFR1_FPDNAN_NAN:
		aprint_verbose(", NaN propagation");
		break;
	}
	switch (__SHIFTOUT(id->ac_mvfr1, MVFR1_FPFTZ)) {
	case MVFR1_FPFTZ_DENORMAL:
		aprint_verbose(", denormals");
		break;
	}
	switch (__SHIFTOUT(id->ac_mvfr0, MVFR0_SIMDREG)) {
	case MVFR0_SIMDREG_16x64:
		aprint_verbose(", 16x64bitRegs");
		break;
	case MVFR0_SIMDREG_32x64:
		aprint_verbose(", 32x64bitRegs");
		break;
	}
	switch (__SHIFTOUT(id->ac_mvfr1, MVFR1_SIMDFMAC)) {
	case MVFR1_SIMDFMAC_FMAC:
		aprint_verbose(", Fused Multiply-Add");
		break;
	}

	aprint_verbose("\n");
}

/*
 * Enable the performance counter, then estimate frequency for
 * the current PE and store the result in cpu_cc_freq.
 */
static void
cpu_init_counter(struct cpu_info *ci)
{
	const uint64_t dfr0 = reg_id_aa64dfr0_el1_read();
	const u_int pmuver = __SHIFTOUT(dfr0, ID_AA64DFR0_EL1_PMUVER);
	if (pmuver == ID_AA64DFR0_EL1_PMUVER_NONE) {
		/* Performance Monitors Extension not implemented. */
		return;
	}
	if (pmuver == ID_AA64DFR0_EL1_PMUVER_IMPL) {
		/* Non-standard Performance Monitors are not supported. */
		return;
	}

	reg_pmcr_el0_write(PMCR_E | PMCR_C | PMCR_LC);
	reg_pmintenclr_el1_write(PMINTEN_C | PMINTEN_P);
	reg_pmcntenset_el0_write(PMCNTEN_C);

	const uint32_t prev = cpu_counter32();
	delay(100000);
	ci->ci_data.cpu_cc_freq = (cpu_counter32() - prev) * 10;
}

/*
 * Fill in this CPUs id data.  Must be called on all cpus.
 */
void __noasan
cpu_setup_id(struct cpu_info *ci)
{
	struct aarch64_sysctl_cpu_id *id = &ci->ci_id;

	/* SCTLR - System Control Register */
	ci->ci_sctlr_el1 = reg_sctlr_el1_read();

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
	id->ac_aa64mmfr2 = reg_id_aa64mmfr2_el1_read();

	id->ac_mvfr0     = reg_mvfr0_el1_read();
	id->ac_mvfr1     = reg_mvfr1_el1_read();
	id->ac_mvfr2     = reg_mvfr2_el1_read();

	id->ac_clidr     = reg_clidr_el1_read();
	id->ac_ctr       = reg_ctr_el0_read();

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

static struct krndsource rndrrs_source;

static void
rndrrs_get(size_t nbytes, void *cookie)
{
	/* Entropy bits per data byte, wild-arse guess.  */
	const unsigned bpb = 4;
	size_t nbits = nbytes*NBBY;
	uint64_t x;
	int error;

	while (nbits) {
		/*
		 * x := random 64-bit sample
		 * error := Z bit, set to 1 if sample is bad
		 *
		 * XXX This should be done by marking the function
		 * __attribute__((target("arch=armv8.5-a+rng"))) and
		 * using `mrs %0, rndrrs', but:
		 *
		 * (a) the version of gcc we use doesn't support that,
		 * and
		 * (b) clang doesn't seem to like `rndrrs' itself.
		 *
		 * So we use the numeric encoding for now.
		 */
		__asm __volatile(""
		    "mrs	%0, s3_3_c2_c4_1\n"
		    "cset	%w1, eq"
		    : "=r"(x), "=r"(error));
		if (error)
			break;
		rnd_add_data_sync(&rndrrs_source, &x, sizeof(x),
		    bpb*sizeof(x));
		nbits -= MIN(nbits, bpb*sizeof(x));
	}

	explicit_memset(&x, 0, sizeof x);
}

/*
 * setup the RNDRRS entropy source
 */
static void
cpu_setup_rng(device_t dv, struct cpu_info *ci)
{
	struct aarch64_sysctl_cpu_id *id = &ci->ci_id;

	/* Verify that it is supported.  */
	switch (__SHIFTOUT(id->ac_aa64isar0, ID_AA64ISAR0_EL1_RNDR)) {
	case ID_AA64ISAR0_EL1_RNDR_RNDRRS:
		break;
	default:
		return;
	}

	/* Attach it.  */
	rndsource_setcb(&rndrrs_source, rndrrs_get, NULL);
	rnd_attach_source(&rndrrs_source, "rndrrs", RND_TYPE_RNG,
	    RND_FLAG_DEFAULT|RND_FLAG_HASCB);
}

/*
 * setup the AES implementation
 */
static void
cpu_setup_aes(device_t dv, struct cpu_info *ci)
{
	struct aarch64_sysctl_cpu_id *id = &ci->ci_id;

	/* Check for ARMv8.0-AES support.  */
	switch (__SHIFTOUT(id->ac_aa64isar0, ID_AA64ISAR0_EL1_AES)) {
	case ID_AA64ISAR0_EL1_AES_AES:
	case ID_AA64ISAR0_EL1_AES_PMUL:
		aes_md_init(&aes_armv8_impl);
		return;
	default:
		break;
	}

	/* Failing that, check for SIMD support.  */
	switch (__SHIFTOUT(id->ac_aa64pfr0, ID_AA64PFR0_EL1_ADVSIMD)) {
	case ID_AA64PFR0_EL1_ADV_SIMD_IMPL:
		aes_md_init(&aes_neon_impl);
		return;
	default:
		break;
	}
}

/*
 * setup the ChaCha implementation
 */
static void
cpu_setup_chacha(device_t dv, struct cpu_info *ci)
{
	struct aarch64_sysctl_cpu_id *id = &ci->ci_id;

	/* Check for SIMD support.  */
	switch (__SHIFTOUT(id->ac_aa64pfr0, ID_AA64PFR0_EL1_ADVSIMD)) {
	case ID_AA64PFR0_EL1_ADV_SIMD_IMPL:
		chacha_md_init(&chacha_neon_impl);
		return;
	default:
		break;
	}
}

#ifdef MULTIPROCESSOR
/*
 * Initialise a secondary processor.
 *
 * printf isn't available as kmutex(9) relies on curcpu which isn't setup yet.
 *
 */
void __noasan
cpu_init_secondary_processor(int cpuindex)
{
	struct cpu_info * ci = &cpu_info_store[cpuindex];
	struct aarch64_sysctl_cpu_id *id = &ci->ci_id;

	aarch64_setcpufuncs(ci);

	/* Sets ci->ci_{sctlr,midr,mpidr}, etc */
	cpu_setup_id(ci);

	arm_cpu_topology_set(ci, id->ac_mpidr);
	aarch64_getcacheinfo(ci);

	cpu_set_hatched(cpuindex);

	/*
	 * return to assembly to wait for cpu_boot_secondary_processors
	 */
}


/*
 * When we are called, the MMU and caches are on and we are running on the stack
 * of the idlelwp for this cpu.
 */
void
cpu_hatch(struct cpu_info *ci)
{
	KASSERT(curcpu() == ci);
	KASSERT((reg_tcr_el1_read() & TCR_EPD0) != 0);

#ifdef DDB
	db_machdep_cpu_init();
#endif

	cpu_init_counter(ci);

	intr_cpu_init(ci);

#ifdef FDT
	arm_fdt_cpu_hatch(ci);
#endif

	/*
	 * clear my bit of arm_cpu_mbox to tell cpu_boot_secondary_processors().
	 * there are cpu0,1,2,3, and if cpu2 is unresponsive,
	 * ci_index are each cpu0=0, cpu1=1, cpu2=undef, cpu3=2.
	 * therefore we have to use device_unit instead of ci_index for mbox.
	 */

	cpu_clr_mbox(device_unit(ci->ci_dev));
}
#endif /* MULTIPROCESSOR */
