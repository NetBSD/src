/*	$NetBSD: cpu.c,v 1.249.6.1 2017/12/08 06:05:15 msaitoh Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: cpu.c,v 1.249.6.1 2017/12/08 06:05:15 msaitoh Exp $");

#include "opt_multiprocessor.h"
#include "opt_lockdebug.h"
#include "opt_ddb.h"
#include "opt_sparc_arch.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/kernel.h>
#include <sys/evcnt.h>
#include <sys/xcall.h>
#include <sys/ipi.h>
#include <sys/cpu.h>

#include <uvm/uvm.h>

#include <machine/promlib.h>
#include <machine/autoconf.h>
#include <machine/cpu.h>
#include <machine/reg.h>
#include <machine/ctlreg.h>
#include <machine/trap.h>
#include <machine/pcb.h>
#include <machine/pmap.h>

#if defined(MULTIPROCESSOR) && defined(DDB)
#include <ddb/db_output.h>
#include <machine/db_machdep.h>
#endif

#include <sparc/sparc/cache.h>
#include <sparc/sparc/asm.h>
#include <sparc/sparc/cpuvar.h>
#include <sparc/sparc/memreg.h>
#if defined(SUN4D)
#include <sparc/sparc/cpuunitvar.h>
#endif

#ifdef DEBUG
#ifndef DEBUG_XCALL
#define DEBUG_XCALL 0
#endif
int	debug_xcall = DEBUG_XCALL;
#else
#define debug_xcall 0
#endif

struct cpu_softc {
	device_t sc_dev;
	struct cpu_info	*sc_cpuinfo;
};

/* The following are used externally (sysctl_hw). */
char	machine[] = MACHINE;		/* from <machine/param.h> */
char	machine_arch[] = MACHINE_ARCH;	/* from <machine/param.h> */
int	cpu_arch;			/* sparc architecture version */
extern char machine_model[];

int	sparc_ncpus;			/* # of CPUs detected by PROM */
struct cpu_info *cpus[_MAXNCPU+1];	/* we only support 4 CPUs. */

/* The CPU configuration driver. */
static void cpu_mainbus_attach(device_t, device_t, void *);
int  cpu_mainbus_match(device_t, cfdata_t, void *);

CFATTACH_DECL_NEW(cpu_mainbus, sizeof(struct cpu_softc),
    cpu_mainbus_match, cpu_mainbus_attach, NULL, NULL);

#if defined(SUN4D)
static int cpu_cpuunit_match(device_t, cfdata_t, void *);
static void cpu_cpuunit_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(cpu_cpuunit, sizeof(struct cpu_softc),
    cpu_cpuunit_match, cpu_cpuunit_attach, NULL, NULL);
#endif /* SUN4D */

static void cpu_init_evcnt(struct cpu_info *cpi);
static void cpu_attach(struct cpu_softc *, int, int);

static const char *fsrtoname(int, int, int);
void cache_print(struct cpu_softc *);
void cpu_setup(void);
void fpu_init(struct cpu_info *);

#define	IU_IMPL(psr)	((u_int)(psr) >> 28)
#define	IU_VERS(psr)	(((psr) >> 24) & 0xf)

#define SRMMU_IMPL(mmusr)	((u_int)(mmusr) >> 28)
#define SRMMU_VERS(mmusr)	(((mmusr) >> 24) & 0xf)

int bootmid;		/* Module ID of boot CPU */

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

#if defined(MULTIPROCESSOR)
u_int	cpu_ready_mask;			/* the set of CPUs marked as READY */
void cpu_spinup(struct cpu_info *);
static void cpu_attach_non_boot(struct cpu_softc *, struct cpu_info *, int);

int go_smp_cpus = 0;	/* non-primary CPUs wait for this to go */

/*
 * This must be locked around all message transactions to ensure only
 * one CPU is generating them.
 */
kmutex_t xpmsg_mutex;

#endif /* MULTIPROCESSOR */

/*
 * 4/110 comment: the 4/110 chops off the top 4 bits of an OBIO address.
 *	this confuses autoconf.  for example, if you try and map
 *	0xfe000000 in obio space on a 4/110 it actually maps 0x0e000000.
 *	this is easy to verify with the PROM.   this causes problems
 *	with devices like "esp0 at obio0 addr 0xfa000000" because the
 *	4/110 treats it as esp0 at obio0 addr 0x0a000000" which is the
 *	address of the 4/110's "sw0" scsi chip.   the same thing happens
 *	between zs1 and zs2.    since the sun4 line is "closed" and
 *	we know all the "obio" devices that will ever be on it we just
 *	put in some special case "if"'s in the match routines of esp,
 *	dma, and zs.
 */

int
cpu_mainbus_match(device_t parent, cfdata_t cf, void *aux)
{
	struct mainbus_attach_args *ma = aux;

	return (strcmp(cf->cf_name, ma->ma_name) == 0);
}

static void
cpu_mainbus_attach(device_t parent, device_t self, void *aux)
{
	struct mainbus_attach_args *ma = aux;
	struct { uint32_t va; uint32_t size; } *mbprop = NULL;
	struct openprom_addr *rrp = NULL;
	struct cpu_info *cpi;
	struct cpu_softc *sc;
	int mid, node;
	int error, n;

	node = ma->ma_node;
	mid = (node != 0) ? prom_getpropint(node, "mid", 0) : 0;
	sc = device_private(self);
	sc->sc_dev = self;
	cpu_attach(sc, node, mid);

	cpi = sc->sc_cpuinfo;
	if (cpi == NULL)
		return;

	/*
	 * Map CPU mailbox if available
	 */
	if (node != 0 && (error = prom_getprop(node, "mailbox-virtual",
					sizeof(*mbprop),
					&n, &mbprop)) == 0) {
		cpi->mailbox = mbprop->va;
		free(mbprop, M_DEVBUF);
	} else if (node != 0 && (error = prom_getprop(node, "mailbox",
					sizeof(struct openprom_addr),
					&n, &rrp)) == 0) {
		/* XXX - map cached/uncached? If cached, deal with
		 *	 cache congruency!
		 */
		if (rrp[0].oa_space == 0)
			printf("%s: mailbox in mem space\n", device_xname(self));

		if (bus_space_map(ma->ma_bustag,
				BUS_ADDR(rrp[0].oa_space, rrp[0].oa_base),
				rrp[0].oa_size,
				BUS_SPACE_MAP_LINEAR,
				&cpi->mailbox) != 0)
			panic("%s: can't map CPU mailbox", device_xname(self));
		free(rrp, M_DEVBUF);
	}

	/*
	 * Map Module Control Space if available
	 */
	if (cpi->mxcc == 0)
		/* We only know what it means on MXCCs */
		return;

	rrp = NULL;
	if (node == 0 || (error = prom_getprop(node, "reg",
					sizeof(struct openprom_addr),
					&n, &rrp)) != 0)
		return;

	/* register set #0 is the MBus port register */
	if (bus_space_map(ma->ma_bustag,
			BUS_ADDR(rrp[0].oa_space, rrp[0].oa_base),
			rrp[0].oa_size,
			BUS_SPACE_MAP_LINEAR,
			&cpi->ci_mbusport) != 0) {
		panic("%s: can't map CPU regs", device_xname(self));
	}
	/* register set #1: MCXX control */
	if (bus_space_map(ma->ma_bustag,
			BUS_ADDR(rrp[1].oa_space, rrp[1].oa_base),
			rrp[1].oa_size,
			BUS_SPACE_MAP_LINEAR,
			&cpi->ci_mxccregs) != 0) {
		panic("%s: can't map CPU regs", device_xname(self));
	}
	/* register sets #3 and #4 are E$ cache data and tags */

	free(rrp, M_DEVBUF);
}

#if defined(SUN4D)
static int
cpu_cpuunit_match(device_t parent, cfdata_t cf, void *aux)
{
	struct cpuunit_attach_args *cpua = aux;

	return (strcmp(cf->cf_name, cpua->cpua_type) == 0);
}

static void
cpu_cpuunit_attach(device_t parent, device_t self, void *aux)
{
	struct cpuunit_attach_args *cpua = aux;
	struct cpu_softc *sc = device_private(self);

	sc->sc_dev = self;
	cpu_attach(sc, cpua->cpua_node, cpua->cpua_device_id);
}
#endif /* SUN4D */

static const char * const hard_intr_names[] = {
	"spur hard",
	"lev1 hard",
	"lev2 hard",
	"lev3 hard",
	"lev4 hard",
	"lev5 hard",
	"lev6 hard",
	"lev7 hard",
	"lev8 hard",
	"lev9 hard",
	"clock hard",
	"lev11 hard",
	"lev12 hard",
	"lev13 hard",
	"prof hard",
	"nmi hard",
};

static const char * const soft_intr_names[] = {
	"spur soft",
	"lev1 soft",
	"lev2 soft",
	"lev3 soft",
	"lev4 soft",
	"lev5 soft",
	"lev6 soft",
	"lev7 soft",
	"lev8 soft",
	"lev9 soft",
	"lev10 soft",
	"lev11 soft",
	"lev12 soft",
	"xcall std",
	"xcall fast",
	"nmi soft",
};

