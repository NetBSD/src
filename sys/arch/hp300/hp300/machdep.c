/*	$NetBSD: machdep.c,v 1.270 2026/04/26 10:52:14 thorpej Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: machdep.c,v 1.270 2026/04/26 10:52:14 thorpej Exp $");

#include "opt_ddb.h"
#include "opt_fpu_emulate.h"
#include "opt_modular.h"
#include "opt_m68k_arch.h"
#include "opt_panicbutton.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/callout.h>
#include <sys/buf.h>
#include <sys/conf.h>
#include <sys/exec.h>
#include <sys/file.h>
#include <sys/ioctl.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/mbuf.h>
#include <sys/mount.h>
#include <sys/msgbuf.h>
#include <sys/proc.h>
#include <sys/reboot.h>
#include <sys/signalvar.h>
#include <sys/syscallargs.h>
#include <sys/tty.h>
#include <sys/core.h>
#include <sys/kcore.h>
#include <sys/vnode.h>
#include <sys/ksyms.h>
#include <sys/module.h>
#include <sys/cpu.h>

#ifdef DDB
#include <machine/db_machdep.h>
#include <ddb/db_sym.h>
#include <ddb/db_extern.h>
#endif /* DDB */
#include <sys/exec_elf.h>

#include <machine/autoconf.h>
#include <machine/bootinfo.h>
#include <machine/bus.h>
#include <machine/cpu.h>
#include <machine/hp300spu.h>
#include <machine/reg.h>
#include <machine/pcb.h>
#include <machine/psl.h>
#include <machine/pte.h>

#include <m68k/seglist.h>

#include <machine/kcore.h>	/* XXX should be pulled in by sys/kcore.h */

#include <dev/cons.h>
#include <dev/mm.h>

#define	MAXMEM	64*1024	/* XXX - from cmap.h */
#include <uvm/uvm_extern.h>

#include <sys/sysctl.h>

#include "opt_useleds.h"

#ifdef USELEDS
#include <hp300/hp300/leds.h>
#endif

#include "ksyms.h"

/*
 * bootinfo base (physical and virtual).  The bootinfo is placed, by
 * the boot loader, into the first page of kernel text, which is zero
 * filled (see locore.s) and not mapped at 0.  It is remapped to a
 * different address in pmap_bootstrap().
 */
paddr_t	bootinfo_pa;
vaddr_t	bootinfo_va;

int	maxmem;			/* max memory per process */

extern	u_int lowram;

/* prototypes for local functions */
static void	parityenable(void);
static int	parityerror(struct frame *);
static int	parityerrorfind(void);

/* functions called from locore.s */
void	machine_init(paddr_t);
void	nmihand(struct frame);

int	delay_divisor;		/* delay constant */

/*
 * machine_bootmap[] is checked in pmap_bootstrap1() of the new m68k pmap
 * and it allocates kernel address space for intio devices.
 */
#define PMBM_INTIO	0
#define PMBM_EXTIO	1
#define PMBM_BOOTINFO	2
#define PMBM_MAXADDR	3
struct pmap_bootmap machine_bootmap[] = {
	{ .pmbm_vaddr_ptr = (vaddr_t *)&intiobase,
	  .pmbm_paddr = INTIOBASE,
	  .pmbm_size  = INTIOSIZE,
	  .pmbm_flags = PMBM_F_CI },

	{ .pmbm_vaddr_ptr = (vaddr_t *)&extiobase,
	  .pmbm_paddr = 0,	/* VAONLY, so no PA mappings */
	  .pmbm_size  = ctob(EIOMAPSIZE),
	  .pmbm_flags = PMBM_F_VAONLY | PMBM_F_CI },

	{ .pmbm_vaddr_ptr = &bootinfo_va,
	  .pmbm_paddr = 0,	/* VAONLY, so no PA mappings */
	  .pmbm_size  = ctob(1),
	  .pmbm_flags = PMBM_F_VAONLY },

