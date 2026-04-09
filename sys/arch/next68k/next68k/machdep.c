/*	$NetBSD: machdep.c,v 1.142 2026/04/09 14:36:55 thorpej Exp $	*/

/*
 * Copyright (c) 1998 Darrin B. Jewell
 * Copyright (c) 1988 University of Utah.
 * Copyright (c) 1982, 1986, 1990, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * the Systems Programming Group of the University of Utah Computer
 * Science Department.
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
 * from: Utah $Hdr: machdep.c 1.74 92/12/20$
 *
 *	@(#)machdep.c	8.10 (Berkeley) 4/20/94
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: machdep.c,v 1.142 2026/04/09 14:36:55 thorpej Exp $");

#include "opt_ddb.h"
#include "opt_kgdb.h"
#include "opt_modular.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/signalvar.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/buf.h>
#include <sys/reboot.h>
#include <sys/conf.h>
#include <sys/file.h>
#include <sys/device.h>
#include <sys/mbuf.h>
#include <sys/msgbuf.h>
#include <sys/ioctl.h>
#include <sys/tty.h>
#include <sys/mount.h>
#include <sys/exec.h>
#include <sys/core.h>
#include <sys/kcore.h>
#include <sys/vnode.h>
#include <sys/syscallargs.h>
#include <sys/ksyms.h>
#include <sys/module.h>
#ifdef KGDB
#include <sys/kgdb.h>
#endif
#include <sys/boot_flag.h>
#include <sys/cpu.h>

#include <uvm/uvm_extern.h>

#ifdef DDB
#include <machine/db_machdep.h>
#include <ddb/db_access.h>
#include <ddb/db_sym.h>
#include <ddb/db_extern.h>
#endif

#ifdef KGDB
/* Is zs configured in? */
#include "zsc.h"
#if (NZSC > 0)
#include <next68k/dev/zs_cons.h>
#endif
#endif

#include <sys/sysctl.h>

#include <machine/cpu.h>
#include <machine/bus.h>
#include <machine/reg.h>
#include <machine/pcb.h>
#include <machine/psl.h>
#include <machine/pte.h>
#include <machine/vmparam.h>
#include <dev/cons.h>

#include <machine/kcore.h>	/* XXX should be pulled in by sys/kcore.h */

#include <next68k/dev/intiovar.h>
#include <next68k/next68k/isr.h>
#include <next68k/next68k/nextrom.h>
#include <next68k/next68k/rtc.h>
#include <m68k/seglist.h>

#include <dev/mm.h>

#include "ksyms.h"

int nsym;
char *ssym;
extern char *esym;

#define	MAXMEM	64*1024	/* XXX - from cmap.h */

extern	u_int lowram;

/* prototypes for local functions */
void	identifycpu(void);

/* functions called from locore.s */
void machine_init(paddr_t);

/*
 * Default to 33MHz 68040.
 */
int	cpuspeed = 33;
int	delay_divisor = delay_divisor_est40(33);

/****************************************************************/

#ifdef __HAVE_NEW_PMAP_68K
const struct pmap_bootmap machine_bootmap[] = {
	{ .pmbm_vaddr_ptr = &intiobase,
	  .pmbm_paddr     = INTIOBASE,
	  .pmbm_size      = INTIOSIZE,
	  .pmbm_flags     = PMBM_F_CI },

	{ .pmbm_vaddr = -1 },
};
#endif

/*
 * Early initialization, before main() is called.
 */
void
machine_init(paddr_t nextpa)
{
	machine_init_common(nextpa);

	{
		char *p = rom_boot_arg;
		boothowto = 0;
		if (*p++ == '-') {
			for (;*p;p++)
				BOOT_FLAG(*p, boothowto);
		}
	}
}

/*
 * Console initialization: called early on from main,
 * before vm init or startup.  Do enough configuration
 * to choose and initialize a console.
 */
void
consinit(void)
{
	static int init = 0;

	/*
	 * Generic console: sys/dev/cons.c
	 *	Initializes either ite or ser as console.
	 *	Can be called from locore.s and init_main.c.
	 */

	if (!init) {
		cninit();
#if defined(KGDB) && (NZSC > 0)
		zs_kgdb_init();
#endif
#if NKSYMS || defined(DDB) || defined(MODULAR)
		/* Initialize kernel symbol table, if compiled in. */
		ksyms_addsyms_elf(nsym, ssym, esym);
#endif
		if (boothowto & RB_KDB) {
#if defined(KGDB)
			kgdb_connect(1);
#elif defined(DDB)
			Debugger();
#endif
		}

		init = 1;
	}
}

/*
 * cpu_startup: allocate memory for variable-sized tables,
 * initialize CPU, and do autoconfiguration.
 */
void
cpu_startup(void)
{
	vaddr_t minaddr, maxaddr;
	char pbuf[9];
#ifdef DEBUG
	extern int pmapdebug;
	int opmapdebug = pmapdebug;

	pmapdebug = 0;
#endif

	/* Initialize the FPU, if present. */
	fpu_init();

	/*
	 * Good {morning,afternoon,evening,night}.
	 */
	printf("%s%s", copyright, version);
	identifycpu();
	format_bytes(pbuf, sizeof(pbuf), ctob(physmem));
	printf("total memory = %s\n", pbuf);

	minaddr = 0;
	/*
	 * Allocate a submap for physio
	 */
	phys_map = uvm_km_suballoc(kernel_map, &minaddr, &maxaddr,
				 VM_PHYS_SIZE, 0, false, NULL);

#ifdef DEBUG
	pmapdebug = opmapdebug;
#endif
	format_bytes(pbuf, sizeof(pbuf), ptoa(uvm_availmem(false)));
	printf("avail memory = %s\n", pbuf);
}

