/*	$NetBSD: cpu.c,v 1.134.2.1 2018/06/25 07:25:45 pgoyette Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: cpu.c,v 1.134.2.1 2018/06/25 07:25:45 pgoyette Exp $");

#include "opt_multiprocessor.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/kernel.h>
#include <sys/reboot.h>
#include <sys/cpu.h>
#include <sys/sysctl.h>
#include <sys/kmem.h>

#include <uvm/uvm.h>

#include <machine/autoconf.h>
#include <machine/cpu.h>
#include <machine/reg.h>
#include <machine/trap.h>
#include <machine/pmap.h>
#include <machine/sparc64.h>
#include <machine/openfirm.h>
#include <machine/hypervisor.h>
#include <machine/mdesc.h>

#include <sparc64/sparc64/cache.h>

#define SUN4V_MONDO_QUEUE_SIZE	32
#define SUN4V_QUEUE_ENTRY_SIZE	64

int ecache_min_line_size;

/* Linked list of all CPUs in system. */
#if defined(MULTIPROCESSOR)
int sparc_ncpus = 0;
#endif
struct cpu_info *cpus = NULL;

volatile sparc64_cpuset_t cpus_active;/* set of active cpus */
struct cpu_bootargs *cpu_args;	/* allocated very early in pmap_bootstrap. */
struct pool_cache *fpstate_cache;

static struct cpu_info *alloc_cpuinfo(u_int);

/* The following are used externally (sysctl_hw). */
char	machine[] = MACHINE;		/* from <machine/param.h> */
char	machine_arch[] = MACHINE_ARCH;	/* from <machine/param.h> */

/* These are used in locore.s, and are maximums */
int	dcache_line_size;
int	dcache_size;
int	icache_line_size;
int	icache_size;

#ifdef MULTIPROCESSOR
static const char *ipi_evcnt_names[IPI_EVCNT_NUM] = IPI_EVCNT_NAMES;
#endif

static void cpu_reset_fpustate(void);

volatile int sync_tick = 0;

/* The CPU configuration driver. */
void cpu_attach(device_t, device_t, void *);
int cpu_match(device_t, cfdata_t, void *);

CFATTACH_DECL_NEW(cpu, 0, cpu_match, cpu_attach, NULL, NULL);

static int
cpuid_from_node(u_int cpu_node)
{
	/*
	 * Determine the cpuid by examining the nodes properties
	 * in the following order:
	 *  upa-portid
	 *  portid
	 *  cpuid
	 *  reg (sun4v only)
	 */
	
	int id;
	 
	id = prom_getpropint(cpu_node, "upa-portid", -1);
	if (id == -1)
		id = prom_getpropint(cpu_node, "portid", -1);
	if (id == -1)
		id = prom_getpropint(cpu_node, "cpuid", -1);
	if (CPU_ISSUN4V) {
		int reg[4];
		int* regp=reg;
		int len = 4;
		int rc = prom_getprop(cpu_node, "reg", sizeof(int), 
		    &len, &regp);
		if ( rc != 0)
			panic("No reg property found\n");
		/* cpuid in the lower 24 bits - sun4v hypervisor arch */
		id = reg[0] & 0x0fffffff;
	}
	if (id == -1)
		panic("failed to determine cpuid");
	
	return id;
}

static int
cpu_cache_info_sun4v(const char *type, int level, const char *prop)
{
	int idx = 0;
	uint64_t val = 0;
	idx = mdesc_find_node_by_idx(idx, "cache");
	while (idx != -1 && val == 0) {
		const char *name = mdesc_name_by_idx(idx);
		if (strcmp("cache", name) == 0) {
			const char *p;
			size_t len = 0;
			p = mdesc_get_prop_data(idx, "type", &len);
			if (p == NULL)
				panic("No type found\n");
			if (len == 0)
				panic("Len is zero");
			if (type == NULL || strcmp(p, type) == 0) {
				uint64_t l;
				l = mdesc_get_prop_val(idx, "level");
				if (l == level)
					val = mdesc_get_prop_val(idx, prop);
			}
		}
		if (val == 0)
			idx = mdesc_next_node(idx);
	}
	return val;
}

