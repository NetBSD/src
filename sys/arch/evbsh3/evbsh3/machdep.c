/*	$NetBSD: machdep.c,v 1.75 2014/06/19 13:20:13 msaitoh Exp $	*/

/*-
 * Copyright (c) 1996, 1997, 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Charles M. Hannum and by Jason R. Thorpe of the Numerical Aerospace
 * Simulation Facility, NASA Ames Research Center.
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
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*-
 * Copyright (c) 1982, 1987, 1990 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * William Jolitz.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
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
 *	@(#)machdep.c	7.4 (Berkeley) 6/3/91
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: machdep.c,v 1.75 2014/06/19 13:20:13 msaitoh Exp $");

#include "opt_ddb.h"
#include "opt_kgdb.h"
#include "opt_memsize.h"
#include "opt_initbsc.h"
#include "opt_kloader.h"
#include "opt_kloader_kernel_path.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/mount.h>
#include <sys/reboot.h>
#include <sys/sysctl.h>
#include <sys/ksyms.h>
#include <sys/device.h>
#include <sys/bus.h>

#include <uvm/uvm_extern.h>

#include <dev/cons.h>

#include <sh3/bscreg.h>
#include <sh3/cpgreg.h>
#include <sh3/cache_sh3.h>
#include <sh3/cache_sh4.h>
#include <sh3/exception.h>

#include <machine/intr.h>
#include <machine/pcb.h>

#ifdef DDB
#include <machine/db_machdep.h>
#include <ddb/db_extern.h>
#endif

#ifdef KLOADER
#include <machine/kloader.h>
#endif

#include "ksyms.h"

/* the following is used externally (sysctl_hw) */
char machine[] = MACHINE;		/* evbsh3 */
char machine_arch[] = MACHINE_ARCH;	/* sh3eb or sh3el */

#ifdef KLOADER
struct kloader_bootinfo kbootinfo;
#endif

void initSH3(void *);
void LoadAndReset(const char *);
void XLoadAndReset(char *);

/*
 * Machine-dependent startup code
 *
 * This is called from main() in kern/main.c.
 */
void
cpu_startup(void)
{

	sh_startup();
}

/*
 * machine dependent system variables.
 */
static int
sysctl_machdep_loadandreset(SYSCTLFN_ARGS)
{
	const char *osimage;
	int error;

	error = sysctl_lookup(SYSCTLFN_CALL(__UNCONST(rnode)));
	if (error || newp == NULL)
		return (error);

	osimage = (const char *)(*(const u_long *)newp);
	LoadAndReset(osimage);
	/* not reach here */
	return (0);
}

SYSCTL_SETUP(sysctl_machdep_setup, "sysctl machdep subtree setup")
{

	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_NODE, "machdep", NULL,
		       NULL, 0, NULL, 0,
		       CTL_MACHDEP, CTL_EOL);

	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_STRUCT, "console_device", NULL,
		       sysctl_consdev, 0, NULL, sizeof(dev_t),
		       CTL_MACHDEP, CPU_CONSDEV, CTL_EOL);
/*
<atatat> okay...your turn to play.  
<atatat> pick a number.
<kjk> 98752.
*/
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_IMMEDIATE,
		       CTLTYPE_INT, "load_and_reset", NULL,
		       sysctl_machdep_loadandreset, 98752, NULL, 0,
		       CTL_MACHDEP, CPU_LOADANDRESET, CTL_EOL);
}

