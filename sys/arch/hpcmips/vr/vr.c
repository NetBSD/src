/*	$NetBSD: vr.c,v 1.9 1999/12/09 02:15:27 sato Exp $	*/

/*-
 * Copyright (c) 1999
 *         Shin Takemura and PocketBSD Project. All rights reserved.
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
 *	This product includes software developed by the PocketBSD project
 *	and its contributors.
 * 4. Neither the name of the project nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */
#include <sys/param.h>
#include <sys/types.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/reboot.h>

#include <machine/cpu.h>
#include <machine/intr.h>
#include <machine/reg.h>
#include <machine/psl.h>
#include <machine/locore.h>
#include <machine/sysconf.h>
#include <machine/bus.h>
#include <machine/autoconf.h>

#include <mips/mips_param.h>		/* hokey spl()s */
#include <mips/mips/mips_mcclock.h>	/* mcclock CPUspeed estimation */

#include <hpcmips/vr/vr.h>
#include <hpcmips/vr/vripreg.h>
#include <hpcmips/vr/rtcreg.h>
#include <hpcmips/hpcmips/machdep.h>	/* XXXjrs replace with vectors */
#include <machine/bootinfo.h>

#include "vrip.h"
#if NVRIP > 0
#include <hpcmips/vr/vripvar.h>
#endif

#include "vrbcu.h"
#if NVRBCU > 0
#include <hpcmips/vr/bcuvar.h>
#endif

#include "vrdsu.h"
#if NVRDSU > 0
#include <hpcmips/vr/vrdsuvar.h>
#endif

#include "com.h"
#if NCOM > 0
#include <sys/termios.h>
#include <sys/ttydefaults.h>
#include <dev/ic/comreg.h>
#include <dev/ic/comvar.h>
#include <hpcmips/vr/siureg.h>
#include <hpcmips/vr/com_vripvar.h>
#ifndef CONSPEED
#define CONSPEED TTYDEF_SPEED
#endif
#endif

#include "fb.h"
#include "vrkiu.h"
#if NFB > 0 || NVRKIU > 0
#include <dev/rcons/raster.h>
#include <dev/wscons/wsdisplayvar.h>
#endif

#if NFB > 0
#include <arch/hpcmips/dev/fbvar.h>
#endif

#if NFB > 0
#include <arch/hpcmips/vr/vrkiuvar.h>
#endif

void	vr_init __P((void));
void	vr_os_init __P((void));
void	vr_bus_reset __P((void));
int	vr_intr __P((u_int32_t mask, u_int32_t pc, u_int32_t statusReg, u_int32_t causeReg));
void	vr_cons_init __P((void));
void	vr_device_register __P((struct device *, void *));
void    vr_fb_init __P((caddr_t*));
int     vr_mem_init __P((caddr_t));
void	vr_reboot __P((int howto, char *bootstr));

extern unsigned nullclkread __P((void));
extern unsigned (*clkread) __P((void));

/*
 * CPU interrupt dispatch table (HwInt[0:3])
 */
int null_handler __P((void*, u_int32_t, u_int32_t));
static int (*intr_handler[4]) __P((void*, u_int32_t, u_int32_t)) = 
{
	null_handler,
	null_handler,
	null_handler,
	null_handler
};
static void *intr_arg[4];

void
vr_init()
{
	/*
	 * Platform Information.
	 */

	/*
	 * Platform Specific Function Hooks
	 */
	platform.os_init = vr_os_init;
	platform.bus_reset = vr_bus_reset;
	platform.cons_init = vr_cons_init;
	platform.device_register = vr_device_register;
	platform.fb_init = vr_fb_init;
	platform.mem_init = vr_mem_init;
	platform.reboot = vr_reboot;

#if NVRBCU > 0
	sprintf(cpu_model, "NEC %s rev%d.%d", 
		vrbcu_vrip_getcpuname(),
		vrbcu_vrip_getcpumajor(),
		vrbcu_vrip_getcpuminor());
#else
	sprintf(cpu_model, "NEC VR41xx");
#endif
}

int
vr_mem_init(kernend)
	caddr_t kernend; /* kseg0 */
{
	u_int32_t startaddr, endaddr, page;
	int npage;
#define VR41_SYSADDR_DRAMSTART 0x0
#define VR41_SYSADDR_DRAM_LEN 0x04000000
	startaddr = MIPS_PHYS_TO_KSEG1(
		(btoc((u_int32_t)kernend - MIPS_KSEG0_START)) << PGSHIFT);
	endaddr = MIPS_PHYS_TO_KSEG1(VR41_SYSADDR_DRAMSTART +
				     VR41_SYSADDR_DRAM_LEN);
	for(page = startaddr, npage = 0; page < endaddr; 
	    page+= NBPG, npage++) {
		if (badaddr((char*)page, 4))
			break;
		((volatile int *)page)[0] = 0xa5a5a5a5;
		((volatile int *)page)[4] = 0x5a5a5a5a;
		wbflush();
		if (*(volatile int *)page != 0xa5a5a5a5)
			break;
	}
	/* Clear currently unused D-RAM area (For reboot Windows CE clearly)*/
	memset((void*)startaddr, 0, npage * NBPG);
	memset((void*)(KERNBASE + 0x400), 0, KERNTEXTOFF - KERNBASE - 0x800); 

	return npage;
}

void
vr_fb_init(kernend)
	caddr_t *kernend;
{
	/* Nothing to do */
}

