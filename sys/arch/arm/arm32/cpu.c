/*	$NetBSD: cpu.c,v 1.89 2012/11/30 08:12:27 msaitoh Exp $	*/

/*
 * Copyright (c) 1995 Mark Brinicombe.
 * Copyright (c) 1995 Brini.
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by Brini.
 * 4. The name of the company nor the name of the author may be used to
 *    endorse or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY BRINI ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL BRINI OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * RiscBSD kernel project
 *
 * cpu.c
 *
 * Probing and configuration for the master CPU
 *
 * Created      : 10/10/95
 */

#include "opt_armfpe.h"
#include "opt_multiprocessor.h"

#include <sys/param.h>

__KERNEL_RCSID(0, "$NetBSD: cpu.c,v 1.89 2012/11/30 08:12:27 msaitoh Exp $");

#include <sys/systm.h>
#include <sys/conf.h>
#include <sys/cpu.h>
#include <sys/device.h>
#include <sys/kmem.h>
#include <sys/proc.h>

#include <uvm/uvm_extern.h>

#include <arm/cpuconf.h>
#include <arm/undefined.h>

#ifdef ARMFPE
#include <machine/bootconfig.h> /* For boot args */
#include <arm/fpe-arm/armfpe.h>
#endif

char cpu_model[256];

#ifdef MULTIPROCESSOR
volatile u_int arm_cpu_hatched = 0;
u_int arm_cpu_max = 0;
uint32_t arm_cpu_mbox __cacheline_aligned = 0;
uint32_t arm_cpu_marker __cacheline_aligned = 1;
#endif

/* Prototypes */
void identify_arm_cpu(device_t dv, struct cpu_info *);
void identify_cortex_caches(device_t dv);
void identify_features(device_t dv);

/*
 * Identify the master (boot) CPU
 */
  
