/*	$NetBSD: cpu.c,v 1.23 2002/03/10 11:06:01 bjh21 Exp $	*/

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
 * Probing and configuration for the master cpu
 *
 * Created      : 10/10/95
 */

#include "opt_armfpe.h"
#include "opt_cputypes.h"

#include <sys/param.h>

__RCSID("$NetBSD: cpu.c,v 1.23 2002/03/10 11:06:01 bjh21 Exp $");

#include <sys/systm.h>
#include <sys/malloc.h>
#include <sys/device.h>
#include <sys/proc.h>
#include <uvm/uvm_extern.h>
#include <machine/conf.h>
#include <machine/cpu.h>
#include <arm/undefined.h>

#include <arm/cpus.h>

#ifdef ARMFPE
#include <machine/bootconfig.h> /* For boot args */
#include <arm/fpe-arm/armfpe.h>
#endif

cpu_t cpus[MAX_CPUS];

char cpu_model[256];
volatile int undefined_test;	/* Used for FPA test */

/* Prototypes */
void identify_master_cpu(struct device *dv, int cpu_number);
void identify_arm_cpu(struct device *dv, int cpu_number, struct cpu_info *);
void identify_arm_fpu(struct device *dv, int cpu_number);
int fpa_test(u_int, u_int, trapframe_t *, int);
int fpa_handler(u_int, u_int, trapframe_t *, int);

/*
 * void cpusattach(struct device *parent, struct device *dev, void *aux)
 *
 * Attach the main cpu
 */
  
void
cpu_attach(struct device *dv)
{

	identify_master_cpu(dv, CPU_MASTER);
}

/*
 * Used to test for an FPA. The following function is installed as a coproc1
 * handler on the undefined instruction vector and then we issue a FPA
 * instruction. If undefined_test is non zero then the FPA did not handle
 * the instruction so must be absent.
 */

int
fpa_test(u_int address, u_int instruction, trapframe_t *frame, int fault_code)
{

	frame->tf_pc += INSN_SIZE;
	++undefined_test;
	return(0);
}

/*
 * If an FPA was found then this function is installed as the coproc1 handler
 * on the undefined instruction vector. Currently we don't support FPA's
 * so this just triggers an exception.
 */

int
fpa_handler(u_int address, u_int instruction, trapframe_t *frame,
    int fault_code)
{
	u_int fpsr;
    
	__asm __volatile("rfs %0" : "=r" (fpsr));

	printf("FPA exception: fpsr = %08x\n", fpsr);

	return(1);
}


/*
 * Identify the master (boot) CPU
 * This also probes for an FPU and will install an FPE if necessary
 */
 