void
vr_os_init()
{
	/*
	 * Set up interrupt handling and I/O addresses.
	 */
	mips_hardware_intr = vr_intr;

	splvec.splbio = MIPS_SPL0;
	splvec.splnet = MIPS_SPL0;
	splvec.spltty = MIPS_SPL0;
	splvec.splimp = MIPS_SPL0;
	splvec.splclock = MIPS_SPL_0_1;
	splvec.splstatclock = MIPS_SPL_0_1;

	/* no high resolution timer circuit; possibly never called */
	clkread = nullclkread;

#ifdef NOT_YET
	mcclock_addr = (volatile struct chiptime *)
		MIPS_PHYS_TO_KSEG1(Vr_SYS_CLOCK);
	mc_cpuspeed(mcclock_addr, MIPS_INT_MASK_1);
#else
	printf("%s(%d): cpuspeed estimation is notimplemented\n",
	       __FILE__, __LINE__);
#endif
#ifdef HPCMIPS_L1CACHE_DISABLE
	cpuspeed = 1;	/* XXX, CPU is very very slow because L1 cache is */
	/* disabled. */
#endif /*  HPCMIPS_L1CAHCE_DISABLE */
}


/*
 * Initalize the memory system and I/O buses.
 */
void
vr_bus_reset()
{
	printf("%s(%d): vr_bus_reset() not implemented.\n",
	       __FILE__, __LINE__);
}

void
vr_cons_init()
{
#if NCOM > 0 || NFB > 0 || NVRKIU > 0
	extern bus_space_tag_t system_bus_iot;
	extern bus_space_tag_t mb_bus_space_init __P((void));
#endif

#if NCOM > 0
	if (bootinfo->bi_cnuse & BI_CNUSE_SERIAL) {
		/* Serial console */
		mb_bus_space_init(); /* At this time, not initialized yet */
		if(com_vrip_cnattach(system_bus_iot, 0x0c000000, CONSPEED,
				     VRCOM_FREQ,
				     (TTYDEF_CFLAG & ~(CSIZE | PARENB)) | CS8)) {
			printf("%s(%d): can't init serial console", __FILE__, __LINE__);
		} else {
			return;
		}
	}
#endif

#if NFB > 0
	mb_bus_space_init(); /* At this time, not initialized yet */
	if(fb_cnattach(system_bus_iot, 0x0c000000, 0, 0)) {
		printf("%s(%d): can't init fb console", __FILE__, __LINE__);
	} else {
		goto find_keyboard;
	}
#endif

 find_keyboard:
#if NVRKIU > 0
	if (vrkiu_cnattach(system_bus_iot, VRIP_KIU_ADDR)) {
		printf("%s(%d): can't init vrkiu as console",
		       __FILE__, __LINE__);
	} else {
		return;
	}
#endif
}

void
vr_device_register(dev, aux)
	struct device *dev;
	void *aux;
{
	printf("%s(%d): vr_device_register() not implemented.\n",
	       __FILE__, __LINE__);
	panic("abort");
}

void
vr_reboot(howto, bootstr)
	int howto;
	char *bootstr;
{
	/*
	 * power down
	 */
	if ((howto & RB_POWERDOWN) == RB_POWERDOWN) {
		printf("fake powerdown\n");
		__asm(__CONCAT(".word	",___STRING(VR_OPCODE_HIBERNATE)));
		__asm("nop");
		__asm("nop");
		__asm("nop");
		__asm("nop");
		__asm("nop");
		__asm(".set reorder");
		/* not reach */
		vr_reboot(howto&~RB_HALT, bootstr);
	}
	/*
	 * halt
	 */
	if (howto & RB_HALT) {
#if NVRIP > 0
		_spllower(~MIPS_INT_MASK_0);
		vrip_intr_suspend();
#else
		splhigh();
#endif
		__asm(".set noreorder");
		__asm(__CONCAT(".word	",___STRING(VR_OPCODE_SUSPEND)));
		__asm("nop");
		__asm("nop");
		__asm("nop");
		__asm("nop");
		__asm("nop");
		__asm(".set reorder");
#if NVRIP > 0
		vrip_intr_resume();
#endif
	}
	/*
	 * reset
	 */
#if NVRDSU
	vrdsu_reset();
#else
	printf("%s(%d): There is no DSU.", __FILE__, __LINE__);
#endif
}

void *
vr_intr_establish(line, ih_fun, ih_arg)
	int line;
	int (*ih_fun) __P((void*, u_int32_t, u_int32_t));
	void *ih_arg;
{
	if (intr_handler[line] != null_handler) {
		panic("vr_intr_establish: can't establish duplicated intr handler.");
	}
	intr_handler[line] = ih_fun;
	intr_arg[line] = ih_arg;

	return (void*)line;
}


void
vr_intr_disestablish(ih)
	void *ih;
{
	int line = (int)ih;
	intr_handler[line] = null_handler;
	intr_arg[line] = NULL;
}

int
null_handler(arg, pc, statusReg)
	void *arg;
	u_int32_t pc;
	u_int32_t statusReg;
{
	printf("null_handler\n");
	return 0;
}

/*
 * Handle interrupts.
 */
int
vr_intr(mask, pc, status, cause)
	u_int32_t mask;
	u_int32_t pc;
	u_int32_t status;
	u_int32_t cause;
{
	int hwintr;

	hwintr = (ffs(mask >> 10) -1) & 0x3;
	(*intr_handler[hwintr])(intr_arg[hwintr], pc, status);
	return (MIPS_SR_INT_IE | (status & ~cause & MIPS_HARD_INT_MASK));
}
