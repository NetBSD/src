/*	$NetBSD: machdep.c,v 1.28 2001/07/08 04:25:36 wdk Exp $	*/

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

__KERNEL_RCSID(0, "$NetBSD: machdep.c,v 1.28 2001/07/08 04:25:36 wdk Exp $");

/* from: Utah Hdr: machdep.c 1.63 91/04/24 */

#include "opt_ddb.h"
#include "opt_kgdb.h"

#include <sys/param.h>
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
#include <sys/sysctl.h>
#include <sys/mount.h>
#include <sys/syscallargs.h>
#include <sys/kcore.h>

#include <uvm/uvm_extern.h>

#include <ufs/mfs/mfs_extern.h>		/* mfs_initminiroot() */

#include <machine/cpu.h>
#include <machine/reg.h>
#include <machine/psl.h>
#include <machine/pte.h>

#ifdef DDB
#include <machine/db_machdep.h>
#include <ddb/db_extern.h>
#endif

#include <machine/intr.h>
#include <machine/mainboard.h>
#include <machine/sysconf.h>
#include <machine/autoconf.h>
#include <machine/bootinfo.h>
#include <machine/prom.h>
#include <dev/clock_subr.h>
#include <dev/cons.h>

#include <sys/boot_flag.h>

#include "fs_mfs.h"
#include "opt_ddb.h"
#include "opt_execfmt.h"

#include "zsc.h"			/* XXX */
#include "com.h"			/* XXX */

/* the following is used externally (sysctl_hw) */
char  machine[] = MACHINE;	/* from <machine/param.h> */
char  machine_arch[] = MACHINE_ARCH;
char  cpu_model[40];

/* Our exported CPU info; we can have only one. */  
struct cpu_info cpu_info_store;

/* maps for VM objects */

struct vm_map *exec_map = NULL;
struct vm_map *mb_map = NULL;
struct vm_map *phys_map = NULL;

int	physmem;		/* max supported memory, changes to actual */
char	*bootinfo = NULL;	/* pointer to bootinfo structure */

phys_ram_seg_t mem_clusters[VM_PHYSSEG_MAX];
int mem_cluster_cnt;

void to_monitor __P((int)) __attribute__((__noreturn__));
void prom_halt __P((int)) __attribute__((__noreturn__));

#ifdef	KGDB
void zs_kgdb_init __P((void));
void kgdb_connect __P((int));
#endif

struct evcnt soft_evcnt[IPL_NSOFT];

/*
 *  Local functions.
 */
int initcpu __P((void));
void configure __P((void));

void mach_init __P((int, char *[], char*[], u_int, char *));
int  memsize_scan __P((caddr_t));

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
extern struct user *proc0paddr;

/* locore callback-vector setup */
extern void mips_vector_init  __P((void));
extern void prom_init  __P((void));
extern void pizazz_init __P((void));

/* platform-specific initialization vector */
static void	unimpl_cons_init __P((void));
static void	unimpl_iointr __P((unsigned, unsigned, unsigned, unsigned));
static int	unimpl_memsize __P((caddr_t));
static unsigned	unimpl_clkread __P((void));
static void	unimpl_todr __P((struct clock_ymdhms *));
static void	unimpl_intr_establish __P((int, int (*)__P((void *)), void *));

struct platform platform = {
	"iobus not set",
	unimpl_cons_init,
	unimpl_iointr,
	unimpl_memsize,
	unimpl_clkread,
	unimpl_todr,
	unimpl_todr,
	unimpl_intr_establish,
};

struct consdev *cn_tab = NULL;
extern struct consdev consdev_prom;
extern struct consdev consdev_zs;

static void null_cnprobe __P((struct consdev *));
static void prom_cninit __P((struct consdev *));
static int  prom_cngetc __P((dev_t));
static void prom_cnputc __P((dev_t, int));
static void null_cnpollc __P((dev_t, int));

struct consdev consdev_prom = {
        null_cnprobe,
	prom_cninit,
	prom_cngetc,
	prom_cnputc,
        null_cnpollc,
};


/*
 * Do all the stuff that locore normally does before calling main().
 * Process arguments passed to us by the prom monitor.
 * Return the first page address following the system.
 */
