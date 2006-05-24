/*	$NetBSD: machdep.c,v 1.212.12.1 2006/05/24 15:48:15 tron Exp $	*/

/*
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
 * 	from: Utah Hdr: machdep.c 1.63 91/04/24
 */
/*
 * Copyright (c) 1988 University of Utah.
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
 * 	from: Utah Hdr: machdep.c 1.63 91/04/24
 */

#include <sys/cdefs.h>			/* RCS ID & Copyright macro defns */
__KERNEL_RCSID(0, "$NetBSD: machdep.c,v 1.212.12.1 2006/05/24 15:48:15 tron Exp $");

#include "fs_mfs.h"
#include "opt_ddb.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/buf.h>
#include <sys/reboot.h>
#include <sys/user.h>
#include <sys/mount.h>
#include <sys/kcore.h>
#include <sys/boot_flag.h>
#include <sys/ksyms.h>

#include <uvm/uvm_extern.h>

#include <dev/cons.h>

#include <ufs/mfs/mfs_extern.h>		/* mfs_initminiroot() */

#include <mips/cache.h>
#include <machine/psl.h>
#include <machine/autoconf.h>
#include <machine/dec_prom.h>
#include <machine/sysconf.h>
#include <machine/bootinfo.h>
#include <machine/locore.h>
#include <pmax/pmax/machdep.h>

#define _PMAX_BUS_DMA_PRIVATE
#include <machine/bus.h>

#if NKSYMS || defined(DDB) || defined(LKM)
#include <sys/exec_aout.h>		/* XXX backwards compatilbity for DDB */
#include <machine/db_machdep.h>
#include <ddb/db_extern.h>
#endif

#include "opt_dec_3min.h"
#include "opt_dec_maxine.h"
#include "opt_dec_3maxplus.h"
#include "ksyms.h"

unsigned ssir;				/* simulated interrupt register */

/* Our exported CPU info; we can have only one. */  
struct cpu_info cpu_info_store;

/* maps for VM objects */
struct vm_map *exec_map = NULL;
struct vm_map *mb_map = NULL;
struct vm_map *phys_map = NULL;

int		systype;		/* mother board type */
char		*bootinfo = NULL;	/* pointer to bootinfo structure */
int		cpuspeed = 30;		/* approx # instr per usec. */
int		physmem;		/* max supported memory, changes to actual */
int		physmem_boardmax;	/* {model,SIMM}-specific bound on physmem */
int		mem_cluster_cnt;
phys_ram_seg_t	mem_clusters[VM_PHYSSEG_MAX];

/*      
 * During autoconfiguration or after a panic, a sleep will simply
 * lower the priority briefly to allow interrupts, then return.
 * The priority to be used (safepri) is machine-dependent, thus this
 * value is initialized and maintained in the machine-dependent layers.
 * This priority will typically be 0, or the lowest priority
 * that is safe for use on the interrupt stack; it can be made
 * higher to block network software interrupts after panics.
 */
/*
 * safepri is a safe priority for sleep to set for a spin-wait
 * during autoconfiguration or after a panic.
 * Used as an argument to splx().
 * XXX disables interrupt 5 to disable mips3 on-chip clock, which also
 * disables mips1 FPU interrupts.
 */
int	safepri = MIPS3_PSL_LOWIPL;	/* XXX */

struct splvec	splvec;			/* XXX will go XXX */

void	mach_init __P((int, char *[], int, int, u_int, char *)); /* XXX */

/* Motherboard or system-specific initialization vector */
static void	unimpl_bus_reset __P((void));
static void	unimpl_cons_init __P((void));
static void	unimpl_iointr __P((unsigned, unsigned, unsigned, unsigned));
static void	unimpl_intr_establish __P((struct device *, void *, int,
		    int (*)(void *), void *));
static int	unimpl_memsize __P((caddr_t));
static unsigned	nullwork __P((void));

struct platform platform = {
	"iobus not set",
	unimpl_bus_reset,
	unimpl_cons_init,
	unimpl_iointr,
	unimpl_intr_establish,
	unimpl_memsize,
	(void *)nullwork,
};