void
cpu_reboot(int howto, char *bootstr)
{
	static int waittime = -1;

	if (cold) {
		howto |= RB_HALT;
		goto haltsys;
	}

#ifdef KLOADER
	if ((howto & RB_HALT) == 0) {
		if ((howto & RB_STRING) && (bootstr != NULL)) {
			kloader_reboot_setup(bootstr);
		}
#ifdef KLOADER_KERNEL_PATH
		else {
			kloader_reboot_setup(KLOADER_KERNEL_PATH);
		}
#endif
	}
#endif

	boothowto = howto;
	if ((howto & RB_NOSYNC) == 0 && waittime < 0) {
		waittime = 0;
		vfs_shutdown();
		/*
		 * If we've been adjusting the clock, the todr
		 * will be out of synch; adjust it now.
		 */
		/* resettodr(); */
	}

	/* Disable interrupts. */
	splhigh();

	/* Do a dump if requested. */
	if ((howto & (RB_DUMP | RB_HALT)) == RB_DUMP)
		dumpsys();

haltsys:
	doshutdownhooks();

	pmf_system_shutdown(boothowto);

	if (howto & RB_HALT) {
		printf("\n");
		printf("The operating system has halted.\n");
		printf("Please press any key to reboot.\n\n");
		cngetc();
	}
#ifdef KLOADER
	else {
		delay(1 * 1000 * 1000);
		kloader_reboot();
		printf("\n");
		printf("Failed to load a new kernel.\n");
		printf("Please press any key to reboot.\n\n");
		cngetc();
	}
#endif

	printf("rebooting...\n");
	cpu_reset();
	for(;;)
		continue;
	/*NOTREACHED*/
}

void
initSH3(void *pc)	/* XXX return address */
{
	extern char edata[], end[];
	vaddr_t kernend;

	/* Clear bss */
	memset(edata, 0, end - edata);

	/* Initilize CPU ops. */
#if defined(SH3) && defined(SH4)
#error "don't define both SH3 and SH4"
#elif defined(SH3)
#if defined(SH7708)
	sh_cpu_init(CPU_ARCH_SH3, CPU_PRODUCT_7708);
#elif defined(SH7708S)
	sh_cpu_init(CPU_ARCH_SH3, CPU_PRODUCT_7708S);
#elif defined(SH7708R)
	sh_cpu_init(CPU_ARCH_SH3, CPU_PRODUCT_7708R);
#elif defined(SH7709)
	sh_cpu_init(CPU_ARCH_SH3, CPU_PRODUCT_7709);
#elif defined(SH7709A)
	sh_cpu_init(CPU_ARCH_SH3, CPU_PRODUCT_7709A);
#elif defined(SH7706)
	sh_cpu_init(CPU_ARCH_SH3, CPU_PRODUCT_7706);
#else
#error "unsupported SH3 variants"
#endif
#elif defined(SH4)
#if defined(SH7750)
	sh_cpu_init(CPU_ARCH_SH4, CPU_PRODUCT_7750);	
#elif defined(SH7750S)
	sh_cpu_init(CPU_ARCH_SH4, CPU_PRODUCT_7750S);
#elif defined(SH7750R)
	sh_cpu_init(CPU_ARCH_SH4, CPU_PRODUCT_7750R);
#elif defined(SH7751)
	sh_cpu_init(CPU_ARCH_SH4, CPU_PRODUCT_7751);
#elif defined(SH7751R)
	sh_cpu_init(CPU_ARCH_SH4, CPU_PRODUCT_7751R);
#else
#error "unsupported SH4 variants"
#endif
#else
#error "define SH3 or SH4"
#endif
	/* Console */
	consinit();

#ifdef KLOADER
	/* copy boot parameter for kloader */
	kloader_bootinfo_set(&kbootinfo, 0, NULL, NULL, true);
#endif

	/* Load memory to UVM */
	kernend = atop(round_page(SH3_P1SEG_TO_PHYS(end)));
	physmem = atop(IOM_RAM_SIZE);
	uvm_page_physload(
		kernend, atop(IOM_RAM_BEGIN + IOM_RAM_SIZE),
		kernend, atop(IOM_RAM_BEGIN + IOM_RAM_SIZE),
		VM_FREELIST_DEFAULT);

	/* Initialize proc0 u-area */
	sh_proc0_init();

	/* Initialize pmap and start to address translation */
	pmap_bootstrap();

	/*
	 * XXX We can't return here, because we change stack pointer.
	 *     So jump to return address directly.
	 */
	__asm volatile (
		"jmp	@%0;"
		"mov	%1, r15"
		:: "r"(pc),"r"(lwp0.l_md.md_pcb->pcb_sf.sf_r7_bank));
}

