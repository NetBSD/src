/*	$NetBSD: machdep.c,v 1.120.2.4 1999/01/18 20:15:14 drochner Exp $ */

/*
 * Copyright (c) 1988 University of Utah.
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * the Systems Programming Group of the University of Utah Computer
 * Science Department, The Mach Operating System project at
 * Carnegie-Mellon University and Ralph Campbell.
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
 *
 *	@(#)machdep.c	8.3 (Berkeley) 1/12/94
 */

#include <sys/cdefs.h>			/* RCS ID & Copyright macro defns */

__KERNEL_RCSID(0, "$NetBSD: machdep.c,v 1.120.2.4 1999/01/18 20:15:14 drochner Exp $");

/* from: Utah Hdr: machdep.c 1.63 91/04/24 */

#include "fs_mfs.h"
#include "opt_ddb.h"
#include "opt_uvm.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/signalvar.h>
#include <sys/kernel.h>
#include <sys/map.h>
#include <sys/proc.h>
#include <sys/buf.h>
#include <sys/reboot.h>
#include <sys/conf.h>
#include <sys/file.h>
#include <sys/callout.h>
#include <sys/malloc.h>
#include <sys/mbuf.h>
#include <sys/msgbuf.h>
#include <sys/ioctl.h>
#include <sys/tty.h>
#include <sys/device.h>
#include <sys/user.h>
#include <sys/exec.h>
#include <vm/vm.h>
#include <sys/sysctl.h>
#include <sys/mount.h>
#include <sys/syscallargs.h>
#include <sys/kcore.h>

#include <dev/cons.h>

#include <vm/vm_kern.h>

#if defined(UVM)
#include <uvm/uvm_extern.h>
#endif

#include <ufs/mfs/mfs_extern.h>		/* mfs_initminiroot() */

#include <machine/cpu.h>
#include <machine/reg.h>
#include <machine/psl.h>
#include <machine/pte.h>
#include <machine/autoconf.h>
#include <machine/sysconf.h>
#include <mips/locore.h>		/* wbflush() */
#include <mips/mips/mips_mcclock.h>	/* mclock CPU setimation */

#ifdef DDB
#include <mips/db_machdep.h>
#include <ddb/db_access.h>
#include <ddb/db_sym.h>
#include <ddb/db_extern.h>
#include <ddb/db_extern.h>
#endif

#include <pmax/stand/dec_prom.h>

#include <pmax/pmax/clockreg.h>
#include <pmax/pmax/pmaxtype.h>
#include <dev/tc/tcvar.h>		/* XXX */
#include <dev/tc/ioasicvar.h>		/* ioasic_base */
#include <dev/tc/ioasicreg.h>		/* cycl-counter on kn03 stepping */
#include <pmax/dev/promiovar.h>		/* prom console I/O vector */

#include "le_ioasic.h"

/* Motherboard or system-specific initialization vector */
void		unimpl_os_init __P((void));
void		unimpl_bus_reset __P((void));
int		unimpl_intr __P((unsigned, unsigned, unsigned, unsigned));
void		unimpl_cons_init __P((void));
void		unimpl_device_register __P((struct device *, void *));
const char*	unimpl_model_name __P((void));
void	 	unimpl_iointr __P ((void *, u_long));
void 		unimpl_clockintr __P ((void *));
void	 	unimpl_errintr __P ((void));

struct platform platform = {
	"iobus not set",
	unimpl_os_init,
	unimpl_bus_reset,
	unimpl_cons_init,
	unimpl_device_register,
	unimpl_iointr,
	unimpl_clockintr
};

/* the following is used externally (sysctl_hw) */
char	machine[] = MACHINE;		/* from <machine/param.h> */
char	machine_arch[] = MACHINE_ARCH;	/* from <machine/param.h> */
char	cpu_model[40];

/*  maps for VM objects */

#if defined(UVM)
vm_map_t exec_map = NULL;
vm_map_t mb_map = NULL;
vm_map_t phys_map = NULL;
#else
vm_map_t buffer_map;
#endif