static int
cpu_icache_size(int node)
{
	if (CPU_ISSUN4V)
		return cpu_cache_info_sun4v("instn", 1, "size");
	else 
		return prom_getpropint(node, "icache-size", 0);
}

static int
cpu_icache_line_size(int node)
{
	if (CPU_ISSUN4V)
		return cpu_cache_info_sun4v("instn", 1, "line-size");
	else
		return prom_getpropint(node, "icache-line-size", 0);
}

static int
cpu_icache_nlines(int node)
{
	if (CPU_ISSUN4V)
		return 0;
	else
		return prom_getpropint(node, "icache-nlines", 64);
}

static int
cpu_icache_associativity(int node)
{
	if (CPU_ISSUN4V) {
		int val;
		val = cpu_cache_info_sun4v("instn", 1, "associativity");
		if (val == 0)
			val = 1;
		return val;
	} else
		return prom_getpropint(node, "icache-associativity", 1);
}

static int
cpu_dcache_size(int node)
{
	if (CPU_ISSUN4V)
		return cpu_cache_info_sun4v("data", 1, "size");
	else
		return prom_getpropint(node, "dcache-size", 0);
}

static int
cpu_dcache_line_size(int node)
{
	if (CPU_ISSUN4V)
		return cpu_cache_info_sun4v("data", 1, "line-size");
	else
		return prom_getpropint(node, "dcache-line-size", 0);
}

static int
cpu_dcache_nlines(int node)
{
	if (CPU_ISSUN4V)
		return 0;
	else
		return prom_getpropint(node, "dcache-nlines", 128);
}

static int
cpu_dcache_associativity(int node)
{
	if (CPU_ISSUN4V) {
		int val;
		val = cpu_cache_info_sun4v("data", 1, "associativity");
		if (val == 0)
			val = 1;
		return val;
	} else
		return prom_getpropint(node, "dcache-associativity", 1);
}

int
cpu_ecache_size(int node)
{
	if (CPU_ISSUN4V)
		return cpu_cache_info_sun4v(NULL, 2, "size");
	else
		return prom_getpropint(node, "ecache-size", 0);
}

static int
cpu_ecache_line_size(int node)
{
	if (CPU_ISSUN4V)
		return cpu_cache_info_sun4v(NULL, 2, "line-size");
	else
		return prom_getpropint(node, "ecache-line-size", 0);
}

static int
cpu_ecache_nlines(int node)
{
	if (CPU_ISSUN4V)
		return 0;
	else
		return prom_getpropint(node, "ecache-nlines", 32768);
}

int
cpu_ecache_associativity(int node)
{
	if (CPU_ISSUN4V) {
		int val;
		val = cpu_cache_info_sun4v(NULL, 2, "associativity");
		if (val == 0)
			val = 1;
		return val;
	} else
		return prom_getpropint(node, "ecache-associativity", 1);
}

