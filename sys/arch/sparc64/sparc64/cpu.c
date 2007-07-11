/*	$NetBSD: cpu.c,v 1.56.10.1 2007/07/11 20:02:38 mjf Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: cpu.c,v 1.56.10.1 2007/07/11 20:02:38 mjf Exp $");

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

int ecache_min_line_size;

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

static void cpu_reset_fpustate(void);

/* The CPU configuration driver. */
void cpu_attach(struct device *, struct device *, void *);
int cpu_match(struct device *, struct cfdata *, void *);

CFATTACH_DECL(cpu, sizeof(struct device),
    cpu_match, cpu_attach, NULL, NULL);

struct cpu_info *
alloc_cpuinfo(u_int cpu_node)
{
	paddr_t pa0, pa;
	vaddr_t va, va0;
	vsize_t sz = 16 * PAGE_SIZE;
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
	va = uvm_km_alloc(kernel_map, sz, 8 * PAGE_SIZE, UVM_KMF_VAONLY);
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
	 * way as is done for the boot CPU in pmap.c.
	 */
	cpi->ci_next = NULL;
	cpi->ci_curlwp = NULL;
	cpi->ci_number = ++cpu_instance;
	cpi->ci_cpuid = portid;
	cpi->ci_upaid = portid;
	cpi->ci_fplwp = NULL;
	cpi->ci_spinup = NULL;						/* XXX */
	cpi->ci_initstack = (void *)INITSTACK_VA;
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

static void
cpu_reset_fpustate(void)
{
	struct fpstate64 *fpstate;
	struct fpstate64 fps[2];

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
	struct mainbus_attach_args *ma = aux;
	struct cpu_info *ci;
	const char *sep;
	register int i, l;
	int bigcache, cachesize;
	char buf[100];
	int 	totalsize = 0;
	int 	linesize;

	/* tell them what we have */
	node = ma->ma_node;

	/*
	 * Allocate cpu_info structure if needed.
	 */
	ci = alloc_cpuinfo((u_int)node);

	/*
	 * Only do this on the boot cpu.  Other cpu's call
	 * cpu_reset_fpustate() from cpu_hatch() before they
	 * call into the idle loop.
	 * For other cpus, we need to call mi_cpu_attach()
	 * and complete setting up cpcb.
	 */
	if (ci->ci_number == 0)
		cpu_reset_fpustate();
#ifdef MULTIPROCESSOR
	else {
		mi_cpu_attach(ci);
		ci->ci_cpcb = (struct pcb *)ci->ci_data.cpu_idlelwp->l_addr;
	}
#endif

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

	snprintf(buf, sizeof buf, "%s @ %s MHz",
		prom_getpropstring(node, "name"), clockfreq(clk));
	snprintf(cpu_model, sizeof cpu_model, "%s (%s)", machine_model, buf);

	printf(": %s, UPA id %d\n", buf, ci->ci_upaid);
	printf("%s:", dev->dv_xname);

	bigcache = 0;

	linesize = l =
		prom_getpropint(node, "icache-line-size", 0);
	for (i = 0; (1 << i) < l && l; i++)
		/* void */;
	if ((1 << i) != l && l)
		panic("bad icache line size %d", l);
	totalsize =
		prom_getpropint(node, "icache-size", 0) *
		prom_getpropint(node, "icache-associativity", 1);
	if (totalsize == 0)
		totalsize = l *
			prom_getpropint(node, "icache-nlines", 64) *
			prom_getpropint(node, "icache-associativity", 1);

	cachesize = totalsize /
	    prom_getpropint(node, "icache-associativity", 1);
	bigcache = cachesize;

	sep = " ";
	if (totalsize > 0) {
		printf("%s%ldK instruction (%ld b/l)", sep,
		       (long)totalsize/1024,
		       (long)linesize);
		sep = ", ";
	}

	linesize = l =
		prom_getpropint(node, "dcache-line-size",0);
	for (i = 0; (1 << i) < l && l; i++)
		/* void */;
	if ((1 << i) != l && l)
		panic("bad dcache line size %d", l);
	totalsize =
		prom_getpropint(node, "dcache-size", 0) *
		prom_getpropint(node, "dcache-associativity", 1);
	if (totalsize == 0)
		totalsize = l *
			prom_getpropint(node, "dcache-nlines", 128) *
			prom_getpropint(node, "dcache-associativity", 1);

	cachesize = totalsize /
	    prom_getpropint(node, "dcache-associativity", 1);
	if (cachesize > bigcache)
		bigcache = cachesize;

	if (totalsize > 0) {
		printf("%s%ldK data (%ld b/l)", sep,
		       (long)totalsize/1024,
		       (long)linesize);
		sep = ", ";
	}

	linesize = l =
		prom_getpropint(node, "ecache-line-size", 0);
	for (i = 0; (1 << i) < l && l; i++)
		/* void */;
	if ((1 << i) != l && l)
		panic("bad ecache line size %d", l);
	totalsize = 
		prom_getpropint(node, "ecache-size", 0) *
		prom_getpropint(node, "ecache-associativity", 1);
	if (totalsize == 0)
		totalsize = l *
			prom_getpropint(node, "ecache-nlines", 32768) *
			prom_getpropint(node, "ecache-associativity", 1);

	cachesize = totalsize /
	     prom_getpropint(node, "ecache-associativity", 1);
	if (cachesize > bigcache)
		bigcache = cachesize;

	if (totalsize > 0) {
		printf("%s%ldK external (%ld b/l)", sep,
		       (long)totalsize/1024,
		       (long)linesize);
	}
	printf("\n");

	if (ecache_min_line_size == 0 ||
	    linesize < ecache_min_line_size)
		ecache_min_line_size = linesize;

	/*
	 * Now that we know the size of the largest cache on this CPU,
	 * re-color our pages.
	 */
	uvm_page_recolor(atop(bigcache)); /* XXX */

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

	for (ci = cpus; ci != NULL; ci = ci->ci_next) {
		if (ci->ci_upaid == CPU_UPAID)
			continue;

		cpu_args->cb_node = ci->ci_node;
		cpu_args->cb_cpuinfo = ci->ci_paddr;
		cpu_args->cb_initstack = ci->ci_initstack;
		membar_sync();

		/* Disable interrupts and start another CPU. */
		pstate = getpstate();
		setpstate(PSTATE_KERN);

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
	}
}

void
cpu_hatch()
{
	char *v = (char*)CPUINFO_VA;
	int i;

	for (i = 0; i < 4*PAGE_SIZE; i += sizeof(long))
		flush(v + i);

	CPUSET_ADD(cpus_active, cpu_number());
	cpu_reset_fpustate();
	curlwp = curcpu()->ci_data.cpu_idlelwp;
	membar_sync();
	spl0();
}
#endif /* MULTIPROCESSOR */