	/* Last page of RAM is mapped VA==PA for the MMU trampoline. */
	{ .pmbm_vaddr = MAXADDR,
	  .pmbm_paddr = MAXADDR,
	  .pmbm_size  = PAGE_SIZE,
	  .pmbm_flags = PMBM_F_FIXEDVA | PMBM_F_CI },

	{ .pmbm_vaddr = -1 },
};

#ifdef __HAVE_NEW_PMAP_68K
/*
 * The new 68k pmap utilizes the MAXADDR page as the NULL segment table
 * to save a page, so we have to preserve PROM workarea for the next reboot.
 * This preservation area is much less than a page size, so the trade-off
 * seems worth it.
 */
#define	BOOTWORKSTART	0xfffffdc0U	/* from hp300/stand/common/srt0.S */
#define	BOOTWORKSIZE	(0U - BOOTWORKSTART)
static char bootwork_savearea[BOOTWORKSIZE];
#endif /* __HAVE_NEW_PMAP_68K */

/*
 * Early initialization, before main() is called.
 */
void
machine_init(paddr_t nextpa)
{
	struct btinfo_magic *bt_mag;

#ifdef M68K_EC_VAC
	/*
	 * Determine VA aliasing distance if any
	 */
	switch (machineid) {
	case HP_320:
		pmap_init_vac(16 * 1024);
		break;
	case HP_350:
		pmap_init_vac(32 * 1024);
		break;
	default:
		break;
	}
#endif

	/* Enable the external cache, if any. */
	ecacheon();

#ifdef __HAVE_NEW_PMAP_68K
	/*
	 * We've used NULL_SEGTAB_PA in <machine/pmap.h> to use the
	 * reserved last-page-of-RAM as the NULL segment table.  But,
	 * we copied code into that page (the MMU trampoline) and it
	 * also contains the PROM's work area.  Preserve the PROM work
	 * area and zero it out now.
	 */
	memcpy(bootwork_savearea, (void *)BOOTWORKSTART, BOOTWORKSIZE);
	memset((void *)MAXADDR, 0, PAGE_SIZE);
#endif

	/*
	 * N.B. we exclude the last page of memory from the physical
	 * address range; if we didn't, ps_end would be 0.
	 */
	phys_seg_list[0].ps_start = lowram;
	phys_seg_list[0].ps_end = MAXADDR;

	machine_init_common(nextpa);

	/* Calibrate the delay loop. */
	hp300_calibrate_delay();

	/*
	 * Map in the bootinfo page, and make sure the bootinfo
	 * exists by searching for the MAGIC record.  If it's not
	 * there, disable bootinfo.
	 */
#ifndef __HAVE_NEW_PMAP_68K
	bootinfo_va = virtual_avail;
	virtual_avail += PAGE_SIZE;
#endif
	pmap_kenter_pa(bootinfo_va, bootinfo_pa,
	    VM_PROT_READ|VM_PROT_WRITE, 0);
	pmap_update(pmap_kernel());
	bt_mag = lookup_bootinfo(BTINFO_MAGIC);
	if (bt_mag == NULL ||
	    bt_mag->magic1 != BOOTINFO_MAGIC1 ||
	    bt_mag->magic2 != BOOTINFO_MAGIC2) {
		pmap_kremove(bootinfo_va, PAGE_SIZE);
		pmap_update(pmap_kernel());
#ifndef __HAVE_NEW_PMAP_68K
		virtual_avail -= PAGE_SIZE;
#endif
		bootinfo_va = 0;
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

	/*
	 * Initialize the external I/O extent map.
	 */
	iomap_init();

	/*
	 * Initialize the console before we print anything out.
	 */

	hp300_cninit();

	/*
	 * Issue a warning if the boot loader didn't provide bootinfo.
	 */
	if (bootinfo_va != 0)
		printf("bootinfo found at 0x%08lx\n", bootinfo_pa);
	else
		printf("WARNING: boot loader did not provide bootinfo\n");

#if NKSYMS || defined(DDB) || defined(MODULAR)
	{
		extern int end;
		extern int *esym;

		ksyms_addsyms_elf((int)esym - (int)&end - sizeof(Elf32_Ehdr),
		    (void *)&end, esym);
	}
#endif
#ifdef DDB
	if (boothowto & RB_KDB)
		Debugger();
#endif
}

/*
 * cpu_startup: allocate memory for variable-sized tables,
 * initialize CPU
 */
void
cpu_startup(void)
{
	hp300_cninit_deferred();

	cpu_startup_common();

	printf("cpu: delay divisor %d", delay_divisor);
	if (mmuid)
		printf(", mmuid %d", mmuid);
	printf("\n");

	/*
	 * Now that we have told the user what they have,
	 * let them know if that machine type isn't configured.
	 */
	switch (machineid) {
	case -1:		/* keep compilers happy */
#if !defined(HP320)
	case HP_320:
#endif
#if !defined(HP330)
	case HP_330:
#endif
#if !defined(HP340)
	case HP_340:
#endif
#if !defined(HP345)
	case HP_345:
#endif
#if !defined(HP350)
	case HP_350:
#endif
#if !defined(HP360)
	case HP_360:
#endif
#if !defined(HP362)
	case HP_362:
#endif
#if !defined(HP370)
	case HP_370:
#endif
#if !defined(HP375)
	case HP_375:
#endif
#if !defined(HP380)
	case HP_380:
#endif
#if !defined(HP382)
	case HP_382:
#endif
#if !defined(HP385)
	case HP_385:
#endif
#if !defined(HP400)
	case HP_400:
#endif
#if !defined(HP425)
	case HP_425:
#endif
#if !defined(HP433)
	case HP_433:
#endif
		panic("SPU type not configured");
	default:
		break;
	}

	parityenable();
#ifdef USELEDS
	ledinit();
#endif
}

struct hp300_model {
	int id;
	int mmuid;
	const char *name;
	int speed_khz;
};

static const struct hp300_model hp300_models[] = {
	{ HP_320,	-1,		"320",		16670	},
	{ HP_330,	-1,		"318/319/330",	16670	},
	{ HP_340,	-1,		"340",		16670	},
	{ HP_345,	-1,		"345",		50000	},
	{ HP_350,	-1,		"350",		25000	},
	{ HP_360,	-1,		"360",		25000	},
	{ HP_362,	-1,		"362",		25000	},
	{ HP_370,	-1,		"370",		33330	},
	{ HP_375,	-1,		"375",		50000	},
	{ HP_380,	-1,		"380",		25000	},
	{ HP_382,	-1,		"382",		25000	},
	{ HP_385,	-1,		"385",		33000	},
	{ HP_400,	-1,		"400",		50000	},
	{ HP_425,	MMUID_425_T,	"425t",		25000	},
	{ HP_425,	MMUID_425_S,	"425s",		25000	},
	{ HP_425,	MMUID_425_E,	"425e",		25000	},
	{ HP_425,	-1,		"425",		25000	},
	{ HP_433,	MMUID_433_T,	"433t",		33000	},
	{ HP_433,	MMUID_433_S,	"433s",		33000	},
	{ HP_433,	-1,		"433",		33000	},
	{ 0,		-1,		NULL,		0	},
};

void
machine_set_model(void)
{
	int i;

	for (i = 0; hp300_models[i].name != NULL; i++) {
		if (hp300_models[i].id == machineid) {
			if (hp300_models[i].mmuid != -1 &&
			    hp300_models[i].mmuid != mmuid)
				continue;
			break;
		}
	}
	if (hp300_models[i].name == NULL) {
		printf("\nunknown machineid %d\n", machineid);
		panic("startup");
	}
	cpu_setmodel("HP 9000/%s", hp300_models[i].name);
	cpuspeed_khz = hp300_models[i].speed_khz;

	if (fputype == FPU_68881) {
		fpuspeed_khz = machineid == HP_350 ? 20000 : 16670;
	}

#ifdef M68K_EC
	if (ectype != EC_NONE) {
		switch (machineid) {
		case HP_320:
			ecsize = 16*1024;
			break;
		case HP_370:
			ecsize = 64*1024;
			break;
		default:
			ecsize = 32*1024;
			break;
		}
	}
#endif /* M68K_EC */
}

/*
 * machine dependent system variables.
 */
SYSCTL_SETUP(sysctl_machdep_setup, "sysctl machdep subtree setup")
{
	static bool broken_rmc;

	broken_rmc = (cputype == M68020);

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

	sysctl_createv(clog, 0, NULL, NULL,
	    CTLFLAG_PERMANENT,
	    CTLTYPE_BOOL, "broken_rmc", NULL,
	    NULL, 0, &broken_rmc, 0,
	    CTL_MACHDEP, CPU_BROKEN_RMC, CTL_EOL);
}

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

	/* If rebooting and a dump is requested do it. */
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

	/* Finally, halt/reboot the system. */
	if (howto & RB_HALT) {
		printf("System halted.  Hit any key to reboot.\n\n");
		cnpollc(true);
		(void)cngetc();
		cnpollc(false);
	}

	printf("rebooting...\n");
	DELAY(1000000);

#ifdef __HAVE_NEW_PMAP_68K
	/* Restore the PROM work area first. */
	memcpy((void *)BOOTWORKSTART, bootwork_savearea, BOOTWORKSIZE);
#endif
	doboot();
	/* NOTREACHED */
}

