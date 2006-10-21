/*	$NetBSD: machdep.c,v 1.309 2006/10/21 05:54:32 mrg Exp $	*/

/*
 * Copyright (c) 1982, 1990 The Regents of the University of California.
 * All rights reserved.
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
 */
/*
 * Copyright (c) 1988 University of Utah.
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
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
 */
/*-
 * Copyright (C) 1993	Allen K. Briggs, Chris P. Caputo,
 *			Michael L. Finch, Bradley A. Grantham, and
 *			Lawrence A. Kesteloot
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
 *	This product includes software developed by the Alice Group.
 * 4. The names of the Alice Group or any of its members may not be used
 *    to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE ALICE GROUP ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE ALICE GROUP BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */
/*
 * from: Utah $Hdr: machdep.c 1.63 91/04/24$
 *
 *	@(#)machdep.c	7.16 (Berkeley) 6/3/91
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: machdep.c,v 1.309 2006/10/21 05:54:32 mrg Exp $");

#include "opt_adb.h"
#include "opt_ddb.h"
#include "opt_ddbparam.h"
#include "opt_kgdb.h"
#include "opt_compat_netbsd.h"
#include "akbd.h"
#include "macfb.h"
#include "zsc.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/buf.h>
#include <sys/conf.h>
#include <sys/core.h>
#include <sys/exec.h>
#include <sys/extent.h>
#include <sys/file.h>
#include <sys/kcore.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/mbuf.h>
#include <sys/mount.h>
#include <sys/msgbuf.h>
#include <sys/pool.h>
#include <sys/proc.h>
#include <sys/queue.h>
#include <sys/reboot.h>
#include <sys/signalvar.h>
#include <sys/sa.h>
#include <sys/syscallargs.h>
#include <sys/user.h>
#include <sys/vnode.h>
#include <sys/ksyms.h>
#ifdef	KGDB
#include <sys/kgdb.h>
#endif
#define ELFSIZE 32
#include <sys/exec_elf.h>

#include <m68k/cacheops.h>

#include <machine/db_machdep.h>
#include <ddb/db_sym.h>
#include <ddb/db_extern.h>

#include <machine/autoconf.h>
#include <machine/cpu.h>
#include <machine/reg.h>
#include <machine/psl.h>
#include <machine/pte.h>
#include <machine/kcore.h>	/* XXX should be pulled in by sys/kcore.h */

#define	MAXMEM	64*1024	/* XXX - from cmap.h */
#include <uvm/uvm_extern.h>

#include <sys/sysctl.h>

#include <dev/cons.h>

#include <machine/iopreg.h>
#include <machine/psc.h>
#include <machine/viareg.h>
#include <mac68k/mac68k/macrom.h>
#include <mac68k/dev/adbvar.h>
#if NAKBD > 0
#include <mac68k/dev/akbdvar.h>
#endif
#if NMACFB > 0
#include <mac68k/dev/macfbvar.h>
#endif
#include <mac68k/dev/zs_cons.h>

#include "ksyms.h"

int symsize, end, *ssym, *esym;

/* The following is used externally (sysctl_hw) */
char	machine[] = MACHINE;	/* from <machine/param.h> */

struct mac68k_machine_S mac68k_machine;

volatile u_char *Via1Base, *Via2Base, *PSCBase = NULL;
u_long	NuBusBase = NBBASE;
u_long	IOBase;

vaddr_t	SCSIBase;

/* These are used to map kernel space: */
extern int numranges;
extern u_long low[8];
extern u_long high[8];

/* These are used to map NuBus space: */
#define	NBMAXRANGES	16
int	nbnumranges;		/* = 0 == don't use the ranges */
u_long	nbphys[NBMAXRANGES];	/* Start physical addr of this range */
u_long	nblog[NBMAXRANGES];	/* Start logical addr of this range */
long	nblen[NBMAXRANGES];	/* Length of this range If the length is */
				/* negative, all phys addrs are the same. */

/* From Booter via locore */
long	videoaddr;		/* Addr used in kernel for video */
long	videorowbytes;		/* Length of row in video RAM */
long	videobitdepth;		/* Number of bits per pixel */
u_long	videosize;		/* height = 31:16, width = 15:0 */

/*
 * Values for IIvx-like internal video
 * -- should be zero if it is not used (usual case).
 */
u_int32_t mac68k_vidlog;	/* logical addr */
u_int32_t mac68k_vidphys;	/* physical addr */
u_int32_t mac68k_vidlen;	/* mem length */

/* Callback and cookie to run bell */
int	(*mac68k_bell_callback)(void *, int, int, int);
caddr_t	mac68k_bell_cookie;

struct vm_map *exec_map = NULL;  
struct vm_map *mb_map = NULL;
struct vm_map *phys_map = NULL;

caddr_t	msgbufaddr;
int	maxmem;			/* max memory per process */
int	physmem = MAXMEM;	/* max supported memory, changes to actual */

/*
 * safepri is a safe priority for sleep to set for a spin-wait
 * during autoconfiguration or after a panic.
 */
int	safepri = PSL_LOWIPL;

/*
 * Extent maps to manage all memory space, including I/O ranges.  Allocate
 * storage for 8 regions in each, initially.  Later, iomem_malloc_safe
 * will indicate that it's safe to use malloc() to dynamically allocate
 * region descriptors.
 *
 * The extent maps are not static!  Machine-dependent NuBus and on-board
 * I/O routines need access to them for bus address space allocation.
 */
static long iomem_ex_storage[EXTENT_FIXED_STORAGE_SIZE(8) / sizeof(long)];
struct extent *iomem_ex;
int iomem_malloc_safe;

/* Our exported CPU info; we can have only one. */  
struct cpu_info cpu_info_store;

static void	identifycpu(void);
static u_long	get_physical(u_int, u_long *);

void	initcpu(void);
int	cpu_dumpsize(void);
int	cpu_dump(int (*)(dev_t, daddr_t, caddr_t, size_t), daddr_t *);
void	cpu_init_kcore_hdr(void);

void		getenvvars(u_long, char *);
static long	getenv(const char *);

/* functions called from locore.s */
void	dumpsys(void);
void	mac68k_init(void);
void	straytrap(int, int);
void	nmihand(struct frame);

/*
 * Machine-dependent crash dump header info.
 */
cpu_kcore_hdr_t cpu_kcore_hdr;

/*
 * XXX: For zs serial driver. We always initialize the base address
 * to avoid a bunch of #ifdefs.
 */
volatile unsigned char *sccA = 0;

/*
 * Early initialization, before main() is called.
 */
void
mac68k_init(void)
{
	int i;
	extern vaddr_t avail_start;

	/*
	 * Tell the VM system about available physical memory.
	 * Notice that we don't need to worry about avail_end here
	 * since it's equal to high[numranges-1].
	 */
	for (i = 0; i < numranges; i++) {
		if (low[i] <= avail_start && avail_start < high[i])
			uvm_page_physload(atop(avail_start), atop(high[i]),
			    atop(avail_start), atop(high[i]),
			    VM_FREELIST_DEFAULT);
		else
			uvm_page_physload(atop(low[i]), atop(high[i]),
			    atop(low[i]), atop(high[i]),
			    VM_FREELIST_DEFAULT);
	}

	/*
	 * Initialize the I/O mem extent map.
	 * Note: we don't have to check the return value since
	 * creation of a fixed extent map will never fail (since
	 * descriptor storage has already been allocated).
	 *
	 * N.B. The iomem extent manages _all_ physical addresses
	 * on the machine.  When the amount of RAM is found, all
	 * extents of RAM are allocated from the map.
	 */
	iomem_ex = extent_create("iomem", 0x0, 0xffffffff, M_DEVBUF,
	    (caddr_t)iomem_ex_storage, sizeof(iomem_ex_storage),
	    EX_NOCOALESCE|EX_NOWAIT);

	/* Initialize the interrupt handlers. */
	intr_init();

	/* Initialize the IOPs (if present) */
	iop_init(1);

	/*
	 * Initialize error message buffer (at end of core).
	 * high[numranges-1] was decremented in pmap_bootstrap.
	 */
	for (i = 0; i < btoc(MSGBUFSIZE); i++)
		pmap_enter(pmap_kernel(), (vaddr_t)msgbufaddr + i * PAGE_SIZE,
		    high[numranges - 1] + i * PAGE_SIZE,
		    VM_PROT_READ|VM_PROT_WRITE,
		    VM_PROT_READ|VM_PROT_WRITE|PMAP_WIRED);
	initmsgbuf(msgbufaddr, m68k_round_page(MSGBUFSIZE));
	pmap_update(pmap_kernel());
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
	 * Generic console: sys/dev/cons.c
	 *	Initializes either ite or ser as console.
	 *	Can be called from locore.s and init_main.c.  (Ugh.)
	 */
	static int init;	/* = 0 */

	if (!init) {
		cninit();
		init = 1;
	} else {
#if NAKBD > 0 && NMACFB > 0
		/*
		 * XXX  This is an evil hack on top of an evil hack!
		 *
		 * With the graybar stuff, we've got a catch-22:  we need
		 * to do at least some console setup really early on, even
		 * before we're running with the mappings we need.  On
		 * the other hand, we're not nearly ready to do anything
		 * with wscons or the ADB driver at that point.
		 *
		 * To get around this, maccninit() ignores the first call
		 * it gets (from cninit(), if not on a serial console).
		 * Once we're here, we call maccninit() again, which sets
		 * up the console devices and does the appropriate wscons
		 * initialization.
		 */
		if (mac68k_machine.serial_console == 0) {
			void maccninit(struct consdev *);
			maccninit(NULL);
		}
#endif

		mac68k_calibrate_delay();

#if NZSC > 0 && defined(KGDB)
		zs_kgdb_init();
#endif
#if NKSYMS || defined(DDB) || defined(LKM)
		/*
		 * Initialize kernel debugger, if compiled in.
		 */

		ksyms_init(symsize, ssym, esym);
#endif

		if (boothowto & RB_KDB) {
#ifdef KGDB
			/* XXX - Ask on console for kgdb_dev? */
			/* Note: this will just return if kgdb_dev==NODEV */
			kgdb_connect(1);
#else	/* KGDB */
#ifdef DDB
			/* Enter DDB.  We don't have a monitor PROM. */
			Debugger();
#endif /* DDB */
#endif	/* KGDB */
		}
	}
}

#define CURRENTBOOTERVER	111

/*
 * cpu_startup: allocate memory for variable-sized tables, make
 * (most of) kernel text read-only, and other miscellaneous bits
 */
void
cpu_startup(void)
{
	int vers;
	vaddr_t minaddr, maxaddr;
	int xdelay;
	char pbuf[9];

	/*
	 * Initialize the kernel crash dump header.
	 */
	cpu_init_kcore_hdr();

	/*
	 * Good {morning,afternoon,evening,night}.
	 */
	printf("%s%s", copyright, version);
	identifycpu();

	vers = mac68k_machine.booter_version;
	if (vers < CURRENTBOOTERVER) {
		/* fix older booters with indicies, not versions */
		if (vers < 100)
			vers += 99;

		printf("\nYou booted with booter version %d.%d.\n",
		    vers / 100, vers % 100);
		printf("Booter version %d.%d is necessary to fully support\n",
		    CURRENTBOOTERVER / 100, CURRENTBOOTERVER % 100);
		printf("this kernel.\n\n");
		for (xdelay = 0; xdelay < 1000000; xdelay++);
	}
	format_bytes(pbuf, sizeof(pbuf), ctob(physmem));
	printf("total memory = %s\n", pbuf);

	minaddr = 0;
	/*
	 * Allocate a submap for exec arguments.  This map effectively
	 * limits the number of processes exec'ing at any time.
	 */
	exec_map = uvm_km_suballoc(kernel_map, &minaddr, &maxaddr,
	    16 * NCARGS, VM_MAP_PAGEABLE, FALSE, NULL);

	/*
	 * Allocate a submap for physio
	 */
	phys_map = uvm_km_suballoc(kernel_map, &minaddr, &maxaddr,
	    VM_PHYS_SIZE, 0, FALSE, NULL);

	/*
	 * Finally, allocate mbuf cluster submap.
	 */
	mb_map = uvm_km_suballoc(kernel_map, &minaddr, &maxaddr,
	    nmbclusters * mclbytes, VM_MAP_INTRSAFE, FALSE, NULL);

	format_bytes(pbuf, sizeof(pbuf), ptoa(uvmexp.free));
	printf("avail memory = %s\n", pbuf);

	/*
	 * Set up CPU-specific registers, cache, etc.
	 */
	initcpu();

	/* Safe for extent allocation to use malloc now. */
	iomem_malloc_safe = 1;
}

void
initcpu(void)
{
	/* Invalidate supervisor mode data cache. */
	DCIS();
}

void doboot(void) __attribute__((__noreturn__));

/*
 * Set registers on exec.
 */
void
setregs(struct lwp *l, struct exec_package *pack, u_long stack)
{
	struct frame *frame = (struct frame *)l->l_md.md_regs;

	frame->f_sr = PSL_USERSET;
	frame->f_pc = pack->ep_entry & ~1;
	frame->f_regs[D0] = 0;
	frame->f_regs[D1] = 0;
	frame->f_regs[D2] = 0;
	frame->f_regs[D3] = 0;
	frame->f_regs[D4] = 0;
	frame->f_regs[D5] = 0;
	frame->f_regs[D6] = 0;
	frame->f_regs[D7] = 0;
	frame->f_regs[A0] = 0;
	frame->f_regs[A1] = 0;
	frame->f_regs[A2] = (int)l->l_proc->p_psstr;
	frame->f_regs[A3] = 0;
	frame->f_regs[A4] = 0;
	frame->f_regs[A5] = 0;
	frame->f_regs[A6] = 0;
	frame->f_regs[SP] = stack;

	/* restore a null state frame */
	l->l_addr->u_pcb.pcb_fpregs.fpf_null = 0;

	if (fputype)
		m68881_restore(&l->l_addr->u_pcb.pcb_fpregs);
}