struct cpu_info *
alloc_cpuinfo(u_int cpu_node)
{
	paddr_t pa0, pa;
	vaddr_t va, va0;
	vsize_t sz = 8 * PAGE_SIZE;
	int cpuid;
	struct cpu_info *cpi, *ci;
	extern paddr_t cpu0paddr;

	/*
	 * Check for matching cpuid in the cpus list.
	 */
	cpuid = cpuid_from_node(cpu_node);

	for (cpi = cpus; cpi != NULL; cpi = cpi->ci_next)
		if (cpi->ci_cpuid == cpuid)
			return cpi;

	/* Allocate the aligned VA and determine the size. */
	va = uvm_km_alloc(kernel_map, sz, 8 * PAGE_SIZE, UVM_KMF_VAONLY);
	if (!va)
		panic("alloc_cpuinfo: no virtual space");
	va0 = va;

	pa0 = cpu0paddr;
	cpu0paddr += sz;

	for (pa = pa0; pa < cpu0paddr; pa += PAGE_SIZE, va += PAGE_SIZE)
		pmap_kenter_pa(va, pa, VM_PROT_READ | VM_PROT_WRITE, 0);

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
	cpi->ci_cpuid = cpuid;
	cpi->ci_fplwp = NULL;
	cpi->ci_eintstack = NULL;
	cpi->ci_spinup = NULL;
	cpi->ci_paddr = pa0;
	cpi->ci_self = cpi;
	if (CPU_ISSUN4V)
		cpi->ci_mmufsa = pa0;
	cpi->ci_node = cpu_node;
	cpi->ci_idepth = -1;
	memset(cpi->ci_intrpending, -1, sizeof(cpi->ci_intrpending));

	/*
	 * Finally, add itself to the list of active cpus.
	 */
	for (ci = cpus; ci->ci_next != NULL; ci = ci->ci_next)
		;
#ifdef MULTIPROCESSOR
	ci->ci_next = cpi;
#endif
	return (cpi);
}

int
cpu_match(device_t parent, cfdata_t cf, void *aux)
{
	struct mainbus_attach_args *ma = aux;

	if (strcmp(cf->cf_name, ma->ma_name) != 0)
		return 0;

	/*
	 * If we are going to only attach a single cpu, make sure
	 * to pick the one we are running on right now.
	 */
	if (cpuid_from_node(ma->ma_node) != cpu_myid()) {
#ifdef MULTIPROCESSOR
		if (boothowto & RB_MD1)
#endif
			return 0;
	}

	return 1;
}

