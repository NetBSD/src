/*	$NetBSD: cpu.c,v 1.54 2006/10/03 21:05:22 mrg Exp $ */

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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: cpu.c,v 1.54 2006/10/03 21:05:22 mrg Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/kernel.h>

#include <uvm/uvm_extern.h>

#include <machine/autoconf.h>
#include <machine/cpu.h>
#include <machine/reg.h>
#include <machine/trap.h>
#include <machine/pmap.h>
#include <machine/sparc64.h>
#include <machine/openfirm.h>

#include <sparc64/sparc64/cache.h>

/* This is declared here so that you must include a CPU for the cache code. */
struct cacheinfo cacheinfo;

/* Linked list of all CPUs in system. */
int sparc_ncpus = 0;
struct cpu_info *cpus = NULL;
static int cpu_instance;

volatile cpuset_t cpus_active;/* set of active cpus */
struct cpu_bootargs *cpu_args;	/* allocated very early in pmap_bootstrap. */

static struct cpu_info *alloc_cpuinfo(u_int);

/* The following are used externally (sysctl_hw). */
char	machine[] = MACHINE;		/* from <machine/param.h> */
char	machine_arch[] = MACHINE_ARCH;	/* from <machine/param.h> */
char	cpu_model[100];			/* machine model (primary CPU) */
extern char machine_model[];

/* The CPU configuration driver. */
void cpu_attach(struct device *, struct device *, void *);
int cpu_match(struct device *, struct cfdata *, void *);

CFATTACH_DECL(cpu, sizeof(struct device),
    cpu_match, cpu_attach, NULL, NULL);

extern struct cfdriver cpu_cd;

#define	IU_IMPL(v)	((((uint64_t)(v)) & VER_IMPL) >> VER_IMPL_SHIFT)
#define	IU_VERS(v)	((((uint64_t)(v)) & VER_MASK) >> VER_MASK_SHIFT)

struct cpu_info *
alloc_cpuinfo(u_int cpu_node)
{
	paddr_t pa0, pa;
	vaddr_t va, va0;
	vsize_t sz = 8 * PAGE_SIZE;
	int portid;
	struct cpu_info *cpi, *ci;
	extern paddr_t cpu0paddr;

	/*
	 * Check for UPAID in the cpus list.
	 */
	if (OF_getprop(cpu_node, "upa-portid", &portid, sizeof(portid)) <= 0)
		panic("alloc_cpuinfo: upa-portid");

	for (cpi = cpus; cpi != NULL; cpi = cpi->ci_next)
		if (cpi->ci_upaid == portid)
			return cpi;

	/* Allocate the aligned VA and determine the size. */
	va = uvm_km_alloc(kernel_map, sz, sz, UVM_KMF_VAONLY);
	if (!va)
		panic("alloc_cpuinfo: no virtual space");
	va0 = va;

	pa0 = cpu0paddr;
	cpu0paddr += sz;

	for (pa = pa0; pa < cpu0paddr; pa += PAGE_SIZE, va += PAGE_SIZE)
		pmap_kenter_pa(va, pa, VM_PROT_READ | VM_PROT_WRITE);
	pmap_update(pmap_kernel());

	cpi = (struct cpu_info *)(va0 + CPUINFO_VA - INTSTACK);

	memset((void *)va0, 0, sz);

	/*
	 * Initialize cpuinfo structure.
	 *
	 * Arrange pcb, idle stack and interrupt stack in the same
	 * way as is done for the boot CPU in locore.
	 */
	cpi->ci_next = NULL;
	cpi->ci_curlwp = NULL;
	cpi->ci_number = ++cpu_instance;
	cpi->ci_cpuid = portid;
	cpi->ci_upaid = portid;
	cpi->ci_fplwp = NULL;
	cpi->ci_spinup = NULL;						/* XXX */
	cpi->ci_eintstack = (void *)EINTSTACK; 				/* XXX */
	cpi->ci_idle_u = (struct pcb *)(CPUINFO_VA + 2 * PAGE_SIZE); 	/* XXX */
	cpi->ci_cpcb = cpi->ci_idle_u;					/* XXX */
	cpi->ci_initstack = (void *)((vaddr_t)cpi->ci_idle_u + 2 * PAGE_SIZE); /* XXX */
	cpi->ci_paddr = pa0;
	cpi->ci_self = cpi;
	cpi->ci_node = cpu_node;

	/*
	 * Finally, add itself to the list of active cpus.
	 */
	for (ci = cpus; ci->ci_next != NULL; ci = ci->ci_next)
		;
	ci->ci_next = cpi;
	return (cpi);
}

int
cpu_match(struct device *parent, struct cfdata *cf, void *aux)
{
	struct mainbus_attach_args *ma = aux;

	return (strcmp(cf->cf_name, ma->ma_name) == 0);
}

/*
 * Attach the CPU.
 * Discover interesting goop about the virtual address cache
 * (slightly funny place to do it, but this is where it is to be found).
 */