extern caddr_t esym;			/* XXX */
extern struct user *proc0paddr;		/* XXX */
extern struct consdev promcd;		/* XXX */

/*
 * Do all the stuff that locore normally does before calling main().
 * The first 4 argments are passed by PROM monitor, and remaining two
 * are built on temporary stack by our boot loader.
 */
void
mach_init(argc, argv, code, cv, bim, bip)
	int argc;
	char *argv[];
	int code, cv;
	u_int bim;
	char *bip;
{
	char *cp;
	const char *bootinfo_msg;
	u_long first, last;
	int i;
	caddr_t kernend;
#if NKSYMS || defined(DDB) || defined(LKM)
	caddr_t ssym = 0;
	struct btinfo_symtab *bi_syms;
	struct exec *aout;		/* XXX backwards compatilbity for DDB */
#endif
	extern char edata[], end[];	/* XXX */

	/* Set up bootinfo structure looking at stack. */
	if (bim == BOOTINFO_MAGIC) {
		struct btinfo_magic *bi_magic;

		bootinfo = bip;
		bi_magic = lookup_bootinfo(BTINFO_MAGIC);
		if (bi_magic == NULL || bi_magic->magic != BOOTINFO_MAGIC)
			bootinfo_msg =
			    "invalid magic number in bootinfo structure.\n";
		else
			bootinfo_msg = NULL;
	}
	else
		bootinfo_msg = "invalid bootinfo pointer (old bootblocks?)\n";

	/* clear the BSS segment */
#if NKSYMS || defined(DDB) || defined(LKM)
	bi_syms = lookup_bootinfo(BTINFO_SYMTAB);
	aout = (struct exec *)edata;

	/* Was it a valid bootinfo symtab info? */
	if (bi_syms != NULL) {
		ssym = (caddr_t)bi_syms->ssym;
		esym = (caddr_t)bi_syms->esym;
		kernend = (caddr_t)mips_round_page(esym);
		memset(edata, 0, end - edata);
	}
	/* XXX: Backwards compatibility with old bootblocks - this should
	 * go soon...
	 */
#ifdef EXEC_AOUT
	/* Exec header and symbols? */
	else if (aout->a_midmag == 0x07018b00 && (i = aout->a_syms) != 0) {
		ssym = end;
		i += (*(long *)(end + i + 4) + 3) & ~3;		/* strings */
		esym = end + i + 4;
		kernend = (caddr_t)mips_round_page(esym);
		memset(edata, 0, end - edata);
	} else
#endif
#endif
	{
		kernend = (caddr_t)mips_round_page(end);
		memset(edata, 0, kernend - edata);
	}

	/* Initialize callv so we can do PROM output... */
	callv = (code == DEC_PROM_MAGIC) ? (void *)cv : &callvec;

	/* Use PROM console output until we initialize a console driver. */
	cn_tab = &promcd;

#if 0
	if (bootinfo_msg != NULL)
		printf(bootinfo_msg);
#endif
	/*
	 * Set the VM page size.
	 */
	uvm_setpagesize();

	/*
	 * Copy exception-dispatch code down to exception vector.
	 * Initialize locore-function vector.
	 * Clear out the I and D caches.
	 */
	mips_vector_init();

	/*
	 * We know the CPU type now.  Initialize our DMA tags (might
	 * need this early, for certain types of console devices!!).
	 */
	pmax_bus_dma_init();

	/* Check for direct boot from DS5000 REX monitor */
	if (argc > 0 && strcmp(argv[0], "boot") == 0) {
		argc--;
		argv++;
	}

	/* Look at argv[0] and compute bootdev */
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

			case 'n': /* ask for names */
				boothowto |= RB_ASKNAME;
				break;

			case 'N': /* don't ask for names */
				boothowto &= ~RB_ASKNAME;
				break;

			default:
				BOOT_FLAG(*cp, boothowto);
				break;
			}
		}
	}