static void
cpu_reset_fpustate(void)
{
	struct fpstate64 *fpstate;
	struct fpstate64 fps[2];

	/* This needs to be 64-byte aligned */
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

/* setup the hw.cpuN.* nodes for this cpu */
static void
cpu_setup_sysctl(struct cpu_info *ci, device_t dev)
{
	const struct sysctlnode *cpunode = NULL;

	sysctl_createv(NULL, 0, NULL, &cpunode,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_NODE, device_xname(dev), NULL,
		       NULL, 0, NULL, 0,
		       CTL_HW,
		       CTL_CREATE, CTL_EOL);

	if (cpunode == NULL)
		return;

#define SETUPS(name, member)					\
	sysctl_createv(NULL, 0, &cpunode, NULL,			\
		       CTLFLAG_PERMANENT,			\
		       CTLTYPE_STRING, name, NULL,		\
		       NULL, 0, member, 0,			\
		       CTL_CREATE, CTL_EOL);

	SETUPS("name", __UNCONST(ci->ci_name))
#undef SETUPS

#define SETUPI(name, member)					\
	sysctl_createv(NULL, 0, &cpunode, NULL,			\
		       CTLFLAG_PERMANENT,			\
		       CTLTYPE_INT, name, NULL,			\
		       NULL, 0, member, 0,			\
		       CTL_CREATE, CTL_EOL);

	SETUPI("id", &ci->ci_cpuid);
#undef SETUPI

#define SETUPQ(name, member)					\
	sysctl_createv(NULL, 0, &cpunode, NULL,			\
		       CTLFLAG_PERMANENT,			\
		       CTLTYPE_QUAD, name, NULL,			\
		       NULL, 0, member, 0,			\
		       CTL_CREATE, CTL_EOL);

	SETUPQ("clock_frequency", &ci->ci_cpu_clockrate[0])
	SETUPQ("ver", &ci->ci_ver)
#undef SETUPI

        sysctl_createv(NULL, 0, &cpunode, NULL, 
                       CTLFLAG_PERMANENT,
                       CTLTYPE_STRUCT, "cacheinfo", NULL,
                       NULL, 0, &ci->ci_cacheinfo, sizeof(ci->ci_cacheinfo),
		       CTL_CREATE, CTL_EOL);

}

/*
 * Attach the CPU.
 * Discover interesting goop about the virtual address cache
 * (slightly funny place to do it, but this is where it is to be found).
 */
void
cpu_attach(device_t parent, device_t dev, void *aux)
{
	int node;
	uint64_t clk, sclk = 0;
	struct mainbus_attach_args *ma = aux;
	struct cpu_info *ci;
	const char *sep;
	register int i, l;
	int bigcache, cachesize;
	char buf[100];
	int 	totalsize = 0;
	int 	linesize, dcachesize, icachesize;

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
	if (ci->ci_flags & CPUF_PRIMARY) {
		fpstate_cache = pool_cache_init(sizeof(struct fpstate64),
					SPARC64_BLOCK_SIZE, 0, 0, "fpstate",
					NULL, IPL_NONE, NULL, NULL, NULL);
		cpu_reset_fpustate();
	}
#ifdef MULTIPROCESSOR
	else {
		mi_cpu_attach(ci);
		ci->ci_cpcb = lwp_getpcb(ci->ci_data.cpu_idlelwp);
	}
	for (i = 0; i < IPI_EVCNT_NUM; ++i)
		evcnt_attach_dynamic(&ci->ci_ipi_evcnt[i], EVCNT_TYPE_INTR,
				     NULL, device_xname(dev), ipi_evcnt_names[i]);
#endif
	evcnt_attach_dynamic(&ci->ci_tick_evcnt, EVCNT_TYPE_INTR, NULL,
			     device_xname(dev), "timer");
	mutex_init(&ci->ci_ctx_lock, MUTEX_SPIN, IPL_VM);

	clk = prom_getpropuint64(node, "clock-frequency64", 0);
	if (clk == 0)
	  clk = prom_getpropint(node, "clock-frequency", 0);
	if (clk == 0) {
		/*
		 * Try to find it in the OpenPROM root...
		 */
		clk = prom_getpropint(findroot(), "clock-frequency", 0);
	}
	if (clk) {
		/* Tell OS what frequency we run on */
		ci->ci_cpu_clockrate[0] = clk;
		ci->ci_cpu_clockrate[1] = clk / 1000000;
	}

	sclk = prom_getpropint(findroot(), "stick-frequency", 0);

	ci->ci_system_clockrate[0] = sclk;
	ci->ci_system_clockrate[1] = sclk / 1000000;

	ci->ci_name = kmem_strdupsize(prom_getpropstring(node, "name"), NULL,
				      KM_NOSLEEP);
	snprintf(buf, sizeof buf, "%s @ %s MHz", ci->ci_name, clockfreq(clk));
	cpu_setmodel("%s (%s)", machine_model, buf);

	aprint_normal(": %s, CPU id %d\n", buf, ci->ci_cpuid);
	aprint_naive("\n");
	if (CPU_ISSUN4U || CPU_ISSUN4US) {
		ci->ci_ver = getver();
		aprint_normal_dev(dev, "manuf %x, impl %x, mask %x\n",
		    (u_int)GETVER_CPU_MANUF(),
		    (u_int)GETVER_CPU_IMPL(),
		    (u_int)GETVER_CPU_MASK());
	}

	if (ci->ci_system_clockrate[0] != 0) {
		aprint_normal_dev(dev, "system tick frequency %s MHz\n",
		    clockfreq(ci->ci_system_clockrate[0]));
	}
	aprint_normal_dev(dev, "");

	bigcache = 0;

	icachesize = cpu_icache_size(node);
	if (icachesize > icache_size)
		icache_size = icachesize;
	linesize = l = cpu_icache_line_size(node);
	if (linesize > icache_line_size)
		icache_line_size = linesize;

	for (i = 0; (1 << i) < l && l; i++)
		/* void */;
	if ((1 << i) != l && l)
		panic("bad icache line size %d", l);
	totalsize = icachesize;
	if (totalsize == 0)
		totalsize = l *
		    cpu_icache_nlines(node) * cpu_icache_associativity(node);

	cachesize = totalsize / cpu_icache_associativity(node);
	bigcache = cachesize;

	sep = "";
	if (totalsize > 0) {
		aprint_normal("%s%ldK instruction (%ld b/l)", sep,
		       (long)totalsize/1024,
		       (long)linesize);
		sep = ", ";
	}
	ci->ci_cacheinfo.c_itotalsize = totalsize;
	ci->ci_cacheinfo.c_ilinesize = linesize;

	dcachesize = cpu_dcache_size(node);
	if (dcachesize > dcache_size)
		dcache_size = dcachesize;
	linesize = l = cpu_dcache_line_size(node);
	if (linesize > dcache_line_size)
		dcache_line_size = linesize;

	for (i = 0; (1 << i) < l && l; i++)
		/* void */;
	if ((1 << i) != l && l)
		panic("bad dcache line size %d", l);
	totalsize = dcachesize;
	if (totalsize == 0)
		totalsize = l *
		    cpu_dcache_nlines(node) * cpu_dcache_associativity(node);

	cachesize = totalsize / cpu_dcache_associativity(node);
	if (cachesize > bigcache)
		bigcache = cachesize;

	if (totalsize > 0) {
		aprint_normal("%s%ldK data (%ld b/l)", sep,
		       (long)totalsize/1024,
		       (long)linesize);
		sep = ", ";
	}
	ci->ci_cacheinfo.c_dtotalsize = totalsize;
	ci->ci_cacheinfo.c_dlinesize = linesize;

	linesize = l = cpu_ecache_line_size(node);
	for (i = 0; (1 << i) < l && l; i++)
		/* void */;
	if ((1 << i) != l && l)
		panic("bad ecache line size %d", l);
	totalsize = cpu_ecache_size(node);
	if (totalsize == 0)
		totalsize = l *
		    cpu_ecache_nlines(node) * cpu_ecache_associativity(node);

	cachesize = totalsize / cpu_ecache_associativity(node);
	if (cachesize > bigcache)
		bigcache = cachesize;

	if (totalsize > 0) {
		aprint_normal("%s%ldK external (%ld b/l)", sep,
		       (long)totalsize/1024,
		       (long)linesize);
	}
	aprint_normal("\n");
	ci->ci_cacheinfo.c_etotalsize = totalsize;
	ci->ci_cacheinfo.c_elinesize = linesize;

	if (ecache_min_line_size == 0 ||
	    linesize < ecache_min_line_size)
		ecache_min_line_size = linesize;

	cpu_setup_sysctl(ci, dev);

	/*
	 * Now that we know the size of the largest cache on this CPU,
	 * re-color our pages.
	 */
	uvm_page_recolor(atop(bigcache)); /* XXX */

	/*
	 * CPU specific ipi setup
	 * Currently only necessary for SUN4V
	 */
	if (CPU_ISSUN4V) {
		paddr_t pa = ci->ci_paddr;
		int err;

		pa += CPUINFO_VA - INTSTACK;
		pa += PAGE_SIZE;

		ci->ci_cpumq = pa;
		err = hv_cpu_qconf(CPU_MONDO_QUEUE, ci->ci_cpumq, SUN4V_MONDO_QUEUE_SIZE);
		if (err != H_EOK)
			panic("Unable to set cpu mondo queue: %d", err);
		pa += SUN4V_MONDO_QUEUE_SIZE * SUN4V_QUEUE_ENTRY_SIZE;
		
		ci->ci_devmq = pa;
		err = hv_cpu_qconf(DEVICE_MONDO_QUEUE, ci->ci_devmq, SUN4V_MONDO_QUEUE_SIZE);
		if (err != H_EOK)
			panic("Unable to set device mondo queue: %d", err);
		pa += SUN4V_MONDO_QUEUE_SIZE * SUN4V_QUEUE_ENTRY_SIZE;
		
		ci->ci_mondo = pa;
		pa += 64; /* mondo message is 64 bytes */
		
		ci->ci_cpuset = pa;
		pa += 64;
	}
	
}

int
cpu_myid(void)
{
	char buf[32];

	if (CPU_ISSUN4V) {
		uint64_t myid;
		hv_cpu_myid(&myid);
		return myid;
	}
	if (OF_getprop(findroot(), "name", buf, sizeof(buf)) > 0 &&
	    strcmp(buf, "SUNW,Ultra-Enterprise-10000") == 0)
		return lduwa(0x1fff40000d0UL, ASI_PHYS_NON_CACHED);
	switch (GETVER_CPU_IMPL()) {
		case IMPL_OLYMPUS_C:
		case IMPL_JUPITER:
			return CPU_JUPITERID;
		case IMPL_CHEETAH:
		case IMPL_CHEETAH_PLUS:
		case IMPL_JAGUAR:
		case IMPL_PANTHER:
			return CPU_FIREPLANEID;
		default:
			return CPU_UPAID;
	}
}

#if defined(MULTIPROCESSOR)
vaddr_t cpu_spinup_trampoline;

/*
 * Start secondary processors in motion.
 */
void
cpu_boot_secondary_processors(void)
{
	int i, pstate;
	struct cpu_info *ci;

	sync_tick = 0;

	sparc64_ipi_init();

	if (boothowto & RB_MD1) {
		cpus[0].ci_next = NULL;
		sparc_ncpus = ncpu = ncpuonline = 1;
		return;
	}

	for (ci = cpus; ci != NULL; ci = ci->ci_next) {
		if (ci->ci_cpuid == cpu_myid())
			continue;

		cpu_pmap_prepare(ci, false);
		cpu_args->cb_node = ci->ci_node;
		cpu_args->cb_cpuinfo = ci->ci_paddr;
		cpu_args->cb_cputyp = cputyp;
		membar_Sync();

		/* Disable interrupts and start another CPU. */
		pstate = getpstate();
		setpstate(PSTATE_KERN);

		int rc = prom_startcpu_by_cpuid(ci->ci_cpuid,
		    (void *)cpu_spinup_trampoline, 0);
		if (rc == -1)
			prom_startcpu(ci->ci_node,
			    (void *)cpu_spinup_trampoline, 0);

		for (i = 0; i < 2000; i++) {
			membar_Sync();
			if (CPUSET_HAS(cpus_active, ci->ci_index))
				break;
			delay(10000);
		}

		/* synchronize %tick ( to some degree at least ) */
		delay(1000);
		sync_tick = 1;
		membar_Sync();
		if (CPU_ISSUN4U || CPU_ISSUN4US)
			settick(0);
		if (ci->ci_system_clockrate[0] != 0)
			if (CPU_ISSUN4U || CPU_ISSUN4US)
				setstick(0);

		setpstate(pstate);

		if (!CPUSET_HAS(cpus_active, ci->ci_index))
			printf("cpu%d: startup failed\n", ci->ci_cpuid);
	}
}

void
cpu_hatch(void)
{
	char *v = (char*)CPUINFO_VA;
	int i;

	/* XXX - why flush the icache here? but should be harmless */
	for (i = 0; i < 4*PAGE_SIZE; i += sizeof(long))
		sparc_flush_icache(v + i);

	cpu_pmap_init(curcpu());
	CPUSET_ADD(cpus_active, cpu_number());
	cpu_reset_fpustate();
	curlwp = curcpu()->ci_data.cpu_idlelwp;
	membar_Sync();

	/* wait for the boot CPU to flip the switch */
	while (sync_tick == 0) {
		/* we do nothing here */
	}
	if (CPU_ISSUN4U || CPU_ISSUN4US)
		settick(0);
	if (curcpu()->ci_system_clockrate[0] != 0) {
		if (CPU_ISSUN4U || CPU_ISSUN4US)
			setstick(0);
		stickintr_establish(PIL_CLOCK, stickintr);
	} else {
		tickintr_establish(PIL_CLOCK, tickintr);
	}
	spl0();
}
#endif /* MULTIPROCESSOR */