int	cpuspeed = 30;		/* Approx # of instructions per usec */
int	systype;		/* Mother board type */
int	maxmem;			/* Max memory per process */
int	physmem;		/* Max supported memory, changes to actual */
int	physmem_boardmax;	/* {model,SIMM}-specific bound on physmem */
u_int32_t latched_cycle_cnt;	/* high resolution timer cycle counter */
u_long	le_iomem;		/* 128K for lance chip via. IOASIC */
volatile struct chiptime *mcclock_addr;

phys_ram_seg_t mem_clusters[VM_PHYSSEG_MAX];
int mem_cluster_cnt;

struct splvec splvec;

struct intrhand intrtab[MAX_DEV_NCOOKIES];
u_int32_t iplmask[IPL_HIGH + 1];	/* interrupt mask bits for each IPL */
u_int32_t oldiplmask[IPL_HIGH + 1];	/* old values for splx(s) */
struct splsw *__spl;			/* model dependent spl call switch */
unsigned (*clkread) __P((void));	/* model dependent clkread() */

extern caddr_t esym;

/*XXXjrs*/
const	struct callback *callv;	/* pointer to PROM entry points */

void mach_init __P((int, char *[], unsigned, const struct callback *));
void prom_haltbutton __P((void));
void prom_halt __P((int, char *)) __attribute__((__noreturn__));
int  initcpu __P((void));
int  atoi __P((const char *));
int  nullintr __P((void *));
unsigned nullclkread __P((void));

#ifdef DEBUG
/* stacktrace code violates prototypes to get callee's registers */
extern void stacktrace __P((void)); /*XXX*/
#endif

/*
 * safepri is a safe priority for sleep to set for a spin-wait
 * during autoconfiguration or after a panic.  Used as an argument to splx().
 * XXX disables interrupt 5 to disable mips3 on-chip clock, which also
 * disables mips1 FPU interrupts.
 */
int	safepri = MIPS3_PSL_LOWIPL;	/* XXX */


/*
 * Do all the stuff that locore normally does before calling main().
 * Process arguments passed to us by the prom monitor.
 */
