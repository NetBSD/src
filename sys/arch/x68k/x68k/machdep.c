/*	$NetBSD: machdep.c,v 1.232 2026/04/26 12:49:39 thorpej Exp $	*/

/*
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
__KERNEL_RCSID(0, "$NetBSD: machdep.c,v 1.232 2026/04/26 12:49:39 thorpej Exp $");

#include "opt_ddb.h"
#include "opt_kgdb.h"
#include "opt_fpu_emulate.h"
#include "opt_m060sp.h"
#include "opt_modular.h"
#include "opt_panicbutton.h"
#include "opt_extmem.h"
#include "opt_m68k_arch.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/callout.h>
#include <sys/signalvar.h>
#include <sys/kauth.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/buf.h>
#include <sys/reboot.h>
#include <sys/conf.h>
#include <sys/file.h>
#include <sys/mbuf.h>
#include <sys/msgbuf.h>
#include <sys/ioctl.h>
#include <sys/tty.h>
#include <sys/mount.h>
#include <sys/exec.h>
#include <sys/vnode.h>
#include <sys/syscallargs.h>
#include <sys/core.h>
#include <sys/kcore.h>
#include <sys/ksyms.h>
#include <sys/module.h>
#include <sys/cpu.h>
#include <sys/sysctl.h>
#include <sys/device.h>

#include "ksyms.h"

#if NKSYMS || defined(DDB) || defined(MODULAR)
#include <sys/exec_elf.h>
#endif

#include <machine/db_machdep.h>
#include <ddb/db_sym.h>
#include <ddb/db_extern.h>

#include <m68k/cacheops.h>
#include <machine/reg.h>
#include <machine/pcb.h>
#include <machine/psl.h>
#include <machine/pte.h>
#include <machine/kcore.h>

#include <m68k/seglist.h>

#include <dev/cons.h>
#include <dev/mm.h>

#define	MAXMEM	64*1024	/* XXX - from cmap.h */
#include <uvm/uvm.h>
#include <uvm/uvm_physseg.h>

#include <machine/bus.h>
#include <machine/autoconf.h>
#include <arch/x68k/dev/intiovar.h>
#include <arch/x68k/x68k/iodevice.h>

extern void doboot(void) __attribute__((__noreturn__));

extern paddr_t avail_start, avail_end;
extern u_int lowram;
extern int end, *esym;

int	maxmem;			/* max memory per process */

/* prototypes for local functions */
static int check_emulator(char *, int);

/* functions called from locore.s */
void	machine_init(paddr_t);
void	nmihand(struct frame);
void	intrhand(int);

/* memory probe function called from locore.s */
void	setmemrange(void);
#ifdef EXTENDED_MEMORY
static bool mem_exists(paddr_t, paddr_t);
#endif

static int basemem;

/*
 * Assume a standard X68030 with 25MHz CPU.  The delay_divisor will
 * be calibrated later.
 */
int	delay_divisor = delay_divisor_est(25);


/*
 * machine_bootmap[] is checked in pmap_bootstrap1() of the new m68k pmap
 * and it allocates kernel address space for intio devices.
 */
static vaddr_t intiova;
#define PMBM_INTIO	0
struct pmap_bootmap machine_bootmap[] = {
	{ .pmbm_vaddr_ptr = &intiova,
	  .pmbm_paddr = INTIOBASE,
	  .pmbm_size  = INTIOSIZE,
	  .pmbm_flags = PMBM_F_CI },
	{ .pmbm_vaddr = -1 },
};

static callout_t candbtimer_ch;

void
machine_init(paddr_t nextpa)
{
	extern paddr_t msgbufpa;

#ifdef __HAVE_NEW_PMAP_68K
	/* load the internal IO space region */
	intiobase = (uint8_t *)intiova;
#endif

	/*
	 * Most m68k ports allocate msgbuf at the end of available memory
	 * (i.e. just after avail_end), but on x68k we allocate msgbuf
	 * at the end of main memory for compatibility.
	 *
	 * (This means we have to fully initialize the first phys seg
	 * entry.)
	 */
	phys_seg_list[0].ps_start = phys_seg_list[0].ps_avail_start =
	    m68k_round_page(phys_seg_list[0].ps_start);
	phys_seg_list[0].ps_end = phys_seg_list[0].ps_avail_end =
	    m68k_trunc_page(phys_seg_list[0].ps_end);

	phys_seg_list[0].ps_avail_end -= m68k_round_page(MSGBUFSIZE);
	msgbufpa = phys_seg_list[0].ps_avail_end;

	machine_init_common(nextpa);
}

/*
 * Console initialization: called early on from main,
 * before vm init or startup.  Do enough configuration
 * to choose and initialize a console.
 */
