/*	$NetBSD: cpu.c,v 1.23 2000/06/26 14:20:36 mrg Exp $	*/

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
#include <sys/systm.h>
#include <sys/malloc.h>
#include <sys/device.h>
#include <sys/proc.h>
#include <vm/vm.h>
#include <machine/bootconfig.h>
#include <machine/io.h>
#include <machine/conf.h>
#include <machine/cpu.h>
#include <machine/cpus.h>
#include <machine/undefined.h>
#ifdef ARMFPE
#include <arm32/fpe-arm/armfpe.h>
#endif	/* ARMFPE */

#include "cpu.h"
#if NCPU != 1
#error Need 1 CPU configured
#endif

struct cpu_softc {
	struct	device sc_device;
	int	sc_open;
};

/* Array of cpu structures, one per possible cpu */
cpu_t cpus[MAX_CPUS];

char cpu_model[64];
volatile int undefined_test;	/* Used for FPA test */
extern int cpuctrl;		/* cpu control register value */

/* Prototypes */
void identify_master_cpu __P((int cpu_number, char *dev_name));
void identify_arm_cpu	__P((int cpu_number));
void identify_arm_fpu	__P((int cpu_number));

extern int initialise_fpe	__P((cpu_t *cpu));


/*
 * int cpumatch(struct device *parent, struct cfdata *cf, void *aux)
 *
 * Probe for the main cpu. Currently all this does is return 1 to
 * indicate that the cpu was found.
 */ 
 
int
cpumatch(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	return(1);
}


/*
 * void cpusattach(struct device *parent, struct device *dev, void *aux)
 *
 * Attach the main cpu
 */
  
void
cpuattach(parent, self, aux)
	struct device *parent;
	struct device *self;
	void *aux;
{
	int loop;

	for (loop = 0; loop < MAX_CPUS; ++loop)
		memset(&cpus[loop], 0, sizeof(cpu_t));

	identify_master_cpu(CPU_MASTER, self->dv_xname);
}

struct cfattach cpu_ca = {
	sizeof(struct cpu_softc), cpumatch, cpuattach
};

extern struct cfdriver cpu_cd;

/*
 * Used to test for an FPA. The following function is installed as a coproc1
 * handler on the undefined instruction vector and then we issue a FPA
 * instruction. If undefined_test is non zero then the FPA did not handle
 * the instruction so must be absent.
 */

int
fpa_test(address, instruction, frame)
	u_int address;
	u_int instruction;
	trapframe_t *frame;
{
	++undefined_test;
	return(0);
}

/*
 * If an FPA was found then this function is installed as the coproc1 handler
 * on the undefined instruction vector. Currently we don't support FPA's
 * so this just triggers an exception.
 */

int
fpa_handler(address, instruction, frame, fault_code)
	u_int address;
	u_int instruction;
	trapframe_t *frame;
	int fault_code;
{
	u_int fpsr;
    
	__asm __volatile("stmfd sp!, {r0}; .word 0xee300110; mov %0, r0; ldmfd sp!, {r0}" : "=r" (fpsr));

	printf("FPA exception: fpsr = %08x\n", fpsr);

	return(1);
}


/*
 * Identify the master (boot) CPU
 * This also probes for an FPU and will install an FPE if necessary
 */
 