void
mach_init(argc, argv, code, cv)
	int argc;
	char *argv[];
	u_int code;
	const struct callback *cv;
{
	char *cp;
	int i;
	u_long first, last;
	caddr_t kernend, v;
	vsize_t size;
	extern char edata[], end[];

	/* clear the BSS segment */
#ifdef DDB
	if (((struct exec *)edata)->a_midmag == 0x07018b00 &&	/* exec hdr? */
	    (i = ((struct exec *)edata)->a_syms) != 0) {	/* a_syms */
		*(long *)end = i;
		i += (*(long *)(end + i + 4) + 3) & ~3;		/* strings */
		esym = end + i + 4;
		kernend = (caddr_t)mips_round_page(esym);
		memset(edata, 0, end - edata);
	} else
#endif
	{
		kernend = (caddr_t)mips_round_page(end);
		memset(edata, 0, kernend - edata);
	}

	/* Initialize callv so we can do PROM output... */
	if (code == DEC_PROM_MAGIC) {
		callv = cv;
	} else {
		callv = &callvec;
	}

	/* Use PROM console output until we initialize a console driver. */
	cn_tab = &promcd;

	/* check for direct boot from DS5000 PROM */
	if (argc > 0 && strcmp(argv[0], "boot") == 0) {
		argc--;
		argv++;
	}

	/*
	 * Set the VM page size.
	 */
#if defined(UVM)
	uvm_setpagesize();
#else
	vm_set_page_size();
#endif

	/*
	 * Copy exception-dispatch code down to exception vector.
	 * Initialize locore-function vector.
	 * Clear out the I and D caches.
	 */
	mips_vector_init();

	/* look at argv[0] and compute bootdev */
	makebootdev(argv[0]);

	/*
	 * Look at arguments passed to us and compute boothowto.
	 */
	boothowto = RB_SINGLE;
#ifdef KADB
	boothowto |= RB_KDB;
#endif
	for (i = 1; i < argc; i++) {
		for (cp = argv[i]; *cp; cp++) {
			switch (*cp) {
			case 'a': /* autoboot */
				boothowto &= ~RB_SINGLE;
				break;

			case 'd': /* break into the kernel debugger ASAP */
				boothowto |= RB_KDB;
				break;

			case 'm': /* mini root present in memory */
				boothowto |= RB_MINIROOT;
				break;

			case 'n': /* ask for names */
				boothowto |= RB_ASKNAME;
				break;

			case 'N': /* don't ask for names */
				boothowto &= ~RB_ASKNAME;
			}
		}
	}

#ifdef MFS
	/*
	 * Check to see if a mini-root was loaded into memory. It resides
	 * at the start of the next page just after the end of BSS.
	 */
	if (boothowto & RB_MINIROOT) {
		boothowto |= RB_DFLTROOT;
		kernend += round_page(mfs_initminiroot(kernend));
	}
#endif

#ifdef DDB
	/*
	 * Initialize machine-dependent DDB commands, in case of early panic.
	 */
	db_machine_init();
	/* init symbols if present */
	if (esym)
		ddb_init(*(int *)&end, ((int *)&end) + 1, (int*)esym);
	if (boothowto & RB_KDB)
		Debugger();
#endif

	/*
	 * Init mapping for u page(s) for proc0, pm_tlbpid 1.
	 * This also initializes nullproc for switch_exit().
	 */
	mips_init_proc0(kernend);

	kernend += 2 * UPAGES * PAGE_SIZE;

	/*
	 * Determine what model of computer we are running on.
	 */
	if (code == DEC_PROM_MAGIC) {
		i = (*cv->_getsysid)();
		cp = "";
	} else {
		cp = (*callv->_getenv)("systype");
		if (cp)
			i = atoi(cp);
		else {
			cp = "";
			i = 0;
		}
	}

	/* Check for MIPS based platform */
	/* 0x82 -> MIPS1, 0x84 -> MIPS3 */
	if (((i >> 24) & 0xFF) != 0x82 && ((i >> 24) & 0xff) != 0x84) {
		printf("Unknown System type '%s' 0x%x\n", cp, i);
		cpu_reboot(RB_HALT | RB_NOSYNC, NULL);
	}

	/*
	 * Initialize physmem_boardmax; assume no SIMM-bank limits.
	 * Adjust later in model-specific code if necessary.
	 */
	physmem_boardmax = MIPS_MAX_MEM_ADDR;

	/* check what model platform we are running on */
	systype = ((i >> 16) & 0xff);

	/*
	 * Find out what hardware we're on, and do basic initialization.
	 */
	if (systype >= nsysinit) {
		platform_not_supported();
		/* NOTREACHED */
	}
	(*sysinit[systype].init)();

	/*
	 * Find out how much memory is available.
	 * Be careful to save and restore the original contents for msgbuf.
	 */
	physmem = btoc((paddr_t)kernend - MIPS_KSEG0_START);
	cp = (char *)MIPS_PHYS_TO_KSEG1(physmem << PGSHIFT);	
	while (cp < (char *)physmem_boardmax) {
	  	int j;
		if (badaddr(cp, 4))
			break;
		i = *(int *)cp;
		j = ((int *)cp)[4];
		*(int *)cp = 0xa5a5a5a5;
		/*
		 * Data will persist on the bus if we read it right away.
		 * Have to be tricky here.
		 */
		((int *)cp)[4] = 0x5a5a5a5a;
		wbflush();
		if (*(int *)cp != 0xa5a5a5a5)
			break;
		*(int *)cp = i;
		((int *)cp)[4] = j;
		cp += NBPG;
		physmem++;
	}

	maxmem = physmem;

	/*
	 * Now that we know how much memory we have, initialize the
	 * mem cluster array.
	 */
	mem_clusters[0].start = 0;		/* XXX is this correct? */
	mem_clusters[0].size  = ctob(physmem);
	mem_cluster_cnt = 1;

	/*
	 * Load the rest of the available pages into the VM system.
	 */
	first = round_page(MIPS_KSEG0_TO_PHYS(kernend));
	last = mem_clusters[0].start + mem_clusters[0].size;
#if defined(UVM)
	uvm_page_physload(atop(first), atop(last), atop(first), atop(last),
	    VM_FREELIST_DEFAULT);
#else
	vm_page_physload(atop(first), atop(last), atop(first), atop(last));
#endif

	/*
	 * Initialize error message buffer (at end of core).
	 */
	mips_init_msgbuf();

	/*
	 * Allocate space for system data structures.  These data structures
	 * are allocated here instead of cpu_startup() because physical
	 * memory is directly addressable.  We don't have to map these into
	 * virtual address space.
	 */
	size = (vsize_t)allocsys(0);
	v = (caddr_t)pmap_steal_memory(size, NULL, NULL); 
	if ((allocsys(v) - v) != size)
		panic("mach_init: table size inconsistency");

	/*
	 * Initialize the virtual memory system.
	 */
	pmap_bootstrap();
}


