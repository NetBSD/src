/*	$NetBSD: machdep.c,v 1.33 1999/12/22 05:55:26 tsubai Exp $	*/

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

__KERNEL_RCSID(0, "$NetBSD: machdep.c,v 1.33 1999/12/22 05:55:26 tsubai Exp $");

/* from: Utah Hdr: machdep.c 1.63 91/04/24 */

#include "fs_mfs.h"
#include "opt_ddb.h"
#include "opt_execfmt.h"

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
#include <sys/device.h>
#include <sys/user.h>
#include <sys/exec.h>
#include <vm/vm.h>
#include <sys/sysctl.h>
#include <sys/mount.h>
#include <sys/syscallargs.h>
#include <sys/kcore.h>

#include <vm/vm_kern.h>

#include <uvm/uvm_extern.h>

#include <ufs/mfs/mfs_extern.h>		/* mfs_initminiroot() */

#include <machine/cpu.h>
#include <machine/reg.h>
#include <machine/psl.h>
#include <machine/pte.h>
#include <machine/autoconf.h>
#include <machine/bootinfo.h>
#include <machine/apbus.h>
#include <machine/apcall.h>
#include <mips/locore.h>		/* wbflush() */

#ifdef DDB
#include <machine/db_machdep.h>
#include <ddb/db_access.h>
#include <ddb/db_extern.h>
#include <ddb/db_sym.h>
#endif

#include <machine/adrsmap.h>
#include <machine/machConst.h>
#include <machine/intr.h>
#include <newsmips/newsmips/clockreg.h>
#include <newsmips/newsmips/machid.h>
#include <dev/cons.h>

/* the following is used externally (sysctl_hw) */
char machine[] = MACHINE;	/* from <machine/param.h> */
char machine_arch[] = MACHINE_ARCH;
char cpu_model[30];

/* maps for VM objects */

vm_map_t exec_map = NULL;
vm_map_t mb_map = NULL;
vm_map_t phys_map = NULL;

char *bootinfo = NULL;		/* pointer to bootinfo structure */
int physmem;			/* max supported memory, changes to actual */
int systype;			/* what type of NEWS we are */
struct apbus_sysinfo *_sip = NULL;

phys_ram_seg_t mem_clusters[VM_PHYSSEG_MAX];
int mem_cluster_cnt;

struct idrom idrom;
void (*enable_intr) __P((void));
void (*disable_intr) __P((void));

/* System type dependent initializations. */
extern void news3400_init __P((void));
extern void news5000_init __P((void));

/*
 * Interrupt-blocking functions defined in locore. These names aren't used
 * directly except here and in interrupt handlers.
 */

/* Block out nested interrupt-enable bits. */
extern int	cpu_spl0 __P((void)), cpu_spl1 __P((void));
extern int	cpu_spl2 __P((void)), cpu_spl3 __P((void));
extern int	splhigh __P((void));

void to_monitor __P((int)) __attribute__((__noreturn__));

/*
 *  Local functions.
 */

/* initialize bss, etc. from kernel start, before main() is called. */
void mach_init __P((int, int, int, int));

void prom_halt __P((int)) __attribute__((__noreturn__));

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
int safepri = MIPS3_PSL_LOWIPL;		/* XXX */

extern struct user *proc0paddr;
extern u_long bootdev;
extern char edata[], end[];

/*
 * Do all the stuff that locore normally does before calling main().
 * Process arguments passed to us by the prom monitor.
 * Return the first page address following the system.
 */