/*
 * consinit:
 * initialize the system console.
 * XXX - shouldn't deal with this initted thing, but then,
 * it shouldn't be called from init386 either.
 */
void
consinit(void)
{
	static int initted;

	if (initted)
		return;
	initted = 1;

	cninit();
}

#if !defined(DONT_INIT_BSC)
/*
 * InitializeBsc
 * : BSC(Bus State Controller)
 */
void InitializeBsc(void);

void
InitializeBsc(void)
{

	/*
	 * Drive RAS,CAS in stand by mode and bus release mode
	 * Area0 = Normal memory, Area5,6=Normal(no burst)
	 * Area2 = Normal memory, Area3 = SDRAM, Area5 = Normal memory
	 * Area4 = Normal Memory
	 * Area6 = Normal memory
	 */
#if defined(SH3)
	_reg_write_2(SH3_BCR1, BSC_BCR1_VAL);
#elif defined(SH4)
	_reg_write_4(SH4_BCR1, BSC_BCR1_VAL);
#endif

	/*
	 * Bus Width
	 * Area4: Bus width = 16bit
	 * Area6,5 = 16bit
	 * Area1 = 8bit
	 * Area2,3: Bus width = 32bit
	 */
	_reg_write_2(SH_(BCR2), BSC_BCR2_VAL);

	/*
	 * Idle cycle number in transition area and read to write
	 * Area6 = 3, Area5 = 3, Area4 = 3, Area3 = 3, Area2 = 3
	 * Area1 = 3, Area0 = 3
	 */
#if defined(SH3)
	_reg_write_2(SH3_WCR1, BSC_WCR1_VAL);
#elif defined(SH4)
	_reg_write_4(SH4_WCR1, BSC_WCR1_VAL);
#endif

	/*
	 * Wait cycle
	 * Area 6 = 6
	 * Area 5 = 2
	 * Area 4 = 10
	 * Area 3 = 3
	 * Area 2,1 = 3
	 * Area 0 = 6
	 */
#if defined(SH3)
	_reg_write_2(SH3_WCR2, BSC_WCR2_VAL);
#elif defined(SH4)
	_reg_write_4(SH4_WCR2, BSC_WCR2_VAL);
#endif

#if defined(SH4) && defined(BSC_WCR3_VAL)
	_reg_write_4(SH4_WCR3, BSC_WCR3_VAL);
#endif

	/*
	 * RAS pre-charge = 2cycle, RAS-CAS delay = 3 cycle,
	 * write pre-charge=1cycle
	 * CAS before RAS refresh RAS assert time = 3 cycle
	 * Disable burst, Bus size=32bit, Column Address=10bit, Refresh ON
	 * CAS before RAS refresh ON, EDO DRAM
	 */
#if defined(SH3)
	_reg_write_2(SH3_MCR, BSC_MCR_VAL);
#elif defined(SH4)
	_reg_write_4(SH4_MCR, BSC_MCR_VAL);
#endif

#if defined(BSC_SDMR2_VAL)
	_reg_write_1(BSC_SDMR2_VAL, 0);
#endif

#if defined(BSC_SDMR3_VAL)
#if !(defined(COMPUTEXEVB) && defined(SH7709A))
	_reg_write_1(BSC_SDMR3_VAL, 0);
#else
	_reg_write_2(0x1a000000, 0);	/* ADDSET */
	_reg_write_1(BSC_SDMR3_VAL, 0);
	_reg_write_2(0x18000000, 0);	/* ADDRST */
#endif /* !(COMPUTEXEVB && SH7709A) */
#endif /* BSC_SDMR3_VAL */

	/*
	 * PCMCIA Control Register
	 * OE/WE assert delay 3.5 cycle
	 * OE/WE negate-address delay 3.5 cycle
	 */
#ifdef BSC_PCR_VAL
	_reg_write_2(SH_(PCR), BSC_PCR_VAL);
#endif

	/*
	 * Refresh Timer Control/Status Register
	 * Disable interrupt by CMF, closk 1/16, Disable OVF interrupt
	 * Count Limit = 1024
	 * In following statement, the reason why high byte = 0xa5(a4 in RFCR)
	 * is the rule of SH3 in writing these register.
	 */
	_reg_write_2(SH_(RTCSR), BSC_RTCSR_VAL);

	/*
	 * Refresh Timer Counter
	 * Initialize to 0
	 */
#ifdef BSC_RTCNT_VAL
	_reg_write_2(SH_(RTCNT), BSC_RTCNT_VAL);
#endif

	/* set Refresh Time Constant Register */
	_reg_write_2(SH_(RTCOR), BSC_RTCOR_VAL);

	/* init Refresh Count Register */
#ifdef BSC_RFCR_VAL
	_reg_write_2(SH_(RFCR), BSC_RFCR_VAL);
#endif

	/*
	 * Clock Pulse Generator
	 */
	/* Set Clock mode (make internal clock double speed) */
	_reg_write_2(SH_(FRQCR), FRQCR_VAL);

	/*
	 * Cache
	 */
#ifndef CACHE_DISABLE
	/* Cache ON */
	_reg_write_4(SH_(CCR), 0x1);
#endif
}
#endif /* !DONT_INIT_BSC */


 /* XXX This value depends on physical available memory */