static void
cpu_init_evcnt(struct cpu_info *cpi)
{
	int i;

	/*
	 * Setup the per-cpu counters.
	 *
	 * The "savefp null" counter should go away when the NULL
	 * struct fpstate * bug is fixed.
	 */
	evcnt_attach_dynamic(&cpi->ci_savefpstate, EVCNT_TYPE_MISC,
			     NULL, cpu_name(cpi), "savefp ipi");
	evcnt_attach_dynamic(&cpi->ci_savefpstate_null, EVCNT_TYPE_MISC,
			     NULL, cpu_name(cpi), "savefp null ipi");
	evcnt_attach_dynamic(&cpi->ci_xpmsg_mutex_fail, EVCNT_TYPE_MISC,
			     NULL, cpu_name(cpi), "IPI mutex_trylock fail");
	evcnt_attach_dynamic(&cpi->ci_xpmsg_mutex_fail_call, EVCNT_TYPE_MISC,
			     NULL, cpu_name(cpi), "IPI mutex_trylock fail/call");
	evcnt_attach_dynamic(&cpi->ci_xpmsg_mutex_not_held, EVCNT_TYPE_MISC,
			     NULL, cpu_name(cpi), "IPI with mutex not held");
	evcnt_attach_dynamic(&cpi->ci_xpmsg_bogus, EVCNT_TYPE_MISC,
			     NULL, cpu_name(cpi), "bogus IPI");

	/*
	 * These are the per-cpu per-IPL hard & soft interrupt counters.
	 */
	for (i = 0; i < 16; i++) {
		evcnt_attach_dynamic(&cpi->ci_intrcnt[i], EVCNT_TYPE_INTR,
				     NULL, cpu_name(cpi), hard_intr_names[i]);
		evcnt_attach_dynamic(&cpi->ci_sintrcnt[i], EVCNT_TYPE_INTR,
				     NULL, cpu_name(cpi), soft_intr_names[i]);
	}
}

/*
 * Attach the CPU.
 * Discover interesting goop about the virtual address cache
 * (slightly funny place to do it, but this is where it is to be found).
 */
static void
cpu_attach(struct cpu_softc *sc, int node, int mid)
{
	char buf[100];
	struct cpu_info *cpi;
	int idx;
	static int cpu_attach_count = 0;

	/*
	 * The first CPU we're attaching must be the boot CPU.
	 * (see autoconf.c and cpuunit.c)
	 */
	idx = cpu_attach_count++;

#if !defined(MULTIPROCESSOR)
	if (cpu_attach_count > 1) {
		printf(": no SMP support in kernel\n");
		return;
	}
#endif

	/*
	 * Initialise this cpu's cpu_info.
	 */
	cpi = sc->sc_cpuinfo = cpus[idx];
	getcpuinfo(cpi, node);

	cpi->ci_cpuid = idx;
	cpi->mid = mid;
	cpi->node = node;
#ifdef DEBUG
	cpi->redzone = (void *)((long)cpi->eintstack + REDSIZE);
#endif

	if (sparc_ncpus > 1) {
		printf(": mid %d", mid);
		if (mid == 0 && !CPU_ISSUN4D)
			printf(" [WARNING: mid should not be 0]");
	}

#if defined(MULTIPROCESSOR)
	if (cpu_attach_count > 1) {
		cpu_attach_non_boot(sc, cpi, node);
		cpu_init_evcnt(cpi);
		return;
	}
#endif /* MULTIPROCESSOR */

	cpu_init_evcnt(cpi);

	/* Stuff to only run on the boot CPU */
	cpu_setup();
	snprintf(buf, sizeof buf, "%s @ %s MHz, %s FPU",
		cpi->cpu_longname, clockfreq(cpi->hz), cpi->fpu_name);
	cpu_setmodel("%s (%s)", machine_model, buf);
	printf(": %s\n", buf);
	cache_print(sc);

	cpi->master = 1;
	cpi->eintstack = eintstack;

	/*
	 * If we haven't been able to determine the Id of the
	 * boot CPU, set it now. In this case we can only boot
	 * from CPU #0 (see also the CPU attach code in autoconf.c)
	 */
	if (bootmid == 0)
		bootmid = mid;
}

/*
 * Finish CPU attach.
 * Must be run by the CPU which is being attached.
 */
void
cpu_setup(void)
{
 	if (cpuinfo.hotfix)
		(*cpuinfo.hotfix)(&cpuinfo);

	/* Initialize FPU */
	fpu_init(&cpuinfo);

	/* Enable the cache */
	cpuinfo.cache_enable();

	cpuinfo.flags |= CPUFLG_HATCHED;
}

#if defined(MULTIPROCESSOR)
/*
 * Perform most of the tasks needed for a non-boot CPU.
 */
static void
cpu_attach_non_boot(struct cpu_softc *sc, struct cpu_info *cpi, int node)
{
	vaddr_t intstack, va;
	int error;

	/*
	 * Arrange interrupt stack.  This cpu will also abuse the bottom
	 * half of the interrupt stack before it gets to run its idle LWP.
	 */
	intstack = uvm_km_alloc(kernel_map, INT_STACK_SIZE, 0, UVM_KMF_WIRED);
	if (intstack == 0)
		panic("%s: no uspace/intstack", __func__);
	cpi->eintstack = (void*)(intstack + INT_STACK_SIZE);

	/* Allocate virtual space for pmap page_copy/page_zero */
	va = uvm_km_alloc(kernel_map, 2*PAGE_SIZE, 0, UVM_KMF_VAONLY);
	if (va == 0)
		panic("%s: no virtual space", __func__);

	cpi->vpage[0] = (void *)(va + 0);
	cpi->vpage[1] = (void *)(va + PAGE_SIZE);

	/*
	 * Call the MI attach which creates an idle LWP for us.
	 */
	error = mi_cpu_attach(cpi);
	if (error != 0) {
		aprint_normal("\n");
		aprint_error("%s: mi_cpu_attach failed with %d\n",
		    device_xname(sc->sc_dev), error);
		return;
	}

	/*
	 * Note: `eintstack' is set in cpu_attach_non_boot() above.
	 * The %wim register will be initialized in cpu_hatch().
	 */
	cpi->ci_curlwp = cpi->ci_data.cpu_idlelwp;
	cpi->curpcb = lwp_getpcb(cpi->ci_curlwp);
	cpi->curpcb->pcb_wim = 1;

	/* for now use the fixed virtual addresses setup in autoconf.c */
	cpi->intreg_4m = (struct icr_pi *)
		(PI_INTR_VA + (_MAXNBPG * CPU_MID2CPUNO(cpi->mid)));

	/* Now start this CPU */
	cpu_spinup(cpi);
	printf(": %s @ %s MHz, %s FPU\n", cpi->cpu_longname,
		clockfreq(cpi->hz), cpi->fpu_name);

	cache_print(sc);

	/*
	 * Now we're on the last CPU to be attaching.
	 */
	if (sparc_ncpus > 1 && cpi->ci_cpuid == sparc_ncpus - 1) {
		CPU_INFO_ITERATOR n;
		/*
		 * Install MP cache flush functions, unless the
		 * single-processor versions are no-ops.
		 */
		for (CPU_INFO_FOREACH(n, cpi)) {
#define SET_CACHE_FUNC(x) \
	if (cpi->x != __CONCAT(noop_,x)) cpi->x = __CONCAT(smp_,x)
			SET_CACHE_FUNC(vcache_flush_page);
			SET_CACHE_FUNC(vcache_flush_segment);
			SET_CACHE_FUNC(vcache_flush_region);
			SET_CACHE_FUNC(vcache_flush_context);
		}
	}
#undef SET_CACHE_FUNC
}

/*
 * Start secondary processors in motion.
 */
void
cpu_boot_secondary_processors(void)
{
	CPU_INFO_ITERATOR n;
	struct cpu_info *cpi;

	printf("cpu0: booting secondary processors:");
	for (CPU_INFO_FOREACH(n, cpi)) {
		if (cpuinfo.mid == cpi->mid ||
		    (cpi->flags & CPUFLG_HATCHED) == 0)
			continue;

		printf(" cpu%d", cpi->ci_cpuid);
		cpu_ready_mask |= (1 << n);
	}

	/* Mark the boot CPU as ready */
	cpu_ready_mask |= (1 << 0);

	/* Tell the other CPU's to start up.  */
	go_smp_cpus = 1;

	printf("\n");
}

/*
 * Early initialisation, before main().
 */
void
cpu_init_system(void)
{

	mutex_init(&xpmsg_mutex, MUTEX_SPIN, IPL_SCHED);
}

/*
 * Allocate per-CPU data, then start up this CPU using PROM.
 */
void
cpu_spinup(struct cpu_info *cpi)
{
	extern void cpu_hatch(void); /* in locore.s */
	struct openprom_addr oa;
	void *pc;
	int n;

	pc = (void *)cpu_hatch;

	/* Setup CPU-specific MMU tables */
	pmap_alloc_cpu(cpi);

	cpi->flags &= ~CPUFLG_HATCHED;

	/*
	 * The physical address of the context table is passed to
	 * the PROM in a "physical address descriptor".
	 */
	oa.oa_space = 0;
	oa.oa_base = (uint32_t)cpi->ctx_tbl_pa;
	oa.oa_size = cpi->mmu_ncontext * sizeof(cpi->ctx_tbl[0]); /*???*/

	/*
	 * Flush entire cache here, since the CPU may start with
	 * caches off, hence no cache-coherency may be assumed.
	 */
	cpuinfo.cache_flush_all();
	prom_cpustart(cpi->node, &oa, 0, pc);

	/*
	 * Wait for this CPU to spin up.
	 */
	for (n = 10000; n != 0; n--) {
		cache_flush((void *) __UNVOLATILE(&cpi->flags),
			    sizeof(cpi->flags));
		if (cpi->flags & CPUFLG_HATCHED)
			return;
		delay(100);
	}
	printf("CPU did not spin up\n");
}