void
mach_init(argc, argv, envp, bim, bip)
	int    argc;
	char   *argv[];
	char   *envp[];
	u_int  bim;
	char   *bip;
{
	u_long first, last;
	caddr_t kernend, v;
	vsize_t size;
	char *cp;
	int i, howto;
	extern char edata[], end[];
	char *bi_msg;
#ifdef DDB
	int nsym = 0;
	caddr_t ssym = 0;
	caddr_t esym = 0;
	struct btinfo_symtab *bi_syms;
#endif


	/* Check for valid bootinfo passed from bootstrap */
	if (bim == BOOTINFO_MAGIC) {
		struct btinfo_magic *bi_magic;

		bootinfo = (char *)BOOTINFO_ADDR; /* XXX */
		bi_magic = lookup_bootinfo(BTINFO_MAGIC);
		if (bi_magic == NULL || bi_magic->magic != BOOTINFO_MAGIC)
			bi_msg = "invalid bootinfo structure.\n";
		else
			bi_msg = NULL;
	} else
		bi_msg = "invalid bootinfo (standalone boot?)\n";

	/* clear the BSS segment */
	kernend = (caddr_t)mips_round_page(end);
	memset(edata, 0, end - edata);

#ifdef DDB
	bi_syms = lookup_bootinfo(BTINFO_SYMTAB);

	/* Load sysmbol table if present */
	if (bi_syms != NULL) {
		nsym = bi_syms->nsym;
		ssym = (caddr_t)bi_syms->ssym;
		esym = (caddr_t)bi_syms->esym;
		kernend = (caddr_t)mips_round_page(esym);
	}
#endif

	prom_init();
	consinit();

	if (bi_msg != NULL)
		printf(bi_msg);

	/*
	 * Set the VM page size.
	 */
	uvm_setpagesize();

	/* Find out how much memory is available. */
	physmem = memsize_scan(kernend);

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

	/* Look at argv[0] and compute bootdev */
	makebootdev(argv[0]);

	/*
	 * Look at arguments passed to us and compute boothowto.
	 */
	boothowto = RB_AUTOBOOT;
	for (i = 1; i < argc; i++) {
		for (cp = argv[i]; *cp; cp++) {
			/* Ignore superfluous '-', if there is one */
			if (*cp == '-')
				continue;

			howto = 0;
			BOOT_FLAG(*cp, howto);
			if (! howto)
				printf("bootflag '%c' not recognised\n", *cp);
			else
				boothowto |= howto;
		}
	}


#ifdef DDB
	/* init symbols if present */
	if (esym)
		ddb_init(esym - ssym, ssym, esym);
	if (boothowto & RB_KDB)
		Debugger();
#endif
#ifdef KGDB
	zs_kgdb_init();			/* XXX */
	if (boothowto & RB_KDB)
		kgdb_connect(0);
#endif

#ifdef MFS
	/*
	 * Check to see if a mini-root was loaded into memory. It resides
	 * at the start of the next page just after the end of BSS.
	 */
	if (boothowto & RB_MINIROOT)
		kernend += round_page(mfs_initminiroot(kernend));
#endif

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
	 * Compute the size of system data structures.  pmap_bootstrap()
	 * needs some of this information.
	 */
	size = (vsize_t)allocsys(NULL, NULL);

	/*
	 * Initialize the virtual memory system.
	 */
	pmap_bootstrap();

	/*
	 * Allocate space for proc0's USPACE.
	 */
	v = (caddr_t)uvm_pageboot_alloc(USPACE); 
	proc0.p_addr = proc0paddr = (struct user *)v;
	proc0.p_md.md_regs = (struct frame *)(v + USPACE) - 1;
	curpcb = &proc0.p_addr->u_pcb;
	curpcb->pcb_context[11] = MIPS_INT_MASK | MIPS_SR_INT_IE; /* SR */

	/*
	 * Allocate space for system data structures.  These data structures
	 * are allocated here instead of cpu_startup() because physical
	 * memory is directly addressable.  We don't have to map these into
	 * virtual address space.
	 */
	v = (caddr_t)uvm_pageboot_alloc(size); 
	if ((allocsys(v, NULL) - v) != size)
		panic("mach_init: table size inconsistency");
	/*
	 * Set up interrupt handling and I/O addresses.
	 */

	pizazz_init();
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
	printf("%s\n", cpu_model);
	format_bytes(pbuf, sizeof(pbuf), ctob(physmem));
	printf("total memory = %s\n", pbuf);

	/*
	 * Allocate virtual address space for file I/O buffers.
	 * Note they are different than the array of headers, 'buf',
	 * and usually occupy more virtual memory than physical.
	 */
	size = MAXBSIZE * nbuf;
	if (uvm_map(kernel_map, (vaddr_t *)&buffers, round_page(size),
		    NULL, UVM_UNKNOWN_OFFSET, 0,
		    UVM_MAPFLAG(UVM_PROT_NONE, UVM_PROT_NONE, UVM_INH_NONE,
				UVM_ADV_NORMAL, 0)) != 0)
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
	pmap_update();

	/*
	 * Allocate a submap for exec arguments.  This map effectively
	 * limits the number of processes exec'ing at any time.
	 */
	exec_map = uvm_km_suballoc(kernel_map, &minaddr, &maxaddr,
				   16 * NCARGS, TRUE, FALSE, NULL);
	/*
	 * Allocate a submap for physio
	 */
	phys_map = uvm_km_suballoc(kernel_map, &minaddr, &maxaddr,
				   VM_PHYS_SIZE, TRUE, FALSE, NULL);

	/*
	 * No need to allocate an mbuf cluster submap.  Mbuf clusters
	 * are allocated via the pool allocator, and we use KSEG to
	 * map those pages.
	 */

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

int	waittime = -1;

/*
 * call PROM to halt or reboot.
 */
void
prom_halt(howto)
	int howto;
{
	if (howto & RB_HALT)
		MIPS_PROM(reinit)();
	MIPS_PROM(reboot)();
	/* NOTREACHED */
}

void
cpu_reboot(howto, bootstr)
	volatile int howto;
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
	static struct timeval lasttime;
	int s = splclock();

	*tvp = time;

	tvp->tv_usec += (*platform.clkread)();

	while (tvp->tv_usec >= 1000000) {
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
	spl0();		/* safe to turn interrupts on now */
	return 0;
}

static void
unimpl_cons_init()
{

	panic("sysconf.init didn't set cons_init");
}

static void
unimpl_iointr(mask, pc, statusreg, causereg)
	u_int mask;
	u_int pc;
	u_int statusreg;
	u_int causereg;
{

	panic("sysconf.init didn't set intr");
}

static int
unimpl_memsize(first)
caddr_t first;
{

	panic("sysconf.init didn't set memsize");
}

static unsigned
unimpl_clkread()
{
	return 0;	/* No microtime available */
}

static void
unimpl_todr(dt)
	struct clock_ymdhms *dt;
{
	panic("sysconf.init didn't init TOD");
}

void
unimpl_intr_establish(level, func, arg)
	int level;
	int (*func) __P((void *));
	void *arg;
{
	panic("sysconf.init didn't init intr_establish\n");
}

void
delay(n)
	int n;
{
	DELAY(n);
}

/*
 * Find out how much memory is available by testing memory.
 * Be careful to save and restore the original contents for msgbuf.
 */
int
memsize_scan(first)
	caddr_t first;
{
	volatile int *vp, *vp0;
	int mem, tmp, tmp0;

#define PATTERN1 0xa5a5a5a5
#define	PATTERN2 ~PATTERN1

	/*
	 * Non destructive scan of memory to determine the size
	 * Use the first page to test for memory aliases.  This
	 * also has the side effect of flushing the bus alignment
	 * buffer
	 */
	mem = btoc((paddr_t)first - MIPS_KSEG0_START);
	vp = (int *)MIPS_PHYS_TO_KSEG1(mem << PGSHIFT);
	vp0 = (int *)MIPS_PHYS_TO_KSEG1(0); /* Start of physical memory */
	tmp0 = *vp0;
	while (vp < (int *)MIPS_MAX_MEM_ADDR) {
		tmp = *vp;
		*vp  = PATTERN1;
		*vp0 = PATTERN2;
		wbflush();
		if (*vp != PATTERN1)
			break;
		*vp  = PATTERN2;
		*vp0 = PATTERN1;
		wbflush();
		if (*vp != PATTERN2)
			break;
		*vp = tmp;
		vp += NBPG/sizeof(int);
		mem++;
	}
	*vp0 = tmp0;
	return mem;
}

/*
 * Console initialization: called early on from main,
 * before vm init or startup.  Do enough configuration
 * to choose and initialize a console.
 */

static void
null_cnprobe(cn)
     struct consdev *cn;
{
}

static void
prom_cninit(cn)
	struct consdev *cn;
{
	cn->cn_dev = makedev(0, 0);
	cn->cn_pri = CN_REMOTE;
}

static int
prom_cngetc(dev)
	dev_t dev;
{
	return MIPS_PROM(getchar)();
}

static void
prom_cnputc(dev, c)
	dev_t dev;
	int c;
{
	MIPS_PROM(putchar)(c);
}

static void
null_cnpollc(dev, on)
	dev_t dev;
	int on;
{
}

void
consinit()
{
	int zs_unit;

	zs_unit = 0;
	cn_tab = &consdev_zs;

	(*cn_tab->cn_init)(cn_tab);
}
