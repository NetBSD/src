/*	$NetBSD: machdep.c,v 1.248.6.1 2015/09/22 12:05:49 skrll Exp $	*/

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
 * 	from: Utah Hdr: machdep.c 1.63 91/04/24
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: machdep.c,v 1.248.6.1 2015/09/22 12:05:49 skrll Exp $");

#include "opt_ddb.h"
#include "opt_modular.h"
#define _PMAX_BUS_DMA_PRIVATE


#include <sys/param.h>
#include <sys/boot_flag.h>
#include <sys/buf.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/kcore.h>
#include <sys/kernel.h>
#include <sys/ksyms.h>
#include <sys/mount.h>
#include <sys/proc.h>
#include <sys/reboot.h>
#include <sys/systm.h>
#include <sys/cpu.h>

#include <uvm/uvm_extern.h>

#include <dev/cons.h>

#include <ufs/mfs/mfs_extern.h>		/* mfs_initminiroot() */

#include <mips/cache.h>
#include <mips/locore.h>
#include <mips/regnum.h>
#include <mips/psl.h>

#include <pmax/autoconf.h>
#include <pmax/dec_prom.h>
#include <pmax/sysconf.h>
#include <pmax/bootinfo.h>

#include <pmax/pmax/machdep.h>

#if NKSYMS || defined(DDB) || defined(MODULAR)
#include <mips/db_machdep.h>
#include <ddb/db_extern.h>
#endif

#include "opt_dec_3min.h"
#include "opt_dec_maxine.h"
#include "opt_dec_3maxplus.h"
#include "ksyms.h"

unsigned int ssir;			/* simulated interrupt register */

/* maps for VM objects */
struct vm_map *phys_map = NULL;

int		systype;		/* mother board type */
char		*bootinfo = NULL;	/* pointer to bootinfo structure */
int		cpuspeed = 30;		/* approx # instr per usec. */
intptr_t	physmem_boardmax;	/* {model,SIMM}-specific bound on physmem */
int		mem_cluster_cnt;
phys_ram_seg_t	mem_clusters[VM_PHYSSEG_MAX];

void	mach_init(int, int32_t *, int, intptr_t, u_int, char *); /* XXX */

/* Motherboard or system-specific initialization vector */
static void	unimpl_bus_reset(void);
static void	unimpl_cons_init(void);
static void	unimpl_iointr(uint32_t, vaddr_t, uint32_t);
static void	unimpl_intr_establish(device_t, void *, int,
		    int (*)(void *), void *);
static int	unimpl_memsize(void *);
static unsigned	nullwork(void);

struct platform platform = {
	"iobus not set",
	unimpl_bus_reset,
	unimpl_cons_init,
	unimpl_iointr,
	unimpl_intr_establish,
	unimpl_memsize,
	(void *)nullwork,
};

extern void *esym;			/* XXX */
extern struct consdev promcd;		/* XXX */

#define	ARGV(i)	((char *)(intptr_t)(argv32[i]))

/*
 * Do all the stuff that locore normally does before calling main().
 * The first 4 argments are passed by PROM monitor, and remaining two
 * are built on temporary stack by our boot loader (or in reg if N32/N64).
 */