/*
 * Call a function on some CPUs.  `cpuset' can be set to CPUSET_ALL
 * to call every CPU, or `1 << cpi->ci_cpuid' for each CPU to call.
 */
void
xcall(xcall_func_t func, xcall_trap_t trap, int arg0, int arg1, int arg2,
      u_int cpuset)
{
	struct cpu_info *cpi;
	int n, i, done, callself, mybit;
	volatile struct xpmsg_func *p;
	u_int pil;
	int fasttrap;
	int is_noop = func == (xcall_func_t)sparc_noop;
	static char errbuf[160];
	char *bufp = errbuf;
	size_t bufsz = sizeof errbuf, wrsz;

	if (is_noop) return;

	mybit = (1 << cpuinfo.ci_cpuid);
	callself = func && (cpuset & mybit) != 0;
	cpuset &= ~mybit;

	/* Mask any CPUs that are not ready */
	cpuset &= cpu_ready_mask;

#if 0
	mutex_spin_enter(&xpmsg_mutex);
#else
	/*
	 * There's a deadlock potential between multiple CPUs trying
	 * to xcall() at the same time, and the thread that loses the
	 * race to get xpmsg_lock is at an IPL above the incoming IPI
	 * IPL level, so it sits around waiting to take the lock while
	 * the other CPU is waiting for this CPU to handle the IPI and
	 * mark it as completed.
	 *
	 * If we fail to get the mutex, and we're at high enough IPL,
	 * call xcallintr() if there is a valid msg.tag.
	 */
	pil = (getpsr() & PSR_PIL) >> 8;
	
	if (cold || pil <= IPL_SCHED)
		mutex_spin_enter(&xpmsg_mutex);
	else {
		/*
		 * Warn about xcall at high IPL.
		 *
		 * XXX This is probably bogus (logging at high IPL),
		 * XXX so we don't do it by default.
		 */
		if (debug_xcall && (void *)func != sparc_noop) {
			u_int pc;

			__asm("mov %%i7, %0" : "=r" (pc) : );
			printf_nolog("%d: xcall %p at lvl %u from 0x%x\n",
			    cpu_number(), func, pil, pc);
		}

		while (mutex_tryenter(&xpmsg_mutex) == 0) {
			cpuinfo.ci_xpmsg_mutex_fail.ev_count++;
			if (cpuinfo.msg.tag) {
				cpuinfo.ci_xpmsg_mutex_fail_call.ev_count++;
				xcallintr(xcallintr);
			}
		}
	}
#endif

	/*
	 * Firstly, call each CPU.  We do this so that they might have
	 * finished by the time we start looking.
	 */
	fasttrap = trap != NULL ? 1 : 0;
	for (CPU_INFO_FOREACH(n, cpi)) {

		/* Note: n == cpi->ci_cpuid */
		if ((cpuset & (1 << n)) == 0)
			continue;

		/*
		 * Write msg.tag last - if another CPU is polling above it may
		 * end up seeing an incomplete message. Not likely but still.
		 */ 
		cpi->msg.complete = 0;
		p = &cpi->msg.u.xpmsg_func;
		p->func = func;
		p->trap = trap;
		p->arg0 = arg0;
		p->arg1 = arg1;
		p->arg2 = arg2;
		__insn_barrier();
		cpi->msg.tag = XPMSG_FUNC;
		__insn_barrier();
		/* Fast cross calls use interrupt level 14 */
		raise_ipi(cpi,13+fasttrap);/*xcall_cookie->pil*/
	}

	/*
	 * Second, call ourselves.
	 */
	if (callself)
		(*func)(arg0, arg1, arg2);

	/*
	 * Lastly, start looping, waiting for all CPUs to register that they
	 * have completed (bailing if it takes "too long", being loud about
	 * this in the process).
	 */
	done = 0;
	i = 1000000;	/* time-out, not too long, but still an _AGE_ */
	while (!done) {
		if (--i < 0) {
			wrsz = snprintf(bufp, bufsz,
			    "xcall(cpu%d,%p) from %p: couldn't ping cpus:",
			    cpu_number(), fasttrap ? trap : func,
			    __builtin_return_address(0));
			if (wrsz > bufsz)
				break;
			bufsz -= wrsz;
			bufp += wrsz;
		}

		done = 1;
		for (CPU_INFO_FOREACH(n, cpi)) {
			if ((cpuset & (1 << n)) == 0)
				continue;

			if (cpi->msg.complete == 0) {
				if (i < 0) {
					wrsz = snprintf(bufp, bufsz,
							" cpu%d", cpi->ci_cpuid);
					if (wrsz > bufsz)
						break;
					bufsz -= wrsz;
					bufp += wrsz;
				} else {
					done = 0;
					break;
				}
			}
		}
	}

	if (i >= 0 || debug_xcall == 0) {
		if (i < 0)
			aprint_error("%s\n", errbuf);
		mutex_spin_exit(&xpmsg_mutex);
		return;
	}

	/*
	 * Let's make this a hard panic for now, and figure out why it
	 * happens.
	 *
	 * We call mp_pause_cpus() so we can capture their state *now*
	 * as opposed to after we've written all the below to the console.
	 */
#ifdef DDB
	mp_pause_cpus_ddb();
#else
	mp_pause_cpus();
#endif
	printf_nolog("%s\n", errbuf);
	mutex_spin_exit(&xpmsg_mutex);

	panic("failed to ping cpus");
}

/*
 * MD support for MI xcall(9) interface.
 */
void
xc_send_ipi(struct cpu_info *target)
{
	u_int cpuset;

	KASSERT(kpreempt_disabled());
	KASSERT(curcpu() != target);

	if (target)
		cpuset = 1 << target->ci_cpuid;
	else
		cpuset = CPUSET_ALL & ~(1 << cpuinfo.ci_cpuid);
	XCALL0(xc_ipi_handler, cpuset);
}

void
cpu_ipi(struct cpu_info *target)
{
	u_int cpuset;

	KASSERT(kpreempt_disabled());
	KASSERT(curcpu() != target);

	if (target)
		cpuset = 1 << target->ci_cpuid;
	else
		cpuset = CPUSET_ALL & ~(1 << cpuinfo.ci_cpuid);
	XCALL0(ipi_cpu_handler, cpuset);
}

/*
 * Tell all CPUs other than the current one to enter the PROM idle loop.
 */
void
mp_pause_cpus(void)
{
	CPU_INFO_ITERATOR n;
	struct cpu_info *cpi;

	for (CPU_INFO_FOREACH(n, cpi)) {
		if (cpuinfo.mid == cpi->mid ||
		    (cpi->flags & CPUFLG_HATCHED) == 0)
			continue;

		/*
		 * This PROM utility will put the OPENPROM_MBX_ABORT
		 * message (0xfc) in the target CPU's mailbox and then
		 * send it a level 15 soft interrupt.
		 */
		if (prom_cpuidle(cpi->node) != 0)
			printf("cpu%d could not be paused\n", cpi->ci_cpuid);
	}
}

/*
 * Resume all idling CPUs.
 */
void
mp_resume_cpus(void)
{
	CPU_INFO_ITERATOR n;
	struct cpu_info *cpi;

	for (CPU_INFO_FOREACH(n, cpi)) {
		if (cpuinfo.mid == cpi->mid ||
		    (cpi->flags & CPUFLG_HATCHED) == 0)
			continue;

		/*
		 * This PROM utility makes the target CPU return
		 * from its prom_cpuidle(0) call (see intr.c:nmi_soft()).
		 */
		if (prom_cpuresume(cpi->node) != 0)
			printf("cpu%d could not be resumed\n", cpi->ci_cpuid);
	}
}

/*
 * Tell all CPUs except the current one to hurry back into the prom
 */
void
mp_halt_cpus(void)
{
	CPU_INFO_ITERATOR n;
	struct cpu_info *cpi;

	for (CPU_INFO_FOREACH(n, cpi)) {
		int r;

		if (cpuinfo.mid == cpi->mid)
			continue;

		/*
		 * This PROM utility will put the OPENPROM_MBX_STOP
		 * message (0xfb) in the target CPU's mailbox and then
		 * send it a level 15 soft interrupt.
		 */
		r = prom_cpustop(cpi->node);
		printf("cpu%d %shalted\n", cpi->ci_cpuid,
			r == 0 ? "" : "(boot CPU?) can not be ");
	}
}

#if defined(DDB)
void
mp_pause_cpus_ddb(void)
{
	CPU_INFO_ITERATOR n;
	struct cpu_info *cpi;

	for (CPU_INFO_FOREACH(n, cpi)) {
		if (cpi == NULL || cpi->mid == cpuinfo.mid ||
		    (cpi->flags & CPUFLG_HATCHED) == 0)
			continue;

		cpi->msg_lev15.tag = XPMSG15_PAUSECPU;
		raise_ipi(cpi,15);	/* high priority intr */
	}
}

void
mp_resume_cpus_ddb(void)
{
	CPU_INFO_ITERATOR n;
	struct cpu_info *cpi;

	for (CPU_INFO_FOREACH(n, cpi)) {
		if (cpi == NULL || cpuinfo.mid == cpi->mid ||
		    (cpi->flags & CPUFLG_PAUSED) == 0)
			continue;

		/* tell it to continue */
		cpi->flags &= ~CPUFLG_PAUSED;
	}
}
#endif /* DDB */
#endif /* MULTIPROCESSOR */

/*
 * fpu_init() must be run on associated CPU.
 */