int	waittime = -1;
struct pcb dumppcb;

void
cpu_reboot(int howto, char *bootstr)
{
	extern u_long maxaddr;

#if __GNUC__	/* XXX work around lame compiler problem (gcc 2.7.2) */
	(void)&howto;
#endif
	/* take a snap shot before clobbering any registers */
	if (curlwp && curlwp->l_addr)
		savectx(&curlwp->l_addr->u_pcb);

	/* If system is cold, just halt. */
	if (cold) {
		howto |= RB_HALT;
		goto haltsys;
	}

	boothowto = howto;
	if ((howto & RB_NOSYNC) == 0 && waittime < 0) {
		waittime = 0;
		vfs_shutdown();
#ifdef notyet
		/*
		 * If we've been adjusting the clock, the todr
		 * will be out of synch; adjust it now.
		 */
		resettodr();
#else
# ifdef DIAGNOSTIC
		printf("NetBSD/mac68k does not trust itself to update the "
		    "RTC on shutdown.\n");
# endif
#endif
	}

	/* Disable interrupts. */
	splhigh();

	/* If rebooting and a dump is requested, do it. */
	if (howto & RB_DUMP)
		dumpsys();

 haltsys:
	/* Run any shutdown hooks. */
	doshutdownhooks();

	if ((howto & RB_POWERDOWN) == RB_POWERDOWN) {
		/* First try to power down under VIA control. */
		via_powerdown();

#ifndef MRG_ADB
		/*
		 * Shut down machines whose power functions are accessed
		 * via modified ADB calls.  adb_poweroff() is available
		 * only when the MRG ADB is not being used.
		 */
		adb_poweroff();
#endif
		/*
		 * RB_POWERDOWN implies RB_HALT... fall into it...
		 */
	}

	if (howto & RB_HALT) {
		printf("\n");
		printf("The operating system has halted.\n");
		printf("Please press any key to reboot.\n\n");
		(void)cngetc();
	}

	/* Map the last physical page VA = PA for doboot() */
	pmap_enter(pmap_kernel(), (vaddr_t)maxaddr, (vaddr_t)maxaddr,
	    VM_PROT_ALL, VM_PROT_ALL|PMAP_WIRED);
	pmap_update(pmap_kernel());

	printf("rebooting...\n");
	DELAY(1000000);
	doboot();
	/* NOTREACHED */
}

/*
 * Initialize the kernel crash dump header.
 */
void
cpu_init_kcore_hdr(void)
{
	extern int end;
	cpu_kcore_hdr_t *h = &cpu_kcore_hdr;
	struct m68k_kcore_hdr *m = &h->un._m68k;
	int i;

	bzero(&cpu_kcore_hdr, sizeof(cpu_kcore_hdr));

	/*
	 * Initialize the `dispatcher' portion of the header.
	 */
	strcpy(h->name, machine);
	h->page_size = PAGE_SIZE;
	h->kernbase = KERNBASE;

	/*
	 * Fill in information about our MMU configuration.
	 */
	m->mmutype	= mmutype;
	m->sg_v		= SG_V;
	m->sg_frame	= SG_FRAME;
	m->sg_ishift	= SG_ISHIFT;
	m->sg_pmask	= SG_PMASK;
	m->sg40_shift1	= SG4_SHIFT1;
	m->sg40_mask2	= SG4_MASK2;
	m->sg40_shift2	= SG4_SHIFT2;
	m->sg40_mask3	= SG4_MASK3;
	m->sg40_shift3	= SG4_SHIFT3;
	m->sg40_addr1	= SG4_ADDR1;
	m->sg40_addr2	= SG4_ADDR2;
	m->pg_v		= PG_V;
	m->pg_frame	= PG_FRAME;

	/*
	 * Initialize pointer to kernel segment table.
	 */
	m->sysseg_pa = (u_int32_t)(pmap_kernel()->pm_stpa);

	/*
	 * Initialize relocation value such that:
	 *
	 *	pa = (va - KERNBASE) + reloc
	 */
	m->reloc = load_addr;

	/*
	 * Define the end of the relocatable range.
	 */
	m->relocend = (u_int32_t)&end;

	/*
	 * mac68k has multiple RAM segments on some models.
	 */
	for (i = 0; i < numranges; i++) {
		m->ram_segs[i].start = low[i];
		m->ram_segs[i].size  = high[i] - low[i];
	}
}

/*
 * Compute the size of the machine-dependent crash dump header.
 * Returns size in disk blocks.
 */

#define CHDRSIZE (ALIGN(sizeof(kcore_seg_t)) + ALIGN(sizeof(cpu_kcore_hdr_t)))
#define MDHDRSIZE roundup(CHDRSIZE, dbtob(1))

int
cpu_dumpsize(void)
{

	return btodb(MDHDRSIZE);
}

/*
 * Called by dumpsys() to dump the machine-dependent header.
 */
int
cpu_dump(int (*dump)(dev_t, daddr_t, caddr_t, size_t), daddr_t *blknop)
{
	int buf[MDHDRSIZE / sizeof(int)];
	cpu_kcore_hdr_t *chdr;
	kcore_seg_t *kseg;
	int error;

	kseg = (kcore_seg_t *)buf;
	chdr = (cpu_kcore_hdr_t *)&buf[ALIGN(sizeof(kcore_seg_t)) /
	    sizeof(int)];

	/* Create the segment header. */
	CORE_SETMAGIC(*kseg, KCORE_MAGIC, MID_MACHINE, CORE_CPU);
	kseg->c_size = MDHDRSIZE - ALIGN(sizeof(kcore_seg_t));

	bcopy(&cpu_kcore_hdr, chdr, sizeof(cpu_kcore_hdr_t));
	error = (*dump)(dumpdev, *blknop, (caddr_t)buf, sizeof(buf));
	*blknop += btodb(sizeof(buf));
	return (error);
}

/*
 * These variables are needed by /sbin/savecore
 */
u_int32_t dumpmag = 0x8fca0101;	/* magic number */
int	dumpsize = 0;		/* pages */
long	dumplo = 0;		/* blocks */

/*
 * This is called by main to set dumplo and dumpsize.
 * Dumps always skip the first PAGE_SIZE of disk space in
 * case there might be a disk label stored there.  If there
 * is extra space, put dump at the end to reduce the chance
 * that swapping trashes it.
 */
void
cpu_dumpconf(void)
{
	cpu_kcore_hdr_t *h = &cpu_kcore_hdr;
	struct m68k_kcore_hdr *m = &h->un._m68k;
	const struct bdevsw *bdev;
	int chdrsize;	/* size of dump header */
	int nblks;	/* size of dump area */
	int i;

	if (dumpdev == NODEV)
		return;

	bdev = bdevsw_lookup(dumpdev);
	if (bdev == NULL) {
		dumpdev = NODEV;
		return;
	}
	if (bdev->d_psize == NULL)
		return;
	nblks = (*bdev->d_psize)(dumpdev);
	chdrsize = cpu_dumpsize();

	dumpsize = 0;
	for (i = 0; m->ram_segs[i].size && i < M68K_NPHYS_RAM_SEGS; i++)
		dumpsize += btoc(m->ram_segs[i].size);

	/*
	 * Check to see if we will fit.  Note we always skip the
	 * first PAGE_SIZE in case there is a disk label there.
	 */
	if (nblks < (ctod(dumpsize) + chdrsize + ctod(1))) {
		dumpsize = 0;
		dumplo = -1;
		return;
	}

	/*
	 * Put dump at the end of the partition.
	 */
	dumplo = (nblks - 1) - ctod(dumpsize) - chdrsize;
}