void
consinit(void)
{

	/*
	 * bring graphics layer up.
	 */
	config_console();

	/*
	 * Initialize the console before we print anything out.
	 */
	cninit();

#ifdef KGDB
	zs_kgdb_init();			/* XXX */
#endif
#if NKSYMS || defined(DDB) || defined(MODULAR)
	ksyms_addsyms_elf((int)esym - (int)&end - sizeof(Elf32_Ehdr),
	    (void *)&end, esym);
#endif
#ifdef DDB
	if (boothowto & RB_KDB)
		Debugger();
#endif
}

/*
 * cpu_startup: allocate memory for variable-sized tables,
 * initialize cpu, and do autoconfiguration.
 */
void
cpu_startup(void)
{
	cpu_startup_common();

	callout_init(&candbtimer_ch, 0);
}

void
machine_set_model(void)
{
	/* there's alot of XXX in here... */
	const char *mach;
	char emubuf[20];

	/*
	 * check machine type constant
	 */
	switch (intio_get_sysport_mpustat()) {
	case 0xdc:
		/*
		 * CPU Type == 68030, Clock == 25MHz
		 */
		mach = "030";
		break;
	case 0xfe:
		/*
		 * CPU Type == 68000, Clock == 16MHz
		 */
		mach = "000XVI";
		break;
	case 0xff:
		/*
		 * CPU Type == 68000, Clock == 10MHz
		 */
		mach = "000/ACE/PRO/EXPERT/SUPER";
		break;
	default:
		/*
		 * unknown type
		 */
		mach = "000?(unknown model)";
		break;
	}

	emubuf[0] = '\0';
	check_emulator(emubuf, sizeof(emubuf));

	switch (cputype) {
	case CPU_68060:
		/*
		 * This delay_divisor method cannot get accurate cpuspeed
		 * for 68060.
		 */
		/* XXX really? */
		break;
	case CPU_68040:
		cpuspeed_khz = delay_divisor_est40(delay_divisor) * 1000;
		break;
	case CPU_68030:
		cpuspeed_khz = delay_divisor_est(delay_divisor) * 1000;
		break;
	case CPU_68020:
		cpuspeed_khz = delay_divisor_est(delay_divisor) * 1000;
		break;
	default:
		panic("impossible cpu");
	}

	cpu_setmodel("X68%s %s%s",
	    mach, emubuf[0] ? " on " : "", emubuf);
}

/*
 * If it is an emulator, store the name in buf and return 1.
 * Otherwise return 0.
 */
static int
check_emulator(char *buf, int bufsize)
{
	int xm6major;
	int xm6minor;
	int xm6imark;
	int xm6imajor;
	int xm6iminor;

	/* XM6 and its family */
	intio_set_sysport_sramwp('X');
	if (intio_get_sysport_sramwp() == '6') {
		xm6major = intio_get_sysport_sramwp();
		xm6minor = intio_get_sysport_sramwp();
		xm6imark = intio_get_sysport_sramwp();
		switch (xm6imark) {
		case 0xff:	/* Original XM6 or unknown compatibles */
			snprintf(buf, bufsize, "XM6 v%x.%02x",
				xm6major, xm6minor);
			break;

		case 'i':	/* XM6i */
			xm6imajor = intio_get_sysport_sramwp();
			xm6iminor = intio_get_sysport_sramwp();
			snprintf(buf, bufsize, "XM6i v%d.%02d",
				xm6imajor, xm6iminor);
			break;

		case 'g':	/* XM6 TypeG */
			snprintf(buf, bufsize, "XM6 TypeG v%x.%02x",
				xm6major, xm6minor);
			break;

		default:	/* Other XM6 compatibles? */
			/* XXX what should I do? */
			return 0;
		}
		return 1;
	}

	return 0;
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

	sysctl_createv(clog, 0, NULL, NULL,
	    CTLFLAG_PERMANENT,
	    CTLTYPE_STRUCT, "console_device", NULL,
	    sysctl_consdev, 0, NULL, sizeof(dev_t),
	    CTL_MACHDEP, CPU_CONSDEV, CTL_EOL);
}

int	waittime = -1;