void
fpu_init(struct cpu_info *sc)
{
	struct fpstate fpstate;
	int fpuvers;

	/*
	 * Get the FSR and clear any exceptions.  If we do not unload
	 * the queue here and it is left over from a previous crash, we
	 * will panic in the first loadfpstate(), due to a sequence
	 * error, so we need to dump the whole state anyway.
	 *
	 * If there is no FPU, trap.c will advance over all the stores,
	 * so we initialize fs_fsr here.
	 */

	/* 7 is reserved for "none" */
	fpstate.fs_fsr = 7 << FSR_VER_SHIFT;
	savefpstate(&fpstate);
	sc->fpuvers = fpuvers =
		(fpstate.fs_fsr >> FSR_VER_SHIFT) & (FSR_VER >> FSR_VER_SHIFT);

	if (fpuvers == 7) {
		sc->fpu_name = "no";
		return;
	}

	sc->fpupresent = 1;
	sc->fpu_name = fsrtoname(sc->cpu_impl, sc->cpu_vers, fpuvers);
	if (sc->fpu_name == NULL) {
		snprintf(sc->fpu_namebuf, sizeof(sc->fpu_namebuf),
		    "version 0x%x", fpuvers);
		sc->fpu_name = sc->fpu_namebuf;
	}
}

void
cache_print(struct cpu_softc *sc)
{
	struct cacheinfo *ci = &sc->sc_cpuinfo->cacheinfo;

	if (sc->sc_cpuinfo->flags & CPUFLG_SUN4CACHEBUG)
		printf("%s: cache chip bug; trap page uncached\n",
		    device_xname(sc->sc_dev));

	printf("%s: ", device_xname(sc->sc_dev));

	if (ci->c_totalsize == 0) {
		printf("no cache\n");
		return;
	}

	if (ci->c_split) {
		const char *sep = "";

		printf("%s", (ci->c_physical ? "physical " : ""));
		if (ci->ic_totalsize > 0) {
			printf("%s%dK instruction (%d b/l)", sep,
			    ci->ic_totalsize/1024, ci->ic_linesize);
			sep = ", ";
		}
		if (ci->dc_totalsize > 0) {
			printf("%s%dK data (%d b/l)", sep,
			    ci->dc_totalsize/1024, ci->dc_linesize);
		}
	} else if (ci->c_physical) {
		/* combined, physical */
		printf("physical %dK combined cache (%d bytes/line)",
		    ci->c_totalsize/1024, ci->c_linesize);
	} else {
		/* combined, virtual */
		printf("%dK byte write-%s, %d bytes/line, %cw flush",
		    ci->c_totalsize/1024,
		    (ci->c_vactype == VAC_WRITETHROUGH) ? "through" : "back",
		    ci->c_linesize,
		    ci->c_hwflush ? 'h' : 's');
	}

	if (ci->ec_totalsize > 0) {
		printf(", %dK external (%d b/l)",
		    ci->ec_totalsize/1024, ci->ec_linesize);
	}
	printf(": ");
	if (ci->c_enabled)
		printf("cache enabled");
	printf("\n");
}


/*------------*/


void cpumatch_unknown(struct cpu_info *, struct module_info *, int);
void cpumatch_sun4(struct cpu_info *, struct module_info *, int);
void cpumatch_sun4c(struct cpu_info *, struct module_info *, int);
void cpumatch_ms1(struct cpu_info *, struct module_info *, int);
void cpumatch_viking(struct cpu_info *, struct module_info *, int);
void cpumatch_hypersparc(struct cpu_info *, struct module_info *, int);
void cpumatch_turbosparc(struct cpu_info *, struct module_info *, int);

void getcacheinfo_sun4(struct cpu_info *, int node);
void getcacheinfo_sun4c(struct cpu_info *, int node);
void getcacheinfo_obp(struct cpu_info *, int node);
void getcacheinfo_sun4d(struct cpu_info *, int node);

void sun4_hotfix(struct cpu_info *);
void viking_hotfix(struct cpu_info *);
void turbosparc_hotfix(struct cpu_info *);
void swift_hotfix(struct cpu_info *);

void ms1_mmu_enable(void);
void viking_mmu_enable(void);
void swift_mmu_enable(void);
void hypersparc_mmu_enable(void);

void srmmu_get_syncflt(void);
void ms1_get_syncflt(void);
void viking_get_syncflt(void);
void swift_get_syncflt(void);
void turbosparc_get_syncflt(void);
void hypersparc_get_syncflt(void);
void cypress_get_syncflt(void);

int srmmu_get_asyncflt(u_int *, u_int *);
int hypersparc_get_asyncflt(u_int *, u_int *);
int cypress_get_asyncflt(u_int *, u_int *);
int no_asyncflt_regs(u_int *, u_int *);

int hypersparc_getmid(void);
/* cypress and hypersparc can share this function, see ctlreg.h */
#define cypress_getmid	hypersparc_getmid
int viking_getmid(void);

#if (defined(SUN4M) && !defined(MSIIEP)) || defined(SUN4D)
extern int (*moduleerr_handler)(void);
int viking_module_error(void);
#endif

struct module_info module_unknown = {
	CPUTYP_UNKNOWN,
	VAC_UNKNOWN,
	cpumatch_unknown
};


void
cpumatch_unknown(struct cpu_info *sc, struct module_info *mp, int node)
{

	panic("Unknown CPU type: "
	      "cpu: impl %d, vers %d; mmu: impl %d, vers %d",
		sc->cpu_impl, sc->cpu_vers,
		sc->mmu_impl, sc->mmu_vers);
}

#if defined(SUN4)
struct module_info module_sun4 = {
	CPUTYP_UNKNOWN,
	VAC_WRITETHROUGH,
	cpumatch_sun4,
	getcacheinfo_sun4,
	sun4_hotfix,
	0,
	sun4_cache_enable,
	0,
	0,			/* ncontext set in `match' function */
	0,			/* get_syncflt(); unused in sun4c */
	0,			/* get_asyncflt(); unused in sun4c */
	sun4_cache_flush,
	sun4_vcache_flush_page, NULL,
	sun4_vcache_flush_segment, NULL,
	sun4_vcache_flush_region, NULL,
	sun4_vcache_flush_context, NULL,
	NULL, NULL,
	noop_pcache_flush_page,
	noop_pure_vcache_flush,
	noop_cache_flush_all,
	0,
	pmap_zero_page4_4c,
	pmap_copy_page4_4c
};

void
getcacheinfo_sun4(struct cpu_info *sc, int node)
{
	struct cacheinfo *ci = &sc->cacheinfo;

	switch (sc->cpu_type) {
	case CPUTYP_4_100:
		ci->c_vactype = VAC_NONE;
		ci->c_totalsize = 0;
		ci->c_hwflush = 0;
		ci->c_linesize = 0;
		ci->c_l2linesize = 0;
		ci->c_split = 0;
		ci->c_nlines = 0;

		/* Override cache flush functions */
		sc->cache_flush = noop_cache_flush;
		sc->sp_vcache_flush_page = noop_vcache_flush_page;
		sc->sp_vcache_flush_segment = noop_vcache_flush_segment;
		sc->sp_vcache_flush_region = noop_vcache_flush_region;
		sc->sp_vcache_flush_context = noop_vcache_flush_context;
		break;
	case CPUTYP_4_200:
		ci->c_vactype = VAC_WRITEBACK;
		ci->c_totalsize = 128*1024;
		ci->c_hwflush = 0;
		ci->c_linesize = 16;
		ci->c_l2linesize = 4;
		ci->c_split = 0;
		ci->c_nlines = ci->c_totalsize >> ci->c_l2linesize;
		break;
	case CPUTYP_4_300:
		ci->c_vactype = VAC_WRITEBACK;
		ci->c_totalsize = 128*1024;
		ci->c_hwflush = 0;
		ci->c_linesize = 16;
		ci->c_l2linesize = 4;
		ci->c_split = 0;
		ci->c_nlines = ci->c_totalsize >> ci->c_l2linesize;
		sc->flags |= CPUFLG_SUN4CACHEBUG;
		break;
	case CPUTYP_4_400:
		ci->c_vactype = VAC_WRITEBACK;
		ci->c_totalsize = 128 * 1024;
		ci->c_hwflush = 0;
		ci->c_linesize = 32;
		ci->c_l2linesize = 5;
		ci->c_split = 0;
		ci->c_nlines = ci->c_totalsize >> ci->c_l2linesize;
		break;
	}
}

void
cpumatch_sun4(struct cpu_info *sc, struct module_info *mp, int node)
{
	struct idprom *idp = prom_getidprom();

	switch (idp->idp_machtype) {
	case ID_SUN4_100:
		sc->cpu_type = CPUTYP_4_100;
		sc->classlvl = 100;
		sc->mmu_ncontext = 8;
		sc->mmu_nsegment = 256;
/*XXX*/		sc->hz = 14280000;
		break;
	case ID_SUN4_200:
		sc->cpu_type = CPUTYP_4_200;
		sc->classlvl = 200;
		sc->mmu_nsegment = 512;
		sc->mmu_ncontext = 16;
/*XXX*/		sc->hz = 16670000;
		break;
	case ID_SUN4_300:
		sc->cpu_type = CPUTYP_4_300;
		sc->classlvl = 300;
		sc->mmu_nsegment = 256;
		sc->mmu_ncontext = 16;
/*XXX*/		sc->hz = 25000000;
		break;
	case ID_SUN4_400:
		sc->cpu_type = CPUTYP_4_400;
		sc->classlvl = 400;
		sc->mmu_nsegment = 1024;
		sc->mmu_ncontext = 64;
		sc->mmu_nregion = 256;
/*XXX*/		sc->hz = 33000000;
		sc->sun4_mmu3l = 1;
		break;
	}

}
#endif /* SUN4 */