void
cpu_attach(device_t dv, cpuid_t id)
{
	const char * const xname = device_xname(dv);
	struct cpu_info *ci;

	if (id == 0) {
		ci = curcpu();

		/* Get the CPU ID from coprocessor 15 */

		ci->ci_arm_cpuid = cpu_id();
		ci->ci_arm_cputype = ci->ci_arm_cpuid & CPU_ID_CPU_MASK;
		ci->ci_arm_cpurev = ci->ci_arm_cpuid & CPU_ID_REVISION_MASK;
	} else {
#ifdef MULTIPROCESSOR
		KASSERT(cpu_info[id] == NULL);
		ci = kmem_zalloc(sizeof(*ci), KM_SLEEP);
		KASSERT(ci != NULL);
		ci->ci_cpl = IPL_HIGH;
		ci->ci_cpuid = id;
		ci->ci_data.cpu_core_id = id;
		ci->ci_data.cpu_cc_freq = cpu_info_store.ci_data.cpu_cc_freq;
		ci->ci_arm_cpuid = cpu_info_store.ci_arm_cpuid;
		ci->ci_arm_cputype = cpu_info_store.ci_arm_cputype;
		ci->ci_arm_cpurev = cpu_info_store.ci_arm_cpurev;
		cpu_info[ci->ci_cpuid] = ci;
		if ((arm_cpu_hatched & (1 << id)) == 0) {
			ci->ci_dev = dv;
			dv->dv_private = ci;
			aprint_naive(": disabled\n");
			aprint_normal(": disabled (unresponsive)\n");
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

	evcnt_attach_dynamic(&ci->ci_arm700bugcount, EVCNT_TYPE_MISC,
	    NULL, xname, "arm700swibug");

	evcnt_attach_dynamic_nozero(&ci->ci_abt_evs[FAULT_WRTBUF_0], EVCNT_TYPE_TRAP,
	    NULL, xname, "vector abort");
	evcnt_attach_dynamic_nozero(&ci->ci_abt_evs[FAULT_WRTBUF_1], EVCNT_TYPE_TRAP,
	    NULL, xname, "terminal abort");
	evcnt_attach_dynamic_nozero(&ci->ci_abt_evs[FAULT_BUSERR_0], EVCNT_TYPE_TRAP,
	    NULL, xname, "external linefetch abort (S)");
	evcnt_attach_dynamic_nozero(&ci->ci_abt_evs[FAULT_BUSERR_1], EVCNT_TYPE_TRAP,
	    NULL, xname, "external linefetch abort (P)");
	evcnt_attach_dynamic_nozero(&ci->ci_abt_evs[FAULT_BUSERR_2], EVCNT_TYPE_TRAP,
	    NULL, xname, "external non-linefetch abort (S)");
	evcnt_attach_dynamic_nozero(&ci->ci_abt_evs[FAULT_BUSERR_3], EVCNT_TYPE_TRAP,
	    NULL, xname, "external non-linefetch abort (P)");
	evcnt_attach_dynamic_nozero(&ci->ci_abt_evs[FAULT_BUSTRNL1], EVCNT_TYPE_TRAP,
	    NULL, xname, "external translation abort (L1)");
	evcnt_attach_dynamic_nozero(&ci->ci_abt_evs[FAULT_BUSTRNL2], EVCNT_TYPE_TRAP,
	    NULL, xname, "external translation abort (L2)");
	evcnt_attach_dynamic_nozero(&ci->ci_abt_evs[FAULT_ALIGN_0], EVCNT_TYPE_TRAP,
	    NULL, xname, "alignment abort (0)");
	evcnt_attach_dynamic_nozero(&ci->ci_abt_evs[FAULT_ALIGN_1], EVCNT_TYPE_TRAP,
	    NULL, xname, "alignment abort (1)");
	evcnt_attach_dynamic_nozero(&ci->ci_abt_evs[FAULT_TRANS_S], EVCNT_TYPE_TRAP,
	    NULL, xname, "translation abort (S)");
	evcnt_attach_dynamic_nozero(&ci->ci_abt_evs[FAULT_TRANS_P], EVCNT_TYPE_TRAP,
	    NULL, xname, "translation abort (P)");
	evcnt_attach_dynamic_nozero(&ci->ci_abt_evs[FAULT_DOMAIN_S], EVCNT_TYPE_TRAP,
	    NULL, xname, "domain abort (S)");
	evcnt_attach_dynamic_nozero(&ci->ci_abt_evs[FAULT_DOMAIN_P], EVCNT_TYPE_TRAP,
	    NULL, xname, "domain abort (P)");
	evcnt_attach_dynamic_nozero(&ci->ci_abt_evs[FAULT_PERM_S], EVCNT_TYPE_TRAP,
	    NULL, xname, "permission abort (S)");
	evcnt_attach_dynamic_nozero(&ci->ci_abt_evs[FAULT_PERM_P], EVCNT_TYPE_TRAP,
	    NULL, xname, "permission abort (P)");

#ifdef MULTIPROCESSOR
	/*
	 * and we are done if this is a secondary processor.
	 */
	if (!CPU_IS_PRIMARY(ci)) {
		aprint_naive(": %s\n", cpu_model);
		aprint_normal(": %s\n", cpu_model);
		mi_cpu_attach(ci);
		return;
	}
#endif

	identify_arm_cpu(dv, ci);

#ifdef CPU_STRONGARM
	if (ci->ci_arm_cputype == CPU_ID_SA110 &&
	    ci->ci_arm_cpurev < 3) {
		aprint_normal_dev(dv, "SA-110 with bugged STM^ instruction\n");
	}
#endif

#ifdef CPU_ARM8
	if ((ci->ci_arm_cpuid & CPU_ID_CPU_MASK) == CPU_ID_ARM810) {
		int clock = arm8_clock_config(0, 0);
		char *fclk;
		aprint_normal_dev(dv, "ARM810 cp15=%02x", clock);
		aprint_normal(" clock:%s", (clock & 1) ? " dynamic" : "");
		aprint_normal("%s", (clock & 2) ? " sync" : "");
		switch ((clock >> 2) & 3) {
		case 0:
			fclk = "bus clock";
			break;
		case 1:
			fclk = "ref clock";
			break;
		case 3:
			fclk = "pll";
			break;
		default:
			fclk = "illegal";
			break;
		}
		aprint_normal(" fclk source=%s\n", fclk);
 	}
#endif

#ifdef ARMFPE
	/*
	 * Ok now we test for an FPA
	 * At this point no floating point emulator has been installed.
	 * This means any FP instruction will cause undefined exception.
	 * We install a temporay coproc 1 handler which will modify
	 * undefined_test if it is called.
	 * We then try to read the FP status register. If undefined_test
	 * has been decremented then the instruction was not handled by
	 * an FPA so we know the FPA is missing. If undefined_test is
	 * still 1 then we know the instruction was handled by an FPA.
	 * We then remove our test handler and look at the
	 * FP status register for identification.
	 */
 
	/*
	 * Ok if ARMFPE is defined and the boot options request the 
	 * ARM FPE then it will be installed as the FPE.
	 * This is just while I work on integrating the new FPE.
	 * It means the new FPE gets installed if compiled int (ARMFPE
	 * defined) and also gives me a on/off option when I boot in
	 * case the new FPE is causing panics.
	 */


	int usearmfpe = 1;
	if (boot_args)
		get_bootconf_option(boot_args, "armfpe",
		    BOOTOPT_TYPE_BOOLEAN, &usearmfpe);
	if (usearmfpe)
		initialise_arm_fpe();
#endif

	vfp_attach();		/* XXX SMP */
}

enum cpu_class {
	CPU_CLASS_NONE,
	CPU_CLASS_ARM2,
	CPU_CLASS_ARM2AS,
	CPU_CLASS_ARM3,
	CPU_CLASS_ARM6,
	CPU_CLASS_ARM7,
	CPU_CLASS_ARM7TDMI,
	CPU_CLASS_ARM8,
	CPU_CLASS_ARM9TDMI,
	CPU_CLASS_ARM9ES,
	CPU_CLASS_ARM9EJS,
	CPU_CLASS_ARM10E,
	CPU_CLASS_ARM10EJ,
	CPU_CLASS_SA1,
	CPU_CLASS_XSCALE,
	CPU_CLASS_ARM11J,
	CPU_CLASS_ARMV4,
	CPU_CLASS_CORTEX,
};

static const char * const generic_steppings[16] = {
	"rev 0",	"rev 1",	"rev 2",	"rev 3",
	"rev 4",	"rev 5",	"rev 6",	"rev 7",
	"rev 8",	"rev 9",	"rev 10",	"rev 11",
	"rev 12",	"rev 13",	"rev 14",	"rev 15",
};

static const char * const pN_steppings[16] = {
	"*p0",	"*p1",	"*p2",	"*p3",	"*p4",	"*p5",	"*p6",	"*p7",
	"*p8",	"*p9",	"*p10",	"*p11",	"*p12",	"*p13",	"*p14",	"*p15",
};

static const char * const sa110_steppings[16] = {
	"rev 0",	"step J",	"step K",	"step S",
	"step T",	"rev 5",	"rev 6",	"rev 7",
	"rev 8",	"rev 9",	"rev 10",	"rev 11",
	"rev 12",	"rev 13",	"rev 14",	"rev 15",
};

static const char * const sa1100_steppings[16] = {
	"rev 0",	"step B",	"step C",	"rev 3",
	"rev 4",	"rev 5",	"rev 6",	"rev 7",
	"step D",	"step E",	"rev 10"	"step G",
	"rev 12",	"rev 13",	"rev 14",	"rev 15",
};

static const char * const sa1110_steppings[16] = {
	"step A-0",	"rev 1",	"rev 2",	"rev 3",
	"step B-0",	"step B-1",	"step B-2",	"step B-3",
	"step B-4",	"step B-5",	"rev 10",	"rev 11",
	"rev 12",	"rev 13",	"rev 14",	"rev 15",
};

static const char * const ixp12x0_steppings[16] = {
	"(IXP1200 step A)",		"(IXP1200 step B)",
	"rev 2",			"(IXP1200 step C)",
	"(IXP1200 step D)",		"(IXP1240/1250 step A)",
	"(IXP1240 step B)",		"(IXP1250 step B)",
	"rev 8",	"rev 9",	"rev 10",	"rev 11",
	"rev 12",	"rev 13",	"rev 14",	"rev 15",
};

static const char * const xscale_steppings[16] = {
	"step A-0",	"step A-1",	"step B-0",	"step C-0",
	"step D-0",	"rev 5",	"rev 6",	"rev 7",
	"rev 8",	"rev 9",	"rev 10",	"rev 11",
	"rev 12",	"rev 13",	"rev 14",	"rev 15",
};

static const char * const i80321_steppings[16] = {
	"step A-0",	"step B-0",	"rev 2",	"rev 3",
	"rev 4",	"rev 5",	"rev 6",	"rev 7",
	"rev 8",	"rev 9",	"rev 10",	"rev 11",
	"rev 12",	"rev 13",	"rev 14",	"rev 15",
};

static const char * const i80219_steppings[16] = {
	"step A-0",	"rev 1",	"rev 2",	"rev 3",
	"rev 4",	"rev 5",	"rev 6",	"rev 7",
	"rev 8",	"rev 9",	"rev 10",	"rev 11",
	"rev 12",	"rev 13",	"rev 14",	"rev 15",
};

/* Steppings for PXA2[15]0 */
static const char * const pxa2x0_steppings[16] = {
	"step A-0",	"step A-1",	"step B-0",	"step B-1",
	"step B-2",	"step C-0",	"rev 6",	"rev 7",
	"rev 8",	"rev 9",	"rev 10",	"rev 11",
	"rev 12",	"rev 13",	"rev 14",	"rev 15",
};

/* Steppings for PXA255/26x.
 * rev 5: PXA26x B0, rev 6: PXA255 A0  
 */
static const char * const pxa255_steppings[16] = {
	"rev 0",	"rev 1",	"rev 2",	"step A-0",
	"rev 4",	"step B-0",	"step A-0",	"rev 7",
	"rev 8",	"rev 9",	"rev 10",	"rev 11",
	"rev 12",	"rev 13",	"rev 14",	"rev 15",
};

/* Stepping for PXA27x */
static const char * const pxa27x_steppings[16] = {
	"step A-0",	"step A-1",	"step B-0",	"step B-1",
	"step C-0",	"rev 5",	"rev 6",	"rev 7",
	"rev 8",	"rev 9",	"rev 10",	"rev 11",
	"rev 12",	"rev 13",	"rev 14",	"rev 15",
};

static const char * const ixp425_steppings[16] = {
	"step 0",	"rev 1",	"rev 2",	"rev 3",
	"rev 4",	"rev 5",	"rev 6",	"rev 7",
	"rev 8",	"rev 9",	"rev 10",	"rev 11",
	"rev 12",	"rev 13",	"rev 14",	"rev 15",
};

struct cpuidtab {
	uint32_t	cpuid;
	enum		cpu_class cpu_class;
	const char	*cpu_classname;
	const char * const *cpu_steppings;
};

const struct cpuidtab cpuids[] = {
	{ CPU_ID_ARM2,		CPU_CLASS_ARM2,		"ARM2",
	  generic_steppings },
	{ CPU_ID_ARM250,	CPU_CLASS_ARM2AS,	"ARM250",
	  generic_steppings },

	{ CPU_ID_ARM3,		CPU_CLASS_ARM3,		"ARM3",
	  generic_steppings },

	{ CPU_ID_ARM600,	CPU_CLASS_ARM6,		"ARM600",
	  generic_steppings },
	{ CPU_ID_ARM610,	CPU_CLASS_ARM6,		"ARM610",
	  generic_steppings },
	{ CPU_ID_ARM620,	CPU_CLASS_ARM6,		"ARM620",
	  generic_steppings },

	{ CPU_ID_ARM700,	CPU_CLASS_ARM7,		"ARM700",
	  generic_steppings },
	{ CPU_ID_ARM710,	CPU_CLASS_ARM7,		"ARM710",
	  generic_steppings },
	{ CPU_ID_ARM7500,	CPU_CLASS_ARM7,		"ARM7500",
	  generic_steppings },
	{ CPU_ID_ARM710A,	CPU_CLASS_ARM7,		"ARM710a",
	  generic_steppings },
	{ CPU_ID_ARM7500FE,	CPU_CLASS_ARM7,		"ARM7500FE",
	  generic_steppings },
	{ CPU_ID_ARM710T,	CPU_CLASS_ARM7TDMI,	"ARM710T",
	  generic_steppings },
	{ CPU_ID_ARM720T,	CPU_CLASS_ARM7TDMI,	"ARM720T",
	  generic_steppings },
	{ CPU_ID_ARM740T8K,	CPU_CLASS_ARM7TDMI, "ARM740T (8 KB cache)",
	  generic_steppings },
	{ CPU_ID_ARM740T4K,	CPU_CLASS_ARM7TDMI, "ARM740T (4 KB cache)",
	  generic_steppings },

	{ CPU_ID_ARM810,	CPU_CLASS_ARM8,		"ARM810",
	  generic_steppings },

	{ CPU_ID_ARM920T,	CPU_CLASS_ARM9TDMI,	"ARM920T",
	  generic_steppings },
	{ CPU_ID_ARM922T,	CPU_CLASS_ARM9TDMI,	"ARM922T",
	  generic_steppings },
	{ CPU_ID_ARM926EJS,	CPU_CLASS_ARM9EJS,	"ARM926EJ-S",
	  generic_steppings },
	{ CPU_ID_ARM940T,	CPU_CLASS_ARM9TDMI,	"ARM940T",
	  generic_steppings },
	{ CPU_ID_ARM946ES,	CPU_CLASS_ARM9ES,	"ARM946E-S",
	  generic_steppings },
	{ CPU_ID_ARM966ES,	CPU_CLASS_ARM9ES,	"ARM966E-S",
	  generic_steppings },
	{ CPU_ID_ARM966ESR1,	CPU_CLASS_ARM9ES,	"ARM966E-S",
	  generic_steppings },
	{ CPU_ID_TI925T,	CPU_CLASS_ARM9TDMI,	"TI ARM925T",
	  generic_steppings },
	{ CPU_ID_MV88SV131,	CPU_CLASS_ARM9ES,	"Sheeva 88SV131",
	  generic_steppings },
	{ CPU_ID_MV88FR571_VD,	CPU_CLASS_ARM9ES,	"Sheeva 88FR571-vd",
	  generic_steppings },

	{ CPU_ID_ARM1020E,	CPU_CLASS_ARM10E,	"ARM1020E",
	  generic_steppings },
	{ CPU_ID_ARM1022ES,	CPU_CLASS_ARM10E,	"ARM1022E-S",
	  generic_steppings },
	{ CPU_ID_ARM1026EJS,	CPU_CLASS_ARM10EJ,	"ARM1026EJ-S",
	  generic_steppings },

	{ CPU_ID_SA110,		CPU_CLASS_SA1,		"SA-110",
	  sa110_steppings },
	{ CPU_ID_SA1100,	CPU_CLASS_SA1,		"SA-1100",
	  sa1100_steppings },
	{ CPU_ID_SA1110,	CPU_CLASS_SA1,		"SA-1110",
	  sa1110_steppings },

	{ CPU_ID_IXP1200,	CPU_CLASS_SA1,		"IXP1200",
	  ixp12x0_steppings },

	{ CPU_ID_80200,		CPU_CLASS_XSCALE,	"i80200",
	  xscale_steppings },

	{ CPU_ID_80321_400,	CPU_CLASS_XSCALE,	"i80321 400MHz",
	  i80321_steppings },
	{ CPU_ID_80321_600,	CPU_CLASS_XSCALE,	"i80321 600MHz",
	  i80321_steppings },
	{ CPU_ID_80321_400_B0,	CPU_CLASS_XSCALE,	"i80321 400MHz",
	  i80321_steppings },
	{ CPU_ID_80321_600_B0,	CPU_CLASS_XSCALE,	"i80321 600MHz",
	  i80321_steppings },

	{ CPU_ID_80219_400,	CPU_CLASS_XSCALE,	"i80219 400MHz",
	  i80219_steppings },
	{ CPU_ID_80219_600,	CPU_CLASS_XSCALE,	"i80219 600MHz",
	  i80219_steppings },

	{ CPU_ID_PXA27X,	CPU_CLASS_XSCALE,	"PXA27x",
	  pxa27x_steppings },
	{ CPU_ID_PXA250A,	CPU_CLASS_XSCALE,	"PXA250",
	  pxa2x0_steppings },
	{ CPU_ID_PXA210A,	CPU_CLASS_XSCALE,	"PXA210",
	  pxa2x0_steppings },
	{ CPU_ID_PXA250B,	CPU_CLASS_XSCALE,	"PXA250",
	  pxa2x0_steppings },
	{ CPU_ID_PXA210B,	CPU_CLASS_XSCALE,	"PXA210",
	  pxa2x0_steppings },
	{ CPU_ID_PXA250C, 	CPU_CLASS_XSCALE,	"PXA255/26x",
	  pxa255_steppings },
	{ CPU_ID_PXA210C, 	CPU_CLASS_XSCALE,	"PXA210",
	  pxa2x0_steppings },

	{ CPU_ID_IXP425_533,	CPU_CLASS_XSCALE,	"IXP425 533MHz",
	  ixp425_steppings },
	{ CPU_ID_IXP425_400,	CPU_CLASS_XSCALE,	"IXP425 400MHz",
	  ixp425_steppings },
	{ CPU_ID_IXP425_266,	CPU_CLASS_XSCALE,	"IXP425 266MHz",
	  ixp425_steppings },

	{ CPU_ID_ARM1136JS,	CPU_CLASS_ARM11J,	"ARM1136J-S r0",
	  pN_steppings },
	{ CPU_ID_ARM1136JSR1,	CPU_CLASS_ARM11J,	"ARM1136J-S r1",
	  pN_steppings },
#if 0
	/* The ARM1156T2-S only has a memory protection unit */
	{ CPU_ID_ARM1156T2S,	CPU_CLASS_ARM11J,	"ARM1156T2-S r0",
	  pN_steppings },
#endif
	{ CPU_ID_ARM1176JZS,	CPU_CLASS_ARM11J,	"ARM1176JZ-S r0",
	  pN_steppings },

	{ CPU_ID_ARM11MPCORE,	CPU_CLASS_ARM11J, 	"ARM11 MPCore",
	  generic_steppings },

	{ CPU_ID_CORTEXA5R0,	CPU_CLASS_CORTEX,	"Cortex-A5 r0",
	  pN_steppings },
	{ CPU_ID_CORTEXA8R1,	CPU_CLASS_CORTEX,	"Cortex-A8 r1",
	  pN_steppings },
	{ CPU_ID_CORTEXA8R2,	CPU_CLASS_CORTEX,	"Cortex-A8 r2",
	  pN_steppings },
	{ CPU_ID_CORTEXA8R3,	CPU_CLASS_CORTEX,	"Cortex-A8 r3",
	  pN_steppings },
	{ CPU_ID_CORTEXA9R2,	CPU_CLASS_CORTEX,	"Cortex-A9 r2",
	  pN_steppings },
	{ CPU_ID_CORTEXA9R3,	CPU_CLASS_CORTEX,	"Cortex-A9 r3",
	  pN_steppings },
	{ CPU_ID_CORTEXA9R4,	CPU_CLASS_CORTEX,	"Cortex-A9 r4",
	  pN_steppings },
	{ CPU_ID_CORTEXA15R2,	CPU_CLASS_CORTEX,	"Cortex-A15 r2",
	  pN_steppings },
	{ CPU_ID_CORTEXA15R3,	CPU_CLASS_CORTEX,	"Cortex-A15 r3",
	  pN_steppings },

	{ CPU_ID_FA526,		CPU_CLASS_ARMV4,	"FA526",
	  generic_steppings },

	{ 0, CPU_CLASS_NONE, NULL, NULL }
};

struct cpu_classtab {
	const char	*class_name;
	const char	*class_option;
};

const struct cpu_classtab cpu_classes[] = {
	[CPU_CLASS_NONE] =	{ "unknown",	NULL },
	[CPU_CLASS_ARM2] =	{ "ARM2",	"CPU_ARM2" },
	[CPU_CLASS_ARM2AS] =	{ "ARM2as",	"CPU_ARM250" },
	[CPU_CLASS_ARM3] =	{ "ARM3",	"CPU_ARM3" },
	[CPU_CLASS_ARM6] =	{ "ARM6",	"CPU_ARM6" },
	[CPU_CLASS_ARM7] =	{ "ARM7",	"CPU_ARM7" },
	[CPU_CLASS_ARM7TDMI] =	{ "ARM7TDMI",	"CPU_ARM7TDMI" },
	[CPU_CLASS_ARM8] =	{ "ARM8",	"CPU_ARM8" },
	[CPU_CLASS_ARM9TDMI] =	{ "ARM9TDMI",	NULL },
	[CPU_CLASS_ARM9ES] =	{ "ARM9E-S",	"CPU_ARM9E" },
	[CPU_CLASS_ARM9EJS] =	{ "ARM9EJ-S",	"CPU_ARM9E" },
	[CPU_CLASS_ARM10E] =	{ "ARM10E",	"CPU_ARM10" },
	[CPU_CLASS_ARM10EJ] =	{ "ARM10EJ",	"CPU_ARM10" },
	[CPU_CLASS_SA1] =	{ "SA-1",	"CPU_SA110" },
	[CPU_CLASS_XSCALE] =	{ "XScale",	"CPU_XSCALE_..." },
	[CPU_CLASS_ARM11J] =	{ "ARM11J",	"CPU_ARM11" },
	[CPU_CLASS_ARMV4] =	{ "ARMv4",	"CPU_ARMV4" },
	[CPU_CLASS_CORTEX] =	{ "Cortex",	"CPU_CORTEX" },
};

/*
 * Report the type of the specified arm processor. This uses the generic and
 * arm specific information in the CPU structure to identify the processor.
 * The remaining fields in the CPU structure are filled in appropriately.
 */

static const char * const wtnames[] = {
	"write-through",
	"write-back",
	"write-back",
	"**unknown 3**",
	"**unknown 4**",
	"write-back-locking",		/* XXX XScale-specific? */
	"write-back-locking-A",
	"write-back-locking-B",
	"**unknown 8**",
	"**unknown 9**",
	"**unknown 10**",
	"**unknown 11**",
	"**unknown 12**",
	"**unknown 13**",
	"write-back-locking-C",
	"write-back-locking-D",
};

static void
print_cache_info(device_t dv, struct arm_cache_info *info, u_int level)
{
	if (info->cache_unified) {
		aprint_normal_dev(dv, "%dKB/%dB %d-way %s L%u Unified cache\n",
		    info->dcache_size / 1024,
		    info->dcache_line_size, info->dcache_ways,
		    wtnames[info->cache_type], level + 1);
	} else {
		aprint_normal_dev(dv, "%dKB/%dB %d-way L%u Instruction cache\n",
		    info->icache_size / 1024,
		    info->icache_line_size, info->icache_ways, level + 1);
		aprint_normal_dev(dv, "%dKB/%dB %d-way %s L%u Data cache\n",
		    info->dcache_size / 1024, 
		    info->dcache_line_size, info->dcache_ways,
		    wtnames[info->cache_type], level + 1);
	}
}

void
identify_arm_cpu(device_t dv, struct cpu_info *ci)
{
	enum cpu_class cpu_class = CPU_CLASS_NONE;
	const u_int cpuid = ci->ci_arm_cpuid;
	const char * const xname = device_xname(dv);
	const char *steppingstr;
	int i;

	if (cpuid == 0) {
		aprint_error("Processor failed probe - no CPU ID\n");
		return;
	}

	for (i = 0; cpuids[i].cpuid != 0; i++)
		if (cpuids[i].cpuid == (cpuid & CPU_ID_CPU_MASK)) {
			cpu_class = cpuids[i].cpu_class;
			steppingstr = cpuids[i].cpu_steppings[cpuid &
			    CPU_ID_REVISION_MASK];
			sprintf(cpu_model, "%s%s%s (%s core)",
			    cpuids[i].cpu_classname,
			    steppingstr[0] == '*' ? "" : " ",
			    &steppingstr[steppingstr[0] == '*'],
			    cpu_classes[cpu_class].class_name);
			break;
		}

	if (cpuids[i].cpuid == 0)
		sprintf(cpu_model, "unknown CPU (ID = 0x%x)", cpuid);

	if (ci->ci_data.cpu_cc_freq != 0) {
		char freqbuf[8];
		humanize_number(freqbuf, sizeof(freqbuf), ci->ci_data.cpu_cc_freq,
		    "Hz", 1000);

		aprint_naive(": %s %s\n", freqbuf, cpu_model);
		aprint_normal(": %s %s\n", freqbuf, cpu_model);
	} else {
		aprint_naive(": %s\n", cpu_model);
		aprint_normal(": %s\n", cpu_model);
	}

	aprint_normal("%s:", xname);

	switch (cpu_class) {
	case CPU_CLASS_ARM6:
	case CPU_CLASS_ARM7:
	case CPU_CLASS_ARM7TDMI:
	case CPU_CLASS_ARM8:
		if ((ci->ci_ctrl & CPU_CONTROL_IDC_ENABLE) == 0)
			aprint_normal(" IDC disabled");
		else
			aprint_normal(" IDC enabled");
		break;
	case CPU_CLASS_ARM9TDMI:
	case CPU_CLASS_ARM9ES:
	case CPU_CLASS_ARM9EJS:
	case CPU_CLASS_ARM10E:
	case CPU_CLASS_ARM10EJ:
	case CPU_CLASS_SA1:
	case CPU_CLASS_XSCALE:
	case CPU_CLASS_ARM11J:
	case CPU_CLASS_ARMV4:
	case CPU_CLASS_CORTEX:
		if ((ci->ci_ctrl & CPU_CONTROL_DC_ENABLE) == 0)
			aprint_normal(" DC disabled");
		else
			aprint_normal(" DC enabled");
		if ((ci->ci_ctrl & CPU_CONTROL_IC_ENABLE) == 0)
			aprint_normal(" IC disabled");
		else
			aprint_normal(" IC enabled");
		break;
	default:
		break;
	}
	if ((ci->ci_ctrl & CPU_CONTROL_WBUF_ENABLE) == 0)
		aprint_normal(" WB disabled");
	else
		aprint_normal(" WB enabled");

	if (ci->ci_ctrl & CPU_CONTROL_LABT_ENABLE)
		aprint_normal(" LABT");
	else
		aprint_normal(" EABT");

	if (ci->ci_ctrl & CPU_CONTROL_BPRD_ENABLE)
		aprint_normal(" branch prediction enabled");

	aprint_normal("\n");

#if defined(CPU_CORTEX)
	if (CPU_ID_CORTEX_P(cpuid)) {
		identify_features(dv);
	}
#endif
	/* Print cache info. */
	if (arm_pcache.icache_line_size != 0 || arm_pcache.dcache_line_size != 0) {
		print_cache_info(dv, &arm_pcache, 0);
	}
	if (arm_scache.icache_line_size != 0 || arm_scache.dcache_line_size != 0) {
		print_cache_info(dv, &arm_scache, 1);
	}


	switch (cpu_class) {
#ifdef CPU_ARM2
	case CPU_CLASS_ARM2:
#endif
#ifdef CPU_ARM250
	case CPU_CLASS_ARM2AS:
#endif
#ifdef CPU_ARM3
	case CPU_CLASS_ARM3:
#endif
#ifdef CPU_ARM6
	case CPU_CLASS_ARM6:
#endif
#ifdef CPU_ARM7
	case CPU_CLASS_ARM7:
#endif
#ifdef CPU_ARM7TDMI
	case CPU_CLASS_ARM7TDMI:
#endif		
#ifdef CPU_ARM8
	case CPU_CLASS_ARM8:
#endif
#ifdef CPU_ARM9
	case CPU_CLASS_ARM9TDMI:
#endif
#if defined(CPU_ARM9E) || defined(CPU_SHEEVA)
	case CPU_CLASS_ARM9ES:
	case CPU_CLASS_ARM9EJS:
#endif
#ifdef CPU_ARM10
	case CPU_CLASS_ARM10E:
	case CPU_CLASS_ARM10EJ:
#endif
#if defined(CPU_SA110) || defined(CPU_SA1100) || \
    defined(CPU_SA1110) || defined(CPU_IXP12X0)
	case CPU_CLASS_SA1:
#endif
#if defined(CPU_XSCALE_80200) || defined(CPU_XSCALE_80321) || \
    defined(__CPU_XSCALE_PXA2XX) || defined(CPU_XSCALE_IXP425)
	case CPU_CLASS_XSCALE:
#endif
#if defined(CPU_ARM11)
	case CPU_CLASS_ARM11J:
#endif
#if defined(CPU_CORTEX)
	case CPU_CLASS_CORTEX:
#endif
#if defined(CPU_FA526)
	case CPU_CLASS_ARMV4:
#endif
		break;
	default:
		if (cpu_classes[cpu_class].class_option == NULL) {
			aprint_error_dev(dv, "%s does not fully support this CPU.\n",
			     ostype);
		} else {
			aprint_error_dev(dv, "This kernel does not fully support "
			       "this CPU.\n");
			aprint_normal_dev(dv, "Recompile with \"options %s\" to "
			       "correct this.\n", cpu_classes[cpu_class].class_option);
		}
		break;
	}
}

#if defined(CPU_CORTEX)
void
identify_features(device_t dv)
{
	uint32_t isar0 = armreg_isar0_read();
	uint32_t isar1 = armreg_isar1_read();
	uint32_t isar2 = armreg_isar2_read();
	uint32_t isar3 = armreg_isar3_read();
	uint32_t isar4 = armreg_isar4_read();
	uint32_t isar5 = armreg_isar5_read();

	uint32_t mmfr0 = armreg_mmfr0_read();
	uint32_t mmfr1 = armreg_mmfr1_read();
	uint32_t mmfr2 = armreg_mmfr2_read();
	uint32_t mmfr3 = armreg_mmfr3_read();

	if (__SHIFTOUT(mmfr3, __BITS(23,20))) {
		/*
		 * Updates to the translation tables do not require a clean
		 * to the point of unification to ensure visibility by subsequent
		 * translation table walks.
		 */
		pmap_needs_pte_sync = 0;
	}

	uint32_t pfr0 = armreg_pfr0_read();
	uint32_t pfr1 = armreg_pfr1_read();

	aprint_verbose_dev(dv,
	    "isar: [0]=%#x [1]=%#x [2]=%#x [3]=%#x, [4]=%#x, [5]=%#x\n",
	    isar0, isar1, isar2, isar3, isar4, isar5);
	aprint_verbose_dev(dv,
	    "mmfr: [0]=%#x [1]=%#x [2]=%#x [3]=%#x\n",
	    mmfr0, mmfr1, mmfr2, mmfr3);
	aprint_verbose_dev(dv,
	    "pfr: [0]=%#x [1]=%#x\n",
	    pfr0, pfr1);
}
#endif /* CPU_CORTEX */