void
identifycpu(void)
{
	const char *cpu_str, *mmu_str, *fpu_str, *cache_str;
	extern int turbo;

	/*
	 * ...and the CPU type.
	 */
	switch (cputype) {
	case CPU_68040:
		cpu_str = "MC68040";
		cpuspeed = turbo ? 33 : 25;
		delay_divisor = delay_divisor_est40(cpuspeed);
		break;
	case CPU_68030:
		cpu_str = "MC68030";
		cpuspeed = 25;
		delay_divisor = delay_divisor_est(cpuspeed);
		break;
#if 0
	case CPU_68020:
		cpu_str = "MC68020";
		break;
#endif
	default:
		printf("\nunknown cputype %d\n", cputype);
		panic("startup");
	}

	/*
	 * ...and the MMU type.
	 */
	switch (mmutype) {
	case MMU_68040:
	case MMU_68030:
		mmu_str = "+MMU";
		break;
#if 0
	case MMU_68851:
		mmu_str = ", MC68851 MMU";
		break;
#endif
	default:
		printf("%s: unknown MMU type %d\n", cpu_str, mmutype);
		panic("startup");
	}

	/*
	 * ...and the FPU type.
	 */
	switch (fputype) {
	case FPU_68040:
		fpu_str = "+FPU";
		break;
	case FPU_68882:
		fpu_str = ", MC68882 FPU";
		break;
	case FPU_68881:
		fpu_str = ", MC68881 FPU";
		break;
	default:
		fpu_str = ", unknown FPU";
	}

	/*
	 * ...and finally, the cache type.
	 */
	if (cputype == CPU_68040)
		cache_str = ", 4k on-chip physical I/D caches";
	else
		cache_str = "";

	cpu_setmodel("NeXT/%s CPU%s%s%s", cpu_str, mmu_str, fpu_str, cache_str);
	printf("%s\n", cpu_getmodel());
}

/*
 * machine dependent system variables.
 */
SYSCTL_SETUP(sysctl_machdep_setup, "sysctl machdep subtree setup")
{

	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_NODE, "machdep", NULL,
		       NULL, 0, NULL, 0,
		       CTL_MACHDEP, CTL_EOL);

#if 0
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_STRUCT, "console_device", NULL,
		       sysctl_consdev, 0, NULL, sizeof(dev_t),
		       CTL_MACHDEP, CPU_CONSDEV, CTL_EOL);
#endif
}

/* See: sig_machdep.c */

int	waittime = -1;

void
cpu_reboot(int howto, char *bootstr)
{
	struct pcb *pcb = lwp_getpcb(curlwp);

	/* take a snap shot before clobbering any registers */
	if (pcb != NULL)
		savectx(pcb);

	/* If system is cold, just halt. */
	if (cold) {
		howto |= RB_HALT;
		goto haltsys;
	}

	boothowto = howto;
	if ((howto & RB_NOSYNC) == 0 && waittime < 0) {
		waittime = 0;
		vfs_shutdown();
	}

	/* Disable interrupts. */
	splhigh();

	/* If rebooting and a dump is requested, do it. */
	if (howto & RB_DUMP)
		dumpsys();

 haltsys:
	/* Run any shutdown hooks. */
	doshutdownhooks();

	pmf_system_shutdown(boothowto);

#if defined(PANICWAIT) && !defined(DDB)
	if ((howto & RB_HALT) == 0 && panicstr) {
		printf("hit any key to reboot...\n");
		cnpollc(true);
		(void)cngetc();
		cnpollc(false);
		printf("\n");
	}
#endif

	if ((howto & RB_POWERDOWN) == RB_POWERDOWN) {
		poweroff();
	}

	/* Finally, halt/reboot the system. */
	if (howto & RB_HALT) {
		monbootflag = 0x2d680000;				/* "-h" */
	}

	printf("rebooting...\n");
	DELAY(1000000);
	doboot();
	/*NOTREACHED*/
}

/* XXX should change the interface, and make one badaddr() function */

int	*nofault;

#if 0
int
badaddr(void *addr, int nbytes)
{
	int i;
	label_t faultbuf;

#ifdef lint
	i = *addr; if (i) return (0);
#endif

	nofault = (int *) &faultbuf;
	if (setjmp((label_t *)nofault)) {
		nofault = (int *) 0;
		return(1);
	}

	switch (nbytes) {
	case 1:
		i = *(volatile char *)addr;
		break;

	case 2:
		i = *(volatile short *)addr;
		break;

	case 4:
		i = *(volatile int *)addr;
		break;

	default:
		panic("badaddr: bad request");
	}
	nofault = NULL;
	return (0);
}
#endif

/*
 * Level 7 interrupts can be caused by the keyboard or parity errors.
 */
int
nmihand(void *frame)
{
	static int innmihand;	/* simple mutex */

	/* Prevent unwanted recursion. */
	if (innmihand)
		return 0;
	innmihand = 1;

	printf("Got a NMI");

	if (!INTR_OCCURRED(NEXT_I_NMI)) {
		printf("But NMI isn't set in intrstat!\n");
	}
	INTR_DISABLE(NEXT_I_NMI);

#if defined(DDB)
	printf(": entering debugger\n");
	Debugger();
	printf("continuing after NMI\n");
#elif defined(KGDB)
	kgdb_connect(1);
#else
	printf(": ignoring\n");
#endif /* DDB */

	INTR_ENABLE(NEXT_I_NMI);

	innmihand = 0;

	return 0;
}
