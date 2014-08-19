/*	$NetBSD: machdep.c,v 1.64.2.1 2014/08/20 00:02:50 tls Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: machdep.c,v 1.64.2.1 2014/08/20 00:02:50 tls Exp $");

#include "opt_bufcache.h"
#include "opt_ddb.h"
#include "opt_kgdb.h"
#include "opt_compat_netbsd.h"
#include "opt_sysv.h"
#include "opt_panicbutton.h"
#include "opt_modular.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/buf.h>
#include <sys/callout.h>
#include <sys/conf.h>
#include <sys/exec.h>
#include <sys/exec_aout.h>		/* for MID_* */
#include <sys/file.h>
#include <sys/ioctl.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
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
#ifdef SYSVMSG
#include <sys/msg.h>
#endif
#ifdef SYSVSEM
#include <sys/sem.h>
#endif
#ifdef SYSVSHM
#include <sys/shm.h>
#endif
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

/* the following is used externally (sysctl_hw) */
char machine[] = MACHINE;		/* CPU "architecture" */

/* Our exported CPU info; we can have only one. */  
struct cpu_info cpu_info_store;

struct vm_map *phys_map = NULL;

/*
 * Declare these as initialized data so we can patch them.
 */
/*int	maxmem;*/			/* max memory per process */
extern int physmem;			/* max supported memory, changes to actual */

extern	u_int lowram;

void fic_init(void);

/* prototypes for local functions */
void    identifycpu(void);
void	dumpmem(int *, int, int);
char	*hexstr(int, int);

/* functions called from locore.s */
void    dumpsys(void);
void    straytrap(int, u_short);
void	nmihand(struct frame);

int	delay_divisor;		/* delay constant */

extern void sicinit(void*);

void fic_init(void)
{
	int i;

	extern paddr_t avail_start, avail_end;

	boothowto = RB_SINGLE; /* XXX for now */
	boothowto |= RB_KDB; /* XXX for now */

	delay_divisor = 30; /* XXX */

	/*
	 * Tell the VM system about available physical memory.  The
	 * fic uses one segment.
	 */
	uvm_page_physload(atop(avail_start), atop(avail_end),
	    atop(avail_start), atop(avail_end), VM_FREELIST_DEFAULT);

	/*
	 * map and init interrupt controller
	 */
	physaccess((void*)virtual_avail, (void*)0x44000000,
	    PAGE_SIZE, PG_RW|PG_CI);
	sicinit((void*)virtual_avail);
	virtual_avail += PAGE_SIZE;

	/*
	 * Initialize error message buffer (at end of core).
	 * avail_end was pre-decremented in pmap_bootstrap to compensate.
	 */
	for (i = 0; i < btoc(MSGBUFSIZE); i++)
		pmap_enter(pmap_kernel(), (vaddr_t)msgbufaddr + i * PAGE_SIZE,
		    avail_end + i * PAGE_SIZE, VM_PROT_READ|VM_PROT_WRITE,
		    VM_PROT_READ|VM_PROT_WRITE|PMAP_WIRED);
	pmap_update(pmap_kernel());
	initmsgbuf(msgbufaddr, m68k_round_page(MSGBUFSIZE));
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
	if (fputype != FPU_NONE)
		m68k_make_fpu_idle_frame();

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
	printf("avail mem = %ld\n", ptoa(uvmexp.free));
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
		/*
		 * If we've been adjusting the clock, the todr
		 * will be out of synch; adjust it now.
		 */
		resettodr();
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
		(void)cngetc();
		printf("\n");
	}
#endif

	/* Finally, halt/reboot the system. */
	if (howto & RB_HALT) {
		printf("System halted.  Hit any key to reboot.\n\n");
		(void)cngetc();
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

void
straytrap(int pc, u_short evec)
{
	printf("unexpected trap (vector offset %x) from %x\n",
	       evec & 0xFFF, pc);
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


/*
 * cpu_exec_aout_makecmds():
 *	CPU-dependent a.out format hook for execve().
 * 
 * Determine of the given exec package refers to something which we
 * understand and, if so, set up the vmcmds for it.
 *
 * XXX what are the special cases for the hp300?
 * XXX why is this COMPAT_NOMID?  was something generating
 *	hp300 binaries with an a_mid of 0?  i thought that was only
 *	done on little-endian machines...  -- cgd
 */
int
cpu_exec_aout_makecmds(struct lwp *l, struct exec_package *epp)
{
#if defined(COMPAT_NOMID) || defined(COMPAT_44)
	u_long midmag, magic;
	u_short mid;
	int error;
	struct exec *execp = epp->ep_hdr;

	midmag = ntohl(execp->a_midmag);
	mid = (midmag >> 16) & 0xffff;
	magic = midmag & 0xffff;

	midmag = mid << 16 | magic;

	switch (midmag) {
#ifdef COMPAT_NOMID
	case (MID_ZERO << 16) | ZMAGIC:
		error = exec_aout_prep_oldzmagic(l, epp);
		return (error);
#endif
#ifdef COMPAT_44
	case (MID_HP300 << 16) | ZMAGIC:
		error = exec_aout_prep_oldzmagic(l, epp);
		return (error);
#endif
	}
#endif /* !(defined(COMPAT_NOMID) || defined(COMPAT_44)) */

	return ENOEXEC;
}

int
mm_md_physacc(paddr_t pa, vm_prot_t prot)
{

	return (pa < lowram || pa >= 0xfffffffc) ? EFAULT : 0;
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
