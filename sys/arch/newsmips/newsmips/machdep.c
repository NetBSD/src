/*	$NetBSD: machdep.c,v 1.116.2.3 2017/02/05 13:40:16 skrll Exp $	*/

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

__KERNEL_RCSID(0, "$NetBSD: machdep.c,v 1.116.2.3 2017/02/05 13:40:16 skrll Exp $");

/* from: Utah Hdr: machdep.c 1.63 91/04/24 */

#include "opt_ddb.h"
#include "opt_execfmt.h"
#include "opt_modular.h"

#define __INTR_PRIVATE

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/signalvar.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/buf.h>
#include <sys/reboot.h>
#include <sys/conf.h>
#include <sys/file.h>
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
#include <sys/intr.h>

#include <uvm/uvm_extern.h>

#include <ufs/mfs/mfs_extern.h>		/* mfs_initminiroot() */

#include <mips/cache.h>
#include <mips/locore.h>

#include <machine/reg.h>
#include <machine/psl.h>
#include <machine/pte.h>
#include <machine/autoconf.h>
#include <machine/bootinfo.h>
#include <machine/apbus.h>
#include <machine/apcall.h>


#define	_NEWSMIPS_BUS_DMA_PRIVATE
#include <machine/bus.h>

#ifdef DDB
#include <machine/db_machdep.h>
#include <ddb/db_access.h>
#include <ddb/db_extern.h>
#include <ddb/db_sym.h>
#endif

#include <machine/adrsmap.h>
#include <machine/machConst.h>
#include <newsmips/newsmips/machid.h>
#include <dev/cons.h>

#include "ksyms.h"

/* maps for VM objects */

struct vm_map *phys_map = NULL;

char *bootinfo = NULL;		/* pointer to bootinfo structure */
int systype;			/* what type of NEWS we are */
struct apbus_sysinfo *_sip = NULL;

phys_ram_seg_t mem_clusters[VM_PHYSSEG_MAX];
int mem_cluster_cnt;

struct idrom idrom;
void (*hardware_intr)(int, vaddr_t, uint32_t);
void (*enable_intr)(void);
void (*disable_intr)(void);
void (*enable_timer)(void);

/*
 *  Local functions.
 */

/* initialize bss, etc. from kernel start, before main() is called. */
void mach_init(int, int, int, int);

void prom_halt(int) __attribute__((__noreturn__));
void to_monitor(int) __attribute__((__noreturn__));

#ifdef DEBUG
/* stacktrace code violates prototypes to get callee's registers */
extern void stacktrace(void); /*XXX*/
#endif

extern u_long bootdev;
extern char edata[], end[];

/*
 * Do all the stuff that locore normally does before calling main().
 * Process arguments passed to us by the prom monitor.
 * Return the first page address following the system.
 */