#ifdef MFS
	/*
	 * Check to see if a mini-root was loaded into memory. It resides
	 * at the start of the next page just after the end of BSS.
	 */
	if (boothowto & RB_MINIROOT)
		kernend += round_page(mfs_initminiroot(kernend));
#endif

#if NKSYMS || defined(DDB) || defined(LKM)
	/* init symbols if present */
	if (esym)
		ksyms_init(esym - ssym, ssym, esym);
#endif
#ifdef DDB
	if (boothowto & RB_KDB)
		Debugger();
#endif

	/*
	 * Alloc u pages for proc0 stealing KSEG0 memory.
	 */
	lwp0.l_addr = proc0paddr = (struct user *)kernend;
	lwp0.l_md.md_regs = (struct frame *)(kernend + USPACE) - 1;
	memset(lwp0.l_addr, 0, USPACE);
	curpcb = &lwp0.l_addr->u_pcb;
	curpcb->pcb_context[11] = MIPS_INT_MASK | MIPS_SR_INT_IE; /* SR */

	kernend += USPACE;

	/*
	 * Initialize physmem_boardmax; assume no SIMM-bank limits.
	 * Adjust later in model-specific code if necessary.
	 */
	physmem_boardmax = MIPS_MAX_MEM_ADDR;

	/*
	 * Determine what model of computer we are running on.
	 */
	systype = ((prom_systype() >> 16) & 0xff);
	if (systype >= nsysinit) {
		platform_not_supported();
		/* NOTREACHED */
	}

	/* Machine specific initialization. */
	(*sysinit[systype].init)();

	/* Interrupt initialization. */
	intr_init();

	/* Find out how much memory is available. */
	physmem = (*platform.memsize)(kernend);

	/*
	 * Load the rest of the available pages into the VM system.
	 * Put the first 8M of RAM onto a lower-priority free list, since
	 * some TC boards (e.g. PixelStamp boards) are only able to DMA
	 * into this region, and we want them to have a fighting chance of
	 * allocating their DMA memory during autoconfiguration.
	 */
	for (i = 0, physmem = 0; i < mem_cluster_cnt; ++i) {
		first = mem_clusters[i].start;
		if (first == 0)
			first = round_page(MIPS_KSEG0_TO_PHYS(kernend));
		last = mem_clusters[i].start + mem_clusters[i].size;
		physmem += atop(mem_clusters[i].size);
		if (i != 0 || last <= (8 * 1024 * 1024)) {
			uvm_page_physload(atop(first), atop(last), atop(first),
			    atop(last), VM_FREELIST_DEFAULT);
		} else {
			uvm_page_physload(atop(first), atop(8 * 1024 * 1024),
			    atop(first), atop(8 * 1024 * 1024), VM_FREELIST_FIRST8);
			uvm_page_physload(atop(8 * 1024 * 1024), atop(last),
			    atop(8 * 1024 * 1024), atop(last), VM_FREELIST_DEFAULT);
		}
	}

	/*
	 * Initialize error message buffer (at end of core).
	 */
	mips_init_msgbuf();

	/*
	 * Initialize the virtual memory system.
	 */
	pmap_bootstrap();
}

void
mips_machdep_cache_config(void)
{
	/* All r4k pmaxen have a 1MB L2 cache. */
	if (CPUISMIPS3)
		mips_sdcache_size = 1024 * 1024;
}

void
consinit()
{

	(*platform.cons_init)();
}

/*
 * Machine-dependent startup code: allocate memory for variable-sized
 * tables.
 */