#define OSIMAGE_BUF_ADDR	(IOM_RAM_BEGIN + 0x00400000)

void
LoadAndReset(const char *osimage)
{
	void *buf_addr;
	u_long size;
	const u_long *src;
	u_long *dest;
	u_long csum = 0;
	u_long csum2 = 0;
	u_long size2;

	printf("LoadAndReset: copy start\n");
	buf_addr = (void *)OSIMAGE_BUF_ADDR;

	size = *(const u_long *)osimage;
	src = (const u_long *)osimage;
	dest = buf_addr;

	size = (size + sizeof(u_long) * 2 + 3) >> 2;
	size2 = size;

	while (size--) {
		csum += *src;
		*dest++ = *src++;
	}

	dest = buf_addr;
	while (size2--)
		csum2 += *dest++;

	printf("LoadAndReset: copy end[%lx,%lx]\n", csum, csum2);
	printf("start XLoadAndReset\n");

	/* mask all externel interrupt (XXX) */

	XLoadAndReset(buf_addr);
}

void
intc_intr(int ssr, int spc, int ssp)
{
	struct intc_intrhand *ih;
	struct clockframe cf;
	int evtcode;

	curcpu()->ci_data.cpu_nintr++;

	switch (cpu_product) {
	case CPU_PRODUCT_7708:
	case CPU_PRODUCT_7708S:
	case CPU_PRODUCT_7708R:
		evtcode = _reg_read_4(SH3_INTEVT);
		break;
	case CPU_PRODUCT_7709:
	case CPU_PRODUCT_7709A:
	case CPU_PRODUCT_7706:
		evtcode = _reg_read_4(SH7709_INTEVT2);
		break;
	case CPU_PRODUCT_7750:
	case CPU_PRODUCT_7750S:
	case CPU_PRODUCT_7750R:
	case CPU_PRODUCT_7751:
	case CPU_PRODUCT_7751R:
		evtcode = _reg_read_4(SH4_INTEVT);
		break;
	default:
#ifdef DIAGNOSTIC
		panic("intr_intc: cpu_product %d unhandled!", cpu_product);
#endif
		return;
	}

	ih = EVTCODE_IH(evtcode);
	KDASSERT(ih->ih_func);
	/* 
	 * On entry, all interrrupts are disabled,
	 * and exception is enabled for P3 access. (kernel stack is P3,
	 * SH3 may or may not cause TLB miss when access stack.)
	 * Enable higher level interrupt here.
	 */
	_cpu_intr_resume(ih->ih_level);

	switch (evtcode) {
	default:
		(*ih->ih_func)(ih->ih_arg);
		break;
	case SH_INTEVT_TMU0_TUNI0:
		cf.spc = spc;
		cf.ssr = ssr;
		cf.ssp = ssp;
		(*ih->ih_func)(&cf);
		break;
	case SH_INTEVT_NMI:
		printf("NMI ignored.\n");
		break;
	}
}