/* XXX should change the interface, and make one badaddr() function */

int
badaddr(void *addr)
{
	int i;
	label_t	faultbuf;

	nofault = &faultbuf;
	if (setjmp(nofault)) {
		nofault = NULL;
		return 1;
	}
	i = *(volatile short *)addr;
	__USE(i);
	nofault = NULL;
	return 0;
}

int
badbaddr(void *addr)
{
	int i;
	label_t	faultbuf;

	nofault = &faultbuf;
	if (setjmp(nofault)) {
		nofault = NULL;
		return 1;
	}
	i = *(volatile char *)addr;
	__USE(i);
	nofault = NULL;
	return 0;
}

/*
 * lookup_bootinfo:
 *
 *	Look up information in bootinfo from boot loader.
 */
void *
lookup_bootinfo(int type)
{
	struct btinfo_common *bt;
	char *help = (char *)bootinfo_va;

	/* Check for a bootinfo record first. */
	if (help == NULL)
		return NULL;

	do {
		bt = (struct btinfo_common *)help;
		if (bt->type == type)
			return help;
		help += bt->next;
	} while (bt->next != 0 &&
		 (size_t)help < (size_t)bootinfo_va + BOOTINFO_SIZE);

	return NULL;
}

#if defined(PANICBUTTON) && !defined(DDB)
/*
 * Declare these so they can be patched.
 */