#if defined(SUN4C)
struct module_info module_sun4c = {
	CPUTYP_UNKNOWN,
	VAC_WRITETHROUGH,
	cpumatch_sun4c,
	getcacheinfo_sun4c,
	sun4_hotfix,
	0,
	sun4_cache_enable,
	0,
	0,			/* ncontext set in `match' function */
	0,			/* get_syncflt(); unused in sun4c */
	0,			/* get_asyncflt(); unused in sun4c */
	sun4_cache_flush,
	sun4_vcache_flush_page, NULL,
	sun4_vcache_flush_segment, NULL,
	sun4_vcache_flush_region, NULL,
	sun4_vcache_flush_context, NULL,
	NULL, NULL,
	noop_pcache_flush_page,
	noop_pure_vcache_flush,
	noop_cache_flush_all,
	0,
	pmap_zero_page4_4c,
	pmap_copy_page4_4c
};

void
cpumatch_sun4c(struct cpu_info *sc, struct module_info *mp, int node)
{
	int	rnode;

	rnode = findroot();
	sc->mmu_npmeg = sc->mmu_nsegment =
		prom_getpropint(rnode, "mmu-npmg", 128);
	sc->mmu_ncontext = prom_getpropint(rnode, "mmu-nctx", 8);

	/* Get clock frequency */
	sc->hz = prom_getpropint(rnode, "clock-frequency", 0);
}

void
getcacheinfo_sun4c(struct cpu_info *sc, int node)
{
	struct cacheinfo *ci = &sc->cacheinfo;
	int i, l;

	if (node == 0)
		/* Bootstrapping */
		return;

	/* Sun4c's have only virtually-addressed caches */
	ci->c_physical = 0;
	ci->c_totalsize = prom_getpropint(node, "vac-size", 65536);
	/*
	 * Note: vac-hwflush is spelled with an underscore
	 * on the 4/75s.
	 */
	ci->c_hwflush =
		prom_getpropint(node, "vac_hwflush", 0) |
		prom_getpropint(node, "vac-hwflush", 0);

	ci->c_linesize = l = prom_getpropint(node, "vac-linesize", 16);
	for (i = 0; (1 << i) < l; i++)
		/* void */;
	if ((1 << i) != l)
		panic("bad cache line size %d", l);
	ci->c_l2linesize = i;
	ci->c_associativity = 1;
	ci->c_nlines = ci->c_totalsize >> i;

	ci->c_vactype = VAC_WRITETHROUGH;

	/*
	 * Machines with "buserr-type" 1 have a bug in the cache
	 * chip that affects traps.  (I wish I knew more about this
	 * mysterious buserr-type variable....)
	 */
	if (prom_getpropint(node, "buserr-type", 0) == 1)
		sc->flags |= CPUFLG_SUN4CACHEBUG;
}
#endif /* SUN4C */

void
sun4_hotfix(struct cpu_info *sc)
{

	if ((sc->flags & CPUFLG_SUN4CACHEBUG) != 0)
		kvm_uncache((char *)trapbase, 1);

	/* Use the hardware-assisted page flush routine, if present */
	if (sc->cacheinfo.c_hwflush)
		sc->vcache_flush_page = sun4_vcache_flush_page_hw;
}

#if defined(SUN4M)
void
getcacheinfo_obp(struct cpu_info *sc, int node)
{
	struct cacheinfo *ci = &sc->cacheinfo;
	int i, l;

#if defined(MULTIPROCESSOR)
	/*
	 * We really really want the cache info early for MP systems,
	 * so figure out the boot node, if we can.
	 *
	 * XXX this loop stolen from mainbus_attach()
	 */
	if (node == 0 && CPU_ISSUN4M && bootmid != 0) {
		const char *cp;
		char namebuf[32];
		int mid, node2;

		for (node2 = firstchild(findroot());
		     node2;
		     node2 = nextsibling(node2)) {
			cp = prom_getpropstringA(node2, "device_type",
					    namebuf, sizeof namebuf);
			if (strcmp(cp, "cpu") != 0)
				continue;

			mid = prom_getpropint(node2, "mid", -1);
			if (mid == bootmid) {
				node = node2;
				break;
			}
		}
	}
#endif

	if (node == 0)
		/* Bootstrapping */
		return;

	/*
	 * Determine the Sun4m cache organization.
	 */
	ci->c_physical = node_has_property(node, "cache-physical?");

	if (prom_getpropint(node, "ncaches", 1) == 2)
		ci->c_split = 1;
	else
		ci->c_split = 0;

	/* hwflush is used only by sun4/4c code */
	ci->c_hwflush = 0;

	if (node_has_property(node, "icache-nlines") &&
	    node_has_property(node, "dcache-nlines") &&
	    ci->c_split) {
		/* Harvard architecture: get I and D cache sizes */
		ci->ic_nlines = prom_getpropint(node, "icache-nlines", 0);
		ci->ic_linesize = l =
			prom_getpropint(node, "icache-line-size", 0);
		for (i = 0; (1 << i) < l && l; i++)
			/* void */;
		if ((1 << i) != l && l)
			panic("bad icache line size %d", l);
		ci->ic_l2linesize = i;
		ci->ic_associativity =
			prom_getpropint(node, "icache-associativity", 1);
		ci->ic_totalsize = l * ci->ic_nlines * ci->ic_associativity;

		ci->dc_nlines = prom_getpropint(node, "dcache-nlines", 0);
		ci->dc_linesize = l =
			prom_getpropint(node, "dcache-line-size",0);
		for (i = 0; (1 << i) < l && l; i++)
			/* void */;
		if ((1 << i) != l && l)
			panic("bad dcache line size %d", l);
		ci->dc_l2linesize = i;
		ci->dc_associativity =
			prom_getpropint(node, "dcache-associativity", 1);
		ci->dc_totalsize = l * ci->dc_nlines * ci->dc_associativity;

		ci->c_l2linesize = min(ci->ic_l2linesize, ci->dc_l2linesize);
		ci->c_linesize = min(ci->ic_linesize, ci->dc_linesize);
		ci->c_totalsize = max(ci->ic_totalsize, ci->dc_totalsize);
		ci->c_nlines = ci->c_totalsize >> ci->c_l2linesize;
	} else {
		/* unified I/D cache */
		ci->c_nlines = prom_getpropint(node, "cache-nlines", 128);
		ci->c_linesize = l =
			prom_getpropint(node, "cache-line-size", 0);
		for (i = 0; (1 << i) < l && l; i++)
			/* void */;
		if ((1 << i) != l && l)
			panic("bad cache line size %d", l);
		ci->c_l2linesize = i;
		ci->c_associativity =
			prom_getpropint(node, "cache-associativity", 1);
		ci->dc_associativity = ci->ic_associativity =
			ci->c_associativity;
		ci->c_totalsize = l * ci->c_nlines * ci->c_associativity;
	}

	if (node_has_property(node, "ecache-nlines")) {
		/* we have a L2 "e"xternal cache */
		ci->ec_nlines = prom_getpropint(node, "ecache-nlines", 32768);
		ci->ec_linesize = l = prom_getpropint(node, "ecache-line-size", 0);
		for (i = 0; (1 << i) < l && l; i++)
			/* void */;
		if ((1 << i) != l && l)
			panic("bad ecache line size %d", l);
		ci->ec_l2linesize = i;
		ci->ec_associativity =
			prom_getpropint(node, "ecache-associativity", 1);
		ci->ec_totalsize = l * ci->ec_nlines * ci->ec_associativity;
	}
	if (ci->c_totalsize == 0)
		printf("warning: couldn't identify cache\n");
}

/*
 * We use the max. number of contexts on the micro and
 * hyper SPARCs. The SuperSPARC would let us use up to 65536
 * contexts (by powers of 2), but we keep it at 4096 since
 * the table must be aligned to #context*4. With 4K contexts,
 * we waste at most 16K of memory. Note that the context
 * table is *always* page-aligned, so there can always be
 * 1024 contexts without sacrificing memory space (given
 * that the chip supports 1024 contexts).
 *
 * Currently known limits: MS1=64, MS2=256, HS=4096, SS=65536
 * 	some old SS's=4096
 */

/* TI Microsparc I */
struct module_info module_ms1 = {
	CPUTYP_MS1,
	VAC_NONE,
	cpumatch_ms1,
	getcacheinfo_obp,
	0,
	ms1_mmu_enable,
	ms1_cache_enable,
	0,
	64,
	ms1_get_syncflt,
	no_asyncflt_regs,
	ms1_cache_flush,
	noop_vcache_flush_page, NULL,
	noop_vcache_flush_segment, NULL,
	noop_vcache_flush_region, NULL,
	noop_vcache_flush_context, NULL,
	noop_vcache_flush_range, NULL,
	noop_pcache_flush_page,
	noop_pure_vcache_flush,
	ms1_cache_flush_all,
	memerr4m,
	pmap_zero_page4m,
	pmap_copy_page4m
};

void
cpumatch_ms1(struct cpu_info *sc, struct module_info *mp, int node)
{

	/*
	 * Turn off page zeroing in the idle loop; an unidentified
	 * bug causes (very sporadic) user process corruption.
	 */
	vm_page_zero_enable = 0;
}

void
ms1_mmu_enable(void)
{
}