void
mach_init(x_boothowto, x_bootdev, x_bootname, x_maxmem)
	int x_boothowto;
	int x_bootdev;
	int x_bootname;
	int x_maxmem;
{
	register int i;
	u_long first, last;
	caddr_t kernend, v;
	vsize_t size;
	struct btinfo_magic *bi_magic;
	struct btinfo_bootarg *bi_arg;
	struct btinfo_systype *bi_systype;
#ifdef DDB
	struct btinfo_symtab *bi_sym;
	int nsym = 0;
	char *ssym, *esym;
#endif

	/* clear the BSS segment */
	bzero(edata, end - edata);

	systype = NEWS3400;			/* XXX compatibility */

	bootinfo = (void *)BOOTINFO_ADDR;	/* XXX */
	bi_magic = lookup_bootinfo(BTINFO_MAGIC);
	if (bi_magic && bi_magic->magic == BOOTINFO_MAGIC) {
		bi_arg = lookup_bootinfo(BTINFO_BOOTARG);
		if (bi_arg) {
			x_boothowto = bi_arg->howto;
			x_bootdev = bi_arg->bootdev;
			x_maxmem = bi_arg->maxmem;
		}
#ifdef DDB
		bi_sym = lookup_bootinfo(BTINFO_SYMTAB);
		if (bi_sym) {
			nsym = bi_sym->nsym;
			ssym = (void *)bi_sym->ssym;
			esym = (void *)bi_sym->esym;
		}
#endif

		bi_systype = lookup_bootinfo(BTINFO_SYSTYPE);
		if (bi_systype)
			systype = bi_systype->type;
	}

#ifdef news5000
	if (systype == NEWS5000) {
		_sip = (void *)bi_arg->sip;
		x_maxmem = _sip->apbsi_memsize;
		x_maxmem -= 0x00100000;	/* reserve 1MB for ROM monitor */
		apcall_write(1, "Hello APbus\n", 12);

		consinit();
	}
#endif

	/*
	 * Save parameters into kernel work area.
	 */
	*(int *)(MIPS_PHYS_TO_KSEG1(MACH_MAXMEMSIZE_ADDR)) = x_maxmem;
	*(int *)(MIPS_PHYS_TO_KSEG1(MACH_BOOTDEV_ADDR)) = x_bootdev;
	*(int *)(MIPS_PHYS_TO_KSEG1(MACH_BOOTSW_ADDR)) = x_boothowto;

	kernend = (caddr_t)mips_round_page(end);
#ifdef DDB
	if (nsym)
		kernend = (caddr_t)mips_round_page(esym);
#endif

	/*
	 * Set the VM page size.
	 */
	uvm_setpagesize();

	boothowto = x_boothowto;
	bootdev = x_bootdev;
	physmem = btoc(x_maxmem);

	/*
	 * Now that we know how much memory we have, initialize the
	 * mem cluster array.
	 */
	mem_clusters[0].start = 0;		/* XXX is this correct? */
	mem_clusters[0].size  = ctob(physmem);
	mem_cluster_cnt = 1;

	/*
	 * Copy exception-dispatch code down to exception vector.
	 * Initialize locore-function vector.
	 * Clear out the I and D caches.
	 */
	mips_vector_init();
#if 0
	if (systype == NEWS5000) {
		mips_L2CacheSize = 1024 * 1024;		/* XXX to be safe */
		mips3_FlushCache();
	}
#endif

#ifdef DDB
	/*
	 * Initialize machine-dependent DDB commands, in case of early panic.
	 */
	db_machine_init();
	if (nsym)
		ddb_init(esym - ssym, ssym, esym);
#endif

	boothowto &= ~RB_ASKNAME;	/* for lack of cn_getc */
#ifdef KADB
	boothowto |= RB_KDB;
#endif

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

	/*
	 * Alloc u pages for proc0 stealing KSEG0 memory.
	 */
	proc0.p_addr = proc0paddr = (struct user *)kernend;
	proc0.p_md.md_regs =
	    (struct frame *)((caddr_t)kernend + UPAGES * PAGE_SIZE) - 1;
	curpcb = &proc0.p_addr->u_pcb;
	memset(kernend, 0, UPAGES * PAGE_SIZE);

	kernend += UPAGES * PAGE_SIZE;

	/*
	 * Load the rest of the available pages into the VM system.
	 */
	first = round_page(MIPS_KSEG0_TO_PHYS(kernend));
	last = mem_clusters[0].start + mem_clusters[0].size;
	uvm_page_physload(atop(first), atop(last), atop(first), atop(last),
	    VM_FREELIST_DEFAULT);

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
	size = (vsize_t)allocsys(NULL, NULL);
	v = (caddr_t)pmap_steal_memory(size, NULL, NULL); 
	if ((allocsys(v, NULL) - v) != size)
		panic("mach_init: table size inconsistency");

	/*
	 * Determine what model of computer we are running on.
	 */
	switch (systype) {
#ifdef news3400
	case NEWS3400:
		news3400_init();
		break;
#endif

#ifdef news5000
	case NEWS5000:
		news5000_init();
		break;
#endif
	}

	i = idrom.id_modelid;

	switch (i) {

	default:
		printf("kernel not configured for systype 0x%x\n", i);
		/* cpu_reboot(RB_HALT | RB_NOSYNC, NULL); */

#ifdef news5000
	case 2: /* NWS-5000U/W */
	case 5: /* NWS-5000R */
	case 7: /* NWS-5000X */
	case 9: /* NWS-5900X */
	case 11:/* NWS-5000G */
		/*
		 * Set up interrupt handling and I/O addresses.
		 */
		mips_hardware_intr = news5000_intr;
		strcpy(cpu_model, "news5000");
		cpuspeed = 50;	/* ??? XXX */
		break;
#endif

#ifdef news3400
	case 3: /* NWS-3410 */
	case 6: /* NWS-3470 */
		/*
		 * Set up interrupt handling and I/O addresses.
		 */
		mips_hardware_intr = news3400_intr;
		strcpy(cpu_model, "news3400");
		cpuspeed = 10;
		break;
#endif
	}

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
	register unsigned i;
	int base, residual;
	vaddr_t minaddr, maxaddr;
	vsize_t size;
	char pbuf[9];
#ifdef DEBUG
	extern int pmapdebug;
	int opmapdebug = pmapdebug;

	pmapdebug = 0;
#endif

	/*
	 * Good {morning,afternoon,evening,night}.
	 */
	printf(version);
	format_bytes(pbuf, sizeof(pbuf), ctob(physmem));
	printf("total memory = %s\n", pbuf);

	/*
	 * Allocate virtual address space for file I/O buffers.
	 * Note they are different than the array of headers, 'buf',
	 * and usually occupy more virtual memory than physical.
	 */
	size = MAXBSIZE * nbuf;
	if (uvm_map(kernel_map, (vaddr_t *)&buffers, round_page(size),
		    NULL, UVM_UNKNOWN_OFFSET,
		    UVM_MAPFLAG(UVM_PROT_NONE, UVM_PROT_NONE, UVM_INH_NONE,
				UVM_ADV_NORMAL, 0)) != KERN_SUCCESS)
		panic("startup: cannot allocate VM for buffers");
	minaddr = (vaddr_t)buffers;
	base = bufpages / nbuf;
	residual = bufpages % nbuf;
	for (i = 0; i < nbuf; i++) {
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
		curbufsize = NBPG * ((i < residual) ? (base+1) : base);

		while (curbufsize) {
			pg = uvm_pagealloc(NULL, 0, NULL, 0);
			if (pg == NULL)
				panic("cpu_startup: not enough memory for "
				    "buffer cache");
			pmap_kenter_pa(curbuf, VM_PAGE_TO_PHYS(pg),
				       VM_PROT_READ|VM_PROT_WRITE);
			curbuf += PAGE_SIZE;
			curbufsize -= PAGE_SIZE;
		}
	}
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
	 * No need to allocate an mbuf cluster submap.  Mbuf clusters
	 * are allocated via the pool allocator, and we use KSEG to
	 * map those pages.
	 */

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
	format_bytes(pbuf, sizeof(pbuf), ptoa(uvmexp.free));
	printf("avail memory = %s\n", pbuf);
	format_bytes(pbuf, sizeof(pbuf), bufpages * NBPG);
	printf("using %d buffers containing %s of memory\n", nbuf, pbuf);

	/*
	 * Set up buffers, so they can be used to read disk labels.
	 */
	bufinit();
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

	default:
		return (EOPNOTSUPP);
	}
	/* NOTREACHED */
}

