/*	$NetBSD: cpu.c,v 1.28 2003/02/05 12:06:52 nakayama Exp $ */

/*
 * Copyright (c) 1996
 *	The President and Fellows of Harvard College. All rights reserved.
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This software was developed by the Computer Systems Engineering group
 * at Lawrence Berkeley Laboratory under DARPA contract BG 91-66 and
 * contributed to Berkeley.
 *
 * All advertising materials mentioning features or use of this software
 * must display the following acknowledgement:
 *	This product includes software developed by Harvard University.
 *	This product includes software developed by the University of
 *	California, Lawrence Berkeley Laboratory.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by Aaron Brown and
 *	Harvard University.
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
 *	@(#)cpu.c	8.5 (Berkeley) 11/23/93
 *
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <uvm/uvm_extern.h>

#include <machine/autoconf.h>
#include <machine/cpu.h>
#include <machine/reg.h>
#include <machine/trap.h>
#include <machine/pmap.h>

#include <sparc64/sparc64/cache.h>

/* This is declared here so that you must include a CPU for the cache code. */
struct cacheinfo cacheinfo;

/* Our exported CPU info; we have only one for now. */  
struct cpu_info cpu_info_store;

/* Linked list of all CPUs in system. */
struct cpu_info *cpus = NULL;

/* The following are used externally (sysctl_hw). */
char	machine[] = MACHINE;		/* from <machine/param.h> */
char	machine_arch[] = MACHINE_ARCH;	/* from <machine/param.h> */
char	cpu_model[100];			/* machine model (primary cpu) */
extern char machine_model[];

/* The CPU configuration driver. */
static void cpu_attach __P((struct device *, struct device *, void *));
int  cpu_match __P((struct device *, struct cfdata *, void *));

CFATTACH_DECL(cpu, sizeof(struct device),
    cpu_match, cpu_attach, NULL, NULL);

extern struct cfdriver cpu_cd;

#define	IU_IMPL(v)	((((uint64_t)(v)) & VER_IMPL) >> VER_IMPL_SHIFT)
#define	IU_VERS(v)	((((uint64_t)(v)) & VER_MASK) >> VER_MASK_SHIFT)

#ifdef notdef
/*
 * IU implementations are parceled out to vendors (with some slight
 * glitches).  Printing these is cute but takes too much space.
 */
static char *iu_vendor[16] = {
	"Fujitsu",	/* and also LSI Logic */
	"ROSS",		/* ROSS (ex-Cypress) */
	"BIT",
	"LSIL",		/* LSI Logic finally got their own */
	"TI",		/* Texas Instruments */
	"Matsushita",
	"Philips",
	"Harvest",	/* Harvest VLSI Design Center */
	"SPEC",		/* Systems and Processes Engineering Corporation */
	"Weitek",
	"vendor#10",
	"vendor#11",
	"vendor#12",
	"vendor#13",
	"vendor#14",
	"vendor#15"
};
#endif

/*
 * Overhead involved in firing up a new CPU:
 * 
 *	Allocate a cpuinfo/interrupt stack
 *	Map that into the kernel
 *	Initialize the cpuinfo
 *	Return the TLB entry for the cpuinfo.
 */
uint64_t
cpu_init(pa, cpu_num)
	paddr_t pa;
	int cpu_num;
{
	struct cpu_info *ci;
	uint64_t pagesize;
	uint64_t pte;
	struct vm_page *pg;
	psize_t size;
	vaddr_t va;
	struct pglist pglist;
	int error;

	size = NBPG; /* XXXX 8K, 64K, 512K, or 4MB */
	if ((error = uvm_pglistalloc(size, (paddr_t)0, (paddr_t)-1,
		(paddr_t)size, (paddr_t)0, &pglist, 1, 0)) != 0)
		panic("cpu_start: no memory, error %d", error);

	va = uvm_km_valloc(kernel_map, size);
	if (va == 0)
		panic("cpu_start: no memory");

	pg = TAILQ_FIRST(&pglist);
	pa = VM_PAGE_TO_PHYS(pg);
	pte = TSB_DATA(0 /* global */,
		pagesize,
		pa,
		1 /* priv */,
		1 /* Write */,
		1 /* Cacheable */,
		1 /* ALIAS -- Disable D$ */,
		1 /* valid */,
		0 /* IE */);

	/* Map the pages */
	for (; pg != NULL; pg = TAILQ_NEXT(pg, pageq)) {
		pa = VM_PAGE_TO_PHYS(pg);
		pmap_zero_page(pa);
		pmap_kenter_pa(va, pa | PMAP_NVC, VM_PROT_READ | VM_PROT_WRITE);
		va += NBPG;
	}
	pmap_update(pmap_kernel());

	if (!cpus)
		cpus = (struct cpu_info *)va;
	else {
		for (ci = cpus; ci->ci_next; ci = ci->ci_next)
			;
		ci->ci_next = (struct cpu_info *)va;
	}

	switch (size) {
#define K	*1024
	case 8 K:
		pagesize = TLB_8K;
		break;
	case 64 K:
		pagesize = TLB_64K;
		break;
	case 512 K:
		pagesize = TLB_512K;
		break;
	case 4 K K:
		pagesize = TLB_4M;
		break;
	default:
		panic("cpu_start: stack size %x not a machine page size",
			(unsigned)size);
	}
	return (pte | TLB_L);
}

int
cpu_match(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct mainbus_attach_args *ma = aux;

	return (strcmp(cf->cf_name, ma->ma_name) == 0);
}

/*
 * Attach the CPU.
 * Discover interesting goop about the virtual address cache
 * (slightly funny place to do it, but this is where it is to be found).
 */