void
cpu_reboot(int howto, char *bootstr)
{
	struct pcb *pcb = lwp_getpcb(curlwp);

	/* take a snap shot before clobbering any registers */
	if (pcb != NULL)
		savectx(pcb);

	boothowto = howto;
	if ((howto & RB_NOSYNC) == 0 && waittime < 0) {
		waittime = 0;
		vfs_shutdown();
		/*
		 * If we've been adjusting the clock, the todr
		 * will be out of synch; adjust it now.
		 */
		/*resettodr();*/
	}

	/* Disable interrupts. */
	splhigh();

	if (howto & RB_DUMP)
		dumpsys();

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

	/* Finally, halt/reboot the system. */
	/* a) RB_POWERDOWN
	 *  a1: the power switch is still on
	 *	Power cannot be removed; simply halt the system (b)
	 *  a2: the power switch is off
	 *	Remove the power
	 * b) RB_HALT
	 *	call cngetc
	 * c) otherwise
	 *	Reboot
	 */
	if ((howto & RB_POWERDOWN) == RB_POWERDOWN) {
		printf("powering off...\n");
		delay(1000000);

		/* Turn off the alarm signal of RTC */
		IODEVbase->io_rtc.bank0.reset = 0x0c;

		intio_set_sysport_powoff(0x00);
		intio_set_sysport_powoff(0x0f);
		intio_set_sysport_powoff(0x0f);
		delay(1000000);
	}
	if ((howto & RB_HALT) != 0) {
		printf("System halted.  Hit any key to reboot.\n\n");
		cnpollc(true);
		(void)cngetc();
		cnpollc(false);
	}

	printf("rebooting...\n");
	DELAY(1000000);
	doboot();
	/* NOTREACHED */
}

void
intrhand(int sr)
{

	printf("intrhand: unexpected sr 0x%x\n", sr);
}

const uint16_t ipl2psl_table[NIPL] = {
	[IPL_NONE]       = PSL_S | PSL_IPL0,
	[IPL_SOFTCLOCK]  = PSL_S | PSL_IPL1,
	[IPL_SOFTBIO]    = PSL_S | PSL_IPL1,
	[IPL_SOFTNET]    = PSL_S | PSL_IPL1,
	[IPL_SOFTSERIAL] = PSL_S | PSL_IPL1,
	[IPL_VM]         = PSL_S | PSL_IPL5,
	[IPL_SCHED]      = PSL_S | PSL_IPL7,
	[IPL_HIGH]       = PSL_S | PSL_IPL7,
};

#if (defined(DDB) || defined(DEBUG)) && !defined(PANICBUTTON)
#define PANICBUTTON
#endif

#ifdef PANICBUTTON
int panicbutton = 1;	/* non-zero if panic buttons are enabled */
int crashandburn = 0;
int candbdelay = 50;	/* give em half a second */
void candbtimer(void *);

void
candbtimer(void *arg)
{

	crashandburn = 0;
}
#endif

/*
 * Level 7 interrupts can be caused by the NMI button.
 */
void
nmihand(struct frame frame)
{

	intio_set_sysport_keyctrl(intio_get_sysport_keyctrl() | 0x04);

	if (1) {
#ifdef PANICBUTTON
		static int innmihand = 0;

		/*
		 * Attempt to reduce the window of vulnerability for recursive
		 * NMIs (e.g. someone holding down the keyboard reset button).
		 */
		if (innmihand == 0) {
			innmihand = 1;
			printf("NMI button pressed\n");
			innmihand = 0;
		}
#ifdef DDB
		Debugger();
#else
		if (panicbutton) {
			if (crashandburn) {
				crashandburn = 0;
				panic(panicstr ?
				    "forced crash, nosync" : "forced crash");
			}
			crashandburn++;
			callout_reset(&candbtimer_ch, candbdelay,
			    candbtimer, NULL);
		}
#endif /* DDB */
#endif /* PANICBUTTON */
		return;
	}
	/* panic?? */
	printf("unexpected level 7 interrupt ignored\n");
}

#ifdef EXTENDED_MEMORY

static const struct memlist {
	paddr_t exstart;
	psize_t minsize;
	psize_t maxsize;
} memlist[] = {
	/* We define two extended memory regions for all possible settings. */

	/*
	 * The first region is at 0x01000000:
	 *
	 *  TS-6BE16:	16MB at 0x01000000 (to 0x01FFFFFF)
	 *  XM6i:	4MB - 240MB at 0x01000000 (upto 0x0FFFFFFF)
	 */
	{ 0x01000000, 0x00400000, 0x0f000000 },

	/*
	 * The second region is at 0x10000000:
	 *
	 * 060turbo:	4MB - 128MB at 0x10000000 (upto 0x17FFFFFF)
	 * XM6i:	4MB - 768MB at 0x10000000 (upto 0x3FFFFFFF)
	 */
	{ 0x10000000, 0x00400000, 0x30000000 },
};

/* check each 4MB region */
#define EXTMEM_RANGE	(4 * 1024 * 1024)

__CTASSERT(__arraycount(memlist) + 1 <= VM_PHYSSEG_MAX);

/*
 * check memory existency
 */
