/*	$NetBSD: machdep.c,v 1.40.2.1 2010/10/22 07:21:13 uebayasi Exp $	*/

/*-
 * Copyright (c) 1996, 1997, 1998, 2002 The NetBSD Foundation, Inc.
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
__KERNEL_RCSID(0, "$NetBSD: machdep.c,v 1.40.2.1 2010/10/22 07:21:13 uebayasi Exp $");

#include "opt_ddb.h"
#include "opt_kgdb.h"
#include "opt_memsize.h"
#include "scif.h"
#include "opt_kloader.h"
#include "opt_modular.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/mount.h>
#include <sys/reboot.h>
#include <sys/sysctl.h>
#include <sys/ksyms.h>
#include <sys/device.h>
#include <sys/module.h>

#ifdef KGDB
#include <sys/kgdb.h>
#include <sh3/dev/scifvar.h>
#endif
#ifdef DDB
#include <machine/db_machdep.h>
#include <ddb/db_extern.h>
#endif

#include <sh3/cpu.h>
#include <sh3/exception.h>
#include <sh3/bscreg.h>
#include <machine/intr.h>
#include <machine/kloader.h>

#include <dev/cons.h>

#include "ksyms.h"

/* the following is used externally (sysctl_hw) */
char machine[] = MACHINE;		/* dreamcast */
char machine_arch[] = MACHINE_ARCH;	/* sh3el */

void main(void) __attribute__((__noreturn__));
void dreamcast_startup(void) __attribute__((__noreturn__));

void
dreamcast_startup(void)
{
	extern char edata[], end[];
	paddr_t kernend;

	/* Clear bss */
	memset(edata, 0, end - edata);

	/* Initialize CPU ops. */
	sh_cpu_init(CPU_ARCH_SH4, CPU_PRODUCT_7750);

	/* Console */
	consinit();

	/* Load memory to UVM */
	physmem = atop(IOM_RAM_SIZE);
	kernend = atop(round_page(SH3_P1SEG_TO_PHYS(end)));
	uvm_page_physload(
		kernend, atop(IOM_RAM_BEGIN + IOM_RAM_SIZE),
		kernend, atop(IOM_RAM_BEGIN + IOM_RAM_SIZE),
		VM_FREELIST_DEFAULT);

	/* Initialize proc0 u-area */
	sh_proc0_init();

	/* Initialize pmap and start to address translation */
	pmap_bootstrap();

	/* Debugger. */
#if defined(KGDB) && (NSCIF > 0)
	if (scif_kgdb_init() == 0) {
		kgdb_debug_init = 1;
		kgdb_connect(1);
	}
#endif /* KGDB && NSCIF > 0 */

	/* Jump to main */
	__asm volatile(
		"jmp	@%0;"
		"mov	%1, sp"
		:: "r"(main),"r"(lwp0.l_md.md_pcb->pcb_sf.sf_r7_bank));
	/* NOTREACHED */
	while (1)
		;
}

void
consinit(void)
{
	static int initted;

	if (initted)
		return;
	initted = 1;

	cninit();
}

void
cpu_startup(void)
{

	strcpy(cpu_model, "SEGA Dreamcast\n");

	sh_startup();
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
}

void
cpu_reboot(int howto, char *bootstr)
{
#ifdef KLOADER
	struct kloader_bootinfo kbi;
#endif
	static int waittime = -1;

	if (cold) {
		howto |= RB_HALT;
		goto haltsys;
	}

#ifdef KLOADER
	/* No bootinfo is required. */
	kloader_bootinfo_set(&kbi, 0, NULL, NULL, true);
	if ((howto & RB_HALT) == 0) {
		if ((howto & RB_STRING) && bootstr != NULL) {
			printf("loading a new kernel: %s\n", bootstr);
			kloader_reboot_setup(bootstr);
		}
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
#if 0
		resettodr();
#endif
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
	else if ((howto & RB_STRING) && bootstr != NULL) {
		kloader_reboot();
		printf("\nFailed to load a new kernel.\n");
		cngetc();
	}
#endif

	printf("rebooting...\n");
	cpu_reset();
	for(;;)
		;
	/*NOTREACHED*/
}

void
intc_intr(int ssr, int spc, int ssp)
{
	struct intc_intrhand *ih;
	int s, evtcode;

	evtcode = _reg_read_4(SH4_INTEVT);

	ih = EVTCODE_IH(evtcode);
	KDASSERT(ih->ih_func);
	/*
	 * On entry, all interrrupts are disabled, and exception is enabled.
	 * Enable higher level interrupt here.
	 */
	s = _cpu_intr_resume(ih->ih_level);

	if (evtcode == SH_INTEVT_TMU0_TUNI0) {	/* hardclock */
		struct clockframe cf;
		cf.spc = spc;
		cf.ssr = ssr;
		cf.ssp = ssp;
		(*ih->ih_func)(&cf);
	} else {
		(*ih->ih_func)(ih->ih_arg);
	}
}

#ifdef MODULAR
/*
 * Push any modules loaded by the bootloader etc.
 */
void
module_init_md(void)
{
}
#endif