void
cpu_startup()
{
	vaddr_t minaddr, maxaddr;
	char pbuf[9];
#ifdef DEBUG
	extern int pmapdebug;		/* XXX */
	int opmapdebug = pmapdebug;

	pmapdebug = 0;
#endif

	/*
	 * Good {morning,afternoon,evening,night}.
	 */
	printf("%s%s", copyright, version);
	printf("%s\n", cpu_model);
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

void
cpu_reboot(howto, bootstr)
	volatile int howto;	/* XXX volatile to keep gcc happy */
	char *bootstr;
{

	/* take a snap shot before clobbering any registers */
	if (curlwp)
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
	if ((howto & RB_DUMP) != 0)
#endif
		dumpsys();

haltsys:

	/* run any shutdown hooks */
	doshutdownhooks();

	/* Finally, halt/reboot the system. */
	printf("%s\n\n", ((howto & RB_HALT) != 0) ? "halted." : "rebooting...");
	prom_halt(howto & RB_HALT, bootstr);
	/*NOTREACHED*/
}

/*
 * Find out how much memory is available by testing memory.
 * Be careful to save and restore the original contents for msgbuf.
 */
int
memsize_scan(first)
	caddr_t first;
{
	int i, mem;
	char *cp;

	mem = btoc((paddr_t)first - MIPS_KSEG0_START);
	cp = (char *)MIPS_PHYS_TO_KSEG1(mem << PGSHIFT);
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
		cp += PAGE_SIZE;
		mem++;
	}

	/*
	 * Now that we know how much memory we have, initialize the
	 * mem cluster array.
	 */
	mem_clusters[0].start = 0;		/* XXX is this correct? */
	mem_clusters[0].size  = ctob(mem);
	mem_cluster_cnt = 1;

	/* clear any memory error conditions possibly caused by probe */
	(*platform.bus_reset)();
	return (mem);
}

/*
 * Find out how much memory is available by using the PROM bitmap.
 */
int
memsize_bitmap(first)
	caddr_t first;
{
	memmap *prom_memmap = (memmap *)first;
	int i, mapbytes;
	int segstart, curaddr, xsize, segnum;

	mapbytes = prom_getbitmap(prom_memmap);
	if (mapbytes == 0)
		return (memsize_scan(first));

	segstart = curaddr = i = segnum = 0;
	xsize = prom_memmap->pagesize * 8;
	while (i < mapbytes) {
		while (prom_memmap->bitmap[i] == 0xff && i < mapbytes) {
			++i;
			curaddr += xsize;
		}
		if (curaddr > segstart) {
			mem_clusters[segnum].start = segstart;
			mem_clusters[segnum].size = curaddr - segstart;
			++segnum;
		}
		while (i < mapbytes && prom_memmap->bitmap[i] != 0xff) {
			++i;
			curaddr += xsize;
		}
		segstart = curaddr;
	}
	mem_cluster_cnt = segnum;
	for (i = 0; i < segnum; ++i) {
		printf("segment %2d start %08lx size %08lx\n", i,
		    (long)mem_clusters[i].start, (long)mem_clusters[i].size);
	}
	return (mapbytes * 8);
}

/*
 *  Ensure all platform vectors are always initialized.
 */
static void
unimpl_bus_reset()
{

	panic("sysconf.init didn't set bus_reset");
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

static void
unimpl_intr_establish(dev, cookie, level, handler, arg)
	struct device *dev;
	void *cookie;
	int level;
	int (*handler) __P((void *));
	void *arg;
{
	panic("sysconf.init didn't set intr_establish");
}

static int
unimpl_memsize(first)
caddr_t first;
{

	panic("sysconf.init didn't set memsize");
}

static unsigned
nullwork()
{

	return (0);
}

/*
 * Return the best possible estimate of the time in the timeval to
 * which tvp points.  We guarantee that the time will be greater than
 * the value obtained by a previous call.  Some models of DECstations
 * provide a high resolution timer circuit.
 */
void
microtime(tvp)
	struct timeval *tvp;
{
	int s = splclock();
	static struct timeval lasttime;

	*tvp = time;
#if defined(DEC_3MIN) || defined(DEC_MAXINE) || defined(DEC_3MAXPLUS)
	tvp->tv_usec += (*platform.clkread)();
#endif
	if (tvp->tv_usec >= 1000000) {
		tvp->tv_usec -= 1000000;
		tvp->tv_sec++;
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

/*
 * Wait "n" microseconds. (scsi code needs this).
 */
void
delay(n)
        int n;
{

        DELAY(n);
}