void
identify_master_cpu(cpu_number, dev_name)
	int cpu_number;
	char *dev_name;
{
	u_int fpsr;

	cpus[cpu_number].cpu_class = CPU_CLASS_ARM;
	cpus[cpu_number].cpu_host = CPU_HOST_MAINBUS;
	cpus[cpu_number].cpu_flags = CPU_FLAG_PRESENT;
	cpus[cpu_number].cpu_ctrl = cpuctrl;

	/* Get the cpu ID from coprocessor 15 */

	cpus[cpu_number].cpu_id = cpu_id();

	identify_arm_cpu(cpu_number);
	strcpy(cpu_model, cpus[cpu_number].cpu_model);

	if (cpus[CPU_MASTER].cpu_class == CPU_CLASS_SARM
	    && (cpus[CPU_MASTER].cpu_id & CPU_ID_REVISION_MASK) < 3) {
		printf("%s: SA-110 with bugged STM^ instruction\n", dev_name);
	}

#ifdef CPU_ARM8
	if (cpus[CPU_MASTER].cpu_class == CPU_CLASS_ARM
	    && (cpus[CPU_MASTER].cpu_id & CPU_ID_CPU_MASK) == ID_ARM810) {
		int clock = arm8_clock_config(0, 0);
		char *fclk;
		printf("%s: ARM810 cp15=%02x", dev_name, clock);
		printf(" clock:%s", (clock & 1) ? " dynamic" : "");
		printf("%s", (clock & 2) ? " sync" : "");
		switch ((clock >> 2) & 3) {
		case 0 :
			fclk = "bus clock";
			break;
		case 1 :
			fclk = "ref clock";
			break;
		case 3 :
			fclk = "pll";
			break;
		default :
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
 
	install_coproc_handler(FP_COPROC, fpa_test);

	undefined_test = 0;

	__asm __volatile("stmfd sp!, {r0}; .word 0xee300110; mov %0, r0; ldmfd sp!, {r0}" : "=r" (fpsr));

	if (undefined_test == 0) {
		cpus[cpu_number].fpu_type = (fpsr >> 24);
	        switch (fpsr >> 24) {
		case 0x81 :
			cpus[cpu_number].fpu_class = FPU_CLASS_FPA;
			break;

		default :
			cpus[cpu_number].fpu_class = FPU_CLASS_FPU;
			break;
		}
		cpus[cpu_number].fpu_flags = 0;
		install_coproc_handler(FP_COPROC, fpa_handler);
	} else {
		cpus[cpu_number].fpu_class = FPU_CLASS_NONE;
		cpus[cpu_number].fpu_flags = 0;

	/*
	 * Ok if ARMFPE is defined and the boot options request the ARM FPE
	 * then it will be installed as the FPE. If the installation fails
	 * the existing FPE is used as a fall back.
	 * If either ARMFPE is not defined or the boot args did not request
	 * it the old FPE is installed.
	 * This is just while I work on integrating the new FPE.
	 * It means the new FPE gets installed if compiled int (ARMFPE
	 * defined) and also gives me a on/off option when I boot in case
	 * the new FPE is causing panics.
	 * In all cases it falls back on the existing FPE is the ARMFPE
	 * was not successfully installed.
	 */

#ifdef ARMFPE
        if (boot_args) {
        	char *ptr;
       
		ptr = strstr(boot_args, "noarmfpe");
		if (!ptr) {
			if (initialise_arm_fpe(&cpus[cpu_number]) != 0) {
				identify_arm_fpu(cpu_number);
#ifdef FPE
				initialise_fpe(&cpus[cpu_number]);
#endif
			}
#ifdef FPE
		} else
			initialise_fpe(&cpus[cpu_number]);

	} else
		initialise_fpe(&cpus[cpu_number]);
#else
		}
	}
#endif

#else
#ifdef FPE
		initialise_fpe(&cpus[cpu_number]);
#else
		/* Now we support softfloat libraries we may not have an FPE. */
#endif
#endif
	}

	identify_arm_fpu(cpu_number);
}



/*
 * Report the type of the specifed arm processor. This uses the generic and
 * arm specific information in the cpu structure to identify the processor.
 * The remaining fields in the cpu structure are filled in appropriately.
 */

void
identify_arm_cpu(cpu_number)
	int cpu_number;
{
	cpu_t *cpu;
	u_int cpuid;

	cpu = &cpus[cpu_number];
	if (cpu->cpu_host == CPU_HOST_NONE || cpu->cpu_class == CPU_CLASS_NONE) {
		printf("No installed processor\n");
		return;
	}
	if (cpu->cpu_class != CPU_CLASS_ARM && cpu->cpu_class != CPU_CLASS_SARM) {
		printf("identify_arm_cpu: Can only identify ARM CPU's\n");
		return;
	}
	cpuid = cpu->cpu_id;

	if (cpuid == 0) {
		printf("Processor failed probe - no CPU ID\n");
		return;
	}

/*	if ((cpuid & CPU_ID_DESIGNER_MASK) != CPU_ID_ARM_LTD)
		printf("Unrecognised designer ID = %08x\n", cpuid);*/

	switch (cpuid & CPU_ID_CPU_MASK) {
#ifdef CPU_ARM6
        case ID_ARM610:
        	cpu->cpu_type = cpuid & CPU_ID_CPU_MASK;
		break;
#endif
#ifdef CPU_ARM7
	case ID_ARM710 :
	case ID_ARM700 :
		cpu->cpu_type = (cpuid & CPU_ID_CPU_MASK) >> 4;
		break;
#endif
#ifdef CPU_ARM8
	case ID_ARM810 :
		cpu->cpu_type = (cpuid & CPU_ID_CPU_MASK) >> 4;
		break;
#endif
#ifdef CPU_SA110
	case ID_SA110 :
		cpu->cpu_type = (cpuid & CPU_ID_CPU_MASK) >> 4;
		cpu->cpu_class = CPU_CLASS_SARM;
		sprintf(cpu->cpu_model, "SA-110 rev %d",
		    cpuid & CPU_ID_REVISION_MASK);
		break;
#endif
	default :
		printf("Unrecognised processor ID = %08x\n", cpuid);
		cpu->cpu_type = cpuid & CPU_ID_CPU_MASK;
		break;
	}

	if (cpu->cpu_class == CPU_CLASS_ARM) {
		sprintf(cpu->cpu_model, "ARM%x rev %d", cpu->cpu_type,
		    cpuid & CPU_ID_REVISION_MASK);

		if ((cpu->cpu_ctrl & CPU_CONTROL_IDC_ENABLE) == 0)
			strcat(cpu->cpu_model, " IDC disabled");
		else
			strcat(cpu->cpu_model, " IDC enabled");
	} else if (cpu->cpu_class == CPU_CLASS_SARM) {
		if ((cpu->cpu_ctrl & CPU_CONTROL_DC_ENABLE) == 0)
			strcat(cpu->cpu_model, " DC disabled");
		else
			strcat(cpu->cpu_model, " DC enabled");
		if ((cpu->cpu_ctrl & CPU_CONTROL_IC_ENABLE) == 0)
			strcat(cpu->cpu_model, " IC disabled");
		else
			strcat(cpu->cpu_model, " IC enabled");
	}
	if ((cpu->cpu_ctrl & CPU_CONTROL_WBUF_ENABLE) == 0)
		strcat(cpu->cpu_model, " WB disabled");
	else
		strcat(cpu->cpu_model, " WB enabled");

	if (cpu->cpu_ctrl & CPU_CONTROL_LABT_ENABLE)
		strcat(cpu->cpu_model, " LABT");
	else
		strcat(cpu->cpu_model, " EABT");

	if (cpu->cpu_ctrl & CPU_CONTROL_BPRD_ENABLE)
		strcat(cpu->cpu_model, " branch prediction enabled");

	/* Print the info */

	printf(": %s\n", cpu->cpu_model);
}