/* TI Microsparc II */
struct module_info module_ms2 = {		/* UNTESTED */
	CPUTYP_MS2,
	VAC_WRITETHROUGH,
	0,
	getcacheinfo_obp,
	0,
	0,
	swift_cache_enable,
	0,
	256,
	srmmu_get_syncflt,
	srmmu_get_asyncflt,
	srmmu_cache_flush,
	srmmu_vcache_flush_page, NULL,
	srmmu_vcache_flush_segment, NULL,
	srmmu_vcache_flush_region, NULL,
	srmmu_vcache_flush_context, NULL,
	srmmu_vcache_flush_range, NULL,
	noop_pcache_flush_page,
	noop_pure_vcache_flush,
	srmmu_cache_flush_all,
	memerr4m,
	pmap_zero_page4m,
	pmap_copy_page4m
};


struct module_info module_swift = {
	CPUTYP_MS2,
	VAC_WRITETHROUGH,
	0,
	getcacheinfo_obp,
	swift_hotfix,
	0,
	swift_cache_enable,
	0,
	256,
	swift_get_syncflt,
	no_asyncflt_regs,
	srmmu_cache_flush,
	srmmu_vcache_flush_page, NULL,
	srmmu_vcache_flush_segment, NULL,
	srmmu_vcache_flush_region, NULL,
	srmmu_vcache_flush_context, NULL,
	srmmu_vcache_flush_range, NULL,
	noop_pcache_flush_page,
	noop_pure_vcache_flush,
	srmmu_cache_flush_all,
	memerr4m,
	pmap_zero_page4m,
	pmap_copy_page4m
};

void
swift_hotfix(struct cpu_info *sc)
{
	int pcr = lda(SRMMU_PCR, ASI_SRMMU);

	/* Turn off branch prediction */
	pcr &= ~SWIFT_PCR_BF;
	sta(SRMMU_PCR, ASI_SRMMU, pcr);
}

void
swift_mmu_enable(void)
{
}


/* ROSS Hypersparc */
struct module_info module_hypersparc = {
	CPUTYP_UNKNOWN,
	VAC_WRITEBACK,
	cpumatch_hypersparc,
	getcacheinfo_obp,
	0,
	hypersparc_mmu_enable,
	hypersparc_cache_enable,
	hypersparc_getmid,
	4096,
	hypersparc_get_syncflt,
	hypersparc_get_asyncflt,
	srmmu_cache_flush,
	srmmu_vcache_flush_page, ft_srmmu_vcache_flush_page,
	srmmu_vcache_flush_segment, ft_srmmu_vcache_flush_segment,
	srmmu_vcache_flush_region, ft_srmmu_vcache_flush_region,
	srmmu_vcache_flush_context, ft_srmmu_vcache_flush_context,
	srmmu_vcache_flush_range, ft_srmmu_vcache_flush_range,
	noop_pcache_flush_page,
	hypersparc_pure_vcache_flush,
	hypersparc_cache_flush_all,
	hypersparc_memerr,
	pmap_zero_page4m,
	pmap_copy_page4m
};

void
cpumatch_hypersparc(struct cpu_info *sc, struct module_info *mp, int node)
{

	sc->cpu_type = CPUTYP_HS_MBUS;/*XXX*/

	if (node == 0) {
		/* Flush I-cache */
		sta(0, ASI_HICACHECLR, 0);

		/* Disable `unimplemented flush' traps during boot-up */
		wrasr(rdasr(HYPERSPARC_ASRNUM_ICCR) | HYPERSPARC_ICCR_FTD,
			HYPERSPARC_ASRNUM_ICCR);
	}
}

void
hypersparc_mmu_enable(void)
{
#if 0
	int pcr;

	pcr = lda(SRMMU_PCR, ASI_SRMMU);
	pcr |= HYPERSPARC_PCR_C;
	pcr &= ~HYPERSPARC_PCR_CE;

	sta(SRMMU_PCR, ASI_SRMMU, pcr);
#endif
}

int
hypersparc_getmid(void)
{
	u_int pcr = lda(SRMMU_PCR, ASI_SRMMU);
	return ((pcr & HYPERSPARC_PCR_MID) >> 15);
}


/* Cypress 605 */
struct module_info module_cypress = {
	CPUTYP_CYPRESS,
	VAC_WRITEBACK,
	0,
	getcacheinfo_obp,
	0,
	0,
	cypress_cache_enable,
	cypress_getmid,
	4096,
	cypress_get_syncflt,
	cypress_get_asyncflt,
	srmmu_cache_flush,
	srmmu_vcache_flush_page, ft_srmmu_vcache_flush_page,
	srmmu_vcache_flush_segment, ft_srmmu_vcache_flush_segment,
	srmmu_vcache_flush_region, ft_srmmu_vcache_flush_region,
	srmmu_vcache_flush_context, ft_srmmu_vcache_flush_context,
	srmmu_vcache_flush_range, ft_srmmu_vcache_flush_range,
	noop_pcache_flush_page,
	noop_pure_vcache_flush,
	cypress_cache_flush_all,
	memerr4m,
	pmap_zero_page4m,
	pmap_copy_page4m
};


/* Fujitsu Turbosparc */
struct module_info module_turbosparc = {
	CPUTYP_MS2,
	VAC_WRITEBACK,
	cpumatch_turbosparc,
	getcacheinfo_obp,
	turbosparc_hotfix,
	0,
	turbosparc_cache_enable,
	0,
	256,
	turbosparc_get_syncflt,
	no_asyncflt_regs,
	srmmu_cache_flush,
	srmmu_vcache_flush_page, NULL,
	srmmu_vcache_flush_segment, NULL,
	srmmu_vcache_flush_region, NULL,
	srmmu_vcache_flush_context, NULL,
	srmmu_vcache_flush_range, NULL,
	noop_pcache_flush_page,
	noop_pure_vcache_flush,
	srmmu_cache_flush_all,
	memerr4m,
	pmap_zero_page4m,
	pmap_copy_page4m
};

void
cpumatch_turbosparc(struct cpu_info *sc, struct module_info *mp, int node)
{
	int i;

	if (node == 0 || sc->master == 0)
		return;

	i = getpsr();
	if (sc->cpu_vers == IU_VERS(i))
		return;

	/*
	 * A cloaked Turbosparc: clear any items in cpuinfo that
	 * might have been set to uS2 versions during bootstrap.
	 */
	sc->cpu_longname = 0;
	sc->mmu_ncontext = 0;
	sc->cpu_type = 0;
	sc->cacheinfo.c_vactype = 0;
	sc->hotfix = 0;
	sc->mmu_enable = 0;
	sc->cache_enable = 0;
	sc->get_syncflt = 0;
	sc->cache_flush = 0;
	sc->sp_vcache_flush_page = 0;
	sc->sp_vcache_flush_segment = 0;
	sc->sp_vcache_flush_region = 0;
	sc->sp_vcache_flush_context = 0;
	sc->pcache_flush_page = 0;
}

void
turbosparc_hotfix(struct cpu_info *sc)
{
	int pcf;

	pcf = lda(SRMMU_PCFG, ASI_SRMMU);
	if (pcf & TURBOSPARC_PCFG_US2) {
		/* Turn off uS2 emulation bit */
		pcf &= ~TURBOSPARC_PCFG_US2;
		sta(SRMMU_PCFG, ASI_SRMMU, pcf);
	}
}
#endif /* SUN4M */

#if defined(SUN4M)
struct module_info module_viking = {
	CPUTYP_UNKNOWN,		/* set in cpumatch() */
	VAC_NONE,
	cpumatch_viking,
	getcacheinfo_obp,
	viking_hotfix,
	viking_mmu_enable,
	viking_cache_enable,
	viking_getmid,
	4096,
	viking_get_syncflt,
	no_asyncflt_regs,
	/* supersparcs use cached DVMA, no need to flush */
	noop_cache_flush,
	noop_vcache_flush_page, NULL,
	noop_vcache_flush_segment, NULL,
	noop_vcache_flush_region, NULL,
	noop_vcache_flush_context, NULL,
	noop_vcache_flush_range, NULL,
	viking_pcache_flush_page,
	noop_pure_vcache_flush,
	noop_cache_flush_all,
	viking_memerr,
	pmap_zero_page4m,
	pmap_copy_page4m
};
#endif /* SUN4M */

#if defined(SUN4M) || defined(SUN4D)
void
cpumatch_viking(struct cpu_info *sc, struct module_info *mp, int node)
{

	if (node == 0)
		viking_hotfix(sc);
}

void
viking_hotfix(struct cpu_info *sc)
{
static	int mxcc = -1;
	int pcr = lda(SRMMU_PCR, ASI_SRMMU);

	/* Test if we're directly on the MBus */
	if ((pcr & VIKING_PCR_MB) == 0) {
		sc->mxcc = 1;
		sc->flags |= CPUFLG_CACHE_MANDATORY;
		sc->zero_page = pmap_zero_page_viking_mxcc;
		sc->copy_page = pmap_copy_page_viking_mxcc;
#if !defined(MSIIEP)
		moduleerr_handler = viking_module_error;
#endif

		/*
		 * Ok to cache PTEs; set the flag here, so we don't
		 * uncache in pmap_bootstrap().
		 */
		if ((pcr & VIKING_PCR_TC) == 0)
			printf("[viking: PCR_TC is off]");
		else
			sc->flags |= CPUFLG_CACHEPAGETABLES;
	} else {
#ifdef MULTIPROCESSOR
		if (sparc_ncpus > 1 && sc->cacheinfo.ec_totalsize == 0)
			sc->cache_flush = srmmu_cache_flush;
#endif
	}
	/* Check all modules have the same MXCC configuration */
	if (mxcc != -1 && sc->mxcc != mxcc)
		panic("MXCC module mismatch");

	mxcc = sc->mxcc;

	/* XXX! */
	if (sc->mxcc)
		sc->cpu_type = CPUTYP_SS1_MBUS_MXCC;
	else
		sc->cpu_type = CPUTYP_SS1_MBUS_NOMXCC;
}