void
identify_master_cpu(struct device *dv, int cpu_number)
{
	u_int fpsr;
	void *uh;

	curcpu()->ci_dev = dv;

	evcnt_attach_dynamic(&curcpu()->ci_arm700bugcount, EVCNT_TYPE_MISC,
	    NULL, dv->dv_xname, "arm700swibug");
	
	/* Get the cpu ID from coprocessor 15 */

	curcpu()->ci_cpuid = cpu_id();

	identify_arm_cpu(dv, cpu_number, curcpu());

	if ((curcpu()->ci_cpuid & CPU_ID_CPU_MASK) == CPU_ID_SA110
	    && (curcpu()->ci_cpuid & CPU_ID_REVISION_MASK) < 3) {
		printf("%s: SA-110 with bugged STM^ instruction\n",
		       dv->dv_xname);
	}

#ifdef CPU_ARM8
	if ((curcpu()->ci_cpuid & CPU_ID_CPU_MASK) == CPU_ID_ARM810) {
		int clock = arm8_clock_config(0, 0);
		char *fclk;
		printf("%s: ARM810 cp15=%02x", dv->dv_xname, clock);
		printf(" clock:%s", (clock & 1) ? " dynamic" : "");
		printf("%s", (clock & 2) ? " sync" : "");
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
		printf(" fclk source=%s\n", fclk);
 	}
#endif

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
 
	uh = install_coproc_handler(FP_COPROC, fpa_test);

	undefined_test = 0;

	__asm __volatile("rfs %0" : "=r" (fpsr));

	remove_coproc_handler(uh);

	if (undefined_test == 0) {
		cpus[cpu_number].fpu_type = (fpsr >> 24);
	        switch (fpsr >> 24) {
		case 0x81:
			cpus[cpu_number].fpu_class = FPU_CLASS_FPA;
			break;

		default:
			cpus[cpu_number].fpu_class = FPU_CLASS_FPU;
			break;
		}
		install_coproc_handler(FP_COPROC, fpa_handler);
	} else {
		cpus[cpu_number].fpu_class = FPU_CLASS_NONE;

		/*
		 * Ok if ARMFPE is defined and the boot options request the 
		 * ARM FPE then it will be installed as the FPE.
		 * This is just while I work on integrating the new FPE.
		 * It means the new FPE gets installed if compiled int (ARMFPE
		 * defined) and also gives me a on/off option when I boot in
		 * case the new FPE is causing panics.
		 */

#ifdef ARMFPE
		if (boot_args) {
			int usearmfpe = 1;

			get_bootconf_option(boot_args, "armfpe",
			    BOOTOPT_TYPE_BOOLEAN, &usearmfpe);
			if (usearmfpe) {
				if (initialise_arm_fpe(&cpus[cpu_number]) != 0)
					identify_arm_fpu(dv, cpu_number);
			}
		}

#endif
	}

	identify_arm_fpu(dv, cpu_number);
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
	CPU_CLASS_SA1,
	CPU_CLASS_XSCALE,
};

static const char *generic_steppings[16] = {
	"rev 0",	"rev 1",	"rev 2",	"rev 3",
	"rev 4",	"rev 5",	"rev 6",	"rev 7",
	"rev 8",	"rev 9",	"rev 10",	"rev 11",
	"rev 12",	"rev 13",	"rev 14",	"rev 15",
};

static const char *sa110_steppings[16] = {
	"rev 0",	"step J",	"step K",	"step S",
	"step T",	"rev 5",	"rev 6",	"rev 7",
	"rev 8",	"rev 9",	"rev 10",	"rev 11",
	"rev 12",	"rev 13",	"rev 14",	"rev 15",
};

static const char *sa1100_steppings[16] = {
	"rev 0",	"step B",	"step C",	"rev 3",
	"rev 4",	"rev 5",	"rev 6",	"rev 7",
	"step D",	"step E",	"rev 10"	"step G",
	"rev 12",	"rev 13",	"rev 14",	"rev 15",
};

static const char *sa1110_steppings[16] = {
	"step A-0",	"rev 1",	"rev 2",	"rev 3",
	"step B-0",	"step B-1",	"step B-2",	"step B-3",
	"step B-4",	"step B-5",	"rev 10",	"rev 11",
	"rev 12",	"rev 13",	"rev 14",	"rev 15",
};

static const char *i80200_steppings[16] = {
	"step A-0",	"step A-1",	"step B-0",	"step C-0",
	"rev 4",	"rev 5",	"rev 6",	"rev 7",
	"rev 8",	"rev 9",	"rev 10",	"rev 11",
	"rev 12",	"rev 13",	"rev 14",	"rev 15",
};

struct cpuidtab {
	u_int32_t	cpuid;
	enum		cpu_class cpu_class;
	const char	*cpu_name;
	const char	**cpu_steppings;
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
	{ CPU_ID_ARM940T,	CPU_CLASS_ARM9TDMI,	"ARM940T",
	  generic_steppings },
	{ CPU_ID_ARM946ES,	CPU_CLASS_ARM9ES,	"ARM946E-S",
	  generic_steppings },
	{ CPU_ID_ARM966ES,	CPU_CLASS_ARM9ES,	"ARM966E-S",
	  generic_steppings },
	{ CPU_ID_ARM966ESR1,	CPU_CLASS_ARM9ES,	"ARM966E-S",
	  generic_steppings },