void
mach_init(int x_boothowto, int x_bootdev, int x_bootname, int x_maxmem)
{
	u_long first, last;
	char *kernend;
	const char *model;
	struct btinfo_magic *bi_magic;
	struct btinfo_bootarg *bi_arg;
	struct btinfo_systype *bi_systype;
#if NKSYMS || defined(DDB) || defined(MODULAR)
	struct btinfo_symtab *bi_sym;
	int nsym = 0;
	char *ssym, *esym;

	ssym = esym = NULL;	/* XXX: gcc */
#endif
	bi_arg = NULL;

	bootinfo = (void *)BOOTINFO_ADDR;	/* XXX */
	bi_magic = lookup_bootinfo(BTINFO_MAGIC);
	if (bi_magic && bi_magic->magic == BOOTINFO_MAGIC) {
		bi_arg = lookup_bootinfo(BTINFO_BOOTARG);
		if (bi_arg) {
			x_boothowto = bi_arg->howto;
			x_bootdev = bi_arg->bootdev;
			x_maxmem = bi_arg->maxmem;
		}
#if NKSYMS || defined(DDB) || defined(MODULAR)
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
	} else {
		/*
		 * Running kernel is loaded by non-native loader;
		 * clear the BSS segment here.
		 */
		memset(edata, 0, end - edata);
	}

	if (systype == 0)
		systype = NEWS3400;	/* XXX compatibility for old boot */

#ifdef news5000
	if (systype == NEWS5000) {
		int i;
		char *bspec = (char *)x_bootdev;

		if (bi_arg == NULL)
			panic("news5000 requires BTINFO_BOOTARG to boot");

		_sip = (void *)bi_arg->sip;
		x_maxmem = _sip->apbsi_memsize;
		x_maxmem -= 0x00100000;	/* reserve 1MB for ROM monitor */
		if (strncmp(bspec, "scsi", 4) == 0) {
			x_bootdev = (5 << 28) | 0;	 /* magic, sd */
			bspec += 4;
			if (*bspec != '(' /*)*/)
				goto bootspec_end;
			i = strtoul(bspec + 1, &bspec, 10);
			x_bootdev |= (i << 24);		/* bus */
			if (*bspec != ',')
				goto bootspec_end;
			i = strtoul(bspec + 1, &bspec, 10);
			x_bootdev |= (i / 10) << 20;	/* controller */
			x_bootdev |= (i % 10) << 16;	/* unit */
			if (*bspec != ',')
				goto bootspec_end;
			i = strtoul(bspec + 1, &bspec, 10);
			x_bootdev |= (i << 8);		/* partition */
		}
 bootspec_end:
		consinit();
	}
#endif

	/*
	 * Save parameters into kernel work area.
	 */
	*(int *)(MIPS_PHYS_TO_KSEG1(MACH_MAXMEMSIZE_ADDR)) = x_maxmem;
	*(int *)(MIPS_PHYS_TO_KSEG1(MACH_BOOTDEV_ADDR)) = x_bootdev;
	*(int *)(MIPS_PHYS_TO_KSEG1(MACH_BOOTSW_ADDR)) = x_boothowto;

	kernend = (char *)mips_round_page(end);
#if NKSYMS || defined(DDB) || defined(MODULAR)
	if (nsym)
		kernend = (char *)mips_round_page(esym);
#endif

	uvm_md_init();

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
	mips_vector_init(NULL, false);

	/*
	 * We know the CPU type now.  Initialize our DMA tags (might
	 * need this early).
	 */
	newsmips_bus_dma_init();

#if NKSYMS || defined(DDB) || defined(MODULAR)
	if (nsym)
		ksyms_addsyms_elf(esym - ssym, ssym, esym);
#endif

#ifdef KADB
	boothowto |= RB_KDB;
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
	 * Determine what model of computer we are running on.
	 */
	switch (systype) {
#ifdef news3400
	case NEWS3400:
		news3400_init();
		cpu_setmodel("%s", idrom.id_machine);
		model = cpu_getmodel();
		if (strcmp(model, "news3400") == 0 ||
		    strcmp(model, "news3200") == 0 ||
		    strcmp(model, "news3700") == 0) {
			/*
			 * Set up interrupt handling and I/O addresses.
			 */
			hardware_intr = news3400_intr;
			cpuspeed = 10;
		} else {
			printf("kernel not configured for machine %s\n", model);
		}
		break;
#endif

#ifdef news5000
	case NEWS5000:
		news5000_init();
		cpu_setmodel("%s", idrom.id_machine);
		model = cpu_getmodel();
		if (strcmp(model, "news5000") == 0 ||
		    strcmp(model, "news5900") == 0) {
			/*
			 * Set up interrupt handling and I/O addresses.
			 */
			hardware_intr = news5000_intr;
			cpuspeed = 50;	/* ??? XXX */
		} else {
			printf("kernel not configured for machine %s\n", model);
		}
		break;
#endif

	default:
		printf("kernel not configured for systype %d\n", systype);
		break;
	}
}

void
mips_machdep_cache_config(void)
{
	/* All r4k news boxen have a 1MB L2 cache. */
	if (CPUISMIPS3)
		mips_cache_info.mci_sdcache_size = 1024 * 1024;
}

/*
 * cpu_startup: allocate memory for variable-sized tables,
 * initialize CPU, and do autoconfiguration.
 */
void
cpu_startup(void)
{
	printf("SONY NET WORK STATION, Model %s, ", idrom.id_model);
	printf("Machine ID #%d\n", idrom.id_serial);

	cpu_startup_common();
}

/*
 * lookup_bootinfo:
 * Look up information in bootinfo of boot loader.
 */
void *
lookup_bootinfo(int type)
{
	struct btinfo_common *bt;
	char *help = bootinfo;

	/* Check for a bootinfo record first. */
	if (help == NULL)
		return NULL;

	do {
		bt = (struct btinfo_common *)help;
		if (bt->type == type)
			return (void *)help;
		help += bt->next;
	} while (bt->next != 0 &&
		(size_t)help < (size_t)bootinfo + BOOTINFO_SIZE);

	return NULL;
}

/*
 * call PROM to halt or reboot.
 */
void
prom_halt(int howto)

{
#ifdef news5000
	if (systype == NEWS5000)
		apcall_exit(howto);
#endif
#ifdef news3400
	if (systype == NEWS3400)
		to_monitor(howto);
#endif
	for (;;);
}

int	waittime = -1;

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

	pmf_system_shutdown(boothowto);

	if ((howto & RB_POWERDOWN) == RB_POWERDOWN)
		prom_halt(0x80);	/* rom monitor RB_PWOFF */

	/* Finally, halt/reboot the system. */
	printf("%s\n\n", howto & RB_HALT ? "halted." : "rebooting...");
	prom_halt(howto & RB_HALT);
	/*NOTREACHED*/
}

void
delay(int n)
{

	DELAY(n);
}

void
cpu_intr(int ppl, vaddr_t pc, uint32_t status)
{
	curcpu()->ci_data.cpu_nintr++;

	/* device interrupts */
	(*hardware_intr)(ppl, pc, status);
}