static void
cpu_attach(parent, dev, aux)
	struct device *parent;
	struct device *dev;
	void *aux;
{
	int node;
	long clk;
	int impl, vers, fver;
	struct mainbus_attach_args *ma = aux;
	struct fpstate64 *fpstate;
	struct fpstate64 fps[2];
	char *sep;
	register int i, l;
	uint64_t ver;
	int bigcache, cachesize;
	char buf[100];

	/* This needs to be 64-bit aligned */
	fpstate = ALIGNFPSTATE(&fps[1]);

	/*
	 * Get the FSR and clear any exceptions.  If we do not unload
	 * the queue here and it is left over from a previous crash, we
	 * will panic in the first loadfpstate(), due to a sequence error,
	 * so we need to dump the whole state anyway.
	 */

	fpstate->fs_fsr = 7 << FSR_VER_SHIFT;	/* 7 is reserved for "none" */
	savefpstate(fpstate);
	fver = (fpstate->fs_fsr >> FSR_VER_SHIFT) & (FSR_VER >> FSR_VER_SHIFT);
	ver = getver();
	impl = IU_IMPL(ver);
	vers = IU_VERS(ver);

	/* tell them what we have */
	node = ma->ma_node;

	clk = PROM_getpropint(node, "clock-frequency", 0);
	if (clk == 0) {

		/*
		 * Try to find it in the OpenPROM root...
		 */
		clk = PROM_getpropint(findroot(), "clock-frequency", 0);
	}
	if (clk) {
		cpu_clockrate[0] = clk; /* Tell OS what frequency we run on */
		cpu_clockrate[1] = clk / 1000000;
	}
	snprintf(buf, sizeof buf, "%s @ %s MHz, version %d FPU",
		PROM_getpropstring(node, "name"),
		clockfreq(clk), fver);
	printf(": %s\n", buf);
	snprintf(cpu_model, sizeof cpu_model, "%s (%s)", machine_model, buf);

	bigcache = 0;

	cacheinfo.ic_linesize = l =
		PROM_getpropint(node, "icache-line-size", 0);
	for (i = 0; (1 << i) < l && l; i++)
		/* void */;
	if ((1 << i) != l && l)
		panic("bad icache line size %d", l);
	cacheinfo.ic_l2linesize = i;
	cacheinfo.ic_totalsize =
		PROM_getpropint(node, "icache-size", 0) *
		PROM_getpropint(node, "icache-associativity", 1);
	if (cacheinfo.ic_totalsize == 0)
		cacheinfo.ic_totalsize = l *
			PROM_getpropint(node, "icache-nlines", 64) *
			PROM_getpropint(node, "icache-associativity", 1);

	cachesize = cacheinfo.ic_totalsize /
	    PROM_getpropint(node, "icache-associativity", 1);
	bigcache = cachesize;

	cacheinfo.dc_linesize = l =
		PROM_getpropint(node, "dcache-line-size",0);
	for (i = 0; (1 << i) < l && l; i++)
		/* void */;
	if ((1 << i) != l && l)
		panic("bad dcache line size %d", l);
	cacheinfo.dc_l2linesize = i;
	cacheinfo.dc_totalsize =
		PROM_getpropint(node, "dcache-size", 0) *
		PROM_getpropint(node, "dcache-associativity", 1);
	if (cacheinfo.dc_totalsize == 0)
		cacheinfo.dc_totalsize = l *
			PROM_getpropint(node, "dcache-nlines", 128) *
			PROM_getpropint(node, "dcache-associativity", 1);

	cachesize = cacheinfo.dc_totalsize /
	    PROM_getpropint(node, "dcache-associativity", 1);
	if (cachesize > bigcache)
		bigcache = cachesize;

	cacheinfo.ec_linesize = l =
		PROM_getpropint(node, "ecache-line-size", 0);
	for (i = 0; (1 << i) < l && l; i++)
		/* void */;
	if ((1 << i) != l && l)
		panic("bad ecache line size %d", l);
	cacheinfo.ec_l2linesize = i;
	cacheinfo.ec_totalsize = 
		PROM_getpropint(node, "ecache-size", 0) *
		PROM_getpropint(node, "ecache-associativity", 1);
	if (cacheinfo.ec_totalsize == 0)
		cacheinfo.ec_totalsize = l *
			PROM_getpropint(node, "ecache-nlines", 32768) *
			PROM_getpropint(node, "ecache-associativity", 1);

	cachesize = cacheinfo.ec_totalsize /
	     PROM_getpropint(node, "ecache-associativity", 1);
	if (cachesize > bigcache)
		bigcache = cachesize;

	sep = " ";
	printf("%s:", dev->dv_xname);
	if (cacheinfo.ic_totalsize > 0) {
		printf("%s%ldK instruction (%ld b/l)", sep,
		       (long)cacheinfo.ic_totalsize/1024,
		       (long)cacheinfo.ic_linesize);
		sep = ", ";
	}
	if (cacheinfo.dc_totalsize > 0) {
		printf("%s%ldK data (%ld b/l)", sep,
		       (long)cacheinfo.dc_totalsize/1024,
		       (long)cacheinfo.dc_linesize);
		sep = ", ";
	}
	if (cacheinfo.ec_totalsize > 0) {
		printf("%s%ldK external (%ld b/l)", sep,
		       (long)cacheinfo.ec_totalsize/1024,
		       (long)cacheinfo.ec_linesize);
	}
	printf("\n");

	/*
	 * Now that we know the size of the largest cache on this CPU,
	 * re-color our pages.
	 */
	uvm_page_recolor(atop(bigcache));
}