	{ CPU_ID_SA110,		CPU_CLASS_SA1,		"SA-110",
	  sa110_steppings },
	{ CPU_ID_SA1100,	CPU_CLASS_SA1,		"SA-1100",
	  sa1100_steppings },
	{ CPU_ID_SA1110,	CPU_CLASS_SA1,		"SA-1110",
	  sa1110_steppings },

	{ CPU_ID_I80200,	CPU_CLASS_XSCALE,	"i80200",
	  i80200_steppings },

	{ 0, CPU_CLASS_NONE, NULL, NULL }
};

struct cpu_classtab {
	const char	*class_name;
	const char	*class_option;
};

const struct cpu_classtab cpu_classes[] = {
	{ "unknown",	NULL },			/* CPU_CLASS_NONE */
	{ "ARM2",	"CPU_ARM2" },		/* CPU_CLASS_ARM2 */
	{ "ARM2as",	"CPU_ARM250" },		/* CPU_CLASS_ARM2AS */
	{ "ARM3",	"CPU_ARM3" },		/* CPU_CLASS_ARM3 */
	{ "ARM6",	"CPU_ARM6" },		/* CPU_CLASS_ARM6 */
	{ "ARM7",	"CPU_ARM7" },		/* CPU_CLASS_ARM7 */
	{ "ARM7TDMI",	"CPU_ARM7TDMI" },	/* CPU_CLASS_ARM7TDMI */
	{ "ARM8",	"CPU_ARM8" },		/* CPU_CLASS_ARM8 */
	{ "ARM9TDMI",	NULL },			/* CPU_CLASS_ARM9TDMI */
	{ "ARM9E-S",	NULL },			/* CPU_CLASS_ARM9ES */
	{ "SA-1",	"CPU_SA110" },		/* CPU_CLASS_SA1 */
	{ "XScale",	"CPU_XSCALE" },		/* CPU_CLASS_XSCALE */
};

/*
 * Report the type of the specifed arm processor. This uses the generic and
 * arm specific information in the cpu structure to identify the processor.
 * The remaining fields in the cpu structure are filled in appropriately.
 */

static const char *wtnames[] = {
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
	"**unknown 14**",
	"**unknown 15**",
};

