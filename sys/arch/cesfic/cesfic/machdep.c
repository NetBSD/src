/*	$NetBSD: machdep.c,v 1.89 2026/04/09 14:36:55 thorpej Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: machdep.c,v 1.89 2026/04/09 14:36:55 thorpej Exp $");

#include "opt_bufcache.h"
#include "opt_ddb.h"
#include "opt_kgdb.h"
#include "opt_sysv.h"
#include "opt_panicbutton.h"
#include "opt_modular.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/buf.h>
#include <sys/callout.h>
#include <sys/conf.h>
#include <sys/exec.h>
#include <sys/file.h>
#include <sys/ioctl.h>
#include <sys/kernel.h>
#include <sys/mbuf.h>
#include <sys/mount.h>
#include <sys/msgbuf.h>
#include <sys/proc.h>
#include <sys/reboot.h>
#include <sys/signalvar.h>
#include <sys/syscallargs.h>
#include <sys/tty.h>
#include <sys/vnode.h>
#include <sys/ksyms.h>
#include <sys/module.h>
#include <sys/cpu.h>
#include <sys/kgdb.h>

#include <machine/db_machdep.h>
#include <ddb/db_sym.h>
#include <ddb/db_extern.h>

#include <machine/autoconf.h>
#include <machine/cpu.h>
#include <machine/reg.h>
#include <machine/pcb.h>
#include <machine/psl.h>
#include <machine/pte.h>

#include <m68k/seglist.h>

#define	MAXMEM	64*1024	/* XXX - from cmap.h */

#include <uvm/uvm_extern.h>

#include <sys/sysctl.h>
#include <sys/device.h>
#include <dev/cons.h>
#include <dev/mm.h>
#include <dev/ic/z8530reg.h>
#include <machine/z8530var.h>
#include <cesfic/dev/zsvar.h>

#include "ksyms.h"

extern	u_int lowram;

void machine_init(paddr_t);

/* prototypes for local functions */
void    identifycpu(void);
char	*hexstr(int, int);

/* functions called from locore.s */
void    dumpsys(void);
void	nmihand(struct frame);

int	delay_divisor;		/* delay constant */

extern void sicinit(void*);

void
machine_init(paddr_t nextpa)
{
	boothowto = RB_SINGLE; /* XXX for now */
	boothowto |= RB_KDB; /* XXX for now */

	delay_divisor = delay_divisor_est40(25); /* XXX */

	phys_seg_list[0].ps_start = lowram;
	phys_seg_list[0].ps_end = lowram + m68k_ptob(physmem);

	machine_init_common(nextpa);

	/*
	 * map and init interrupt controller
	 */
	physaccess((void*)virtual_avail, (void*)0x44000000,
	    PAGE_SIZE, PG_RW|PG_CI);
	sicinit((void*)virtual_avail);
	virtual_avail += PAGE_SIZE;
}

int
zs_check_kgdb(struct zs_chanstate *cs, int dev)
{

	if((boothowto & RB_KDB) && (dev == makedev(10, 0)))
		return (1);
	return (0);
}

void zs_kgdb_cnputc(dev_t, int);
void zs_kgdb_cnputc(dev_t dev, int c)
{
	zscnputc(dev, c);
}
int zs_kgdb_cngetc(dev_t);
int zs_kgdb_cngetc(dev_t dev)
{
	return (zscngetc(dev));
}

/*
 * Console initialization: called early on from main,
 * before vm init or startup.  Do enough configuration
 * to choose and initialize a console.
 */
extern void sic_enable_int(int, int, int, int, int);
void
consinit(void)
{

	/*
	 * Initialize the console before we print anything out.
	 */
	physaccess((void*)virtual_avail,
	    (void*)0x58000000, PAGE_SIZE, PG_RW|PG_CI);
	zs_cnattach((void*)virtual_avail);
	virtual_avail += PAGE_SIZE;

#ifdef KGDB
        kgdb_dev = 1;
        kgdb_attach((void*)zscngetc, (void*)zscnputc, (void *)0);

	if (boothowto & RB_KDB) {
		kgdb_connect(1);
		zscons.cn_putc = zs_kgdb_cnputc;
		zscons.cn_getc = zs_kgdb_cngetc;
	}
#endif
#ifdef DDB
	if (boothowto & RB_KDB)
		Debugger();
#endif
	sic_enable_int(39, 2, 1, 7, 0); /* NMI */
}

/*
 * cpu_startup: allocate memory for variable-sized tables,
 * initialize CPU, and do autoconfiguration.
 */
void
cpu_startup(void)
{
	vaddr_t minaddr, maxaddr;
#ifdef DEBUG
	extern int pmapdebug;
	int opmapdebug = pmapdebug;

	pmapdebug = 0;
#endif

	cpu_setmodel("FIC8234");

	/* Initialize the FPU, if present. */
	fpu_init();

	/*
	 * Good {morning,afternoon,evening,night}.
	 */
	printf("%s%s", copyright, version);
	identifycpu();
	printf("real mem  = %d\n", ctob(physmem));

	minaddr = 0;

	/*
	 * Allocate a submap for physio
	 */
	phys_map = uvm_km_suballoc(kernel_map, &minaddr, &maxaddr,
				   VM_PHYS_SIZE, 0, false, NULL);

#ifdef DEBUG
	pmapdebug = opmapdebug;
#endif
	printf("avail mem = %ld\n", ptoa(uvm_availmem(false)));
}