int panicbutton = 1;	/* non-zero if panic buttons are enabled */
int candbdiv = 2;	/* give em half a second (hz / candbdiv) */

static void	candbtimer(void *);

int crashandburn;

callout_t candbtimer_ch;

void
candbtimer(void *arg)
{

	crashandburn = 0;
}
#endif /* PANICBUTTON & !DDB */

static int innmihand;	/* simple mutex */

/*
 * Level 7 interrupts can be caused by HIL keyboards (in cooked mode only,
 * but we run them in raw mode) or parity errors.
 */
void
nmihand(struct frame frame)
{

	/* Prevent unwanted recursion. */
	if (innmihand)
		return;
	innmihand = 1;

	if (parityerror(&frame))
		return;
	/* panic?? */
	printf("unexpected level 7 interrupt ignored\n");

	innmihand = 0;
}

/*
 * Parity error section.  Contains magic.
 */
#define PARREG		((volatile short *)IIOV(0x5B0000))
static int gotparmem = 0;
#ifdef DEBUG
int ignorekperr = 0;	/* ignore kernel parity errors */
#endif

/*
 * Enable parity detection
 */
static void
parityenable(void)
{
	label_t	faultbuf;

	nofault = &faultbuf;
	if (setjmp(nofault)) {
		nofault = NULL;
		printf("Parity detection disabled\n");
		return;
	}
	*PARREG = 1;
	nofault = NULL;
	gotparmem = 1;
}