/*
 * Report the type of the specifed arm fpu. This uses the generic and arm
 * specific information in the cpu structure to identify the fpu. The
 * remaining fields in the cpu structure are filled in appropriately.
 */

void
identify_arm_fpu(cpu_number)
	int cpu_number;
{
	cpu_t *cpu;

	cpu = &cpus[cpu_number];
	if (cpu->cpu_host == CPU_HOST_NONE || cpu->cpu_class == CPU_CLASS_NONE) {
		printf("No installed processor\n");
		return;
	}

	if (cpu->cpu_class != CPU_CLASS_ARM && cpu->cpu_class != CPU_CLASS_SARM) {
		printf("identify_arm_cpu: Can only identify ARM hosted FPUs\n");
		return;
	}

	/* Now for the FP info */

	switch (cpu->fpu_class) {
	case FPU_CLASS_NONE :
		strcpy(cpu->fpu_model, "None");
		break;
	case FPU_CLASS_FPE :
		printf("fpe%d at cpu%d: %s\n", cpu_number, cpu_number,
		    cpu->fpu_model);
		printf("fpe%d: no hardware found\n", cpu_number);
		break;
	case FPU_CLASS_FPA :
		printf("fpe%d at cpu%d: %s\n", cpu_number, cpu_number,
		    cpu->fpu_model);
		if (cpu->fpu_type == FPU_TYPE_FPA11) {
			strcpy(cpu->fpu_model, "FPA11");
			printf("fpe%d: FPA11 found\n", cpu_number);
		} else {
			strcpy(cpu->fpu_model, "FPA");
			printf("fpe%d: FPA10 found\n", cpu_number);
		}
		if ((cpu->fpu_flags & 4) == 0)
			strcat(cpu->fpu_model, "");
		else
			strcat(cpu->fpu_model, " clk/2");
		break;
	case FPU_CLASS_FPU :
		sprintf(cpu->fpu_model, "Unknown FPU (ID=%02x)\n",
		    cpu->fpu_type);
		printf("fpu%d at cpu%d: %s\n", cpu_number, cpu_number,
		    cpu->fpu_model);
		break;
	}
}


int
cpuopen(dev, flag, mode, p)
	dev_t dev;
	int flag;
	int mode;
	struct proc *p;
{
	struct cpu_softc *sc;
	int unit;
	int s;
    
    	unit = minor(dev);
	if (unit >= cpu_cd.cd_ndevs)
		return(ENXIO);

	sc = cpu_cd.cd_devs[unit];
	if (!sc) return(ENXIO);

	s = splhigh();
	if (sc->sc_open) {
		(void)splx(s);
		return(EBUSY);
	}

	++sc->sc_open;   
	(void)splx(s);

	return(0);
}


int
cpuclose(dev, flag, mode, p)
	dev_t dev;
	int flag;
	int mode;
	struct proc *p;
{
	struct cpu_softc *sc;
	int unit;
	int s;

	unit = minor(dev);
	sc = cpu_cd.cd_devs[unit];

	if (sc->sc_open == 0) return(ENXIO);
    
	s = splhigh();
	--sc->sc_open;
	(void)splx(s);
      
	return(0);
}


int
cpuioctl(dev, cmd, data, flag, p)
	dev_t dev;
	u_long cmd;
	caddr_t data;
	int flag;
	struct proc *p;
{
	struct cpu_softc *sc;
	int unit;

	unit = minor(dev);
	sc = cpu_cd.cd_devs[unit];

	switch (cmd) {
	default:
		return(ENXIO);
		break;
	}   

	return(0);
}

/* End of cpu.c */
