/*	$NetBSD: machdep.c,v 1.146 2026/04/26 10:52:15 thorpej Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: machdep.c,v 1.146 2026/04/26 10:52:15 thorpej Exp $");

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

/* functions called from locore.s */
void machine_init(paddr_t);

/*
 * Default to 33MHz 68040.
 */
int	delay_divisor = delay_divisor_est40(33);

/****************************************************************/

struct pmap_bootmap machine_bootmap[] = {
[PMBM_I_INTIO]	=	{ .pmbm_vaddr_ptr = &intiobase,
			  .pmbm_paddr     = INTIOBASE,
			  .pmbm_size      = INTIOSIZE,
			  .pmbm_flags     = PMBM_F_CI },

[PMBM_I_FB]	=	{ .pmbm_vaddr_ptr = &fbbase,
			  .pmbm_paddr     = 0,	/* filled in by... */
			  .pmbm_size      = 0,	/* ...next68k_bootargs() */
			  .pmbm_flags     = PMBM_F_CWT },

			{ .pmbm_vaddr = -1 },
};

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

void
machine_set_model(void)
{
	extern int turbo;
	const char *shape_str = "cube";
	const char *color_str = iscolor ? " Color" : "";
	const char *turbo_str = turbo ? " Turbo" : "";

	switch (rom_machine_type) {
	case NeXT_WARP9:	/* 68040 25MHz NeXTstation */
	case NeXT_WARP9C:	/* 68040 25MHz NeXTstation Color */
	case NeXT_TURBO_MONO:	/* 68040 33MHz NeXTstation */
	case NeXT_TURBO_COLOR:	/* 68040 33MHz NeXTstation Color */
		shape_str = "station";
		/* FALLTHROUGH */
	case NeXT_CUBE:		/* 68030 25MHz NeXTcube */
	case NeXT_X15:		/* 68040 25MHz NeXTcubr */
	case NeXT_CUBE_TURBO:	/* 68040 33MHz NeXTcube */
		cpu_setmodel("NeXT%s%s%s", shape_str, color_str, turbo_str);
		break;

	default:
		cpu_setmodel("NeXT machine type %d", rom_machine_type);
		break;
	}

	switch (cputype) {
	case CPU_68040:
		cpuspeed_khz = (turbo ? 33 : 25) * 1000;
		delay_divisor = delay_divisor_est40(cpuspeed_khz / 1000);
		break;
	case CPU_68030:
		cpuspeed_khz = 25*1000;
		delay_divisor = delay_divisor_est(cpuspeed_khz / 1000);
		break;
	default:
		printf("\nunknown cputype %d\n", cputype);
		panic("startup");
	}
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