/*
 * lookup_bootinfo:
 * Look up information in bootinfo of boot loader.
 */
void *
lookup_bootinfo(type)
	int type;
{
	struct btinfo_common *bt;
	char *help = bootinfo;

	/* Check for a bootinfo record first. */
	if (help == NULL)
		return (NULL);

	do {
		bt = (struct btinfo_common *)help;
		if (bt->type == type)
			return ((void *)help);
		help += bt->next;
	} while (bt->next != 0 &&
		(size_t)help < (size_t)bootinfo + BOOTINFO_SIZE);

	return (NULL);
}

/*
 * call PROM to halt or reboot.
 */
void
prom_halt(howto)
	int howto;

{
#ifdef news5000
	if (systype == NEWS5000) {
		howto = RB_HALT;	/* XXX */
		apcall_exit(howto);
	}
#endif
#ifdef news3400
	if (systype == NEWS3400)
		to_monitor(howto);
#endif
	for (;;);
}

int	waittime = -1;

void
cpu_reboot(howto, bootstr)
	volatile int howto;
	char *bootstr;
{

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
	if ((howto & RB_NOSYNC) == 0 && waittime < 0) {
		/*
		 * Synchronize the disks....
		 */
		waittime = 0;
		vfs_shutdown();

		/*
		 * If we've been adjusting the clock, the todr
		 * will be out of synch; adjust it now.
		 */
		resettodr();
	}

	/* Disable interrupts. */
	disable_intr();

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

	if ((howto & RB_POWERDOWN) == RB_POWERDOWN)
		prom_halt(0x80);	/* rom monitor RB_PWOFF */

	/* Finally, halt/reboot the system. */
	printf("%s\n\n", howto & RB_HALT ? "halted." : "rebooting...");
	prom_halt(howto & RB_HALT);
	/*NOTREACHED*/
}

/*
 * Return the best possible estimate of the time in the timeval
 * to which tvp points.  Unfortunately, we can't read the hardware registers.
 * We guarantee that the time will be greater than the value obtained by a
 * previous call.
 */
void
microtime(tvp)
	register struct timeval *tvp;
{
	int s = splclock();
	static struct timeval lasttime;

	*tvp = time;
#if 0
	tvp->tv_usec += clkread();
	if (tvp->tv_usec >= 1000000) {
		tvp->tv_usec -= 1000000;
		tvp->tv_sec++;
	}
#endif

	if (tvp->tv_sec == lasttime.tv_sec &&
	    tvp->tv_usec <= lasttime.tv_usec &&
	    (tvp->tv_usec = lasttime.tv_usec + 1) > 1000000) {
		tvp->tv_sec++;
		tvp->tv_usec -= 1000000;
	}
	lasttime = *tvp;
	splx(s);
}

void
delay(n)
	int n;
{
	DELAY(n);
}

#ifdef EXEC_ECOFF
#include <sys/exec_ecoff.h>

int
cpu_exec_ecoff_hook(p, epp)
	struct proc *p;
	struct exec_package *epp;
{
	extern struct emul emul_netbsd;

	epp->ep_emul = &emul_netbsd;

	return 0;
}
#endif