void
dumpsys(void)
{
	cpu_kcore_hdr_t *h = &cpu_kcore_hdr;
	struct m68k_kcore_hdr *m = &h->un._m68k;
	const struct bdevsw *bdev;
	daddr_t blkno;		/* current block to write */
				/* dump routine */
	int (*dump)(dev_t, daddr_t, caddr_t, size_t);
	int pg;			/* page being dumped */
	paddr_t maddr;		/* PA being dumped */
	int seg;		/* RAM segment being dumped */
	int error;		/* error code from (*dump)() */

	/* XXX initialized here because of gcc lossage */
	seg = 0;
	maddr = m->ram_segs[seg].start;
	pg = 0;

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
	if (dumplo <= 0) {
		printf("\ndump to dev %u,%u not possible\n", major(dumpdev),
		    minor(dumpdev));
		return;
	}
	dump = bdev->d_dump;
	blkno = dumplo;

	printf("\ndumping to dev %u,%u offset %ld\n", major(dumpdev),
	    minor(dumpdev), dumplo);

	printf("dump ");

	/* Write the dump header. */
	error = cpu_dump(dump, &blkno);
	if (error)
		goto bad;

	for (pg = 0; pg < dumpsize; pg++) {
#define NPGMB	(1024*1024/PAGE_SIZE)
		/* print out how many MBs we have dumped */
		if (pg && (pg % NPGMB) == 0)
			printf("%d ", pg / NPGMB);
#undef NPGMB
		while (maddr >=
		    (m->ram_segs[seg].start + m->ram_segs[seg].size)) {
			if (++seg >= M68K_NPHYS_RAM_SEGS ||
			    m->ram_segs[seg].size == 0) {
				error = EINVAL;		/* XXX ?? */
				goto bad;
			}
			maddr = m->ram_segs[seg].start;
		}
		pmap_enter(pmap_kernel(), (vaddr_t)vmmap, maddr,
		    VM_PROT_READ, VM_PROT_READ|PMAP_WIRED);
		pmap_update(pmap_kernel());

		error = (*dump)(dumpdev, blkno, vmmap, PAGE_SIZE);
 bad:
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

/*
 * Return the best possible estimate of the time in the timeval
 * to which tvp points.  We do this by returning the current time
 * plus the amount of time since the last clock interrupt (clock.c:clkread).
 *
 * Check that this time is no less than any previously-reported time,
 * which could happen around the time of a clock adjustment.  Just for fun,
 * we guarantee that the time will be greater than the value obtained by a
 * previous call.
 */
void
microtime(struct timeval *tvp)
{
	int s = splhigh();
	static struct timeval lasttime;

	*tvp = time;
	tvp->tv_usec += clkread();
	while (tvp->tv_usec >= 1000000) {
		tvp->tv_sec++;
		tvp->tv_usec -= 1000000;
	}
	if (tvp->tv_sec == lasttime.tv_sec &&
	    tvp->tv_usec <= lasttime.tv_usec &&
	    (tvp->tv_usec = lasttime.tv_usec + 1) >= 1000000) {
		tvp->tv_sec++;
		tvp->tv_usec -= 1000000;
	}
	lasttime = *tvp;
	splx(s);
}

void straytrap(int, int);

void
straytrap(int pc, int evec)
{
	printf("unexpected trap; vector offset 0x%x from 0x%x.\n",
	    (int)(evec & 0xfff), pc);
#ifdef DDB
	Debugger();
#endif
}

/*
 * Level 7 interrupts can be caused by the keyboard or parity errors.
 */
void	nmihand(struct frame);

void
nmihand(struct frame frame)
{
	static int nmihanddeep = 0;

	if (nmihanddeep++)
		return;
/*	regdump((struct trapframe *)&frame, 128);
	dumptrace(); */
#ifdef DDB
	printf("Panic switch: PC is 0x%x.\n", frame.f_pc);
	Debugger();
#endif
	nmihanddeep = 0;
}

/*
 * It should be possible to probe for the top of RAM, but Apple has
 * memory structured so that in at least some cases, it's possible
 * for RAM to be aliased across all memory--or for it to appear that
 * there is more RAM than there really is.
 */
int	get_top_of_ram(void);

int
get_top_of_ram(void)
{
	return ((mac68k_machine.mach_memsize * (1024 * 1024)) - 4096);
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

int
cpu_exec_aout_makecmds(struct lwp *l, struct exec_package *epp)
{
	int error = ENOEXEC;

#ifdef COMPAT_NOMID
	/* Check to see if MID == 0. */
	if (((struct exec *)epp->ep_hdr)->a_midmag == ZMAGIC)
		return exec_aout_prep_oldzmagic(l->l_proc, epp);
#endif

	return error;
}

static char *envbuf = NULL;

/*
 * getenvvars: Grab a few useful variables
 */

void
getenvvars(u_long flag, char *buf)
{
	extern u_long bootdev;
	extern u_long macos_boottime, MacOSROMBase;
	extern long macos_gmtbias;
	int root_scsi_id;
	u_long root_ata_dev;
#ifdef	__ELF__
	int i;
	Elf_Ehdr *ehdr;
	Elf_Shdr *shp;
	vaddr_t minsym;
#endif

	/*
	 * If flag & 0x80000000 == 0, then we're booting with the old booter
	 * and we should freak out.
	 */
	if ((flag & 0x80000000) == 0) {
		/* Freak out; print something if that becomes available */
	} else
		envbuf = buf;

	/* These next two should give us mapped video & serial */
	/* We need these for pre-mapping graybars & echo, but probably */
	/* only on MacII or LC.  --  XXX */
	/* videoaddr = getenv("MACOS_VIDEO"); */

	/*
	 * The following are not in a structure so that they can be
	 * accessed more quickly.
	 */
	videoaddr = getenv("VIDEO_ADDR");
	videorowbytes = getenv("ROW_BYTES");
	videobitdepth = getenv("SCREEN_DEPTH");
	videosize = getenv("DIMENSIONS");

	/*
	 * More misc stuff from booter.
	 */
	mac68k_machine.machineid = getenv("MACHINEID");
	mac68k_machine.mach_processor = getenv("PROCESSOR");
	mac68k_machine.mach_memsize = getenv("MEMSIZE");
	mac68k_machine.do_graybars = getenv("GRAYBARS");
	mac68k_machine.serial_boot_echo = getenv("SERIALECHO");
	mac68k_machine.serial_console = getenv("SERIALCONSOLE");

	mac68k_machine.modem_flags = getenv("SERIAL_MODEM_FLAGS");
	mac68k_machine.modem_cts_clk = getenv("SERIAL_MODEM_HSKICLK");
	mac68k_machine.modem_dcd_clk = getenv("SERIAL_MODEM_GPICLK");
	mac68k_machine.modem_d_speed = getenv("SERIAL_MODEM_DSPEED");
	mac68k_machine.print_flags = getenv("SERIAL_PRINT_FLAGS");
	mac68k_machine.print_cts_clk = getenv("SERIAL_PRINT_HSKICLK");
	mac68k_machine.print_dcd_clk = getenv("SERIAL_PRINT_GPICLK");
	mac68k_machine.print_d_speed = getenv("SERIAL_PRINT_DSPEED");
	mac68k_machine.booter_version = getenv("BOOTERVER");

	/*
	 * For now, we assume that the boot device is off the first controller.
	 * Booter versions 1.11.0 and later set a flag to tell us to construct
	 * bootdev using the SCSI ID passed in via the environment.
	 */
	root_scsi_id = getenv("ROOT_SCSI_ID");
	root_ata_dev = getenv("ROOT_ATA_DEV");
	if (((mac68k_machine.booter_version < CURRENTBOOTERVER) ||
	    (flag & 0x40000)) && bootdev == 0) {
		if (root_ata_dev) {
			/*
			 * Consider only internal IDE drive.
			 * Buses(=channel) will be always 0.
			 * Because 68k Mac has only single channel.
			 */
			switch (root_ata_dev) {
			default: /* fall through */
			case 0xffffffe0: /* buses,drive = 0,0 */
			case 0x20: /* buses,drive = 1,0 */
			case 0x21: /* buses,drive = 1,1 */
				bootdev = MAKEBOOTDEV(22, 0, 0, 0, 0);
				break;
			case 0xffffffe1: /* buses,drive = 0,1 */
				bootdev = MAKEBOOTDEV(22, 0, 0, 1, 0);
				break;
			}
		} else {
			bootdev = MAKEBOOTDEV(4, 0, 0, root_scsi_id, 0);
		}
	}

	/*
	 * Booter 1.11.3 and later pass a BOOTHOWTO variable with the
	 * appropriate bits set.
	 */
	boothowto = getenv("BOOTHOWTO");
	if (boothowto == 0)
		boothowto = getenv("SINGLE_USER");

	/*
	 * Get end of symbols for kernel debugging
	 */
	esym = (int *)getenv("END_SYM");
#ifndef SYMTAB_SPACE
	if (esym == (int *)0)
#endif
		esym = (int *)&end;

	/* Get MacOS time */
	macos_boottime = getenv("BOOTTIME");

	/* Save GMT BIAS saved in Booter parameters dialog box */
	macos_gmtbias = getenv("GMTBIAS");

	/*
	 * Save globals stolen from MacOS
	 */

	ROMBase = (caddr_t)getenv("ROMBASE");
	if (ROMBase == (caddr_t)0) {
		ROMBase = (caddr_t)ROMBASE;
	}
	MacOSROMBase = (unsigned long)ROMBase;
	TimeDBRA = getenv("TIMEDBRA");
	ADBDelay = (u_short)getenv("ADBDELAY");
	HwCfgFlags  = getenv("HWCFGFLAGS");
	HwCfgFlags2 = getenv("HWCFGFLAG2");
	HwCfgFlags3 = getenv("HWCFGFLAG3");
 	ADBReInit_JTBL = getenv("ADBREINIT_JTBL");
 	mrg_ADBIntrPtr = (caddr_t)getenv("ADBINTERRUPT");

#ifdef	__ELF__
	/*
	 * Check the ELF headers.
	 */

	ehdr = (void *)getenv("MARK_SYM");
	if (memcmp(ehdr->e_ident, ELFMAG, SELFMAG) != 0 ||
	    ehdr->e_ident[EI_CLASS] != ELFCLASS32) {
		return;
	}

	/*
	 * Find the end of the symbols and strings.
	 */

	minsym = ~0;
	shp = (Elf_Shdr *)(end + ehdr->e_shoff);
	for (i = 0; i < ehdr->e_shnum; i++) {
		if (shp[i].sh_type != SHT_SYMTAB &&
		    shp[i].sh_type != SHT_STRTAB) {
			continue;
		}
		minsym = MIN(minsym, (vaddr_t)end + shp[i].sh_offset);
	}

	symsize = 1;
	ssym = (int *)ehdr;
#else
	symsize = *(int *)&end;
	ssym = ((int *)&end) + 1;
#endif
}

static long
getenv(const char *str)
{
	/*
	 * Returns the value of the environment variable "str".
	 *
	 * Format of the buffer is "var=val\0var=val\0...\0var=val\0\0".
	 *
	 * Returns 0 if the variable is not there, and 1 if the variable is
	 * there without an "=val".
	 */

	char *s;
	const char *s1;
	int val, base;

	s = envbuf;
	while (1) {
		for (s1 = str; *s1 && *s && *s != '='; s1++, s++) {
			if (toupper(*s1) != toupper(*s)) {
				break;
			}
		}
		if (*s1) {	/* No match */
			while (*s) {
				s++;
			}
			s++;
			if (*s == '\0') {	/* Not found */
				/* Boolean flags are FALSE (0) if not there */
				return 0;
			}
			continue;
		}
		if (*s == '=') {/* Has a value */
			s++;
			val = 0;
			base = 10;
			if (*s == '0' && (*(s + 1) == 'x' || *(s + 1) == 'X')) {
				base = 16;
				s += 2;
			} else
				if (*s == '0') {
					base = 8;
				}
			while (*s) {
				if (toupper(*s) >= 'A' && toupper(*s) <= 'F') {
					val = val * base + toupper(*s) - 'A' + 10;
				} else {
					val = val * base + (*s - '0');
				}
				s++;
			}
			return val;
		} else {	/* TRUE (1) */
			return 1;
		}
	}
}

/*
 * ROM Vector information for calling drivers in ROMs
 *
 * According to information published on the Web by Apple, there have
 * been 9 different ROM families used in the Mac since the introduction
 * of the Lisa/XL through the latest PowerMacs (May 96).  Each family
 * has zero or more version variants and in some cases a version variant
 * may exist in one than one length format.  Generally any one specific
 * Mac will use a common set of routines within the ROM and a model-specific
 * set also in the ROM.  Luckily most of the routines used by NetBSD fall
 * into the common set and can therefore be defined in the ROM Family.
 * The offset addresses (address minus the ROM Base) of these common routines
 * is the same for all machines which use that ROM.  The offset addresses of
 * the machine-specific routines is generally different for each machine.
 * The machine-specific routines currently used by NetBSD/mac68k include:
 *       ADB_interrupt, PM_interrpt, ADBBase+130_interrupt,
 *       PMgrOp, jClkNoMem, Egret, InitEgret, and ADBReInit_JTBL
 *
 * It is possible that the routine at "jClkNoMem" is a common routine, but
 * some variation in addresses has been seen.  Also, execept for the very
 * earliest machines which used Egret, the machine-specific value of the
 * Egret routine may be unimportant as the machine-specific InitEgret code
 * seems to always set the OS Trap vector for Egret.
 *
 * Only three of the nine different ROMs are important to NetBSD/mac68k.
 * All other ROMs are used in early model Macs which are unable to run
 * NetBSD due to other hardware limitations such as 68000 CPU, no MMU
 * capability, or used only in PowerMacs.  The three that we are interested
 * in are:
 *
 * ROM Family $0178 - used in the II, IIx, IIcx, and SE/30
 *            All machines which use this ROM are now supported by NetBSD.
 *            There are no machine-dependent routines in these ROMs used by
 *            NetBSD/mac68k.  This ROM is always 256K in length.
 *
 * ROM Family $067c - used in Classic, Color Classic, Color Classic II,
 *                      IIci, IIsi, IIvi, IIvx, IIfx, LC, LC II, LC III,
 *                      LC III+, LC475, LC520, LC550, LC575, LC580, LC630,
 *                      MacTV, P200, P250, P275, P400/405/410/430, P450,
 *                      P460/466/467, P475/476, P520, P550/560, P575/577/578,
 *                      P580/588, P600, P630/631/635/636/637/638/640, Q605,
 *                      Q610, C610, Q630, C650, Q650, Q700, Q800, Q900, Q950,
 *                      PB140, PB145/145B, PB150, PB160, PB165, PB165c, PB170,
 *                      PB180, PB180c, Duo 210, Duo 230, Duo 250, Duo 270c,
 *                      Duo280, Duo 280c, PB 520/520c/540/540c/550
 *             This is the so-called "Universal" ROM used in almost all 68K
 *             machines. There are machine-dependent and machine-independent
 *             routines used by NetBSD/mac68k in this ROM, and except for the
 *             PowerBooks and the Duos, this ROM seems to be fairly well
 *             known by NetBSD/mac68k.  Desktop machines listed here that are
 *             not yet running NetBSD probably only lack the necessary
 *             addresses for the machine-dependent routines, or are waiting
 *             for IDE disk support.  This ROM is generally 1Meg in length,
 *             however when used in the IIci, IIfx, IIsi, LC, Classic II, and
 *             P400/405/410/430 it is 512K in length, and when used in the
 *             PB 520/520c/540/540c/550 it is 2Meg in length.
 *
 * ROM Family - $077d - used in C660AV/Q660AV, Q840AV
 *             The "Universal" ROM used on the PowerMacs and used in the
 *             68K line for the AV Macs only.  When used in the 68K AV
 *             machines the ROM is 2Meg in length; all uses in the PowerMac
 *             use a length of 4Meg.
 *
 *		Bob Nestor - <rnestor@metronet.com>
 */
static romvec_t romvecs[] =
{
	/* Vectors verified for II, IIx, IIcx, SE/30 */
	{			/* 0 */
		"Mac II class ROMs",
		(caddr_t)0x40807002,	/* where does ADB interrupt */
		(caddr_t)0x0,		/* PM interrupt (?) */
		(caddr_t)0x4080a4d8,	/* ADBBase + 130 interrupt; whatzit? */
		(caddr_t)0x40807778,	/* CountADBs */
		(caddr_t)0x40807792,	/* GetIndADB */
		(caddr_t)0x408077be,	/* GetADBInfo */
		(caddr_t)0x408077c4,	/* SetADBInfo */
		(caddr_t)0x40807704,	/* ADBReInit */
		(caddr_t)0x408072fa,	/* ADBOp */
		(caddr_t)0x0,		/* PMgrOp */
		(caddr_t)0x4080d6d0,	/* WriteParam */
		(caddr_t)0x4080d6fa,	/* SetDateTime */
		(caddr_t)0x4080dbe8,	/* InitUtil */
		(caddr_t)0x4080dd78,	/* ReadXPRam */
		(caddr_t)0x4080dd82,	/* WriteXPRam */
		(caddr_t)0x4080ddd6,	/* jClkNoMem */
		(caddr_t)0x0,		/* ADBAlternateInit */
		(caddr_t)0x0,		/* Egret */
		(caddr_t)0x0,		/* InitEgret */
		(caddr_t)0x0,		/* ADBReInit_JTBL */
		(caddr_t)0x0,		/* ROMResourceMap List Head */
		(caddr_t)0x40814c58,	/* FixDiv */
		(caddr_t)0x40814b64,	/* FixMul */
	},
	/*
	 * Vectors verified for PB 140, PB 145, PB 170
	 * (PB 100?)
	 */
	{			/* 1 */
		"Powerbook class ROMs",
		(caddr_t)0x4088ae5e,	/* ADB interrupt */
		(caddr_t)0x408885ec,	/* PB ADB interrupt */
		(caddr_t)0x4088ae0e,	/* ADBBase + 130 interrupt; whatzit? */
		(caddr_t)0x4080a360,	/* CountADBs */
		(caddr_t)0x4080a37a,	/* GetIndADB */
		(caddr_t)0x4080a3a6,	/* GetADBInfo */
		(caddr_t)0x4080a3ac,	/* SetADBInfo */
		(caddr_t)0x4080a752,	/* ADBReInit */
		(caddr_t)0x4080a3dc,	/* ADBOp */
		(caddr_t)0x408888ec,	/* PMgrOp */
		(caddr_t)0x4080c05c,	/* WriteParam */
		(caddr_t)0x4080c086,	/* SetDateTime */
		(caddr_t)0x4080c5cc,	/* InitUtil */
		(caddr_t)0x4080b186,	/* ReadXPRam */
		(caddr_t)0x4080b190,	/* WriteXPRam */
		(caddr_t)0x4080b1e4,	/* jClkNoMem */
		(caddr_t)0x4080a818,	/* ADBAlternateInit */
		(caddr_t)0x40814800,	/* Egret */
		(caddr_t)0x408147c4,	/* InitEgret */
		(caddr_t)0x0,		/* ADBReInit_JTBL */
		(caddr_t)0x4087eb90,	/* ROMResourceMap List Head */
		(caddr_t)0x4081c406,	/* FixDiv */
		(caddr_t)0x4081c312,	/* FixMul */
	},
	/*
	 * Vectors verified for IIsi, IIvx, IIvi
	 */
	{			/* 2 */
		"Mac IIsi class ROMs",
		(caddr_t)0x40814912,	/* ADB interrupt */
		(caddr_t)0x0,		/* PM ADB interrupt */
		(caddr_t)0x408150f0,	/* ADBBase + 130 interrupt; whatzit? */
		(caddr_t)0x4080a360,	/* CountADBs */
		(caddr_t)0x4080a37a,	/* GetIndADB */
		(caddr_t)0x4080a3a6,	/* GetADBInfo */
		(caddr_t)0x4080a3ac,	/* SetADBInfo */
		(caddr_t)0x4080a752,	/* ADBReInit */
		(caddr_t)0x4080a3dc,	/* ADBOp */
		(caddr_t)0x0,		/* PMgrOp */
		(caddr_t)0x4080c05c,	/* WriteParam */
		(caddr_t)0x4080c086,	/* SetDateTime */
		(caddr_t)0x4080c5cc,	/* InitUtil */
		(caddr_t)0x4080b186,	/* ReadXPRam */
		(caddr_t)0x4080b190,	/* WriteXPRam */
		(caddr_t)0x4080b1e4,	/* jClkNoMem */
		(caddr_t)0x4080a818,	/* ADBAlternateInit */
		(caddr_t)0x40814800,	/* Egret */
		(caddr_t)0x408147c4,	/* InitEgret */
		(caddr_t)0x0,		/* ADBReInit_JTBL */
		(caddr_t)0x4087eb90,	/* ROMResourceMap List Head */
		(caddr_t)0x4081c406,	/* FixDiv */
		(caddr_t)0x4081c312,	/* FixMul */
	},
	/*
	 * Vectors verified for Mac Classic II and LC II
	 * (Other LC's?  680x0 Performas?)
	 */
	{			/* 3 */
		"Mac Classic II ROMs",
		(caddr_t)0x40a14912,	/* ADB interrupt */
		(caddr_t)0x0,		/* PM ADB interrupt */
		(caddr_t)0x40a150f0,	/* ADBBase + 130 interrupt; whatzit? */
		(caddr_t)0x40a0a360,	/* CountADBs */
		(caddr_t)0x40a0a37a,	/* GetIndADB */
		(caddr_t)0x40a0a3a6,	/* GetADBInfo */
		(caddr_t)0x40a0a3ac,	/* SetADBInfo */
		(caddr_t)0x40a0a752,	/* ADBReInit */
		(caddr_t)0x40a0a3dc,	/* ADBOp */
		(caddr_t)0x0,		/* PMgrOp */
		(caddr_t)0x40a0c05c,	/* WriteParam */
		(caddr_t)0x40a0c086,	/* SetDateTime */
		(caddr_t)0x40a0c5cc,	/* InitUtil */
		(caddr_t)0x40a0b186,	/* ReadXPRam */
		(caddr_t)0x40a0b190,	/* WriteXPRam */
		(caddr_t)0x40a0b1e4,	/* jClkNoMem */
		(caddr_t)0x40a0a818,	/* ADBAlternateInit */
		(caddr_t)0x40a14800,	/* Egret */
		(caddr_t)0x40a147c4,	/* InitEgret */
		(caddr_t)0x40a03ba6,	/* ADBReInit_JTBL */
		(caddr_t)0x40a7eb90,	/* ROMResourceMap List Head */
		(caddr_t)0x40a1c406,	/* FixDiv, wild guess */
		(caddr_t)0x40a1c312,	/* FixMul, wild guess */
	},
	/*
	 * Vectors verified for IIci, Q700
	 */
	{			/* 4 */
		"Mac IIci/Q700 ROMs",
		(caddr_t)0x4080a700,	/* ADB interrupt */
		(caddr_t)0x0,		/* PM ADB interrupt */
		(caddr_t)0x4080a5aa,	/* ADBBase + 130 interrupt; whatzit? */
		(caddr_t)0x4080a360,	/* CountADBs */
		(caddr_t)0x4080a37a,	/* GetIndADB */
		(caddr_t)0x4080a3a6,	/* GetADBInfo */
		(caddr_t)0x4080a3ac,	/* SetADBInfo */
		(caddr_t)0x4080a752,	/* ADBReInit */
		(caddr_t)0x4080a3dc,	/* ADBOp */
		(caddr_t)0x0,		/* PMgrOp */
		(caddr_t)0x4080c05c,	/* WriteParam */
		(caddr_t)0x4080c086,	/* SetDateTime */
		(caddr_t)0x4080c5cc,	/* InitUtil */
		(caddr_t)0x4080b186,	/* ReadXPRam */
		(caddr_t)0x4080b190,	/* WriteXPRam */
		(caddr_t)0x4080b1e4,	/* jClkNoMem */
		(caddr_t)0x4080a818,	/* ADBAlternateInit */
		(caddr_t)0x0,		/* Egret */
		(caddr_t)0x408147c4,	/* InitEgret */
		(caddr_t)0x0,		/* ADBReInit_JTBL */
		(caddr_t)0x4087eb90,	/* ROMResourceMap List Head */
		(caddr_t)0x4081c406,	/* FixDiv */
		(caddr_t)0x4081c312,	/* FixMul */
	},
	/*
	 * Vectors verified for Duo 230, PB 180, PB 160, PB 165/165C
	 * (Duo 210?  Duo 250?  Duo 270?)
	 */
	{			/* 5 */
		"2nd Powerbook class ROMs",
		(caddr_t)0x408b2eec,	/* ADB interrupt */
		(caddr_t)0x408885ec,	/* PB ADB interrupt */
		(caddr_t)0x408b2e76,	/* ADBBase + 130 interrupt; whatzit? */
		(caddr_t)0x4080a360,	/* CountADBs */
		(caddr_t)0x4080a37a,	/* GetIndADB */
		(caddr_t)0x4080a3a6,	/* GetADBInfo */
		(caddr_t)0x4080a3ac,	/* SetADBInfo */
		(caddr_t)0x4080a752,	/* ADBReInit */
		(caddr_t)0x4080a3dc,	/* ADBOp */
		(caddr_t)0x408888ec,	/* PMgrOp */
		(caddr_t)0x4080c05c,	/* WriteParam */
		(caddr_t)0x4080c086,	/* SetDateTime */
		(caddr_t)0x4080c5cc,	/* InitUtil */
		(caddr_t)0x4080b186,	/* ReadXPRam */
		(caddr_t)0x4080b190,	/* WriteXPRam */
		(caddr_t)0x408b39b2,	/* jClkNoMem */	/* From PB180 */
		(caddr_t)0x4080a818,	/* ADBAlternateInit */
		(caddr_t)0x40814800,	/* Egret */
		(caddr_t)0x40888400,	/* InitPwrMgr */ /* From PB180 */
		(caddr_t)0x408cce28,	/* ADBReInit_JTBL -- from PB160*/
		(caddr_t)0x4087eb90,	/* ROMRsrcMap List Head -- from PB160*/
		(caddr_t)0x4081c406,	/* FixDiv, wild guess */
		(caddr_t)0x4081c312,	/* FixMul, wild guess */
	},
	/*
	 * Vectors verified for the Quadra, Centris 650
	 *  (610, Q800?)
	 */
	{			/* 6 */
		"Quadra/Centris ROMs",
		(caddr_t)0x408b2dea,	/* ADB int */
		(caddr_t)0x0,		/* PM intr */
 		(caddr_t)0x408b2c72,	/* ADBBase + 130 */
		(caddr_t)0x4080a360,	/* CountADBs */
		(caddr_t)0x4080a37a,	/* GetIndADB */
		(caddr_t)0x4080a3a6,	/* GetADBInfo */
		(caddr_t)0x4080a3ac,	/* SetADBInfo */
		(caddr_t)0x4080a752,	/* ADBReInit */
		(caddr_t)0x4080a3dc,	/* ADBOp */
		(caddr_t)0x40809ae6,	/* PMgrOp */
		(caddr_t)0x4080c05c,	/* WriteParam */
		(caddr_t)0x4080c086,	/* SetDateTime */
		(caddr_t)0x4080c5cc,	/* InitUtil */
		(caddr_t)0x4080b186,	/* ReadXPRam */
		(caddr_t)0x4080b190,	/* WriteXPRam */
		(caddr_t)0x408b39b6,	/* jClkNoMem */
		(caddr_t)0x4080a818,	/* ADBAlternateInit */
		(caddr_t)0x40814800,	/* Egret */
		(caddr_t)0x408147c4,	/* InitEgret */
		(caddr_t)0x408d2b64,	/* ADBReInit_JTBL */
		(caddr_t)0x4087eb90,	/* ROMResourceMap List Head */
		(caddr_t)0x4081c406,	/* FixDiv, wild guess */
		(caddr_t)0x4081c312,	/* FixMul, wild guess */
	},
	/*
	 * Vectors verified for the Quadra 660AV
	 *  (Quadra 840AV?)
	 */
	{			/* 7 */
		"Quadra AV ROMs",
		(caddr_t)0x4080cac6,	/* ADB int */
		(caddr_t)0x0,		/* PM int */
		(caddr_t)0x40805cd4,	/* ADBBase + 130 */
		(caddr_t)0x40839600,	/* CountADBs */
		(caddr_t)0x4083961a,	/* GetIndADB */
		(caddr_t)0x40839646,	/* GetADBInfo */
		(caddr_t)0x4083964c,	/* SetADBInfo */
		(caddr_t)0x408397b8,	/* ADBReInit */
		(caddr_t)0x4083967c,	/* ADBOp */
		(caddr_t)0x0,		/* PMgrOp */
		(caddr_t)0x4081141c,	/* WriteParam */
		(caddr_t)0x4081144e,	/* SetDateTime */
		(caddr_t)0x40811930,	/* InitUtil */
		(caddr_t)0x4080b624,	/* ReadXPRam */
		(caddr_t)0x4080b62e,	/* WriteXPRam */
		(caddr_t)0x40806884,	/* jClkNoMem */
		(caddr_t)0x408398c2,	/* ADBAlternateInit */
		(caddr_t)0x4080cada,	/* Egret */
		(caddr_t)0x4080de14,	/* InitEgret */
		(caddr_t)0x408143b8,	/* ADBReInit_JTBL */
		(caddr_t)0x409bdb60,	/* ROMResourceMap List Head */
		(caddr_t)0x4083b3d8,	/* FixDiv */
		(caddr_t)0x4083b2e4,	/* FixMul */
	},
	/*
	 * PB 540, PB 550
	 * (PB 520?  Duo 280?)
	 */
	{			/* 8 */
		"68040 PowerBook ROMs",
		(caddr_t)0x400b2efc,	/* ADB int */
		(caddr_t)0x400d8e66,	/* PM int */
		(caddr_t)0x400b2e86,	/* ADBBase + 130 */
		(caddr_t)0x4000a360,	/* CountADBs */
		(caddr_t)0x4000a37a,	/* GetIndADB */
		(caddr_t)0x4000a3a6,	/* GetADBInfo */
		(caddr_t)0x4000a3ac,	/* SetADBInfo */
		(caddr_t)0x4000a752,	/* ADBReInit */
		(caddr_t)0x4000a3dc,	/* ADBOp */
		(caddr_t)0x400d9302,	/* PmgrOp */
		(caddr_t)0x4000c05c,	/* WriteParam */
		(caddr_t)0x4000c086,	/* SetDateTime */
		(caddr_t)0x4000c5cc,	/* InitUtil */
		(caddr_t)0x4000b186,	/* ReadXPRam */
		(caddr_t)0x4000b190,	/* WriteXPRam */
		(caddr_t)0x400b3c08,	/* jClkNoMem */
		(caddr_t)0x4000a818,	/* ADBAlternateInit */
		(caddr_t)0x40009ae6,	/* Egret */ /* From PB520 */
		(caddr_t)0x400147c4,	/* InitEgret */
		(caddr_t)0x400a7a5c,	/* ADBReInit_JTBL */
		(caddr_t)0x4007eb90,	/* ROMResourceMap List Head */
		(caddr_t)0x4001c406,	/* FixDiv, wild guess */
		(caddr_t)0x4001c312,	/* FixMul, wild guess */
	},
	/*
	 * Verified for the Q605
	 */
	{			/* 9 */
		"Quadra/Centris 605 ROMs",
		(caddr_t)0x408a9b56,	/* ADB int */
		(caddr_t)0x0,		/* PM int */
		(caddr_t)0x408b2f94,	/* ADBBase + 130 */
		(caddr_t)0x4080a360,	/* CountADBs */
		(caddr_t)0x4080a37a,	/* GetIndADB */
		(caddr_t)0x4080a3a6,	/* GetADBInfo */
		(caddr_t)0x4080a3ac,	/* SetADBInfo */
		(caddr_t)0x4080a752,	/* ADBReInit */
		(caddr_t)0x4080a3dc,	/* ADBOp */
		(caddr_t)0x0,		/* PmgrOp */
		(caddr_t)0x4080c05c,	/* WriteParam */
		(caddr_t)0x4080c086,	/* SetDateTime */
		(caddr_t)0x4080c5cc,	/* InitUtil */
		(caddr_t)0x4080b186,	/* ReadXPRam */
		(caddr_t)0x4080b190,	/* WriteXPRam */
		(caddr_t)0x408b3bf8,	/* jClkNoMem */
		(caddr_t)0x4080a818,	/* ADBAlternateInit */
		(caddr_t)0x408a99c0,	/* Egret */
		(caddr_t)0x408147c4,	/* InitEgret */
		(caddr_t)0x408a82c0,	/* ADBReInit_JTBL */
		(caddr_t)0x4087eb90,	/* ROMResourceMap List Head */
		(caddr_t)0x4081c406,	/* FixDiv */
		(caddr_t)0x4081c312,	/* FixMul */
	},
	/*
	 * Vectors verified for Duo 270c, PB150
	 */
	{			/* 10 */
		"Duo 270C ROMs",
		(caddr_t)0x408b2efc,	/* ADB interrupt */
		(caddr_t)0x408885ec,	/* PB ADB interrupt */
		(caddr_t)0x408b2e86,	/* ADBBase + 130 interrupt; whatzit? */
		(caddr_t)0x4080a360,	/* CountADBs */
		(caddr_t)0x4080a37a,	/* GetIndADB */
		(caddr_t)0x4080a3a6,	/* GetADBInfo */
		(caddr_t)0x4080a3ac,	/* SetADBInfo */
		(caddr_t)0x4080a752,	/* ADBReInit */
		(caddr_t)0x4080a3dc,	/* ADBOp */
		(caddr_t)0x408888ec,	/* PMgrOp */
		(caddr_t)0x4080c05c,	/* WriteParam */
		(caddr_t)0x4080c086,	/* SetDateTime */
		(caddr_t)0x4080c5cc,	/* InitUtil */
		(caddr_t)0x4080b186,	/* ReadXPRam */
		(caddr_t)0x4080b190,	/* WriteXPRam */
		(caddr_t)0x408b3bf8,	/* jClkNoMem */ /* from PB 150 */
		(caddr_t)0x4080a818,	/* ADBAlternateInit */
		(caddr_t)0x40814800,	/* Egret */
		(caddr_t)0x408147c4,	/* InitEgret */
		(caddr_t)0x0,		/* ADBReInit_JTBL */
		(caddr_t)0x4087eb90,	/* ROMResourceMap List Head */
		(caddr_t)0x4081c406,	/* FixDiv, wild guess */
		(caddr_t)0x4081c312,	/* FixMul, wild guess */
	},
	/*
	 * Vectors verified for Performa/LC 550
	 */
	{			/* 11 */
		"P/LC 550 ROMs",
		(caddr_t)0x408d16d6,	/* ADB interrupt */
		(caddr_t)0x0,		/* PB ADB interrupt */
		(caddr_t)0x408b2f84,	/* ADBBase + 130 interrupt; whatzit? */
		(caddr_t)0x4080a360,	/* CountADBs */
		(caddr_t)0x4080a37a,	/* GetIndADB */
		(caddr_t)0x4080a3a6,	/* GetADBInfo */
		(caddr_t)0x4080a3ac,	/* SetADBInfo */
		(caddr_t)0x4080a752,	/* ADBReInit */
		(caddr_t)0x4080a3dc,	/* ADBOp */
		(caddr_t)0x0,		/* PMgrOp */
		(caddr_t)0x4080c05c,	/* WriteParam */
		(caddr_t)0x4080c086,	/* SetDateTime */
		(caddr_t)0x4080c5cc,	/* InitUtil */
		(caddr_t)0x4080b186,	/* ReadXPRam */
		(caddr_t)0x4080b190,	/* WriteXPRam */
		(caddr_t)0x408b3c04,	/* jClkNoMem */
		(caddr_t)0x4080a818,	/* ADBAlternateInit */
		(caddr_t)0x408d1450,	/* Egret */
		(caddr_t)0x408147c4,	/* InitEgret */
		(caddr_t)0x408d24a4,	/* ADBReInit_JTBL */
		(caddr_t)0x4087eb90,	/* ROMResourceMap List Head */
		(caddr_t)0x4081c406,	/* FixDiv for P550 */
		(caddr_t)0x4081c312,	/* FixMul for P550 */
	},
	/*
	 * Vectors verified for the MacTV
	 */
	{			/* 12 */
		"MacTV ROMs",
		(caddr_t)0x40acfed6,	/* ADB interrupt */
		(caddr_t)0x0,		/* PB ADB interrupt */
		(caddr_t)0x40ab2f84,	/* ADBBase + 130 interrupt; whatzit? */
		(caddr_t)0x40a0a360,	/* CountADBs */
		(caddr_t)0x40a0a37a,	/* GetIndADB */	
		(caddr_t)0x40a0a3a6,	/* GetADBInfo */
		(caddr_t)0x40a0a3ac,	/* SetADBInfo */
		(caddr_t)0x40a0a752,	/* ADBReInit */
		(caddr_t)0x40a0a3dc,	/* ADBOp */
		(caddr_t)0x0,		/* PMgrOp */
		(caddr_t)0x40a0c05c,	/* WriteParam */
		(caddr_t)0x40a0c086,	/* SetDateTime */
		(caddr_t)0x40a0c5cc,	/* InitUtil */
		(caddr_t)0x40a0b186,	/* ReadXPRam */
		(caddr_t)0x40a0b190,	/* WriteXPRam */
		(caddr_t)0x40ab3bf4,	/* jClkNoMem */
		(caddr_t)0x40a0a818,	/* ADBAlternateInit */
		(caddr_t)0x40acfd40,	/* Egret */
		(caddr_t)0x40a147c4,	/* InitEgret */
		(caddr_t)0x40a038a0,	/* ADBReInit_JTBL */
		(caddr_t)0x40a7eb90,	/* ROMResourceMap List Head */
		(caddr_t)0x40a1c406,	/* FixDiv */
		(caddr_t)0x40a1c312,	/* FixMul */
	},
	/*
	 * Vectors verified for the Quadra630
	 */
	{			/* 13 */
		"Quadra630 ROMs",
		(caddr_t)0x408a9bd2,	/* ADB int */
		(caddr_t)0x0,		/* PM intr */
 		(caddr_t)0x408b2f94,	/* ADBBase + 130 */
		(caddr_t)0x4080a360,	/* CountADBs */
		(caddr_t)0x4080a37a,	/* GetIndADB */
		(caddr_t)0x4080a3a6,	/* GetADBInfo */
		(caddr_t)0x4080a3ac,	/* SetADBInfo */
		(caddr_t)0x4080a752,	/* ADBReInit */
		(caddr_t)0x4080a3dc,	/* ADBOp */
		(caddr_t)0,		/* PMgrOp */
		(caddr_t)0x4080c05c,	/* WriteParam */
		(caddr_t)0x4080c086,	/* SetDateTime */
		(caddr_t)0x4080c5cc,	/* InitUtil */
		(caddr_t)0x4080b186,	/* Wild guess at ReadXPRam */
		(caddr_t)0x4080b190,	/* Wild guess at WriteXPRam */
		(caddr_t)0x408b39f4,	/* jClkNoMem */
		(caddr_t)0x4080a818,	/* ADBAlternateInit */
		(caddr_t)0x408a99c0,	/* Egret */
		(caddr_t)0x408147c8,	/* InitEgret */
		(caddr_t)0x408a7ef8,	/* ADBReInit_JTBL */
		(caddr_t)0x4087eb90,	/* ROMResourceMap List Head */
		(caddr_t)0x4081c406,	/* FixDiv */
		(caddr_t)0x4081c312,	/* FixMul */
	},
	/*
	 * Vectors verified for LC III
	 */
	{			/* 14 */
		"LC III ROMs",
		(caddr_t)0x40814912,	/* ADB interrupt */
		(caddr_t)0x0,		/* PM ADB interrupt */
		(caddr_t)0x408b2f94,	/* ADBBase + 130 interrupt */
		(caddr_t)0x4080a360,	/* CountADBs */
		(caddr_t)0x4080a37a,	/* GetIndADB */
		(caddr_t)0x4080a3a6,	/* GetADBInfo */
		(caddr_t)0x4080a3ac,	/* SetADBInfo */
		(caddr_t)0x4080a752,	/* ADBReInit */
		(caddr_t)0x4080a3dc,	/* ADBOp */
		(caddr_t)0x0,		/* PMgrOp */
		(caddr_t)0x4080c05c,	/* WriteParam */
		(caddr_t)0x4080c086,	/* SetDateTime */
		(caddr_t)0x4080c5cc,	/* InitUtil */
		(caddr_t)0x4080b186,	/* ReadXPRam */
		(caddr_t)0x4080b190,	/* WriteXPRam */
		(caddr_t)0x408b39b6,	/* jClkNoMem */
		(caddr_t)0x4080a818,	/* ADBAlternateInit */
		(caddr_t)0x40814800,	/* Egret */
		(caddr_t)0x408147c4,	/* InitEgret */
		(caddr_t)0x408d2918,	/* ADBReInit_JTBL */
		(caddr_t)0x4087eb90,	/* ROMResourceMap List Head */
		(caddr_t)0x4081c406,	/* FixDiv */
		(caddr_t)0x4081c312,	/* FixMul */
	},
	/*
	 * Vectors verified for the LC520
	 */
	{			/* 15 */
		"MacLC520 ROMs",
		(caddr_t)0x408d16d6,	/* ADB interrupt */
		(caddr_t)0x0,		/* PB ADB interrupt */
		(caddr_t)0x408b2f84,	/* ADBBase + 130 interrupt; whatzit? */
		(caddr_t)0x4080a360,	/* CountADBs */
		(caddr_t)0x4080a37a,	/* GetIndADB */
		(caddr_t)0x4080a3a6,	/* GetADBInfo */
		(caddr_t)0x4080a3ac,	/* SetADBInfo */
		(caddr_t)0x4080a752,	/* ADBReInit */
		(caddr_t)0x4080a3dc,	/* ADBOp */
		(caddr_t)0x0,		/* PMgrOp */
		(caddr_t)0x4080c05c,	/* WriteParam */
		(caddr_t)0x4080c086,	/* SetDateTime */
		(caddr_t)0x4080c5cc,	/* InitUtil */
		(caddr_t)0x4080b186,	/* ReadXPRam */
		(caddr_t)0x4080b190,	/* WriteXPRam */
		(caddr_t)0x408b3c04,	/* jClkNoMem */
		(caddr_t)0x4080a818,	/* ADBAlternateInit */
		(caddr_t)0x408d1450,	/* Egret */
		(caddr_t)0x408147c4,	/* InitEgret */
		(caddr_t)0x408d2460,	/* ADBReInit_JTBL */
		(caddr_t)0x4087eb90,	/* ROMResourceMap List Head */
		(caddr_t)0x4081c406,	/* FixDiv for P520 */
		(caddr_t)0x4081c312,	/* FixMul for P520 */
	},
	/*
	 * Vectors verified for the LC 575/577/578
	 */
	{			/* 16 */
		"MacLC575 ROMs",
		(caddr_t)0x408a9b56,	/* ADB interrupt */
		(caddr_t)0x0,		/* PB ADB interrupt */
		(caddr_t)0x408b2f94,	/* ADBBase + 130 interrupt; whatzit? */
		(caddr_t)0x4080a360,	/* CountADBs */
		(caddr_t)0x4080a37a,	/* GetIndADB */
		(caddr_t)0x4080a3a6,	/* GetADBInfo */
		(caddr_t)0x4080a3ac,	/* SetADBInfo */
		(caddr_t)0x4080a752,	/* ADBReInit */
		(caddr_t)0x4080a3dc,	/* ADBOp */
		(caddr_t)0x0,		/* PMgrOp */
		(caddr_t)0x4080c05c,	/* WriteParam */
		(caddr_t)0x4080c086,	/* SetDateTime */
		(caddr_t)0x4080c5cc,	/* InitUtil */
		(caddr_t)0x4080b186,	/* ReadXPRam */
		(caddr_t)0x4080b190,	/* WriteXPRam */
		(caddr_t)0x408b3bf8,	/* jClkNoMem */
		(caddr_t)0x4080a818,	/* ADBAlternateInit */
		(caddr_t)0x408a99c0,	/* Egret */
		(caddr_t)0x408147c4,	/* InitEgret */
		(caddr_t)0x408a81a0,	/* ADBReInit_JTBL */
		(caddr_t)0x4087eb90,	/* ROMResourceMap List Head */
		(caddr_t)0x4081c406,	/* FixDiv for P520 */
		(caddr_t)0x4081c312,	/* FixMul for P520 */
	},
	/*
	 * Vectors verified for the Quadra 950
	 */
	{			/* 17 */
		"Quadra950 class ROMs",
		(caddr_t)0x40814912,	/* ADB interrupt */
		(caddr_t)0x0,		/* PM ADB interrupt */
		(caddr_t)0x4080a4d8,	/* ADBBase + 130 interrupt; whatzit? */
		(caddr_t)0x4080a360,	/* CountADBs */
		(caddr_t)0x4080a37a,	/* GetIndADB */
		(caddr_t)0x4080a3a6,	/* GetADBInfo */
		(caddr_t)0x4080a3ac,	/* SetADBInfo */
		(caddr_t)0x4080a752,	/* ADBReInit */
		(caddr_t)0x4080a3dc,	/* ADBOp */
		(caddr_t)0x0,		/* PMgrOp */
		(caddr_t)0x4080c05c,	/* WriteParam */
		(caddr_t)0x4080c086,	/* SetDateTime */
		(caddr_t)0x4080c5cc,	/* InitUtil */
		(caddr_t)0x4080b186,	/* ReadXPRam */
		(caddr_t)0x4080b190,	/* WriteXPRam */
		(caddr_t)0x4080b1e4,	/* jClkNoMem */
		(caddr_t)0x4080a818,	/* ADBAlternateInit */
		(caddr_t)0x40814800,	/* Egret */
		(caddr_t)0x408147c4,	/* InitEgret */
		(caddr_t)0x408038bc,	/* ADBReInit_JTBL */
		(caddr_t)0x4087eb90,	/* ROMResourceMap List Head */
		(caddr_t)0x4081c406,	/* FixDiv */
		(caddr_t)0x4081c312,	/* FixMul */
	},
	/*
	 * Vectors verified for the Mac IIfx
	 */
	{			/* 18 */
		"Mac IIfx ROMs",
		(caddr_t)0x40809f4a,	/* ADB interrupt */
		(caddr_t)0x0,		/* PM ADB interrupt */
		(caddr_t)0x4080a4d8,	/* ADBBase + 130 interrupt */
		(caddr_t)0x4080a360,	/* CountADBs */
		(caddr_t)0x4080a37a,	/* GetIndADB */
		(caddr_t)0x4080a3a6,	/* GetADBInfo */
		(caddr_t)0x4080a3ac,	/* SetADBInfo */
		(caddr_t)0x4080a752,	/* ADBReInit */
		(caddr_t)0x4080a3dc,	/* ADBOp */
		(caddr_t)0x0,		/* PMgrOp */
		(caddr_t)0x4080c05c,	/* WriteParam */
		(caddr_t)0x4080c086,	/* SetDateTime */
		(caddr_t)0x4080c5cc,	/* InitUtil */
		(caddr_t)0x4080b186,	/* ReadXPRam */
		(caddr_t)0x4080b190,	/* WriteXPRam */
		(caddr_t)0x4080b1e4,	/* jClkNoMem */
		(caddr_t)0x4080a818,	/* ADBAlternateInit */
		(caddr_t)0x0,		/* Egret */
		(caddr_t)0x0,		/* InitEgret */
		(caddr_t)0x408037c0,	/* ADBReInit_JTBL */
		(caddr_t)0x4087eb90,	/* ROMResourceMap List Head */
		(caddr_t)0x4081c406,	/* FixDiv */
		(caddr_t)0x4081c312,	/* FixMul */
	},
	/*
	 * Vectors verified for the Performa 588 (and 580?)
	 */
	{			/* 19 */
		"Performa 580 ROMs",
		(caddr_t) 0x4089a8be,	/* ADB interrupt */
		(caddr_t) 0x0,		/* PM ADB interrupt */
		(caddr_t) 0x408b2f94,	/* ADBBase + 130 interrupt */
		(caddr_t) 0x4080a360,	/* CountADBs */
		(caddr_t) 0x4080a37a,	/* GetIndADB */
		(caddr_t) 0x4080a3a6,	/* GetADBInfo */
		(caddr_t) 0x4080a3ac,	/* SetADBInfo */
		(caddr_t) 0x4080a752,	/* ADBReInit */
		(caddr_t) 0x4080a3dc,	/* ADBOp */
		(caddr_t) 0x0,		/* PMgrOp */
		(caddr_t) 0x4080c05c,	/* WriteParam */
		(caddr_t) 0x4080c086,	/* SetDateTime */
		(caddr_t) 0x4080c5cc,	/* InitUtil */
		(caddr_t) 0x4080b186,	/* ReadXPRam */
		(caddr_t) 0x4080b190,	/* WriteXPRam */
		(caddr_t) 0x408b3bf4,	/* jClkNoMem */
		(caddr_t) 0x4080a818,	/* ADBAlternateInit */
		(caddr_t) 0x408a99c0,	/* Egret */
		(caddr_t) 0x408147c8,	/* InitEgret */
		(caddr_t) 0x408a7f74,	/* ADBReInit_JTBL */
		(caddr_t) 0x4087eb90,	/* ROMResourceMap List Head */
		(caddr_t) 0x4081c406,	/* FixDiv */
		(caddr_t) 0x4081c312,	/* FixMul */
	},
	/* Please fill these in! -BG */
};


struct cpu_model_info cpu_models[] = {

/* The first four. */
	{MACH_MACII, "II ", "", MACH_CLASSII, &romvecs[0]},
	{MACH_MACIIX, "IIx ", "", MACH_CLASSII, &romvecs[0]},
	{MACH_MACIICX, "IIcx ", "", MACH_CLASSII, &romvecs[0]},
	{MACH_MACSE30, "SE/30 ", "", MACH_CLASSII, &romvecs[0]},

/* The rest of the II series... */
	{MACH_MACIICI, "IIci ", "", MACH_CLASSIIci, &romvecs[4]},
	{MACH_MACIISI, "IIsi ", "", MACH_CLASSIIsi, &romvecs[2]},
	{MACH_MACIIVI, "IIvi ", "", MACH_CLASSIIvx, &romvecs[2]},
	{MACH_MACIIVX, "IIvx ", "", MACH_CLASSIIvx, &romvecs[2]},
	{MACH_MACIIFX, "IIfx ", "", MACH_CLASSIIfx, &romvecs[18]},

/* The Centris/Quadra series. */
	{MACH_MACQ700, "Quadra", " 700 ", MACH_CLASSQ, &romvecs[4]},
	{MACH_MACQ900, "Quadra", " 900 ", MACH_CLASSQ, &romvecs[6]},
	{MACH_MACQ950, "Quadra", " 950 ", MACH_CLASSQ, &romvecs[17]},
	{MACH_MACQ800, "Quadra", " 800 ", MACH_CLASSQ, &romvecs[6]},
	{MACH_MACQ650, "Quadra", " 650 ", MACH_CLASSQ, &romvecs[6]},
	{MACH_MACC650, "Centris", " 650 ", MACH_CLASSQ, &romvecs[6]},
	{MACH_MACQ605, "Quadra", " 605 ", MACH_CLASSQ, &romvecs[9]},
	{MACH_MACQ605_33, "Quadra", " 605/33 ", MACH_CLASSQ, &romvecs[9]},
	{MACH_MACC610, "Centris", " 610 ", MACH_CLASSQ, &romvecs[6]},
	{MACH_MACQ610, "Quadra", " 610 ", MACH_CLASSQ, &romvecs[6]},
	{MACH_MACQ630, "Quadra", " 630 ", MACH_CLASSQ2, &romvecs[13]},
	{MACH_MACC660AV, "Centris", " 660AV ", MACH_CLASSAV, &romvecs[7]},
	{MACH_MACQ840AV, "Quadra", " 840AV ", MACH_CLASSAV, &romvecs[7]},

/* The Powerbooks/Duos... */
	{MACH_MACPB100, "PowerBook", " 100 ", MACH_CLASSPB, &romvecs[1]},
	/* PB 100 has no MMU! */
	{MACH_MACPB140, "PowerBook", " 140 ", MACH_CLASSPB, &romvecs[1]},
	{MACH_MACPB145, "PowerBook", " 145 ", MACH_CLASSPB, &romvecs[1]},
	{MACH_MACPB150, "PowerBook", " 150 ", MACH_CLASSDUO, &romvecs[10]},
	{MACH_MACPB160, "PowerBook", " 160 ", MACH_CLASSPB, &romvecs[5]},
	{MACH_MACPB165, "PowerBook", " 165 ", MACH_CLASSPB, &romvecs[5]},
	{MACH_MACPB165C, "PowerBook", " 165c ", MACH_CLASSPB, &romvecs[5]},
	{MACH_MACPB170, "PowerBook", " 170 ", MACH_CLASSPB, &romvecs[1]},
	{MACH_MACPB180, "PowerBook", " 180 ", MACH_CLASSPB, &romvecs[5]},
	{MACH_MACPB180C, "PowerBook", " 180c ", MACH_CLASSPB, &romvecs[5]},
	{MACH_MACPB190, "PowerBook", " 190 ", MACH_CLASSPB, &romvecs[8]},
	{MACH_MACPB190CS, "PowerBook", " 190cs ", MACH_CLASSPB, &romvecs[8]},
	{MACH_MACPB500, "PowerBook", " 500 ", MACH_CLASSPB, &romvecs[8]},

/* The Duos */
	{MACH_MACPB210, "PowerBook Duo", " 210 ", MACH_CLASSDUO, &romvecs[5]},
	{MACH_MACPB230, "PowerBook Duo", " 230 ", MACH_CLASSDUO, &romvecs[5]},
	{MACH_MACPB250, "PowerBook Duo", " 250 ", MACH_CLASSDUO, &romvecs[5]},
	{MACH_MACPB270, "PowerBook Duo", " 270C ", MACH_CLASSDUO, &romvecs[5]},
	{MACH_MACPB280, "PowerBook Duo", " 280 ", MACH_CLASSDUO, &romvecs[5]},
	{MACH_MACPB280C, "PowerBook Duo", " 280C ", MACH_CLASSDUO, &romvecs[5]},

/* The Performas... */
	{MACH_MACP600, "Performa", " 600 ", MACH_CLASSIIvx, &romvecs[2]},
	{MACH_MACP460, "Performa", " 460 ", MACH_CLASSLC, &romvecs[14]},
	{MACH_MACP550, "Performa", " 550 ", MACH_CLASSLC, &romvecs[11]},
	{MACH_MACP580, "Performa", " 580 ", MACH_CLASSQ2, &romvecs[19]},
	{MACH_MACTV,   "TV ",      "",      MACH_CLASSLC, &romvecs[12]},

/* The LCs... */
	{MACH_MACLCII,  "LC", " II ",  MACH_CLASSLC, &romvecs[3]},
	{MACH_MACLCIII, "LC", " III ", MACH_CLASSLC, &romvecs[14]},
	{MACH_MACLC475, "LC", " 475 ", MACH_CLASSQ,  &romvecs[9]},
	{MACH_MACLC475_33, "LC", " 475/33 ", MACH_CLASSQ,  &romvecs[9]},
	{MACH_MACLC520, "LC", " 520 ", MACH_CLASSLC, &romvecs[15]},
	{MACH_MACLC575, "LC", " 575 ", MACH_CLASSQ2, &romvecs[16]},
	{MACH_MACCCLASSIC, "Color Classic ", "", MACH_CLASSLC, &romvecs[3]},
	{MACH_MACCCLASSICII, "Color Classic"," II ", MACH_CLASSLC, &romvecs[3]},
/* Does this belong here? */
	{MACH_MACCLASSICII, "Classic", " II ", MACH_CLASSLC, &romvecs[3]},

/* The unknown one and the end... */
	{0, "Unknown", "", MACH_CLASSII, NULL},
	{0, NULL, NULL, 0, NULL},
};				/* End of cpu_models[] initialization. */

struct intvid_info_t {
	int	machineid;
	u_long	fbbase;
	u_long	fbmask;
	u_long	fblen;
} intvid_info[] = {
	{ MACH_MACCLASSICII,	0x009f9a80,	0x0,		21888 },
	{ MACH_MACPB140,	0xfee08000,	0x0,		32 * 1024 },
	{ MACH_MACPB145,	0xfee08000,	0x0,		32 * 1024 },
	{ MACH_MACPB170,	0xfee08000,	0x0,		32 * 1024 },
	{ MACH_MACPB150,	0x60000000,	0x0,		128 * 1024 },
	{ MACH_MACPB160,	0x60000000,	0x0ffe0000,	128 * 1024 },
	{ MACH_MACPB165,	0x60000000,	0x0ffe0000,	128 * 1024 },
	{ MACH_MACPB180,	0x60000000,	0x0ffe0000,	128 * 1024 },
	{ MACH_MACPB210,	0x60000000,	0x0,		128 * 1024 },
	{ MACH_MACPB230,	0x60000000,	0x0,		128 * 1024 },
	{ MACH_MACPB250,	0x60000000,	0x0,		128 * 1024 },
	{ MACH_MACPB270,	0x60000000,	0x0,		128 * 1024 },
	{ MACH_MACPB280,	0x60000000,	0x0,		128 * 1024 },
	{ MACH_MACPB280C,	0x60000000,	0x0,		128 * 1024 },
	{ MACH_MACIICI,		0x0,		0x0,		320 * 1024 },
	{ MACH_MACIISI,		0x0,		0x0,		320 * 1024 },
	{ MACH_MACCCLASSIC,	0x50f40000,	0x0,		512 * 1024 },
/*??*/	{ MACH_MACLCII,		0x50f40000,	0x0,		512 * 1024 },
	{ MACH_MACPB165C,	0xfc040000,	0x0,		512 * 1024 },
	{ MACH_MACPB180C,	0xfc040000,	0x0,		512 * 1024 },
	{ MACH_MACPB190,	0x60000000,	0x0,		512 * 1024 },
	{ MACH_MACPB190CS,	0x60000000,	0x0,		512 * 1024 },
	{ MACH_MACPB500,	0x60000000,	0x0,		512 * 1024 },
	{ MACH_MACLCIII,	0x60b00000,	0x0,		768 * 1024 },
	{ MACH_MACLC520,	0x60000000,	0x0,		1024 * 1024 },
	{ MACH_MACP550,		0x60000000,	0x0,		1024 * 1024 },
	{ MACH_MACTV,		0x60000000,	0x0,		1024 * 1024 },
	{ MACH_MACLC475,	0xf9000000,	0x0,		1024 * 1024 },
	{ MACH_MACLC475_33,	0xf9000000,	0x0,		1024 * 1024 },
	{ MACH_MACLC575,	0xf9000000,	0x0,		1024 * 1024 },
	{ MACH_MACC610,		0xf9000000,	0x0,		1024 * 1024 },
	{ MACH_MACC650,		0xf9000000,	0x0,		1024 * 1024 },
	{ MACH_MACP580,		0xf9000000,	0x0,		1024 * 1024 },
	{ MACH_MACQ605,		0xf9000000,	0x0,		1024 * 1024 },
	{ MACH_MACQ605_33,	0xf9000000,	0x0,		1024 * 1024 },
	{ MACH_MACQ610,		0xf9000000,	0x0,		1024 * 1024 },
	{ MACH_MACQ630,		0xf9000000,	0x0,		1024 * 1024 },
	{ MACH_MACQ650,		0xf9000000,	0x0,		1024 * 1024 },
	{ MACH_MACC660AV,	0x50100000,	0x0,		1024 * 1024 },
	{ MACH_MACQ700,		0xf9000000,	0x0,		1024 * 1024 },
	{ MACH_MACQ800,		0xf9000000,	0x0,		1024 * 1024 },
	{ MACH_MACQ900,		0xf9000000,	0x0,		1024 * 1024 },
	{ MACH_MACQ950,		0xf9000000,	0x0,		1024 * 1024 },
	{ MACH_MACQ840AV,	0x50100000,	0x0,		2048 * 1024 },
	{ 0,			0x0,		0x0,		0 },
};				/* End of intvid_info[] initialization. */

/*
 * Missing Mac Models:
 *	PowerMac 6100
 *	PowerMac 7100
 *	PowerMac 8100
 *	PowerBook 540
 *	PowerBook 520
 *	PowerBook 150
 *	Duo 280
 *	Performa 6000s
 * 	...?
 */

char	cpu_model[120];		/* for sysctl() */

int	mach_cputype(void);

int
mach_cputype(void)
{
	return (mac68k_machine.mach_processor);
}

static void
identifycpu(void)
{
	extern u_int delay_factor;
	const char *mpu;

	switch (cputype) {
	case CPU_68020:
		mpu = ("(68020)");
		break;
	case CPU_68030:
		mpu = ("(68030)");
		break;
	case CPU_68040:
		mpu = ("(68040)");
		break;
	default:
		mpu = ("(unknown processor)");
		break;
	}
	sprintf(cpu_model, "Apple Macintosh %s%s %s",
	    cpu_models[mac68k_machine.cpu_model_index].model_major,
	    cpu_models[mac68k_machine.cpu_model_index].model_minor,
	    mpu);
	printf("%s\n", cpu_model);
	printf("cpu: delay factor %d\n", delay_factor);
}

static void	get_machine_info(void);

static void
get_machine_info(void)
{
	int i;

	for (i = 0; cpu_models[i].model_major; i++)
		if (mac68k_machine.machineid == cpu_models[i].machineid)
			break;

	if (cpu_models[i].model_major == NULL)
		i--;

	mac68k_machine.cpu_model_index = i;
}

struct cpu_model_info *current_mac_model;
romvec_t *mrg_MacOSROMVectors = 0;

/*
 * Sets a bunch of machine-specific variables
 */
void	setmachdep(void);

void
setmachdep(void)
{
	struct cpu_model_info *cpui;

	/*
	 * First, set things that need to be set on the first pass only
	 * Ideally, we'd only call this once, but for some reason, the
	 * VIAs need interrupts turned off twice !?
	 */
	get_machine_info();

	load_addr = 0;
	cpui = &(cpu_models[mac68k_machine.cpu_model_index]);
	current_mac_model = cpui;

	mac68k_machine.via1_ipl = 1;
	mac68k_machine.via2_ipl = 2;
	mac68k_machine.aux_interrupts = 0;

	/*
	 * Set up any machine specific stuff that we have to before
	 * ANYTHING else happens
	 */
	switch (cpui->class) {	/* Base this on class of machine... */
	case MACH_CLASSII:
		VIA2 = VIA2OFF;
		IOBase = 0x50f00000;
		Via1Base = (volatile u_char *)IOBase;
		mac68k_machine.scsi80 = 1;
		mac68k_machine.zs_chip = 0;
		via_reg(VIA1, vIER) = 0x7f;	/* disable VIA1 int */
		via_reg(VIA2, vIER) = 0x7f;	/* disable VIA2 int */
		break;
	case MACH_CLASSPB:
		VIA2 = VIA2OFF;
		IOBase = 0x50f00000;
		Via1Base = (volatile u_char *)IOBase;
		mac68k_machine.scsi80 = 1;
		mac68k_machine.zs_chip = 0;
		/* Disable everything but PM; we need it. */
		via_reg(VIA1, vIER) = 0x6f;	/* disable VIA1 int */
		/* Are we disabling something important? */
		via_reg(VIA2, vIER) = 0x7f;	/* disable VIA2 int */
		if (cputype == CPU_68040)
			mac68k_machine.sonic = 1;
		break;
	case MACH_CLASSDUO:
		/*
		 * The Duo definitely does not use a VIA2, but it looks
		 * like the VIA2 functions might be on the MSC at the RBV
		 * locations.  The rest is copied from the Powerbooks.
		 */
		VIA2 = RBVOFF;
		IOBase = 0x50f00000;
		Via1Base = (volatile u_char *)IOBase;
		mac68k_machine.scsi80 = 1;
		mac68k_machine.zs_chip = 0;
		/* Disable everything but PM; we need it. */
		via_reg(VIA1, vIER) = 0x6f;	/* disable VIA1 int */
		/* Are we disabling something important? */
		via_reg(VIA2, rIER) = 0x7f;	/* disable VIA2 int */
		break;
	case MACH_CLASSQ:
	case MACH_CLASSQ2:
		VIA2 = VIA2OFF;
		IOBase = 0x50f00000;
		Via1Base = (volatile u_char *)IOBase;
		mac68k_machine.sonic = 1;
		mac68k_machine.scsi96 = 1;
		mac68k_machine.zs_chip = 0;
		via_reg(VIA1, vIER) = 0x7f;	/* disable VIA1 int */
		via_reg(VIA2, vIER) = 0x7f;	/* disable VIA2 int */

#if 1
		switch (current_mac_model->machineid) {
		default:
	/*	case MACH_MACQ900: These three, at least, support the
		case MACH_MACQ950: A/UX interrupts.  What Quadras don't?
		case MACH_MACQ700: */
			/* Enable A/UX interrupt scheme */
			mac68k_machine.aux_interrupts = 1;

			via_reg(VIA1, vBufB) &= (0xff ^ DB1O_AuxIntEnb);
			via_reg(VIA1, vDirB) |= DB1O_AuxIntEnb;
			mac68k_machine.via1_ipl = 6;
			mac68k_machine.via2_ipl = 2;
			break;
		}
#endif

		break;
	case MACH_CLASSAV:
	case MACH_CLASSP580:
		VIA2 = VIA2OFF;
		IOBase = 0x50f00000;
		Via1Base = (volatile u_char *)IOBase;
		mac68k_machine.scsi96 = 1;
		mac68k_machine.zs_chip = 0;
		via_reg(VIA1, vIER) = 0x7f;	/* disable VIA1 int */
		via_reg(VIA2, vIER) = 0x7f;	/* disable VIA2 int */
		break;
	case MACH_CLASSIIci:
		VIA2 = RBVOFF;
		IOBase = 0x50f00000;
		Via1Base = (volatile u_char *)IOBase;
		mac68k_machine.scsi80 = 1;
		mac68k_machine.zs_chip = 0;
		via_reg(VIA1, vIER) = 0x7f;	/* disable VIA1 int */
		via_reg(VIA2, rIER) = 0x7f;	/* disable RBV int */
		break;
	case MACH_CLASSIIsi:
		VIA2 = RBVOFF;
		IOBase = 0x50f00000;
		Via1Base = (volatile u_char *)IOBase;
		mac68k_machine.scsi80 = 1;
		mac68k_machine.zs_chip = 0;
		via_reg(VIA1, vIER) = 0x7f;	/* disable VIA1 int */
		via_reg(VIA2, rIER) = 0x7f;	/* disable RBV int */
		break;
	case MACH_CLASSIIvx:
		VIA2 = RBVOFF;
		IOBase = 0x50f00000;
		Via1Base = (volatile u_char *)IOBase;
		mac68k_machine.scsi80 = 1;
		mac68k_machine.zs_chip = 0;
		via_reg(VIA1, vIER) = 0x7f;	/* disable VIA1 int */
		via_reg(VIA2, rIER) = 0x7f;	/* disable RBV int */
		break;
	case MACH_CLASSLC:
		VIA2 = RBVOFF;
		IOBase = 0x50f00000;
		Via1Base = (volatile u_char *)IOBase;
		mac68k_machine.scsi80 = 1;
		mac68k_machine.zs_chip = 0;
		via_reg(VIA1, vIER) = 0x7f;	/* disable VIA1 int */
		via_reg(VIA2, rIER) = 0x7f;	/* disable RBV int */
		break;
	case MACH_CLASSIIfx:
		VIA2 = OSSOFF;
		IOBase = 0x50f00000;
		Via1Base = (volatile u_char *)IOBase;
		mac68k_machine.scsi80 = 1;
		mac68k_machine.zs_chip = 0;
		via_reg(VIA1, vIER) = 0x7f;  /* disable VIA1 int */
		break;
	default:
	case MACH_CLASSH:
		break;
	}

	/*
	 * Set up current ROM Glue vectors.  Actually now all we do
	 * is save the address of the ROM Glue Vector table. This gets
	 * used later when we re-map the vectors from MacOS Address
	 * Space to NetBSD Address Space.
	 */
	mrg_MacOSROMVectors = cpui->rom_vectors;
}

/*
 * Set IO offsets.
 */
void
mac68k_set_io_offsets(vaddr_t base)
{

	switch (current_mac_model->class) {
	case MACH_CLASSQ:
		Via1Base = (volatile u_char *)base;

		/* The following two may be overridden. */
		sccA = (volatile u_char *)base + 0xc000;
		SCSIBase = base + 0xf000;

		switch (current_mac_model->machineid) {
		case MACH_MACQ900:
		case MACH_MACQ950:
			mac68k_machine.scsi96_2 = 1;
			sccA = (volatile u_char *)base + 0xc020;
			iop_init(0);	/* For console */
			break;
		case MACH_MACQ700:
			break;
		default:
			SCSIBase = base + 0x10000;
			break;
		}
		break;
	case MACH_CLASSQ2:
		/*
		 * Note the different offset for sccA for this class of
		 * machines.  This seems to be common on many of the
		 * Quadra-type machines.
		 */
		Via1Base = (volatile u_char *)base;
		sccA = (volatile u_char *)base + 0xc020;
		SCSIBase = base + 0x10000;
		break;
	case MACH_CLASSP580:
		/*
		 * Here's a queer bird... it seems to be a cross between
		 * the two different Quadra classes.
		 */
		Via1Base = (volatile u_char *) base;
		sccA = (volatile u_char *) base + 0xc020;
		SCSIBase = base;
		break;
	case MACH_CLASSAV:
		Via1Base = (volatile u_char *)base;
		sccA = (volatile u_char *)base + 0x4000;
		SCSIBase = base + 0x18000;
		PSCBase = (volatile u_char *)base + 0x31000;
		break;
	case MACH_CLASSII:
	case MACH_CLASSPB:
	case MACH_CLASSDUO:
	case MACH_CLASSIIci:
	case MACH_CLASSIIsi:
	case MACH_CLASSIIvx:
	case MACH_CLASSLC:
		Via1Base = (volatile u_char *)base;
		sccA = (volatile u_char *) base + 0x4000;
		SCSIBase = base;
		break;
	case MACH_CLASSIIfx:
		/*
		 * Note that sccA base address is based on having
		 * the serial port in `compatible' mode (set in
		 * the Serial Switch control panel before booting).
		 */
		Via1Base = (volatile u_char *)base;
		sccA = (volatile u_char *)base + 0x4020;
		SCSIBase = base;
		iop_init(0);	/* For console */
		break;
	default:
	case MACH_CLASSH:
		panic("Unknown/unsupported machine class (%d).",
		    current_mac_model->class);
		break;
	}
	Via2Base = Via1Base + 0x2000 * VIA2;
}

#if GRAYBARS
static u_long gray_nextaddr = 0;

void
gray_bar(void)
{
	static int i = 0;
	static int flag = 0;

/* MF basic premise as I see it:
	1) Save the scratch regs as they are not saved by the compilier.
   	2) Check to see if we want gray bars, if so,
		display some lines of gray,
		a couple of lines of white(about 8),
		and loop to slow this down.
   	3) restore regs
*/

	__asm volatile (
			"	movl %a0,%sp@-;"
			"	movl %a1,%sp@-;"
			"	movl %d0,%sp@-;"
			"	movl %d1,%sp@-");

/* check to see if gray bars are turned off */
	if (mac68k_machine.do_graybars) {
		/* MF the 10*rowbytes/4 is done lots, but we want this to be
		 * slow */
		for (i = 0; i < 10 * videorowbytes / 4; i++)
			((u_long *)videoaddr)[gray_nextaddr++] = 0xaaaaaaaa;
		for (i = 0; i < 2 * videorowbytes / 4; i++)
			((u_long *)videoaddr)[gray_nextaddr++] = 0x00000000;
	}

	__asm volatile (
			"	movl %sp@+,%d1;"
			"	movl %sp@+,%d0;"
			"	movl %sp@+,%a1;"
			"	movl %sp@+,%a0");
}
#endif

/* in locore */
extern u_long ptest040(caddr_t, u_int);
extern int get_pte(u_int, u_long *, u_short *);

/*
 * LAK (7/24/94): given a logical address, puts the physical address
 *  in *phys and return 1, or returns 0 on failure.  This is intended
 *  to look through MacOS page tables.
 */

static u_long
get_physical(u_int addr, u_long * phys)
{
	extern u_int macos_tc;
	u_long pte[2], ph, mask;
	u_short psr;
	int i, numbits;

	if (mmutype == MMU_68040) {
		ph = ptest040((caddr_t)addr, FC_SUPERD);
		if ((ph & MMU40_RES) == 0) {
			ph = ptest040((caddr_t)addr, FC_USERD);
			if ((ph & MMU40_RES) == 0)
				return 0;
		}
		if ((ph & MMU40_TTR) != 0)
			ph = addr;

		mask = (macos_tc & 0x4000) ? 0x00001fff : 0x00000fff;
		ph &= (~mask);
	} else {
		switch (get_pte(addr, pte, &psr)) {
		case (-1):
			return 0;
		case 0:
			ph = pte[0] & 0xFFFFFF00;
			break;
		case 1:
			ph = pte[1] & 0xFFFFFF00;
			break;
		default:
			panic("get_physical(): bad get_pte()");
		}

		/*
		 * We must now figure out how many levels down we went and
		 * mask the bits appropriately -- the returned value may only
		 * be the upper n bits, and we have to take the rest from addr.
		 */
		numbits = 0;
		psr &= 0x0007;		/* Number of levels we went */
		for (i = 0; i < psr; i++)
			numbits += (macos_tc >> (12 - i * 4)) & 0x0f;

		/*
		 * We have to take the most significant "numbits" from
		 * the returned value "ph", and the rest from our addr.
		 * Assume that numbits != 0.
		 */
		mask = (1 << (32 - numbits)) - 1;
	}
	*phys = ph + (addr & mask);

	return 1;
}

static void	check_video(const char *, u_long, u_long);

static void
check_video(const char *id, u_long limit, u_long maxm)
{
	u_long addr, phys;

	if (!get_physical(videoaddr, &phys)) {
		if (mac68k_machine.do_graybars)
			printf("get_mapping(): %s.  False start.\n", id);
	} else {
		mac68k_vidlog = videoaddr;
		mac68k_vidphys = phys;
		mac68k_vidlen = 32768;
		addr = videoaddr + 32768;
		while (get_physical(addr, &phys)) {
			if ((phys - mac68k_vidphys)
			    != mac68k_vidlen)
				break;
			if (mac68k_vidlen + 32768 > limit) {
				if (mac68k_machine.do_graybars) {
					printf("mapping: %s.  Does it never end?\n",
					    id);
					printf("    Forcing VRAM size ");
					printf("to a conservative %ldK.\n",
					    maxm/1024);
				}
				mac68k_vidlen = maxm;
				break;
			}
			mac68k_vidlen += 32768;
			addr += 32768;
		}
		if (mac68k_machine.do_graybars) {
			printf("  %s internal video at addr 0x%x (phys 0x%x), ",
			    id, mac68k_vidlog, mac68k_vidphys);
			printf("len 0x%x.\n", mac68k_vidlen);
		}
	}
}

/*
 * Find out how MacOS has mapped itself so we can do the same thing.
 * Returns the address of logical 0 so that locore can map the kernel
 * properly.
 */
u_int
get_mapping(void)
{
	struct intvid_info_t *iip;
	u_long addr, lastpage, phys, len, limit;
	int i, last, same;

	numranges = 0;
	for (i = 0; i < 8; i++) {
		low[i] = 0;
		high[i] = 0;
	}

	lastpage = get_top_of_ram();

	get_physical(0, &load_addr);

	if (mac68k_machine.do_graybars)
		printf("Loaded at 0x%0lx\n", load_addr);

	last = 0;
	for (addr = 0; addr <= lastpage && get_physical(addr, &phys);
	    addr += PAGE_SIZE) {
		if (numranges > 0 && phys != high[last]) {
			/*
			 * Attempt to find if this page is already
			 * accounted for in an existing physical segment.
			 */
			for (i = 0; i < numranges; i++) {
				if (low[i] <= phys && phys <= high[i]) {
					last = i;
					break;
				}
			}
			if (i >= numranges)
				last = numranges - 1;

			if (low[last] <= phys && phys < high[last])
				continue;	/* Skip pages we've seen. */
		}

		if (numranges > 0 && phys == high[last]) {
			/* Common case:  extend existing segment on high end */
			high[last] += PAGE_SIZE;
		} else {
			/* This is a new physical segment. */
			for (last = 0; last < numranges; last++)
				if (phys < low[last])
					break;

			/* Create space for segment, if necessary */
			if (last < numranges && phys < low[last]) {
				for (i = numranges; i > last; i--) {
					low[i] = low[i - 1];
					high[i] = high[i - 1];
				}
			}

			numranges++;
			low[last] = phys;
			high[last] = phys + PAGE_SIZE;
		}

		/* Coalesce adjoining segments as appropriate */
		if (last < (numranges - 1) && high[last] == low[last + 1] &&
		    low[last + 1] != load_addr) {
			high[last] = high[last + 1];
			for (i = last + 1; i < numranges; i++) {
				low[i] = low[i + 1];
				high[i] = high[i + 1];
			}
			--numranges;
		}
	}
	if (mac68k_machine.do_graybars) {
		printf("System RAM: %ld bytes in %ld pages.\n",
		    addr, addr / PAGE_SIZE);
		for (i = 0; i < numranges; i++) {
			printf("     Low = 0x%lx, high = 0x%lx\n",
			    low[i], high[i]);
		}
	}

	/*
	 * If we can't figure out the PA of the frame buffer by groveling
	 * the page tables, assume that we already have the correct
	 * address.  This is the case on several of the PowerBook 1xx
	 * series, in particular.
	 */
	if (!get_physical(videoaddr, &phys))
		phys = videoaddr;

	/*
	 * Find on-board video, if we have an idea of where to look
	 * on this system.
	 */
	for (iip = intvid_info; iip->machineid; iip++)
		if (mac68k_machine.machineid == iip->machineid)
			break;

	if (mac68k_machine.machineid == iip->machineid &&
	    (phys & ~iip->fbmask) >= iip->fbbase &&
	    (phys & ~iip->fbmask) < (iip->fbbase + iip->fblen)) {
		mac68k_vidphys = phys & ~iip->fbmask;
		mac68k_vidlen = 32768 - (phys & 0x7fff);

		limit = iip->fbbase + iip->fblen - mac68k_vidphys;
		if (mac68k_vidlen > limit) {
			mac68k_vidlen = limit;
		} else {
			addr = videoaddr + mac68k_vidlen;
			while (get_physical(addr, &phys)) {
				phys &= ~iip->fbmask;
				if ((phys - mac68k_vidphys) != mac68k_vidlen)
					break;
				if ((mac68k_vidphys + 32768) > limit) {
					mac68k_vidlen = limit;
					break;
				}
				mac68k_vidlen += 32768;
				addr += 32768;
			}
		}
	}

	if (mac68k_vidlen > 0) {
		/*
		 * We've already figured out where internal video is.
		 * Tell the user what we know.
		 */
		if (mac68k_machine.do_graybars)
			printf("On-board video at addr 0x%lx (phys 0x%x), len 0x%x.\n",
			    videoaddr, mac68k_vidphys, mac68k_vidlen);
	} else {
		/*
		 * We should now look through all of NuBus space to find where
		 * the internal video is being mapped.  Just to be sure we
		 * handle all the cases, we simply map our NuBus space exactly
		 * how MacOS did it.  As above, we find a bunch of ranges that
		 * are contiguously mapped.  Since there are a lot of pages
		 * that are all mapped to 0, we handle that as a special case
		 * where the length is negative.  We search in increments of
		 * 32768 because that's the page size that MacOS uses.
		 */
		nbnumranges = 0;
		for (i = 0; i < NBMAXRANGES; i++) {
			nbphys[i] = 0;
			nblog[i] = 0;
			nblen[i] = 0;
		}

		same = 0;
		for (addr = 0xF9000000; addr < 0xFF000000; addr += 32768) {
			if (!get_physical(addr, &phys)) {
				continue;
			}
			len = nbnumranges == 0 ? 0 : nblen[nbnumranges - 1];

#ifdef __debug_mondo_verbose__
			if (mac68k_machine.do_graybars)
				printf ("0x%lx --> 0x%lx\n", addr, phys);
#endif

			if (nbnumranges > 0
			    && addr == nblog[nbnumranges - 1] + len
			    && phys == nbphys[nbnumranges - 1]) {
				/* Same as last one */
				nblen[nbnumranges - 1] += 32768;
				same = 1;
			} else {
				if ((nbnumranges > 0)
				    && !same
				    && (addr == nblog[nbnumranges - 1] + len)
				    && (phys == nbphys[nbnumranges - 1] + len))
					nblen[nbnumranges - 1] += 32768;
				else {
					if (same) {
						nblen[nbnumranges - 1] = -len;
						same = 0;
					}
					if (nbnumranges == NBMAXRANGES) {
						if (mac68k_machine.do_graybars)
							printf("get_mapping(): Too many NuBus ranges.\n");
						break;
					}
					nbnumranges++;
					nblog[nbnumranges - 1] = addr;
					nbphys[nbnumranges - 1] = phys;
					nblen[nbnumranges - 1] = 32768;
				}
			}
		}
		if (same) {
			nblen[nbnumranges - 1] = -nblen[nbnumranges - 1];
			same = 0;
		}
		if (mac68k_machine.do_graybars) {
			printf("Non-system RAM (nubus, etc.):\n");
			for (i = 0; i < nbnumranges; i++) {
				printf("     Log = 0x%lx, Phys = 0x%lx, Len = 0x%lx (%lu)\n",
				    nblog[i], nbphys[i], nblen[i], nblen[i]);
			}
		}

		/*
		 * We must now find the logical address of internal video in the
		 * ranges we made above.  Internal video is at physical 0, but
		 * a lot of pages map there.  Instead, we look for the logical
		 * page that maps to 32768 and go back one page.
		 */
		for (i = 0; i < nbnumranges; i++) {
			if (nblen[i] > 0
			    && nbphys[i] <= 32768
			    && 32768 <= nbphys[i] + nblen[i]) {
				mac68k_vidlog = nblog[i] - nbphys[i];
				mac68k_vidlen = nblen[i] + nbphys[i];
				mac68k_vidphys = 0;
				break;
			}
		}
		if (i == nbnumranges) {
			if (0x60000000 <= videoaddr && videoaddr < 0x70000000) {
				if (mac68k_machine.do_graybars)
					printf("Checking for Internal Video ");
				/*
				 * Kludge for IIvx internal video (60b0 0000).
				 * PB 520 (6000 0000)
				 */
				check_video("PB/IIvx (0x60?00000)",
				    1 * 1024 * 1024, 1 * 1024 * 1024);
			} else if (0x50F40000 <= videoaddr
			    && videoaddr < 0x50FBFFFF) {
				/*
				 * Kludge for LC internal video
				 */
				check_video("LC video (0x50f40000)",
				    512 * 1024, 512 * 1024);
			} else if (0x50100100 <= videoaddr
			    && videoaddr < 0x50400000) {
				/*
				 * Kludge for AV internal video
				 */
				check_video("AV video (0x50100100)",
				    1 * 1024 * 1024, 1 * 1024 * 1024);
			} else {
				if (mac68k_machine.do_graybars)
					printf( "  no internal video at address 0 -- "
					    "videoaddr is 0x%lx.\n", videoaddr);
			}
		} else if (mac68k_machine.do_graybars) {
			printf("  Video address = 0x%lx\n", videoaddr);
			printf("  Int video starts at 0x%x\n",
			    mac68k_vidlog);
			printf("  Length = 0x%x (%d) bytes\n",
			    mac68k_vidlen, mac68k_vidlen);
		}
	}

	return load_addr;	/* Return physical address of logical 0 */
}

/*
 * Debugging code for locore page-traversal routine.
 */
void printstar(void);
void
printstar(void)
{
	/*
	 * Be careful as we assume that no registers are clobbered
	 * when we call this from assembly.
	 */
	__asm volatile (
			"	movl %a0,%sp@-;"
			"	movl %a1,%sp@-;"
			"	movl %d0,%sp@-;"
			"	movl %d1,%sp@-");

	/* printf("*"); */

	__asm volatile (
			"	movl %sp@+,%d1;"
			"	movl %sp@+,%d0;"
			"	movl %sp@+,%a1;"
			"	movl %sp@+,%a0");
}

/*
 * Console bell callback; modularizes the console terminal emulator
 * and the audio system, so neither requires direct knowledge of the
 * other.
 */

void
mac68k_set_bell_callback(int (*callback)(void *, int, int, int), void *cookie)
{
	mac68k_bell_callback = callback;
	mac68k_bell_cookie = (caddr_t)cookie;
}

int
mac68k_ring_bell(int freq, int length, int volume)
{
	if (mac68k_bell_callback)
		return ((*mac68k_bell_callback)(mac68k_bell_cookie,
		    freq, length, volume));
	else
		return (ENXIO);
}