void
mach_init(int argc, int32_t *argv32, int code, intptr_t cv, u_int bim, char *bip)
{
	char *cp;
	const char *bootinfo_msg;
	int i;
	char *kernend;
#if NKSYMS || defined(DDB) || defined(MODULAR)
	void *ssym = 0;
	struct btinfo_symtab *bi_syms;
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
#if NKSYMS || defined(DDB) || defined(MODULAR)
	bi_syms = lookup_bootinfo(BTINFO_SYMTAB);
#ifdef EXEC_AOUT
	struct exec *aout = (struct exec *)edata;
#endif

	/* Was it a valid bootinfo symtab info? */
	if (bi_syms != NULL) {
		ssym = (void *)(intptr_t)bi_syms->ssym;
		esym = (void *)(intptr_t)bi_syms->esym;
		kernend = (void *)mips_round_page(esym);
#if 0	/* our bootloader clears BSS properly */
		memset(edata, 0, end - edata);
#endif
	} else
#ifdef EXEC_AOUT
	/* XXX: Backwards compatibility with old bootblocks - this should
	 * go soon...
	 */
	/* Exec header and symbols? */
	if (aout->a_midmag == 0x07018b00 && (i = aout->a_syms) != 0) {
		ssym = end;
		i += (*(long *)(end + i + 4) + 3) & ~3;		/* strings */
		esym = end + i + 4;
		kernend = (void *)mips_round_page(esym);
		memset(edata, 0, end - edata);
	} else
#endif
#endif
	{
		kernend = (void *)mips_round_page(end);
		memset(edata, 0, kernend - edata);
	}

	/* Initialize callv so we can do PROM output... */
	if (code == DEC_PROM_MAGIC) {
#ifdef _LP64
		/*
		 * Convert the call vector from using 32bit function pointers
		 * to using 64bit function pointers.
		 */
		for (i = 0; i < sizeof(callvec) / sizeof(void *); i++)
			((intptr_t *)&callvec)[i] = ((int32_t *)cv)[i];
		callv = &callvec;
#else
		callv = (void *)cv;
#endif
	} else {
		callv = &callvec;
	}

	/* Use PROM console output until we initialize a console driver. */
	cn_tab = &promcd;

#if 0
	if (bootinfo_msg != NULL)
		printf(bootinfo_msg);
#else
	__USE(bootinfo_msg);
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
	mips_vector_init(NULL, false);

	/*
	 * We know the CPU type now.  Initialize our DMA tags (might
	 * need this early, for certain types of console devices!!).
	 */
	pmax_bus_dma_init();

	/* Check for direct boot from DS5000 REX monitor */
	if (argc > 0 && strcmp(ARGV(0), "boot") == 0) {
		argc--;
		argv32++;
	}

	/* Look at argv[0] and compute bootdev */
	makebootdev(ARGV(0));

	/*
	 * Look at arguments passed to us and compute boothowto.
	 */
	boothowto = RB_SINGLE;
#ifdef KADB
	boothowto |= RB_KDB;
#endif
	for (i = 1; i < argc; i++) {
		for (cp = ARGV(i); *cp; cp++) {
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

	/*
	 * Check to see if a mini-root was loaded into memory. It resides
	 * at the start of the next page just after the end of BSS.
	 */
	if (boothowto & RB_MINIROOT)
		kernend += round_page(mfs_initminiroot(kernend));

#if NKSYMS || defined(DDB) || defined(MODULAR)
	/* init symbols if present */
	if (esym)
		ksyms_addsyms_elf((char *)esym - (char *)ssym, ssym, esym);
#endif
#ifdef DDB
	if (boothowto & RB_KDB)
		Debugger();
#endif

	/*
	 * We need to do this early for badaddr().
	 */
	lwp0.l_addr = (struct user *)kernend;
	kernend += USPACE;
	mips_init_lwp0_uarea();

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
		physmem += atop(mem_clusters[i].size);
	}

	static const struct mips_vmfreelist first8 = {
		.fl_start = 0,
		.fl_end = 8 * 1024 * 1024,
		.fl_freelist = VM_FREELIST_FIRST8
	};
	mips_page_physload(MIPS_KSEG0_START, (vaddr_t)kernend,
	    mem_clusters, mem_cluster_cnt, &first8, 1);
		

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
		mips_cache_info.mci_sdcache_size = 1024 * 1024;
}

void
consinit(void)
{

	(*platform.cons_init)();
}

/*
 * Machine-dependent startup code: allocate memory for variable-sized
 * tables.
 */
void
cpu_startup(void)
{
	cpu_startup_common();
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

void
cpu_reboot(int howto, char *bootstr)
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

	pmf_system_shutdown(boothowto);

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
memsize_scan(void *first)
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
memsize_bitmap(void *first)
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
unimpl_bus_reset(void)
{

	panic("sysconf.init didn't set bus_reset");
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

static void
unimpl_intr_establish(device_t dev, void *cookie, int level,
    int (*handler)(void *), void *arg)
{
	panic("sysconf.init didn't set intr_establish");
}

static int
unimpl_memsize(void *first)
{

	panic("sysconf.init didn't set memsize");
}

static unsigned
nullwork(void)
{

	return (0);
}

/*
 * Wait "n" microseconds. (scsi code needs this).
 */
void
delay(int n)
{

        DELAY(n);
}