void
cpu_attach(struct device *parent, struct device *dev, void *aux)
{
	int node;
	long clk;
	int impl, vers, fver;
	struct mainbus_attach_args *ma = aux;
	struct cpu_info *ci;
	struct fpstate64 *fpstate;
	struct fpstate64 fps[2];
	const char *sep;
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

	clk = prom_getpropint(node, "clock-frequency", 0);
	if (clk == 0) {

		/*
		 * Try to find it in the OpenPROM root...
		 */
		clk = prom_getpropint(findroot(), "clock-frequency", 0);
	}
	if (clk) {
		cpu_clockrate[0] = clk; /* Tell OS what frequency we run on */
		cpu_clockrate[1] = clk / 1000000;
	}
	snprintf(buf, sizeof buf, "%s @ %s MHz, version %d FPU",
		prom_getpropstring(node, "name"),
		clockfreq(clk), fver);
	printf(": %s\n", buf);
	snprintf(cpu_model, sizeof cpu_model, "%s (%s)", machine_model, buf);

	bigcache = 0;

	cacheinfo.ic_linesize = l =
		prom_getpropint(node, "icache-line-size", 0);
	for (i = 0; (1 << i) < l && l; i++)
		/* void */;
	if ((1 << i) != l && l)
		panic("bad icache line size %d", l);
	cacheinfo.ic_l2linesize = i;
	cacheinfo.ic_totalsize =
		prom_getpropint(node, "icache-size", 0) *
		prom_getpropint(node, "icache-associativity", 1);
	if (cacheinfo.ic_totalsize == 0)
		cacheinfo.ic_totalsize = l *
			prom_getpropint(node, "icache-nlines", 64) *
			prom_getpropint(node, "icache-associativity", 1);

	cachesize = cacheinfo.ic_totalsize /
	    prom_getpropint(node, "icache-associativity", 1);
	bigcache = cachesize;

	cacheinfo.dc_linesize = l =
		prom_getpropint(node, "dcache-line-size",0);
	for (i = 0; (1 << i) < l && l; i++)
		/* void */;
	if ((1 << i) != l && l)
		panic("bad dcache line size %d", l);
	cacheinfo.dc_l2linesize = i;
	cacheinfo.dc_totalsize =
		prom_getpropint(node, "dcache-size", 0) *
		prom_getpropint(node, "dcache-associativity", 1);
	if (cacheinfo.dc_totalsize == 0)
		cacheinfo.dc_totalsize = l *
			prom_getpropint(node, "dcache-nlines", 128) *
			prom_getpropint(node, "dcache-associativity", 1);

	cachesize = cacheinfo.dc_totalsize /
	    prom_getpropint(node, "dcache-associativity", 1);
	if (cachesize > bigcache)
		bigcache = cachesize;

	cacheinfo.ec_linesize = l =
		prom_getpropint(node, "ecache-line-size", 0);
	for (i = 0; (1 << i) < l && l; i++)
		/* void */;
	if ((1 << i) != l && l)
		panic("bad ecache line size %d", l);
	cacheinfo.ec_l2linesize = i;
	cacheinfo.ec_totalsize = 
		prom_getpropint(node, "ecache-size", 0) *
		prom_getpropint(node, "ecache-associativity", 1);
	if (cacheinfo.ec_totalsize == 0)
		cacheinfo.ec_totalsize = l *
			prom_getpropint(node, "ecache-nlines", 32768) *
			prom_getpropint(node, "ecache-associativity", 1);

	cachesize = cacheinfo.ec_totalsize /
	     prom_getpropint(node, "ecache-associativity", 1);
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
	uvm_page_recolor(atop(bigcache)); /* XXX */

	/*
	 * Allocate cpu_info structure if needed and save cache information
	 * in there.
	 */
	ci = alloc_cpuinfo((u_int)node);
	printf("%s: upa id %d\n", dev->dv_xname, ci->ci_upaid);
}

#if defined(MULTIPROCESSOR)
vaddr_t cpu_spinup_trampoline;

/*
 * Start secondary processors in motion.
 */
void
cpu_boot_secondary_processors()
{
	int i, pstate;
	struct cpu_info *ci;

	sparc64_ipi_init();

	printf("cpu0: booting secondary processors:\n");

	for (ci = cpus; ci != NULL; ci = ci->ci_next) {
		if (ci->ci_upaid == CPU_UPAID)
			continue;

		cpu_args->cb_node = ci->ci_node;
		cpu_args->cb_cpuinfo =  ci->ci_paddr;
		cpu_args->cb_initstack = ci->ci_initstack;
		membar_sync();

#ifdef DEBUG
		printf("node %x, cpuinfo %llx, initstack %p\n",
		       cpu_args->cb_node,
		       (unsigned long long)cpu_args->cb_cpuinfo,
		       cpu_args->cb_initstack);
#endif

		/* Disable interrupts and start another CPU. */
		pstate = getpstate();
		setpstate(PSTATE_KERN);

		printf("mp_tramp: %p\n", (void*)cpu_spinup_trampoline);
		prom_startcpu(ci->ci_node, (void *)cpu_spinup_trampoline, 0);

		for (i = 0; i < 2000; i++) {
			membar_sync();
			if (CPUSET_HAS(cpus_active, ci->ci_number))
				break;
			delay(10000);
		}
		setpstate(pstate);

		if (!CPUSET_HAS(cpus_active, ci->ci_number))
			printf("cpu%d: startup failed\n", ci->ci_upaid);
		else
			printf("cpu%d now spinning idle (waited %d iterations)\n",
			       ci->ci_number, i);
	}

	printf("\n");
}

void
cpu_hatch()
{
	int i;
	char *v = (char*)CPUINFO_VA;

	for (i = 0; i < 4*PAGE_SIZE; i += sizeof(long))
		flush(v + i);

	printf("cpu%d fired up.\n", cpu_number());
	CPUSET_ADD(cpus_active, cpu_number());
	for (i = 0; i < 5000000; i++)
		;
	printf("cpu%d enters idle loop.\n", cpu_number());
	membar_sync();
	spl0();
}
#endif /* MULTIPROCESSOR */