/*
 * cpu_startup: allocate memory for variable-sized tables,
 * initialize cpu, and do autoconfiguration.
 */
void
cpu_startup()
{
	unsigned i;
	int base, residual;
	paddr_t minaddr, maxaddr;
	vsize_t size;
#ifdef DEBUG
	extern int pmapdebug;
	int opmapdebug = pmapdebug;

	pmapdebug = 0;
#endif

	/*
	 * Good {morning,afternoon,evening,night}.
	 */
	printf(version);

	printf("%s\n", cpu_model);

	printf("real mem  = %d\n", ctob(physmem));

	/*
	 * Allocate virtual address space for file I/O buffers.
	 * Note they are different than the array of headers, 'buf',
	 * and usually occupy more virtual memory than physical.
	 */
	size = MAXBSIZE * nbuf;
#if defined(UVM)
	if (uvm_map(kernel_map, (vaddr_t *)&buffers, round_page(size),
		    NULL, UVM_UNKNOWN_OFFSET,
		    UVM_MAPFLAG(UVM_PROT_NONE, UVM_PROT_NONE, UVM_INH_NONE,
				UVM_ADV_NORMAL, 0)) != KERN_SUCCESS)
		panic("startup: cannot allocate VM for buffers");
#else
	buffer_map = kmem_suballoc(kernel_map, (vaddr_t *)&buffers,
				   &maxaddr, size, TRUE);
	minaddr = (vaddr_t)buffers;
	if (vm_map_find(buffer_map, vm_object_allocate(size), (vaddr_t)0,
			&minaddr, size, FALSE) != KERN_SUCCESS)
		panic("startup: cannot allocate buffers");
#endif /* UVM */
	base = bufpages / nbuf;
	residual = bufpages % nbuf;
	for (i = 0; i < nbuf; i++) {
#if defined(UVM)
		vsize_t curbufsize;
		vaddr_t curbuf;
		struct vm_page *pg;

		/*
		 * Each buffer has MAXBSIZE bytes of VM space allocated.  Of
		 * that MAXBSIZE space, we allocate and map (base+1) pages
		 * for the first "residual" buffers, and then we allocate
		 * "base" pages for the rest.
		 */
		curbuf = (vaddr_t) buffers + (i * MAXBSIZE);
		curbufsize = CLBYTES * ((i < residual) ? (base+1) : base);

		while (curbufsize) {
			pg = uvm_pagealloc(NULL, 0, NULL);
			if (pg == NULL)
				panic("cpu_startup: not enough memory for "
				    "buffer cache");
#if defined(PMAP_NEW)
			pmap_kenter_pgs(curbuf, &pg, 1);
#else
			pmap_enter(kernel_map->pmap, curbuf,
				   VM_PAGE_TO_PHYS(pg), VM_PROT_ALL, TRUE);
#endif
			curbuf += PAGE_SIZE;
			curbufsize -= PAGE_SIZE;
		}
#else /* ! UVM */
		vsize_t curbufsize;
		vaddr_t curbuf;

		/*
		 * First <residual> buffers get (base+1) physical pages
		 * allocated for them.  The rest get (base) physical pages.
		 *
		 * The rest of each buffer occupies virtual space,
		 * but has no physical memory allocated for it.
		 */
		curbuf = (vaddr_t)buffers + i * MAXBSIZE;
		curbufsize = CLBYTES * (i < residual ? base+1 : base);
		vm_map_pageable(buffer_map, curbuf, curbuf+curbufsize, FALSE);
		vm_map_simplify(buffer_map, curbuf);
#endif /* UVM */
	}
	/*
	 * Allocate a submap for exec arguments.  This map effectively
	 * limits the number of processes exec'ing at any time.
	 */
#if defined(UVM)
	exec_map = uvm_km_suballoc(kernel_map, &minaddr, &maxaddr,
				   16 * NCARGS, TRUE, FALSE, NULL);
#else
	exec_map = kmem_suballoc(kernel_map, &minaddr, &maxaddr,
				 16 * NCARGS, TRUE);
#endif
	/*
	 * Allocate a submap for physio
	 */
#if defined(UVM)
	phys_map = uvm_km_suballoc(kernel_map, &minaddr, &maxaddr,
				   VM_PHYS_SIZE, TRUE, FALSE, NULL);
#else
	phys_map = kmem_suballoc(kernel_map, &minaddr, &maxaddr,
				 VM_PHYS_SIZE, TRUE);
#endif

	/*
	 * Finally, allocate mbuf cluster submap.
	 */
#if defined(UVM)
	mb_map = uvm_km_suballoc(kernel_map, &minaddr, &maxaddr,
			         VM_MBUF_SIZE, FALSE, FALSE, NULL);
#else
	mb_map = kmem_suballoc(kernel_map, &minaddr, &maxaddr,
			       VM_MBUF_SIZE, FALSE);
#endif
	/*
	 * Initialize callouts
	 */
	callfree = callout;
	for (i = 1; i < ncallout; i++)
		callout[i-1].c_next = &callout[i];
	callout[i-1].c_next = NULL;

#ifdef DEBUG
	pmapdebug = opmapdebug;
#endif
#if defined(UVM)
	printf("avail mem = %ld\n", ptoa(uvmexp.free));
#else
	printf("avail mem = %ld\n", ptoa(cnt.v_free_count));
#endif
	printf("using %d buffers containing %d bytes of memory\n",
		nbuf, bufpages * CLBYTES);

	/*
	 * Set up buffers, so they can be used to read disk labels.
	 */
	bufinit();

	/*
	 * Set up CPU-specific registers, cache, etc.
	 */
	initcpu();

	/*
	 * XXX THE FOLLOWING SECTION NEEDS TO BE REPLACED
	 * XXX WITH BUS_DMA(9).
	 * XXXXXX Huh?  BUS_DMA(9)  doesnt support  gap16 lance copy buffers.
	 * XXXXXX We use the copy suport  in am7990 instead.
	 */

#if NLE_IOASIC > 0
	/*
	 * Steal 128k of memory for the LANCE chip on machine where
	 * it does DMA through the IOCTL ASIC.  It must be physically
	 * contiguous and aligned on a 128k boundary.
	 */
	{
		extern paddr_t avail_start, avail_end;
		struct pglist mlist;

		TAILQ_INIT(&mlist);
#if defined(UVM)
		if (uvm_pglistalloc(128 * 1024, avail_start,
		    avail_end - PAGE_SIZE, 128 * 1024, 0, &mlist, 1, FALSE))
#else
		if (vm_page_alloc_memory(128 * 1024, avail_start,
		    avail_end - PAGE_SIZE, 128 * 1024, 0, &mlist, 1, FALSE))
#endif
			panic("startup: unable to steal LANCE DMA area");
		le_iomem = VM_PAGE_TO_PHYS(mlist.tqh_first);
	}
#endif /* NLE_IOASIC */

	/* Initialize interrupt table array */
	for (i = 0; i < MAX_DEV_NCOOKIES; i++) {
		intrtab[i].ih_func = nullintr;
		intrtab[i].ih_arg = (void *)i;
	}

	/*
	 * Configure the system.
	 */
	configure();
}