void
viking_mmu_enable(void)
{
	int pcr;

	pcr = lda(SRMMU_PCR, ASI_SRMMU);

	if (cpuinfo.mxcc) {
		if ((pcr & VIKING_PCR_TC) == 0) {
			printf("[viking: turn on PCR_TC]");
		}
		pcr |= VIKING_PCR_TC;
		cpuinfo.flags |= CPUFLG_CACHEPAGETABLES;
	} else
		pcr &= ~VIKING_PCR_TC;
	sta(SRMMU_PCR, ASI_SRMMU, pcr);
}

int
viking_getmid(void)
{

	if (cpuinfo.mxcc) {
		u_int v = ldda(MXCC_MBUSPORT, ASI_CONTROL) & 0xffffffff;
		return ((v >> 24) & 0xf);
	}
	return (0);
}

#if !defined(MSIIEP)
int
viking_module_error(void)
{
	uint64_t v;
	int fatal = 0;
	CPU_INFO_ITERATOR n;
	struct cpu_info *cpi;

	/* Report on MXCC error registers in each module */
	for (CPU_INFO_FOREACH(n, cpi)) {
		if (cpi->ci_mxccregs == 0) {
			printf("\tMXCC registers not mapped\n");
			continue;
		}

		printf("module%d:\n", cpi->ci_cpuid);
		v = *((uint64_t *)(cpi->ci_mxccregs + 0xe00));
		printf("\tmxcc error 0x%llx\n", v);
		v = *((uint64_t *)(cpi->ci_mxccregs + 0xb00));
		printf("\tmxcc status 0x%llx\n", v);
		v = *((uint64_t *)(cpi->ci_mxccregs + 0xc00));
		printf("\tmxcc reset 0x%llx", v);
		if (v & MXCC_MRST_WD)
			printf(" (WATCHDOG RESET)"), fatal = 1;
		if (v & MXCC_MRST_SI)
			printf(" (SOFTWARE RESET)"), fatal = 1;
		printf("\n");
	}
	return (fatal);
}
#endif /* MSIIEP */
#endif /* SUN4M || SUN4D */

#if defined(SUN4D)
void
getcacheinfo_sun4d(struct cpu_info *sc, int node)
{
	struct cacheinfo *ci = &sc->cacheinfo;
	int i, l;

	if (node == 0)
		/* Bootstrapping */
		return;

	/*
	 * The Sun4d always has TI TMS390Z55 Viking CPUs; we hard-code
	 * much of the cache information here.
	 */

	ci->c_physical = 1;
	ci->c_split = 1;

	/* hwflush is used only by sun4/4c code */
	ci->c_hwflush = 0;

	ci->ic_nlines = 0x00000040;
	ci->ic_linesize = 0x00000040;
	ci->ic_l2linesize = 6;
	ci->ic_associativity = 0x00000005;
	ci->ic_totalsize = ci->ic_linesize * ci->ic_nlines *
	    ci->ic_associativity;

	ci->dc_nlines = 0x00000080;
	ci->dc_linesize = 0x00000020;
	ci->dc_l2linesize = 5;
	ci->dc_associativity = 0x00000004;
	ci->dc_totalsize = ci->dc_linesize * ci->dc_nlines *
	    ci->dc_associativity;

	ci->c_l2linesize = min(ci->ic_l2linesize, ci->dc_l2linesize);
	ci->c_linesize = min(ci->ic_linesize, ci->dc_linesize);
	ci->c_totalsize = max(ci->ic_totalsize, ci->dc_totalsize);
	ci->c_nlines = ci->c_totalsize >> ci->c_l2linesize;

	if (node_has_property(node, "ecache-nlines")) {
		/* we have a L2 "e"xternal cache */
		ci->ec_nlines = prom_getpropint(node, "ecache-nlines", 32768);
		ci->ec_linesize = l = prom_getpropint(node, "ecache-line-size", 0);
		for (i = 0; (1 << i) < l && l; i++)
			/* void */;
		if ((1 << i) != l && l)
			panic("bad ecache line size %d", l);
		ci->ec_l2linesize = i;
		ci->ec_associativity =
			prom_getpropint(node, "ecache-associativity", 1);
		ci->ec_totalsize = l * ci->ec_nlines * ci->ec_associativity;
	}
}

struct module_info module_viking_sun4d = {
	CPUTYP_UNKNOWN,		/* set in cpumatch() */
	VAC_NONE,
	cpumatch_viking,
	getcacheinfo_sun4d,
	viking_hotfix,
	viking_mmu_enable,
	viking_cache_enable,
	viking_getmid,
	4096,
	viking_get_syncflt,
	no_asyncflt_regs,
	/* supersparcs use cached DVMA, no need to flush */
	noop_cache_flush,
	noop_vcache_flush_page, NULL,
	noop_vcache_flush_segment, NULL,
	noop_vcache_flush_region, NULL,
	noop_vcache_flush_context, NULL,
	noop_vcache_flush_range, NULL,
	viking_pcache_flush_page,
	noop_pure_vcache_flush,
	noop_cache_flush_all,
	viking_memerr,
	pmap_zero_page4m,
	pmap_copy_page4m
};
#endif /* SUN4D */

#define	ANY	-1	/* match any version */

struct cpu_conf {
	int	arch;
	int	cpu_impl;
	int	cpu_vers;
	int	mmu_impl;
	int	mmu_vers;
	const char	*name;
	struct	module_info *minfo;
} cpu_conf[] = {
#if defined(SUN4)
	{ CPU_SUN4, 0, 0, ANY, ANY, "MB86900/1A or L64801", &module_sun4 },
	{ CPU_SUN4, 1, 0, ANY, ANY, "L64811", &module_sun4 },
	{ CPU_SUN4, 1, 1, ANY, ANY, "CY7C601", &module_sun4 },
#endif

#if defined(SUN4C)
	{ CPU_SUN4C, 0, 0, ANY, ANY, "MB86900/1A or L64801", &module_sun4c },
	{ CPU_SUN4C, 1, 0, ANY, ANY, "L64811", &module_sun4c },
	{ CPU_SUN4C, 1, 1, ANY, ANY, "CY7C601", &module_sun4c },
	{ CPU_SUN4C, 9, 0, ANY, ANY, "W8601/8701 or MB86903", &module_sun4c },
#endif

#if defined(SUN4M)
	{ CPU_SUN4M, 0, 4, 0, 4, "MB86904", &module_swift },
	{ CPU_SUN4M, 0, 5, 0, 5, "MB86907", &module_turbosparc },
	{ CPU_SUN4M, 1, 1, 1, 0, "CY7C601/604", &module_cypress },
	{ CPU_SUN4M, 1, 1, 1, 0xb, "CY7C601/605 (v.b)", &module_cypress },
	{ CPU_SUN4M, 1, 1, 1, 0xc, "CY7C601/605 (v.c)", &module_cypress },
	{ CPU_SUN4M, 1, 1, 1, 0xf, "CY7C601/605 (v.f)", &module_cypress },
	{ CPU_SUN4M, 1, 3, 1, ANY, "CY7C611", &module_cypress },
	{ CPU_SUN4M, 1, 0xe, 1, 7, "RT620/625", &module_hypersparc },
	{ CPU_SUN4M, 1, 0xf, 1, 7, "RT620/625", &module_hypersparc },
	{ CPU_SUN4M, 4, 0, 0, 1, "SuperSPARC v3", &module_viking },
	{ CPU_SUN4M, 4, 0, 0, 2, "SuperSPARC v4", &module_viking },
	{ CPU_SUN4M, 4, 0, 0, 3, "SuperSPARC v5", &module_viking },
	{ CPU_SUN4M, 4, 0, 0, 8, "SuperSPARC II v1", &module_viking },
	{ CPU_SUN4M, 4, 0, 0, 10, "SuperSPARC II v2", &module_viking },
	{ CPU_SUN4M, 4, 0, 0, 12, "SuperSPARC II v3", &module_viking },
	{ CPU_SUN4M, 4, 0, 0, ANY, "TMS390Z50 v0 or TMS390Z55", &module_viking },
	{ CPU_SUN4M, 4, 1, 0, ANY, "TMS390Z50 v1", &module_viking },
	{ CPU_SUN4M, 4, 1, 4, ANY, "TMS390S10", &module_ms1 },
	{ CPU_SUN4M, 4, 2, 0, ANY, "TI_MS2", &module_ms2 },
	{ CPU_SUN4M, 4, 3, ANY, ANY, "TI_4_3", &module_viking },
	{ CPU_SUN4M, 4, 4, ANY, ANY, "TI_4_4", &module_viking },
#endif

#if defined(SUN4D)
	{ CPU_SUN4D, 4, 0, 0, ANY, "TMS390Z50 v0 or TMS390Z55",
	  &module_viking_sun4d },
#endif

	{ ANY, ANY, ANY, ANY, ANY, "Unknown", &module_unknown }
};