/*
 * Info for CTL_HW
 */

void
identifycpu(void)
{
	printf("%s\n", cpu_getmodel());
	printf("delay constant: %d\n", delay_divisor);
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
	doboot();
	/*NOTREACHED*/
}

/*
 * These variables are needed by /sbin/savecore
 */
u_int32_t dumpmag = 0x8fca0101;	/* magic number */
int	dumpsize = 0;		/* pages */
long	dumplo = 0;		/* blocks */

/*
 * This is called by main to set dumplo and dumpsize.
 * Dumps always skip the first CLBYTES of disk space
 * in case there might be a disk label stored there.
 * If there is extra space, put dump at the end to
 * reduce the chance that swapping trashes it.
 */
void
cpu_dumpconf(void)
{
	int nblks;	/* size of dump area */

	if (dumpdev == NODEV)
		return;
	nblks = bdev_size(dumpdev);
	if (nblks <= ctod(1))
		return;

	/*
	 * XXX include the final RAM page which is not included in physmem.
	 */
	dumpsize = physmem + 1;

	/* Always skip the first CLBYTES, in case there is a label there. */
	if (dumplo < ctod(1))
		dumplo = ctod(1);

	/* Put dump at end of partition, and make it fit. */
	if (dumpsize > dtoc(nblks - dumplo))
		dumpsize = dtoc(nblks - dumplo);
	if (dumplo < nblks - ctod(dumpsize))
		dumplo = nblks - ctod(dumpsize);
}

/*
 * Dump physical memory onto the dump device.  Called by doadump()
 * in locore.s or by cpu_reboot() here in machdep.c
 */
void
dumpsys(void)
{
	const struct bdevsw *bdev;
	daddr_t blkno;		/* current block to write */
				/* dump routine */
	int (*dump)(dev_t, daddr_t, void *, size_t);
	int pg;			/* page being dumped */
	vm_offset_t maddr;	/* PA being dumped */
	int error;		/* error code from (*dump)() */

	/* Don't put dump messages in msgbuf. */
	msgbufmapped = 0;

	/* Make sure dump device is valid. */
	if (dumpdev == NODEV)
		return;
	bdev = bdevsw_lookup(dumpdev);
	if (bdev == NULL)
		return;
	if (dumpsize == 0) {
		cpu_dumpconf();
		if (dumpsize == 0)
			return;
	}
	if (dumplo < 0)
		return;
	dump = bdev->d_dump;
	blkno = dumplo;

	printf("\ndumping to dev %"PRIx64", offset %ld\n", dumpdev, dumplo);

	printf("dump ");
	maddr = lowram;
	for (pg = 0; pg < dumpsize; pg++) {
#define NPGMB	(1024*1024/PAGE_SIZE)
		/* print out how many MBs we have dumped */
		if (pg && (pg % NPGMB) == 0)
			printf("%d ", pg / NPGMB);
#undef NPGMB
		pmap_enter(pmap_kernel(), (vm_offset_t)vmmap, maddr,
		    VM_PROT_READ, VM_PROT_READ|PMAP_WIRED);
		pmap_update(pmap_kernel());

		error = (*dump)(dumpdev, blkno, vmmap, PAGE_SIZE);
		switch (error) {
		case 0:
			maddr += PAGE_SIZE;
			blkno += btodb(PAGE_SIZE);
			break;

		case ENXIO:
			printf("device bad\n");
			return;

		case EFAULT:
			printf("device not ready\n");
			return;

		case EINVAL:
			printf("area improper\n");
			return;

		case EIO:
			printf("i/o error\n");
			return;

		case EINTR:
			printf("aborted from console\n");
			return;

		default:
			printf("error %d\n", error);
			return;
		}
	}
	printf("succeeded\n");
}

/* XXX should change the interface, and make one badaddr() function */

int	*nofault;

int
badaddr(void *addr)
{
	int i;
	label_t	faultbuf;

	nofault = (int *) &faultbuf;
	if (setjmp((label_t *)nofault)) {
		nofault = (int *) 0;
		return (1);
	}
	i = *(volatile short *)addr;
	__USE(i);
	nofault = (int *) 0;
	return (0);
}

int
badbaddr(void *addr)
{
	int i;
	label_t	faultbuf;

	nofault = (int *) &faultbuf;
	if (setjmp((label_t *)nofault)) {
		nofault = (int *) 0;
		return (1);
	}
	i = *(volatile char *)addr;
	__USE(i);
	nofault = (int *) 0;
	return (0);
}

#ifdef PANICBUTTON
/*
 * Declare these so they can be patched.
 */
int panicbutton = 1;	/* non-zero if panic buttons are enabled */
int candbdiv = 2;	/* give em half a second (hz / candbdiv) */

void	candbtimer(void *);

int crashandburn;

void
candbtimer(void *arg)
{

	crashandburn = 0;
}
#endif /* PANICBUTTON */

static int innmihand;	/* simple mutex */

/*
 * Level 7 interrupts can be caused by the keyboard or parity errors.
 */
void
nmihand(struct frame frame)
{

	/* Prevent unwanted recursion. */
	if (innmihand)
		return;
	innmihand = 1;

	printf("NMI\n");
#if defined(DDB) || defined(KGDB)
	Debugger();
#endif

	innmihand = 0;
}