/*
 * machine dependent system variables.
 */
int
cpu_sysctl(name, namelen, oldp, oldlenp, newp, newlen, p)
	int *name;
	u_int namelen;
	void *oldp;
	size_t *oldlenp;
	void *newp;
	size_t newlen;
	struct proc *p;
{

	/* all sysctl names at this level are terminal */
	if (namelen != 1)
		return (ENOTDIR);		/* overloaded */

	switch (name[0]) {
	case CPU_CONSDEV:
		return (sysctl_rdstruct(oldp, oldlenp, newp, &cn_tab->cn_dev,
		    sizeof cn_tab->cn_dev));
	default:
		return (EOPNOTSUPP);
	}
	/* NOTREACHED */
}


/*
 * PROM reset callback for reset switch.
 * XXX enter ddb instead?
 */
void
prom_haltbutton()
{
	(*callv->_halt)((int *)0, 0);
}


/*
 * call PROM to halt or reboot.
 */
volatile void
prom_halt(howto, bootstr)
	int howto;
	char *bootstr;

{
	if (callv != &callvec) {
		if (howto & RB_HALT)
			(*callv->_rex)('h');
		else {
			(*callv->_rex)('b');
		}
	} else if (howto & RB_HALT) {
		volatile void (*f) __P((void)) = 
		    (volatile void (*) __P((void))) DEC_PROM_REINIT;

		(*f)();	/* jump back to prom monitor */
	} else {
		volatile void (*f) __P((void)) = 
		    (volatile void (*) __P((void)))DEC_PROM_AUTOBOOT;
		(*f)();	/* jump back to prom monitor and do 'auto' cmd */
	}

	while(1) ;	/* fool gcc */
	/*NOTREACHED*/
}


