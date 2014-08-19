/*	$NetBSD: machdep.c,v 1.76.2.1 2014/08/20 00:03:13 tls Exp $	*/

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
 *	@(#)machdep.c	8.3 (Berkeley) 1/12/94
 */

#include <sys/cdefs.h>			/* RCS ID & Copyright macro defns */

__KERNEL_RCSID(0, "$NetBSD: machdep.c,v 1.76.2.1 2014/08/20 00:03:13 tls Exp $");

/* from: Utah Hdr: machdep.c 1.63 91/04/24 */

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
#include <sys/callout.h>
#include <sys/malloc.h>
#include <sys/mbuf.h>
#include <sys/msgbuf.h>
#include <sys/ioctl.h>
#include <sys/device.h>
#include <sys/exec.h>
#include <sys/mount.h>
#include <sys/syscallargs.h>
#include <sys/kcore.h>
#include <sys/ksyms.h>
#include <sys/cpu.h>

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

#include "opt_execfmt.h"

#include "zsc.h"			/* XXX */
#include "com.h"			/* XXX */
#include "ksyms.h"

/* maps for VM objects */

struct vm_map *phys_map = NULL;

char	*bootinfo = NULL;	/* pointer to bootinfo structure */

phys_ram_seg_t mem_clusters[VM_PHYSSEG_MAX];
int mem_cluster_cnt;

void to_monitor(int) __dead;
void prom_halt(int) __dead;

#ifdef	KGDB
void zs_kgdb_init(void);
void kgdb_connect(int);
#endif

/*
 *  Local functions.
 */
int initcpu(void);
void configure(void);

void mach_init(int, char *[], char*[], u_int, char *);
int  memsize_scan(void *);

#ifdef DEBUG
/* stacktrace code violates prototypes to get callee's registers */
extern void stacktrace(void); /*XXX*/
#endif

/* locore callback-vector setup */
extern void prom_init(void);
extern void pizazz_init(void);

/* platform-specific initialization vector */
static void	unimpl_cons_init(void);
static void	unimpl_iointr(uint32_t, vaddr_t, uint32_t);
static int	unimpl_memsize(void *);
static void	unimpl_intr_establish(int, int (*)(void *), void *);

struct platform platform = {
	.iobus = "iobus not set",
	.cons_init = unimpl_cons_init,
	.iointr = unimpl_iointr,
	.memsize = unimpl_memsize,
	.intr_establish = unimpl_intr_establish,
	.clkinit = NULL,
};

extern struct consdev consdev_prom;
extern struct consdev consdev_zs;

static void null_cnprobe(struct consdev *);
static void prom_cninit(struct consdev *);
static int  prom_cngetc(dev_t);
static void prom_cnputc(dev_t, int);
static void null_cnpollc(dev_t, int);

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
mach_init(int argc, char *argv[], char *envp[], u_int bim, char *bip)
{
	u_long first, last;
	char *kernend;
	char *cp;
	int i, howto;
	extern char edata[], end[];
	const char *bi_msg;
#if NKSYMS || defined(DDB) || defined(MODULAR)
	char *ssym = 0;
	char *esym = 0;
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
	kernend = (void *)mips_round_page(end);
	memset(edata, 0, end - edata);

	/*
	 * Copy exception-dispatch code down to exception vector.
	 * Initialize locore-function vector.
	 * Clear out the I and D caches.
	 */
	mips_vector_init(NULL, false);

#if NKSYMS || defined(DDB) || defined(LKM)
	bi_syms = lookup_bootinfo(BTINFO_SYMTAB);

	/* Load sysmbol table if present */
	if (bi_syms != NULL) {
		ssym = (void *)bi_syms->ssym;
		esym = (void *)bi_syms->esym;
		kernend = (void *)mips_round_page(esym);
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


#if NKSYMS || defined(DDB) || defined(MODULAR)
	/* init symbols if present */
	if (esym)
		ksyms_addsyms_elf(esym - ssym, ssym, esym);
#endif
#ifdef DDB
	if (boothowto & RB_KDB)
		Debugger();
#endif
#ifdef KGDB
	zs_kgdb_init();			/* XXX */
	if (boothowto & RB_KDB)
		kgdb_connect(0);
#endif

	/*
	 * Check to see if a mini-root was loaded into memory. It resides
	 * at the start of the next page just after the end of BSS.
	 */
	if (boothowto & RB_MINIROOT)
		kernend += round_page(mfs_initminiroot(kernend));

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
	 * Initialize the virtual memory system.
	 */
	pmap_bootstrap();

	/*
	 * Allocate uarea page for lwp0 and set it.
	 */
	mips_init_lwp0_uarea();

	/*
	 * Set up interrupt handling and I/O addresses.
	 */
	pizazz_init();
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

	/*
	 * Good {morning,afternoon,evening,night}.
	 */
	printf("%s%s", copyright, version);
	printf("%s\n", cpu_getmodel());
	format_bytes(pbuf, sizeof(pbuf), ctob(physmem));
	printf("total memory = %s\n", pbuf);

	minaddr = 0;
	/*
	 * Allocate a submap for physio
	 */
	phys_map = uvm_km_suballoc(kernel_map, &minaddr, &maxaddr,
				   VM_PHYS_SIZE, true, false, NULL);

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
}

/*
 * Look up information in bootinfo of boot loader.
 */
void *
lookup_bootinfo(int type)
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
prom_halt(int howto)
{
	if (howto & RB_HALT)
		MIPS_PROM(reinit)();
	MIPS_PROM(reboot)();
	/* NOTREACHED */
}

void
cpu_reboot(volatile int howto, char *bootstr)
{
	/* take a snap shot before clobbering any registers */
	savectx(curpcb);

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

	pmf_system_shutdown(boothowto);

	if ((howto & RB_POWERDOWN) == RB_POWERDOWN)
		prom_halt(0x80);	/* rom monitor RB_PWOFF */

	/* Finally, halt/reboot the system. */
	printf("%s\n\n", howto & RB_HALT ? "halted." : "rebooting...");
	prom_halt(howto & RB_HALT);
	/*NOTREACHED*/
}

int
initcpu(void)
{
	spl0();		/* safe to turn interrupts on now */
	return 0;
}

static void
unimpl_cons_init(void)
{

	panic("sysconf.init didn't set cons_init");
}

static void
unimpl_iointr(uint32_t status, vaddr_t pc, uint32_t ipending)
{

	panic("sysconf.init didn't set intr");
}

static int
unimpl_memsize(void *first)
{

	panic("sysconf.init didn't set memsize");
}

void
unimpl_intr_establish(int level, int (*func)(void *), void *arg)
{
	panic("sysconf.init didn't init intr_establish");
}

void
delay(int n)
{
	DELAY(n);
}

/*
 * Find out how much memory is available by testing memory.
 * Be careful to save and restore the original contents for msgbuf.
 */
int
memsize_scan(void *first)
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
		vp += PAGE_SIZE/sizeof(int);
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
null_cnprobe(struct consdev *cn)
{
}

static void
prom_cninit(struct consdev *cn)
{
	extern const struct cdevsw cons_cdevsw;

	cn->cn_dev = makedev(cdevsw_lookup_major(&cons_cdevsw), 0);
	cn->cn_pri = CN_REMOTE;
}

static int
prom_cngetc(dev_t dev)
{
	return MIPS_PROM(getchar)();
}

static void
prom_cnputc(dev_t dev, int c)
{
	MIPS_PROM(putchar)(c);
}

static void
null_cnpollc(dev_t dev, int on)
{
}

void
consinit(void)
{

	cn_tab = &consdev_zs;

	(*cn_tab->cn_init)(cn_tab);
}