void
identify_arm_cpu(struct device *dv, int cpu_number, struct cpu_info *ci)
{
	cpu_t *cpu;
	u_int cpuid;
	enum cpu_class cpu_class;
	int i;

	cpu = &cpus[cpu_number];
	cpuid = ci->ci_cpuid;

	if (cpuid == 0) {
		printf("Processor failed probe - no CPU ID\n");
		return;
	}

	for (i = 0; cpuids[i].cpuid != 0; i++)
		if (cpuids[i].cpuid == (cpuid & CPU_ID_CPU_MASK)) {
			cpu_class = cpuids[i].cpu_class;
			sprintf(cpu_model, "%s %s (%s core)",
			    cpuids[i].cpu_name,
			    cpuids[i].cpu_steppings[cpuid &
						    CPU_ID_REVISION_MASK],
			    cpu_classes[cpu_class].class_name);
			break;
		}

	if (cpuids[i].cpuid == 0)
		sprintf(cpu_model, "unknown CPU (ID = 0x%x)", cpuid);

	switch (cpu_class) {
	case CPU_CLASS_ARM6:
	case CPU_CLASS_ARM7:
	case CPU_CLASS_ARM7TDMI:
	case CPU_CLASS_ARM8:
		if ((ci->ci_ctrl & CPU_CONTROL_IDC_ENABLE) == 0)
			strcat(cpu_model, " IDC disabled");
		else
			strcat(cpu_model, " IDC enabled");
		break;
	case CPU_CLASS_ARM9TDMI:
	case CPU_CLASS_SA1:
	case CPU_CLASS_XSCALE:
		if ((ci->ci_ctrl & CPU_CONTROL_DC_ENABLE) == 0)
			strcat(cpu_model, " DC disabled");
		else
			strcat(cpu_model, " DC enabled");
		if ((ci->ci_ctrl & CPU_CONTROL_IC_ENABLE) == 0)
			strcat(cpu_model, " IC disabled");
		else
			strcat(cpu_model, " IC enabled");
		break;
	default:
		break;
	}
	if ((ci->ci_ctrl & CPU_CONTROL_WBUF_ENABLE) == 0)
		strcat(cpu_model, " WB disabled");
	else
		strcat(cpu_model, " WB enabled");

	if (ci->ci_ctrl & CPU_CONTROL_LABT_ENABLE)
		strcat(cpu_model, " LABT");
	else
		strcat(cpu_model, " EABT");

	if (ci->ci_ctrl & CPU_CONTROL_BPRD_ENABLE)
		strcat(cpu_model, " branch prediction enabled");

	/* Print the info */
	printf(": %s\n", cpu_model);

	/* Print cache info. */
	if (arm_picache_line_size == 0 && arm_pdcache_line_size == 0)
		goto skip_pcache;

	if (arm_pcache_unified) {
		printf("%s: %dKB/%dB %d-way %s unified cache\n",
		    dv->dv_xname, arm_pdcache_size / 1024,
		    arm_pdcache_line_size, arm_pdcache_ways,
		    wtnames[arm_pcache_type]);
	} else {
		printf("%s: %dKB/%dB %d-way Instruction cache\n",
		    dv->dv_xname, arm_picache_size / 1024,
		    arm_picache_line_size, arm_picache_ways);
		printf("%s: %dKB/%dB %d-way %s Data cache\n",
		    dv->dv_xname, arm_pdcache_size / 1024, 
		    arm_pdcache_line_size, arm_pdcache_ways,
		    wtnames[arm_pcache_type]);
	}

 skip_pcache:

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
#ifdef CPU_SA110
	case CPU_CLASS_SA1:
#endif
#ifdef CPU_XSCALE
	case CPU_CLASS_XSCALE:
#endif
		break;
	default:
		if (cpu_classes[cpu_class].class_option != NULL)
			printf("%s: %s does not fully support this CPU."
			       "\n", dv->dv_xname, ostype);
		else {
			printf("%s: This kernel does not fully support "
			       "this CPU.\n", dv->dv_xname);
			printf("%s: Recompile with \"options %s\" to "
			       "correct this.\n", dv->dv_xname,
			       cpu_classes[cpu_class].class_option);
		}
		break;
	}
			       
}


/*
 * Report the type of the specifed arm fpu. This uses the generic and arm
 * specific information in the cpu structure to identify the fpu. The
 * remaining fields in the cpu structure are filled in appropriately.
 */

void
identify_arm_fpu(struct device *dv, int cpu_number)
{
	cpu_t *cpu;

	cpu = &cpus[cpu_number];

	/* Now for the FP info */

	switch (cpu->fpu_class) {
	case FPU_CLASS_NONE :
		strcpy(cpu->fpu_model, "None");
		break;
	case FPU_CLASS_FPE :
		printf("%s: FPE: %s\n", dv->dv_xname, cpu->fpu_model);
		printf("%s: no FP hardware found\n", dv->dv_xname);
		break;
	case FPU_CLASS_FPA :
		printf("%s: FPE: %s\n", dv->dv_xname, cpu->fpu_model);
		if (cpu->fpu_type == FPU_TYPE_FPA11) {
			strcpy(cpu->fpu_model, "FPA11");
			printf("%s: FPA11 found\n", dv->dv_xname);
		} else {
			strcpy(cpu->fpu_model, "FPA");
			printf("%s: FPA10 found\n", dv->dv_xname);
		}
	case FPU_CLASS_FPU :
		sprintf(cpu->fpu_model, "Unknown FPU (ID=%02x)\n",
		    cpu->fpu_type);
		printf("%s: %s\n", dv->dv_xname, cpu->fpu_model);
		break;
	}
}

/* End of cpu.c */