void
cpu_reboot(howto, bootstr)
	int howto;
	char *bootstr;
{
	extern int cold;

	/* take a snap shot before clobbering any registers */
	if (curproc)
		savectx((struct user *)curpcb);

#ifdef DEBUG
	if (panicstr)
		stacktrace();
#endif

	/* If system is cold, just halt. */
	if (cold) {
		howto |= RB_HALT;
		goto haltsys;
	}

	/* If "always halt" was specified as a boot flag, obey. */
	if ((boothowto & RB_HALT) != 0)
		howto |= RB_HALT;

	boothowto = howto;
	if ((howto & RB_NOSYNC) == 0) {
		/*
		 * Synchronize the disks....
		 */
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
#if 0
	if ((howto & (RB_DUMP | RB_HALT)) == RB_DUMP)
#else
	if (howto & RB_DUMP)
#endif
		dumpsys();

haltsys:

	/* run any shutdown hooks */
	doshutdownhooks();


	/* Finally, halt/reboot the system. */
	printf("%s\n\n", howto & RB_HALT ? "halted." : "rebooting...");
	prom_halt(howto & RB_HALT, bootstr);
	/*NOTREACHED*/
}

#include "opt_dec_3min.h"
#include "opt_dec_maxine.h"
#include "opt_dec_3maxplus.h"

/*
 * Return the best possible estimate of the time in the timeval
 * to which tvp points.  Unfortunately, we can't read the hardware registers.
 * We guarantee that the time will be greater than the value obtained by a
 * previous call.
 */
void
microtime(tvp)
	struct timeval *tvp;
{
	int s = splclock();
	static struct timeval lasttime;

	*tvp = time;
#if (DEC_3MIN + DEC_MAXINE + DEC_3MAXPLUS) > 0
	tvp->tv_usec += (*clkread)();
#endif
	if (tvp->tv_usec >= 1000000) {
		tvp->tv_usec -= 1000000;
		tvp->tv_sec++;
	}

	if (tvp->tv_sec == lasttime.tv_sec &&
	    tvp->tv_usec <= lasttime.tv_usec &&
	    (tvp->tv_usec = lasttime.tv_usec + 1) > 1000000) {
		tvp->tv_sec++;
		tvp->tv_usec -= 1000000;
	}
	lasttime = *tvp;
	splx(s);
}

int
initcpu()
{
	volatile struct chiptime *c;
	int i = 0;

	/*
	 * reset after autoconfig probe:
	 * clear  any memory errors, reset any pending interrupts.
	 */

	(*platform.bus_reset)();	/* XXX_cf_alpha */

	/*
	 * With newconf, this should be  done elswhere, but without it
	 * we hang (?)
	 */
#if 1 /*XXX*/
	/* disable clock interrupts (until startrtclock()) */
	if (mcclock_addr) {
		c = mcclock_addr;
		c->regb = REGB_DATA_MODE | REGB_HOURS_FORMAT;
		i = c->regc;
	}
	return (i);
#endif
}

/*
 * Wait "n" microseconds. (scsi code needs this).
 */
void
delay(n)
        unsigned n;
{
        DELAY(n);
}

/*
 * Convert an ASCII string into an integer.
 */
int
atoi(s)
	const char *s;
{
	int c;
	unsigned base = 10, d;
	int neg = 0, val = 0;

	if (s == 0 || (c = *s++) == 0)
		goto out;

	/* skip spaces if any */
	while (c == ' ' || c == '\t')
		c = *s++;

	/* parse sign, allow more than one (compat) */
	while (c == '-') {
		neg = !neg;
		c = *s++;
	}

	/* parse base specification, if any */
	if (c == '0') {
		c = *s++;
		switch (c) {
		case 'X':
		case 'x':
			base = 16;
			break;
		case 'B':
		case 'b':
			base = 2;
			break;
		default:
			base = 8;
		}
	}

	/* parse number proper */
	for (;;) {
		if (c >= '0' && c <= '9')
			d = c - '0';
		else if (c >= 'a' && c <= 'z')
			d = c - 'a' + 10;
		else if (c >= 'A' && c <= 'Z')
			d = c - 'A' + 10;
		else
			break;
		val *= base;
		val += d;
		c = *s++;
	}
	if (neg)
		val = -val;
out:
	return val;	
}

/*
 *  Ensure all platform vectors are always initialized.
 */

void
unimpl_os_init() 
{
	panic("sysconf.init didnt set os_init");
}

void
unimpl_bus_reset()
{
	panic("sysconf.init didnt set bus_reset");
}

int
unimpl_intr(mask, pc, statusreg, causereg)
	u_int mask;
	u_int pc;
	u_int statusreg;
	u_int causereg;
{
	panic("sysconf.init didnt set intr");	
}

void
unimpl_cons_init()
{
	panic("sysconf.init didnt set cons_init");	
}

void
unimpl_device_register(sc, arg)
	struct device *sc;
	void *arg;
{
	panic("sysconf.init didnt set device_register");	
}

const char*
unimpl_model_name()
{
	panic("sysconf.init didnt set model_name");	
}

void
unimpl_clockintr(arg)
	void *arg;
{
	panic("sysconf.init didnt set clockintr");
}

void
unimpl_iointr(arg, arg2)
	void *arg;
	u_long arg2;
{
	panic("sysconf.init didnt set iointr");
}

void
unimpl_errintr()
{
	panic("sysconf.init didnt set errintr_name");	
}

int
nullintr(v)
	void *v;
{
	printf("uncaught interrupt: cookie %d\n", (int)v);
	return 1;
}

unsigned
nullclkread()
{
	return 0;
}