static bool
mem_exists(paddr_t mem, paddr_t basemax)
{
	/* most variables must be register! */
	volatile unsigned char *m, *b;
	unsigned char save_m, save_b=0;	/* XXX: shutup gcc */
	bool baseismem;
	bool exists = false;
	void *base;
	void *begin_check, *end_check;
	label_t	faultbuf;

	/*
	 * In this function we assume:
	 *  - MMU is not enabled yet but PA == VA
	 *    (i.e. no RELOC() macro to convert PA to VA)
	 *  - bus error can be caught by setjmp()
	 *    (i.e. %vbr register is initialized properly)
	 *  - all memory cache is not enabled
	 */

	/* only 24bits are significant on normal X680x0 systems */
	base = (void *)(mem & 0x00FFFFFF);

	m = (void *)mem;
	b = (void *)base;

	/* This is somewhat paranoid -- avoid overwriting myself */
	__asm("lea %%pc@(begin_check_mem),%0" : "=a"(begin_check));
	__asm("lea %%pc@(end_check_mem),%0" : "=a"(end_check));
	if (base >= begin_check && base < end_check) {
		size_t off = (char *)end_check - (char *)begin_check;

		m -= off;
		b -= off;
	}

	nofault = &faultbuf;
	if (setjmp(nofault)) {
		nofault = NULL;
		return false;
	}

	(void)*m;
	/*
	 * Can't check by writing if the corresponding
	 * base address isn't memory.
	 *
	 * I hope this would be no harm....
	 */
	baseismem = base < (void *)basemax;

__asm("begin_check_mem:");
	/* save original value (base must be saved first) */
	if (baseismem)
		save_b = *b;
	save_m = *m;

	/*
	 * stack and other data segment variables are unusable
	 * til end_check_mem, because they may be clobbered.
	 */

	/*
	 * check memory by writing/reading
	 */
	if (baseismem)
		*b = 0x55;
	*m = 0xAA;
	if ((baseismem && *b != 0x55) || *m != 0xAA)
		goto out;

	*m = 0x55;
	if (baseismem)
		*b = 0xAA;
	if (*m != 0x55 || (baseismem && *b != 0xAA))
		goto out;

	exists = true;
out:
	*m = save_m;
	if (baseismem)
		*b = save_b;

__asm("end_check_mem:");

	nofault = NULL;

	return exists;
}
#endif

void
setmemrange(void)
{
#ifdef EXTENDED_MEMORY
	int i;
	paddr_t exstart, exend;
	psize_t size, minsize, maxsize;
	const struct memlist *mlist = memlist;
	paddr_t basemax = m68k_ptob(physmem);
#endif

	/*
	 * VM system is not started yet, and even MMU is not enabled here.
	 * We assume VA == PA and don't bother to use RELOC() macro
	 * as pmap_bootstrap() does.
	 */

	/* save the original base memory range */
	basemem = physmem;

	/*
	 * XXX
	 * Probably we should also probe the main memory region
	 * for machines which might have a wrong value in a dead SRAM.
	 */
	phys_seg_list[0].ps_start = lowram;
	phys_seg_list[0].ps_end = lowram + m68k_ptob(basemem);

#ifdef EXTENDED_MEMORY
	/* discover extended memory */
	for (i = 0; i < __arraycount(memlist); i++) {
		exstart = mlist[i].exstart;
		minsize = mlist[i].minsize;
		maxsize = mlist[i].maxsize;
		/*
		 * Normally, x68k hardware is NOT 32bit-clean.
		 * But some type of extended memory is in 32bit address space.
		 * Check whether.
		 */
		if (!mem_exists(exstart, basemax))
			continue;
		exend = 0;
		/* range check */
		for (size = minsize; size <= maxsize; size += EXTMEM_RANGE) {
			if (!mem_exists(exstart + size - 4, basemax))
				break;
			exend = exstart + size;
		}
		if (exstart < exend) {
			phys_seg_list[i + 1].ps_start = exstart;
			phys_seg_list[i + 1].ps_end = exend;
			physmem += m68k_btop(exend - exstart);
			if (maxmem < m68k_btop(exend))
				maxmem = m68k_btop(exend);
		}
	}
#endif
}

volatile unsigned int intr_depth;

bool
cpu_intr_p(void)
{

	return intr_depth != 0;
}

int
mm_md_physacc(paddr_t pa, vm_prot_t prot)
{
	int rv = mm_md_physacc_regular(pa, prot);

	if (rv != 0) {
		/* I/O space */
		if (INTIOBASE <= pa && pa < INTIOTOP) {
			rv = kauth_authorize_machdep(kauth_cred_get(),
			    KAUTH_MACHDEP_UNMANAGEDMEM, NULL, NULL, NULL, NULL);
		}
	}

	return rv;
}
