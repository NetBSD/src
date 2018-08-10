/*	$NetBSD: cpu.c,v 1.117 2018/08/10 16:17:30 maxv Exp $	*/

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

__KERNEL_RCSID(0, "$NetBSD: cpu.c,v 1.117 2018/08/10 16:17:30 maxv Exp $");

#include <sys/systm.h>
#include <sys/conf.h>
#include <sys/cpu.h>
#include <sys/device.h>
#include <sys/kmem.h>
#include <sys/proc.h>

#include <uvm/uvm_extern.h>

#include <arm/locore.h>
#include <arm/undefined.h>

extern const char *cpu_arch;

#ifdef MULTIPROCESSOR
volatile u_int arm_cpu_hatched = 0;
volatile uint32_t arm_cpu_mbox __cacheline_aligned = 0;
uint32_t arm_cpu_marker[2] __cacheline_aligned = { 0, 0 };
u_int arm_cpu_max = 1;
#endif

/* Prototypes */
void identify_arm_cpu(device_t, struct cpu_info *);
void identify_cortex_caches(device_t);
void identify_features(device_t);

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

		ci->ci_arm_cpuid = cpu_idnum();
		ci->ci_arm_cputype = ci->ci_arm_cpuid & CPU_ID_CPU_MASK;
		ci->ci_arm_cpurev = ci->ci_arm_cpuid & CPU_ID_REVISION_MASK;
	} else {
#ifdef MULTIPROCESSOR
		KASSERT(cpu_info[id] == NULL);
		ci = kmem_zalloc(sizeof(*ci), KM_SLEEP);
		ci->ci_cpl = IPL_HIGH;
		ci->ci_cpuid = id;
		uint32_t mpidr = armreg_mpidr_read();
		if (mpidr & MPIDR_MT) {
			ci->ci_data.cpu_smt_id = mpidr & MPIDR_AFF0;
			ci->ci_data.cpu_core_id = mpidr & MPIDR_AFF1;
			ci->ci_data.cpu_package_id = mpidr & MPIDR_AFF2;
		} else {
			ci->ci_data.cpu_core_id = mpidr & MPIDR_AFF0;
			ci->ci_data.cpu_package_id = mpidr & MPIDR_AFF1;
		}
		ci->ci_data.cpu_core_id = id;
		ci->ci_data.cpu_cc_freq = cpu_info_store.ci_data.cpu_cc_freq;
		ci->ci_arm_cpuid = cpu_info_store.ci_arm_cpuid;
		ci->ci_arm_cputype = cpu_info_store.ci_arm_cputype;
		ci->ci_arm_cpurev = cpu_info_store.ci_arm_cpurev;
		ci->ci_ctrl = cpu_info_store.ci_ctrl;
		ci->ci_undefsave[2] = cpu_info_store.ci_undefsave[2];
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
	evcnt_attach_dynamic_nozero(&ci->ci_und_ev, EVCNT_TYPE_TRAP,
	    NULL, xname, "undefined insn traps");
	evcnt_attach_dynamic_nozero(&ci->ci_und_cp15_ev, EVCNT_TYPE_TRAP,
	    NULL, xname, "undefined cp15 insn traps");

#ifdef MULTIPROCESSOR
	/*
	 * and we are done if this is a secondary processor.
	 */
	if (id != 0) {
#if 1
		aprint_naive("\n");
		aprint_normal("\n");
#else
		aprint_naive(": %s\n", cpu_getmodel());
		aprint_normal(": %s\n", cpu_getmodel());
#endif
		mi_cpu_attach(ci);
#ifdef ARM_MMU_EXTENDED
		pmap_tlb_info_attach(&pmap_tlb0_info, ci);
#endif
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

	vfp_attach(ci);		/* XXX SMP */
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
	CPU_CLASS_PJ4B,
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
	char		cpu_arch[8];
};

const struct cpuidtab cpuids[] = {
	{ CPU_ID_ARM2,		CPU_CLASS_ARM2,		"ARM2",
	  generic_steppings, "2" },
	{ CPU_ID_ARM250,	CPU_CLASS_ARM2AS,	"ARM250",
	  generic_steppings, "2" },

	{ CPU_ID_ARM3,		CPU_CLASS_ARM3,		"ARM3",
	  generic_steppings, "2A" },

	{ CPU_ID_ARM600,	CPU_CLASS_ARM6,		"ARM600",
	  generic_steppings, "3" },
	{ CPU_ID_ARM610,	CPU_CLASS_ARM6,		"ARM610",
	  generic_steppings, "3" },
	{ CPU_ID_ARM620,	CPU_CLASS_ARM6,		"ARM620",
	  generic_steppings, "3" },

	{ CPU_ID_ARM700,	CPU_CLASS_ARM7,		"ARM700",
	  generic_steppings, "3" },
	{ CPU_ID_ARM710,	CPU_CLASS_ARM7,		"ARM710",
	  generic_steppings, "3" },
	{ CPU_ID_ARM7500,	CPU_CLASS_ARM7,		"ARM7500",
	  generic_steppings, "3" },
	{ CPU_ID_ARM710A,	CPU_CLASS_ARM7,		"ARM710a",
	  generic_steppings, "3" },
	{ CPU_ID_ARM7500FE,	CPU_CLASS_ARM7,		"ARM7500FE",
	  generic_steppings, "3" },

	{ CPU_ID_ARM810,	CPU_CLASS_ARM8,		"ARM810",
	  generic_steppings, "4" },

	{ CPU_ID_SA110,		CPU_CLASS_SA1,		"SA-110",
	  sa110_steppings, "4" },
	{ CPU_ID_SA1100,	CPU_CLASS_SA1,		"SA-1100",
	  sa1100_steppings, "4" },
	{ CPU_ID_SA1110,	CPU_CLASS_SA1,		"SA-1110",
	  sa1110_steppings, "4" },

	{ CPU_ID_FA526,		CPU_CLASS_ARMV4,	"FA526",
	  generic_steppings, "4" },

	{ CPU_ID_IXP1200,	CPU_CLASS_SA1,		"IXP1200",
	  ixp12x0_steppings, "4" },

	{ CPU_ID_ARM710T,	CPU_CLASS_ARM7TDMI,	"ARM710T",
	  generic_steppings, "4T" },
	{ CPU_ID_ARM720T,	CPU_CLASS_ARM7TDMI,	"ARM720T",
	  generic_steppings, "4T" },
	{ CPU_ID_ARM740T8K,	CPU_CLASS_ARM7TDMI, "ARM740T (8 KB cache)",
	  generic_steppings, "4T" },
	{ CPU_ID_ARM740T4K,	CPU_CLASS_ARM7TDMI, "ARM740T (4 KB cache)",
	  generic_steppings, "4T" },
	{ CPU_ID_ARM920T,	CPU_CLASS_ARM9TDMI,	"ARM920T",
	  generic_steppings, "4T" },
	{ CPU_ID_ARM922T,	CPU_CLASS_ARM9TDMI,	"ARM922T",
	  generic_steppings, "4T" },
	{ CPU_ID_ARM940T,	CPU_CLASS_ARM9TDMI,	"ARM940T",
	  generic_steppings, "4T" },
	{ CPU_ID_TI925T,	CPU_CLASS_ARM9TDMI,	"TI ARM925T",
	  generic_steppings, "4T" },

	{ CPU_ID_ARM946ES,	CPU_CLASS_ARM9ES,	"ARM946E-S",
	  generic_steppings, "5TE" },
	{ CPU_ID_ARM966ES,	CPU_CLASS_ARM9ES,	"ARM966E-S",
	  generic_steppings, "5TE" },
	{ CPU_ID_ARM966ESR1,	CPU_CLASS_ARM9ES,	"ARM966E-S",
	  generic_steppings, "5TE" },
	{ CPU_ID_MV88SV131,	CPU_CLASS_ARM9ES,	"Sheeva 88SV131",
	  generic_steppings, "5TE" },
	{ CPU_ID_MV88FR571_VD,	CPU_CLASS_ARM9ES,	"Sheeva 88FR571-vd",
	  generic_steppings, "5TE" },

	{ CPU_ID_80200,		CPU_CLASS_XSCALE,	"i80200",
	  xscale_steppings, "5TE" },

	{ CPU_ID_80321_400,	CPU_CLASS_XSCALE,	"i80321 400MHz",
	  i80321_steppings, "5TE" },
	{ CPU_ID_80321_600,	CPU_CLASS_XSCALE,	"i80321 600MHz",
	  i80321_steppings, "5TE" },
	{ CPU_ID_80321_400_B0,	CPU_CLASS_XSCALE,	"i80321 400MHz",
	  i80321_steppings, "5TE" },
	{ CPU_ID_80321_600_B0,	CPU_CLASS_XSCALE,	"i80321 600MHz",
	  i80321_steppings, "5TE" },

	{ CPU_ID_80219_400,	CPU_CLASS_XSCALE,	"i80219 400MHz",
	  i80219_steppings, "5TE" },
	{ CPU_ID_80219_600,	CPU_CLASS_XSCALE,	"i80219 600MHz",
	  i80219_steppings, "5TE" },

	{ CPU_ID_PXA27X,	CPU_CLASS_XSCALE,	"PXA27x",
	  pxa27x_steppings, "5TE" },
	{ CPU_ID_PXA250A,	CPU_CLASS_XSCALE,	"PXA250",
	  pxa2x0_steppings, "5TE" },
	{ CPU_ID_PXA210A,	CPU_CLASS_XSCALE,	"PXA210",
	  pxa2x0_steppings, "5TE" },
	{ CPU_ID_PXA250B,	CPU_CLASS_XSCALE,	"PXA250",
	  pxa2x0_steppings, "5TE" },
	{ CPU_ID_PXA210B,	CPU_CLASS_XSCALE,	"PXA210",
	  pxa2x0_steppings, "5TE" },
	{ CPU_ID_PXA250C, 	CPU_CLASS_XSCALE,	"PXA255/26x",
	  pxa255_steppings, "5TE" },
	{ CPU_ID_PXA210C, 	CPU_CLASS_XSCALE,	"PXA210",
	  pxa2x0_steppings, "5TE" },

	{ CPU_ID_IXP425_533,	CPU_CLASS_XSCALE,	"IXP425 533MHz",
	  ixp425_steppings, "5TE" },
	{ CPU_ID_IXP425_400,	CPU_CLASS_XSCALE,	"IXP425 400MHz",
	  ixp425_steppings, "5TE" },
	{ CPU_ID_IXP425_266,	CPU_CLASS_XSCALE,	"IXP425 266MHz",
	  ixp425_steppings, "5TE" },

	{ CPU_ID_ARM1020E,	CPU_CLASS_ARM10E,	"ARM1020E",
	  generic_steppings, "5TE" },
	{ CPU_ID_ARM1022ES,	CPU_CLASS_ARM10E,	"ARM1022E-S",
	  generic_steppings, "5TE" },

	{ CPU_ID_ARM1026EJS,	CPU_CLASS_ARM10EJ,	"ARM1026EJ-S",
	  generic_steppings, "5TEJ" },
	{ CPU_ID_ARM926EJS,	CPU_CLASS_ARM9EJS,	"ARM926EJ-S",
	  generic_steppings, "5TEJ" },

	{ CPU_ID_ARM1136JS,	CPU_CLASS_ARM11J,	"ARM1136J-S r0",
	  pN_steppings, "6J" },
	{ CPU_ID_ARM1136JSR1,	CPU_CLASS_ARM11J,	"ARM1136J-S r1",
	  pN_steppings, "6J" },
#if 0
	/* The ARM1156T2-S only has a memory protection unit */
	{ CPU_ID_ARM1156T2S,	CPU_CLASS_ARM11J,	"ARM1156T2-S r0",
	  pN_steppings, "6T2" },
#endif
	{ CPU_ID_ARM1176JZS,	CPU_CLASS_ARM11J,	"ARM1176JZ-S r0",
	  pN_steppings, "6ZK" },

	{ CPU_ID_ARM11MPCORE,	CPU_CLASS_ARM11J, 	"ARM11 MPCore",
	  generic_steppings, "6K" },

	{ CPU_ID_CORTEXA5R0,	CPU_CLASS_CORTEX,	"Cortex-A5 r0",
	  pN_steppings, "7A" },
	{ CPU_ID_CORTEXA7R0,	CPU_CLASS_CORTEX,	"Cortex-A7 r0",
	  pN_steppings, "7A" },
	{ CPU_ID_CORTEXA8R1,	CPU_CLASS_CORTEX,	"Cortex-A8 r1",
	  pN_steppings, "7A" },
	{ CPU_ID_CORTEXA8R2,	CPU_CLASS_CORTEX,	"Cortex-A8 r2",
	  pN_steppings, "7A" },
	{ CPU_ID_CORTEXA8R3,	CPU_CLASS_CORTEX,	"Cortex-A8 r3",
	  pN_steppings, "7A" },
	{ CPU_ID_CORTEXA9R1,	CPU_CLASS_CORTEX,	"Cortex-A9 r1",
	  pN_steppings, "7A" },
	{ CPU_ID_CORTEXA9R2,	CPU_CLASS_CORTEX,	"Cortex-A9 r2",
	  pN_steppings, "7A" },
	{ CPU_ID_CORTEXA9R3,	CPU_CLASS_CORTEX,	"Cortex-A9 r3",
	  pN_steppings, "7A" },
	{ CPU_ID_CORTEXA9R4,	CPU_CLASS_CORTEX,	"Cortex-A9 r4",
	  pN_steppings, "7A" },
	{ CPU_ID_CORTEXA15R2,	CPU_CLASS_CORTEX,	"Cortex-A15 r2",
	  pN_steppings, "7A" },
	{ CPU_ID_CORTEXA15R3,	CPU_CLASS_CORTEX,	"Cortex-A15 r3",
	  pN_steppings, "7A" },
	{ CPU_ID_CORTEXA17R1,	CPU_CLASS_CORTEX,	"Cortex-A17 r1",
	  pN_steppings, "7A" },
	{ CPU_ID_CORTEXA35R0,	CPU_CLASS_CORTEX,	"Cortex-A35 r0",
	  pN_steppings, "8A" },
	{ CPU_ID_CORTEXA53R0,	CPU_CLASS_CORTEX,	"Cortex-A53 r0",
	  pN_steppings, "8A" },
	{ CPU_ID_CORTEXA57R0,	CPU_CLASS_CORTEX,	"Cortex-A57 r0",
	  pN_steppings, "8A" },
	{ CPU_ID_CORTEXA57R1,	CPU_CLASS_CORTEX,	"Cortex-A57 r1",
	  pN_steppings, "8A" },
	{ CPU_ID_CORTEXA72R0,	CPU_CLASS_CORTEX,	"Cortex-A72 r0",
	  pN_steppings, "8A" },

	{ CPU_ID_MV88SV581X_V6, CPU_CLASS_PJ4B,      "Sheeva 88SV581x",
	  generic_steppings },
	{ CPU_ID_ARM_88SV581X_V6, CPU_CLASS_PJ4B,    "Sheeva 88SV581x",
	  generic_steppings },
	{ CPU_ID_MV88SV581X_V7, CPU_CLASS_PJ4B,      "Sheeva 88SV581x",
	  generic_steppings },
	{ CPU_ID_ARM_88SV581X_V7, CPU_CLASS_PJ4B,    "Sheeva 88SV581x",
	  generic_steppings },
	{ CPU_ID_MV88SV584X_V6, CPU_CLASS_PJ4B,      "Sheeva 88SV584x",
	  generic_steppings },
	{ CPU_ID_ARM_88SV584X_V6, CPU_CLASS_PJ4B,    "Sheeva 88SV584x",
	  generic_steppings },
	{ CPU_ID_MV88SV584X_V7, CPU_CLASS_PJ4B,      "Sheeva 88SV584x",
	  generic_steppings },


	{ 0, CPU_CLASS_NONE, NULL, NULL, "" }
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
	[CPU_CLASS_PJ4B] =	{ "Marvell",	"CPU_PJ4B" },
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
	"write-back",
	"write-back-locking-line",
	"write-back-locking-C",
	"write-back-locking-D",
};

static void
print_cache_info(device_t dv, struct arm_cache_info *info, u_int level)
{
	if (info->cache_unified) {
		aprint_normal_dev(dv, "%dKB/%dB %d-way %s L%u %cI%cT Unified cache\n",
		    info->dcache_size / 1024,
		    info->dcache_line_size, info->dcache_ways,
		    wtnames[info->cache_type], level + 1,
		    info->dcache_type & CACHE_TYPE_PIxx ? 'P' : 'V',
		    info->dcache_type & CACHE_TYPE_xxPT ? 'P' : 'V');
	} else {
		aprint_normal_dev(dv, "%dKB/%dB %d-way L%u %cI%cT Instruction cache\n",
		    info->icache_size / 1024,
		    info->icache_line_size, info->icache_ways, level + 1,
		    info->icache_type & CACHE_TYPE_PIxx ? 'P' : 'V',
		    info->icache_type & CACHE_TYPE_xxPT ? 'P' : 'V');
		aprint_normal_dev(dv, "%dKB/%dB %d-way %s L%u %cI%cT Data cache\n",
		    info->dcache_size / 1024, 
		    info->dcache_line_size, info->dcache_ways,
		    wtnames[info->cache_type], level + 1,
		    info->dcache_type & CACHE_TYPE_PIxx ? 'P' : 'V',
		    info->dcache_type & CACHE_TYPE_xxPT ? 'P' : 'V');
	}
}

static enum cpu_class
identify_arm_model(uint32_t cpuid, char *buf, size_t len)
{
	enum cpu_class cpu_class = CPU_CLASS_NONE;
	for (const struct cpuidtab *id = cpuids; id->cpuid != 0; id++) {
		if (id->cpuid == (cpuid & CPU_ID_CPU_MASK)) {
			const char *steppingstr =
			    id->cpu_steppings[cpuid & CPU_ID_REVISION_MASK];
			cpu_arch = id->cpu_arch;
			cpu_class = id->cpu_class;
			snprintf(buf, len, "%s%s%s (%s V%s core)",
			    id->cpu_classname,
			    steppingstr[0] == '*' ? "" : " ",
			    &steppingstr[steppingstr[0] == '*'],
			    cpu_classes[cpu_class].class_name,
			    cpu_arch);
			return cpu_class;
		}
	}

	snprintf(buf, len, "unknown CPU (ID = 0x%x)", cpuid);
	return cpu_class;
}

void
identify_arm_cpu(device_t dv, struct cpu_info *ci)
{
	const uint32_t arm_cpuid = ci->ci_arm_cpuid;
	const char * const xname = device_xname(dv);
	char model[128];

	if (arm_cpuid == 0) {
		aprint_error("Processor failed probe - no CPU ID\n");
		return;
	}

	const enum cpu_class cpu_class = identify_arm_model(arm_cpuid,
	     model, sizeof(model));
	if (ci->ci_cpuid == 0) {
		cpu_setmodel("%s", model);
	}

	if (ci->ci_data.cpu_cc_freq != 0) {
		char freqbuf[10];
		humanize_number(freqbuf, sizeof(freqbuf), ci->ci_data.cpu_cc_freq,
		    "Hz", 1000);

		aprint_naive(": %s %s\n", freqbuf, model);
		aprint_normal(": %s %s\n", freqbuf, model);
	} else {
		aprint_naive(": %s\n", model);
		aprint_normal(": %s\n", model);
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
	case CPU_CLASS_PJ4B:
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

	if (CPU_ID_CORTEX_P(arm_cpuid) || CPU_ID_ARM11_P(arm_cpuid) || CPU_ID_MV88SV58XX_P(arm_cpuid)) {
		identify_features(dv);
	}

	/* Print cache info. */
	if (arm_pcache.icache_line_size != 0 || arm_pcache.dcache_line_size != 0) {
		print_cache_info(dv, &arm_pcache, 0);
	}
	if (arm_scache.icache_line_size != 0 || arm_scache.dcache_line_size != 0) {
		print_cache_info(dv, &arm_scache, 1);
	}


	switch (cpu_class) {
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
#if defined(CPU_PJ4B)
	case CPU_CLASS_PJ4B:
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

extern int cpu_instruction_set_attributes[6];
extern int cpu_memory_model_features[4];
extern int cpu_processor_features[2];
extern int cpu_simd_present;
extern int cpu_simdex_present;

void
identify_features(device_t dv)
{
	cpu_instruction_set_attributes[0] = armreg_isar0_read();
	cpu_instruction_set_attributes[1] = armreg_isar1_read();
	cpu_instruction_set_attributes[2] = armreg_isar2_read();
	cpu_instruction_set_attributes[3] = armreg_isar3_read();
	cpu_instruction_set_attributes[4] = armreg_isar4_read();
	cpu_instruction_set_attributes[5] = armreg_isar5_read();

	cpu_hwdiv_present =
	    ((cpu_instruction_set_attributes[0] >> 24) & 0x0f) >= 2;
	cpu_simd_present =
	    ((cpu_instruction_set_attributes[3] >> 4) & 0x0f) >= 3;
	cpu_simdex_present = cpu_simd_present
	    && ((cpu_instruction_set_attributes[1] >> 12) & 0x0f) >= 2;
	cpu_synchprim_present =
	    ((cpu_instruction_set_attributes[3] >> 8) & 0xf0)
	    | ((cpu_instruction_set_attributes[4] >> 20) & 0x0f);

	cpu_memory_model_features[0] = armreg_mmfr0_read();
	cpu_memory_model_features[1] = armreg_mmfr1_read();
	cpu_memory_model_features[2] = armreg_mmfr2_read();
	cpu_memory_model_features[3] = armreg_mmfr3_read();

#if 0
	if (__SHIFTOUT(cpu_memory_model_features[3], __BITS(23,20))) {
		/*
		 * Updates to the translation tables do not require a clean
		 * to the point of unification to ensure visibility by
		 * subsequent translation table walks.
		 */
		pmap_needs_pte_sync = 0;
	}
#endif

	cpu_processor_features[0] = armreg_pfr0_read();
	cpu_processor_features[1] = armreg_pfr1_read();

	aprint_debug_dev(dv, "sctlr: %#x\n", armreg_sctlr_read());
	aprint_debug_dev(dv, "actlr: %#x\n", armreg_auxctl_read());
	aprint_debug_dev(dv, "revidr: %#x\n", armreg_revidr_read());
#ifdef MULTIPROCESSOR
	aprint_debug_dev(dv, "mpidr: %#x\n", armreg_mpidr_read());
#endif
	aprint_debug_dev(dv,
	    "isar: [0]=%#x [1]=%#x [2]=%#x [3]=%#x, [4]=%#x, [5]=%#x\n",
	    cpu_instruction_set_attributes[0],
	    cpu_instruction_set_attributes[1],
	    cpu_instruction_set_attributes[2],
	    cpu_instruction_set_attributes[3],
	    cpu_instruction_set_attributes[4],
	    cpu_instruction_set_attributes[5]);
	aprint_debug_dev(dv,
	    "mmfr: [0]=%#x [1]=%#x [2]=%#x [3]=%#x\n",
	    cpu_memory_model_features[0], cpu_memory_model_features[1],
	    cpu_memory_model_features[2], cpu_memory_model_features[3]);
	aprint_debug_dev(dv,
	    "pfr: [0]=%#x [1]=%#x\n",
	    cpu_processor_features[0], cpu_processor_features[1]);
}