/*
 * Determine if level 7 interrupt was caused by a parity error
 * and deal with it if it was.  Returns 1 if it was a parity error.
 */
static int
parityerror(struct frame *fp)
{
	if (!gotparmem)
		return 0;
	*PARREG = 0;
	DELAY(10);
	*PARREG = 1;
	if (panicstr) {
		printf("parity error after panic ignored\n");
		return 1;
	}
	if (!parityerrorfind())
		printf("WARNING: transient parity error ignored\n");
	else if (USERMODE(fp->f_sr)) {
		printf("pid %d: parity error\n", curproc->p_pid);
		uprintf("sorry, pid %d killed due to memory parity error\n",
		    curproc->p_pid);
		psignal(curproc, SIGKILL);
#ifdef DEBUG
	} else if (ignorekperr) {
		printf("WARNING: kernel parity error ignored\n");
#endif
	} else {
		regdump((struct trapframe *)fp, 128);
		panic("kernel parity error");
	}
	return 1;
}

/*
 * Yuk!  There has got to be a better way to do this!
 * Searching all of memory with interrupts blocked can lead to disaster.
 */
static int
parityerrorfind(void)
{
	static label_t parcatch;
	static int looking = 0;
	volatile int pg, o, s;
	volatile int *ip;
	int i;
	int found;

	/*
	 * If looking is true we are searching for a known parity error
	 * and it has just occurred.  All we do is return to the higher
	 * level invocation.
	 */
	if (looking)
		longjmp(&parcatch);
	s = splhigh();
	/*
	 * If setjmp returns true, the parity error we were searching
	 * for has just occurred (longjmp above) at the current pg+o
	 */
	if (setjmp(&parcatch)) {
		printf("Parity error at 0x%x\n", ctob(pg)|o);
		found = 1;
		goto done;
	}
	/*
	 * If we get here, a parity error has occurred for the first time
	 * and we need to find it.  We turn off any external caches and
	 * loop thru memory, testing every longword til a fault occurs and
	 * we regain control at setjmp above.  Note that because of the
	 * setjmp, pg and o need to be volatile or their values will be lost.
	 */
	looking = 1;
	ecacheoff();
	for (pg = btoc(lowram); pg < btoc(lowram)+physmem; pg++) {
		pmap_enter(pmap_kernel(), (vaddr_t)vmmap, ctob(pg),
		    VM_PROT_READ, VM_PROT_READ|PMAP_WIRED);
		pmap_update(pmap_kernel());
		ip = (int *)vmmap;
		for (o = 0; o < PAGE_SIZE; o += sizeof(int))
			i = *ip++;
	}
	__USE(i);
	/*
	 * Getting here implies no fault was found.  Should never happen.
	 */
	printf("Couldn't locate parity error\n");
	found = 0;
 done:
	looking = 0;
	pmap_remove(pmap_kernel(), (vaddr_t)vmmap, (vaddr_t)&vmmap[PAGE_SIZE]);
	pmap_update(pmap_kernel());
	ecacheon();
	splx(s);
	return found;
}

int
mm_md_physacc(paddr_t pa, vm_prot_t prot)
{
	int rv = mm_md_physacc_regular(pa, prot);

	if (rv != 0) {
		/* phys_seg_list[] doesn't include the last page of RAM. */
		if (pa >= MAXADDR) {
			rv = 0;
		}
	}
	return rv;
}

int
mm_md_kernacc(void *ptr, vm_prot_t prot, bool *handled)
{

	/*
	 * Do not allow reading intio or dio device space.  This could lead
	 * to corruption of device registers.
	 */
	*handled = false;
	return (ISIIOVA(ptr) || ((uint8_t *)ptr >= extiobase &&
	    (uint8_t *)ptr < extiobase + (EIOMAPSIZE * PAGE_SIZE)))
	    ? EFAULT : 0;
}