void
getcpuinfo(struct cpu_info *sc, int node)
{
	struct cpu_conf *mp;
	int i;
	int cpu_impl, cpu_vers;
	int mmu_impl, mmu_vers;

	/*
	 * Set up main criteria for selection from the CPU configuration
	 * table: the CPU implementation/version fields from the PSR
	 * register, and -- on sun4m machines -- the MMU
	 * implementation/version from the SCR register.
	 */
	if (sc->master) {
		i = getpsr();
		if (node == 0 ||
		    (cpu_impl =
		     prom_getpropint(node, "psr-implementation", -1)) == -1)
			cpu_impl = IU_IMPL(i);

		if (node == 0 ||
		    (cpu_vers = prom_getpropint(node, "psr-version", -1)) == -1)
			cpu_vers = IU_VERS(i);

		if (CPU_HAS_SRMMU) {
			i = lda(SRMMU_PCR, ASI_SRMMU);
			if (node == 0 ||
			    (mmu_impl =
			     prom_getpropint(node, "implementation", -1)) == -1)
				mmu_impl = SRMMU_IMPL(i);

			if (node == 0 ||
			    (mmu_vers = prom_getpropint(node, "version", -1)) == -1)
				mmu_vers = SRMMU_VERS(i);
		} else {
			mmu_impl = ANY;
			mmu_vers = ANY;
		}
	} else {
		/*
		 * Get CPU version/implementation from ROM. If not
		 * available, assume same as boot CPU.
		 */
		cpu_impl = prom_getpropint(node, "psr-implementation",
					   cpuinfo.cpu_impl);
		cpu_vers = prom_getpropint(node, "psr-version",
					   cpuinfo.cpu_vers);

		/* Get MMU version/implementation from ROM always */
		mmu_impl = prom_getpropint(node, "implementation", -1);
		mmu_vers = prom_getpropint(node, "version", -1);
	}

	for (mp = cpu_conf; ; mp++) {
		if (mp->arch != cputyp && mp->arch != ANY)
			continue;

#define MATCH(x)	(mp->x == x || mp->x == ANY)
		if (!MATCH(cpu_impl) ||
		    !MATCH(cpu_vers) ||
		    !MATCH(mmu_impl) ||
		    !MATCH(mmu_vers))
			continue;
#undef MATCH

		/*
		 * Got CPU type.
		 */
		sc->cpu_impl = cpu_impl;
		sc->cpu_vers = cpu_vers;
		sc->mmu_impl = mmu_impl;
		sc->mmu_vers = mmu_vers;

		if (mp->minfo->cpu_match) {
			/* Additional fixups */
			mp->minfo->cpu_match(sc, mp->minfo, node);
		}
		if (sc->cpu_longname == 0)
			sc->cpu_longname = mp->name;

		if (sc->mmu_ncontext == 0)
			sc->mmu_ncontext = mp->minfo->ncontext;

		if (sc->cpu_type == 0)
			sc->cpu_type = mp->minfo->cpu_type;

		if (sc->cacheinfo.c_vactype == VAC_UNKNOWN)
			sc->cacheinfo.c_vactype = mp->minfo->vactype;

		if (sc->master && mp->minfo->getmid != NULL)
			bootmid = mp->minfo->getmid();

		mp->minfo->getcacheinfo(sc, node);

		if (node && sc->hz == 0 && !CPU_ISSUN4/*XXX*/) {
			sc->hz = prom_getpropint(node, "clock-frequency", 0);
			if (sc->hz == 0) {
				/*
				 * Try to find it in the OpenPROM root...
				 */
				sc->hz = prom_getpropint(findroot(),
						    "clock-frequency", 0);
			}
		}

		/*
		 * Copy CPU/MMU/Cache specific routines into cpu_info.
		 */
#define MPCOPY(x)	if (sc->x == 0) sc->x = mp->minfo->x;
		MPCOPY(hotfix);
		MPCOPY(mmu_enable);
		MPCOPY(cache_enable);
		MPCOPY(get_syncflt);
		MPCOPY(get_asyncflt);
		MPCOPY(cache_flush);
		MPCOPY(sp_vcache_flush_page);
		MPCOPY(sp_vcache_flush_segment);
		MPCOPY(sp_vcache_flush_region);
		MPCOPY(sp_vcache_flush_context);
		MPCOPY(sp_vcache_flush_range);
		MPCOPY(ft_vcache_flush_page);
		MPCOPY(ft_vcache_flush_segment);
		MPCOPY(ft_vcache_flush_region);
		MPCOPY(ft_vcache_flush_context);
		MPCOPY(ft_vcache_flush_range);
		MPCOPY(pcache_flush_page);
		MPCOPY(pure_vcache_flush);
		MPCOPY(cache_flush_all);
		MPCOPY(memerr);
		MPCOPY(zero_page);
		MPCOPY(copy_page);
#undef MPCOPY
		/*
		 * Use the single-processor cache flush functions until
		 * all CPUs are initialized.
		 */
		sc->vcache_flush_page = sc->sp_vcache_flush_page;
		sc->vcache_flush_segment = sc->sp_vcache_flush_segment;
		sc->vcache_flush_region = sc->sp_vcache_flush_region;
		sc->vcache_flush_context = sc->sp_vcache_flush_context;
		(*sc->cache_flush_all)();
		return;
	}
	panic("Out of CPUs");
}

/*
 * The following tables convert <IU impl, IU version, FPU version> triples
 * into names for the CPU and FPU chip.  In most cases we do not need to
 * inspect the FPU version to name the IU chip, but there is one exception
 * (for Tsunami), and this makes the tables the same.
 *
 * The table contents (and much of the structure here) are from Guy Harris.
 *
 */
struct info {
	int	valid;
	int	iu_impl;
	int	iu_vers;
	int	fpu_vers;
	const char	*name;
};

/* XXX trim this table on a per-ARCH basis */
/* NB: table order matters here; specific numbers must appear before ANY. */
static struct info fpu_types[] = {
	/*
	 * Vendor 0, IU Fujitsu0.
	 */
	{ 1, 0x0, ANY, 0, "MB86910 or WTL1164/5" },
	{ 1, 0x0, ANY, 1, "MB86911 or WTL1164/5" },
	{ 1, 0x0, ANY, 2, "L64802 or ACT8847" },
	{ 1, 0x0, ANY, 3, "WTL3170/2" },
	{ 1, 0x0, 4,   4, "on-chip" },		/* Swift */
	{ 1, 0x0, 5,   5, "on-chip" },		/* TurboSparc */
	{ 1, 0x0, ANY, 4, "L64804" },

	/*
	 * Vendor 1, IU ROSS0/1 or Pinnacle.
	 */
	{ 1, 0x1, 0xf, 0, "on-chip" },		/* Pinnacle */
	{ 1, 0x1, 0xe, 0, "on-chip" },		/* Hypersparc RT 625/626 */
	{ 1, 0x1, ANY, 0, "L64812 or ACT8847" },
	{ 1, 0x1, ANY, 1, "L64814" },
	{ 1, 0x1, ANY, 2, "TMS390C602A" },
	{ 1, 0x1, ANY, 3, "RT602 or WTL3171" },

	/*
	 * Vendor 2, IU BIT0.
	 */
	{ 1, 0x2, ANY, 0, "B5010 or B5110/20 or B5210" },

	/*
	 * Vendor 4, Texas Instruments.
	 */
	{ 1, 0x4, ANY, 0, "on-chip" },		/* Viking */
	{ 1, 0x4, ANY, 4, "on-chip" },		/* Tsunami */

	/*
	 * Vendor 5, IU Matsushita0.
	 */
	{ 1, 0x5, ANY, 0, "on-chip" },

	/*
	 * Vendor 9, Weitek.
	 */
	{ 1, 0x9, ANY, 3, "on-chip" },

	{ 0 }
};

static const char *
fsrtoname(int impl, int vers, int fver)
{
	struct info *p;

	for (p = fpu_types; p->valid; p++) {
		if (p->iu_impl == impl &&
		    (p->iu_vers == vers || p->iu_vers == ANY) &&
		    (p->fpu_vers == fver))
			return (p->name);
	}
	return (NULL);
}

#ifdef DDB

#include <ddb/db_output.h>
#include <machine/db_machdep.h>

#include "ioconf.h"

/*
 * Dump CPU information from ddb.
 */
void
cpu_debug_dump(void)
{
	struct cpu_info *ci;
	CPU_INFO_ITERATOR cii;

	db_printf("%-4s %-10s %-8s %-10s %-10s %-10s %-10s\n",
	    "CPU#", "CPUINFO", "FLAGS", "CURLWP", "CURPROC", "FPLWP", "CPCB");
	for (CPU_INFO_FOREACH(cii, ci)) {
		db_printf("%-4d %-10p %-8x %-10p %-10p %-10p %-10p\n",
		    ci->ci_cpuid,
		    ci,
		    ci->flags,
		    ci->ci_curlwp,
		    ci->ci_curlwp == NULL ? NULL : ci->ci_curlwp->l_proc,
		    ci->fplwp,
		    ci->curpcb);
	}
}

#if defined(MULTIPROCESSOR)
/*
 * Dump CPU xcall from ddb.
 */
void
cpu_xcall_dump(void)
{
	struct cpu_info *ci;
	CPU_INFO_ITERATOR cii;

	db_printf("%-4s %-10s %-10s %-10s %-10s %-10s "
		    "%-4s %-4s %-4s\n",
	          "CPU#", "FUNC", "TRAP", "ARG0", "ARG1", "ARG2",
	            "TAG", "RECV", "COMPL");
	for (CPU_INFO_FOREACH(cii, ci)) {
		db_printf("%-4d %-10p %-10p 0x%-8x 0x%-8x 0x%-8x "
			    "%-4d %-4d %-4d\n",
		    ci->ci_cpuid,
		    ci->msg.u.xpmsg_func.func,
		    ci->msg.u.xpmsg_func.trap,
		    ci->msg.u.xpmsg_func.arg0,
		    ci->msg.u.xpmsg_func.arg1,
		    ci->msg.u.xpmsg_func.arg2,
		    ci->msg.tag,
		    ci->msg.received,
		    ci->msg.complete);
	}
}
#endif

#endif
