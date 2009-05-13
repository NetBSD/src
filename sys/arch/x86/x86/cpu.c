/*	$NetBSD: cpu.c,v 1.62.2.1 2009/05/13 17:18:45 jym Exp $	*/

/*-
 * Copyright (c) 2000, 2006, 2007, 2008 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Bill Sommerfeld of RedBack Networks Inc, and by Andrew Doran.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Copyright (c) 1999 Stefan Grefen
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
 *      This product includes software developed by the NetBSD
 *      Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY AUTHOR AND CONTRIBUTORS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR AND CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: cpu.c,v 1.62.2.1 2009/05/13 17:18:45 jym Exp $");

#include "opt_ddb.h"
#include "opt_mpbios.h"		/* for MPDEBUG */
#include "opt_mtrr.h"

#include "lapic.h"
#include "ioapic.h"

#ifdef i386
#include "npx.h"
#endif

#include <sys/param.h>
#include <sys/proc.h>
#include <sys/user.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/kmem.h>
#include <sys/cpu.h>
#include <sys/atomic.h>
#include <sys/reboot.h>

#include <uvm/uvm_extern.h>

#include <machine/cpufunc.h>
#include <machine/cpuvar.h>
#include <machine/pmap.h>
#include <machine/vmparam.h>
#include <machine/mpbiosvar.h>
#include <machine/pcb.h>
#include <machine/specialreg.h>
#include <machine/segments.h>
#include <machine/gdt.h>
#include <machine/mtrr.h>
#include <machine/pio.h>
#include <machine/cpu_counter.h>

#ifdef i386
#include <machine/tlog.h>
#endif

#include <machine/apicvar.h>
#include <machine/i82489reg.h>
#include <machine/i82489var.h>

#include <dev/ic/mc146818reg.h>
#include <i386/isa/nvram.h>
#include <dev/isa/isareg.h>

#include "tsc.h"

#if MAXCPUS > 32
#error cpu_info contains 32bit bitmasks
#endif

int     cpu_match(device_t, cfdata_t, void *);
void    cpu_attach(device_t, device_t, void *);

static bool	cpu_suspend(device_t PMF_FN_PROTO);
static bool	cpu_resume(device_t PMF_FN_PROTO);

struct cpu_softc {
	device_t sc_dev;		/* device tree glue */
	struct cpu_info *sc_info;	/* pointer to CPU info */
	bool sc_wasonline;
};

int mp_cpu_start(struct cpu_info *, paddr_t); 
void mp_cpu_start_cleanup(struct cpu_info *);
const struct cpu_functions mp_cpu_funcs = { mp_cpu_start, NULL,
					    mp_cpu_start_cleanup };


CFATTACH_DECL_NEW(cpu, sizeof(struct cpu_softc),
    cpu_match, cpu_attach, NULL, NULL);

/*
 * Statically-allocated CPU info for the primary CPU (or the only
 * CPU, on uniprocessors).  The CPU info list is initialized to
 * point at it.
 */
#ifdef TRAPLOG
struct tlog tlog_primary;
#endif
struct cpu_info cpu_info_primary __aligned(CACHE_LINE_SIZE) = {
	.ci_dev = 0,
	.ci_self = &cpu_info_primary,
	.ci_idepth = -1,
	.ci_curlwp = &lwp0,
	.ci_curldt = -1,
#ifdef TRAPLOG
	.ci_tlog_base = &tlog_primary,
#endif /* !TRAPLOG */
};

struct cpu_info *cpu_info_list = &cpu_info_primary;

static void	cpu_set_tss_gates(struct cpu_info *);

#ifdef i386
static void	tss_init(struct i386tss *, void *, void *);
#endif

static void	cpu_init_idle_lwp(struct cpu_info *);

uint32_t cpus_attached = 0;
uint32_t cpus_running = 0;

extern char x86_64_doubleflt_stack[];

bool x86_mp_online;
paddr_t mp_trampoline_paddr = MP_TRAMPOLINE;
static vaddr_t cmos_data_mapping;
struct cpu_info *cpu_starting;

void    	cpu_hatch(void *);
static void    	cpu_boot_secondary(struct cpu_info *ci);
static void    	cpu_start_secondary(struct cpu_info *ci);
static void	cpu_copy_trampoline(void);

/*
 * Runs once per boot once multiprocessor goo has been detected and
 * the local APIC on the boot processor has been mapped.
 *
 * Called from lapic_boot_init() (from mpbios_scan()).
 */
void
cpu_init_first(void)
{

	cpu_info_primary.ci_cpuid = lapic_cpu_number();
	cpu_copy_trampoline();

	cmos_data_mapping = uvm_km_alloc(kernel_map, PAGE_SIZE, 0, UVM_KMF_VAONLY);
	if (cmos_data_mapping == 0)
		panic("No KVA for page 0");
	pmap_kenter_pa(cmos_data_mapping, 0, VM_PROT_READ|VM_PROT_WRITE);
	pmap_update(pmap_kernel());
}

int
cpu_match(device_t parent, cfdata_t match, void *aux)
{

	return 1;
}

static void
cpu_vm_init(struct cpu_info *ci)
{
	int ncolors = 2, i;

	for (i = CAI_ICACHE; i <= CAI_L2CACHE; i++) {
		struct x86_cache_info *cai;
		int tcolors;

		cai = &ci->ci_cinfo[i];

		tcolors = atop(cai->cai_totalsize);
		switch(cai->cai_associativity) {
		case 0xff:
			tcolors = 1; /* fully associative */
			break;
		case 0:
		case 1:
			break;
		default:
			tcolors /= cai->cai_associativity;
		}
		ncolors = max(ncolors, tcolors);
		/*
		 * If the desired number of colors is not a power of
		 * two, it won't be good.  Find the greatest power of
		 * two which is an even divisor of the number of colors,
		 * to preserve even coloring of pages.
		 */
		if (ncolors & (ncolors - 1) ) {
			int try, picked = 1;
			for (try = 1; try < ncolors; try *= 2) {
				if (ncolors % try == 0) picked = try;
			}
			if (picked == 1) {
				panic("desired number of cache colors %d is "
			      	" > 1, but not even!", ncolors);
			}
			ncolors = picked;
		}
	}

	/*
	 * Knowing the size of the largest cache on this CPU, re-color
	 * our pages.
	 */
	if (ncolors <= uvmexp.ncolors)
		return;
	aprint_debug_dev(ci->ci_dev, "%d page colors\n", ncolors);
	uvm_page_recolor(ncolors);
}


void
cpu_attach(device_t parent, device_t self, void *aux)
{
	struct cpu_softc *sc = device_private(self);
	struct cpu_attach_args *caa = aux;
	struct cpu_info *ci;
	uintptr_t ptr;
	int cpunum = caa->cpu_number;
	static bool again;

	sc->sc_dev = self;

	if (cpus_attached == ~0) {
		aprint_error(": increase MAXCPUS\n");
		return;
	}

	/*
	 * If we're an Application Processor, allocate a cpu_info
	 * structure, otherwise use the primary's.
	 */
	if (caa->cpu_role == CPU_ROLE_AP) {
		if ((boothowto & RB_MD1) != 0) {
			aprint_error(": multiprocessor boot disabled\n");
			if (!pmf_device_register(self, NULL, NULL))
				aprint_error_dev(self,
				    "couldn't establish power handler\n");
			return;
		}
		aprint_naive(": Application Processor\n");
		ptr = (uintptr_t)kmem_alloc(sizeof(*ci) + CACHE_LINE_SIZE - 1,
		    KM_SLEEP);
		ci = (struct cpu_info *)((ptr + CACHE_LINE_SIZE - 1) &
		    ~(CACHE_LINE_SIZE - 1));
		memset(ci, 0, sizeof(*ci));
		ci->ci_curldt = -1;
#ifdef TRAPLOG
		ci->ci_tlog_base = kmem_zalloc(sizeof(struct tlog), KM_SLEEP);
#endif
	} else {
		aprint_naive(": %s Processor\n",
		    caa->cpu_role == CPU_ROLE_SP ? "Single" : "Boot");
		ci = &cpu_info_primary;
		if (cpunum != lapic_cpu_number()) {
			/* XXX should be done earlier. */
			uint32_t reg;
			aprint_verbose("\n");
			aprint_verbose_dev(self, "running CPU at apic %d"
			    " instead of at expected %d", lapic_cpu_number(),
			    cpunum);
			reg = i82489_readreg(LAPIC_ID);
			i82489_writereg(LAPIC_ID, (reg & ~LAPIC_ID_MASK) |
			    (cpunum << LAPIC_ID_SHIFT));
		}
		if (cpunum != lapic_cpu_number()) {
			aprint_error_dev(self, "unable to reset apic id\n");
		}
	}

	ci->ci_self = ci;
	sc->sc_info = ci;
	ci->ci_dev = self;
	ci->ci_cpuid = caa->cpu_number;
	ci->ci_func = caa->cpu_func;

	/* Must be before mi_cpu_attach(). */
	cpu_vm_init(ci);

	if (caa->cpu_role == CPU_ROLE_AP) {
		int error;

		error = mi_cpu_attach(ci);
		if (error != 0) {
			aprint_normal("\n");
			aprint_error_dev(self,
			    "mi_cpu_attach failed with %d\n", error);
			return;
		}
		cpu_init_tss(ci);
	} else {
		KASSERT(ci->ci_data.cpu_idlelwp != NULL);
	}

	ci->ci_cpumask = (1 << cpu_index(ci));
	pmap_reference(pmap_kernel());
	ci->ci_pmap = pmap_kernel();
	ci->ci_tlbstate = TLBSTATE_STALE;

	/*
	 * Boot processor may not be attached first, but the below
	 * must be done to allow booting other processors.
	 */
	if (!again) {
		atomic_or_32(&ci->ci_flags, CPUF_PRESENT | CPUF_PRIMARY);
		/* Basic init. */
		cpu_intr_init(ci);
		cpu_get_tsc_freq(ci);
		cpu_init(ci);
		cpu_set_tss_gates(ci);
		pmap_cpu_init_late(ci);
		if (caa->cpu_role != CPU_ROLE_SP) {
			/* Enable lapic. */
			lapic_enable();
			lapic_set_lvt();
			lapic_calibrate_timer(ci);
		}
		/* Make sure DELAY() is initialized. */
		DELAY(1);
		again = true;
	}

	/* further PCB init done later. */

	switch (caa->cpu_role) {
	case CPU_ROLE_SP:
		atomic_or_32(&ci->ci_flags, CPUF_SP);
		cpu_identify(ci);
		x86_errata();
		x86_cpu_idle_init();
		break;

	case CPU_ROLE_BP:
		atomic_or_32(&ci->ci_flags, CPUF_BSP);
		cpu_identify(ci);
		x86_errata();
		x86_cpu_idle_init();
		break;

	case CPU_ROLE_AP:
		/*
		 * report on an AP
		 */
		cpu_intr_init(ci);
		gdt_alloc_cpu(ci);
		cpu_set_tss_gates(ci);
		pmap_cpu_init_early(ci);
		pmap_cpu_init_late(ci);
		cpu_start_secondary(ci);
		if (ci->ci_flags & CPUF_PRESENT) {
			struct cpu_info *tmp;

			cpu_identify(ci);
			tmp = cpu_info_list;
			while (tmp->ci_next)
				tmp = tmp->ci_next;

			tmp->ci_next = ci;
		}
		break;

	default:
		aprint_normal("\n");
		panic("unknown processor type??\n");
	}

	atomic_or_32(&cpus_attached, ci->ci_cpumask);

	if (!pmf_device_register(self, cpu_suspend, cpu_resume))
		aprint_error_dev(self, "couldn't establish power handler\n");

	if (mp_verbose) {
		struct lwp *l = ci->ci_data.cpu_idlelwp;

		aprint_verbose_dev(self,
		    "idle lwp at %p, idle sp at %p\n",
		    l,
#ifdef i386
		    (void *)l->l_addr->u_pcb.pcb_esp
#else
		    (void *)l->l_addr->u_pcb.pcb_rsp
#endif
		);
	}
}

/*
 * Initialize the processor appropriately.
 */

void
cpu_init(struct cpu_info *ci)
{

	lcr0(rcr0() | CR0_WP);

	/*
	 * On a P6 or above, enable global TLB caching if the
	 * hardware supports it.
	 */
	if (cpu_feature & CPUID_PGE)
		lcr4(rcr4() | CR4_PGE);	/* enable global TLB caching */

	/*
	 * If we have FXSAVE/FXRESTOR, use them.
	 */
	if (cpu_feature & CPUID_FXSR) {
		lcr4(rcr4() | CR4_OSFXSR);

		/*
		 * If we have SSE/SSE2, enable XMM exceptions.
		 */
		if (cpu_feature & (CPUID_SSE|CPUID_SSE2))
			lcr4(rcr4() | CR4_OSXMMEXCPT);
	}

#ifdef MTRR
	/*
	 * On a P6 or above, initialize MTRR's if the hardware supports them.
	 */
	if (cpu_feature & CPUID_MTRR) {
		if ((ci->ci_flags & CPUF_AP) == 0)
			i686_mtrr_init_first();
		mtrr_init_cpu(ci);
	}

#ifdef i386
	if (strcmp((char *)(ci->ci_vendor), "AuthenticAMD") == 0) {
		/*
		 * Must be a K6-2 Step >= 7 or a K6-III.
		 */
		if (CPUID2FAMILY(ci->ci_signature) == 5) {
			if (CPUID2MODEL(ci->ci_signature) > 8 ||
			    (CPUID2MODEL(ci->ci_signature) == 8 &&
			     CPUID2STEPPING(ci->ci_signature) >= 7)) {
				mtrr_funcs = &k6_mtrr_funcs;
				k6_mtrr_init_first();
				mtrr_init_cpu(ci);
			}
		}
	}
#endif	/* i386 */
#endif /* MTRR */

	atomic_or_32(&cpus_running, ci->ci_cpumask);

	if (ci != &cpu_info_primary) {
		/* Synchronize TSC again, and check for drift. */
		wbinvd();
		atomic_or_32(&ci->ci_flags, CPUF_RUNNING);
		tsc_sync_ap(ci);
	} else {
		atomic_or_32(&ci->ci_flags, CPUF_RUNNING);
	}
}

void
cpu_boot_secondary_processors(void)
{
	struct cpu_info *ci;
	u_long i;

	/* Now that we know the number of CPUs, patch the text segment. */
	x86_patch(false);

	for (i=0; i < maxcpus; i++) {
		ci = cpu_lookup(i);
		if (ci == NULL)
			continue;
		if (ci->ci_data.cpu_idlelwp == NULL)
			continue;
		if ((ci->ci_flags & CPUF_PRESENT) == 0)
			continue;
		if (ci->ci_flags & (CPUF_BSP|CPUF_SP|CPUF_PRIMARY))
			continue;
		cpu_boot_secondary(ci);
	}

	x86_mp_online = true;

	/* Now that we know about the TSC, attach the timecounter. */
	tsc_tc_init();

	/* Enable zeroing of pages in the idle loop if we have SSE2. */
	vm_page_zero_enable = ((cpu_feature & CPUID_SSE2) != 0);
}

static void
cpu_init_idle_lwp(struct cpu_info *ci)
{
	struct lwp *l = ci->ci_data.cpu_idlelwp;
	struct pcb *pcb = &l->l_addr->u_pcb;

	pcb->pcb_cr0 = rcr0();
}

void
cpu_init_idle_lwps(void)
{
	struct cpu_info *ci;
	u_long i;

	for (i = 0; i < maxcpus; i++) {
		ci = cpu_lookup(i);
		if (ci == NULL)
			continue;
		if (ci->ci_data.cpu_idlelwp == NULL)
			continue;
		if ((ci->ci_flags & CPUF_PRESENT) == 0)
			continue;
		cpu_init_idle_lwp(ci);
	}
}

void
cpu_start_secondary(struct cpu_info *ci)
{
	extern paddr_t mp_pdirpa;
	u_long psl;
	int i;

	mp_pdirpa = pmap_init_tmp_pgtbl(mp_trampoline_paddr);
	atomic_or_32(&ci->ci_flags, CPUF_AP);
	ci->ci_curlwp = ci->ci_data.cpu_idlelwp;
	if (CPU_STARTUP(ci, mp_trampoline_paddr) != 0) {
		return;
	}

	/*
	 * Wait for it to become ready.   Setting cpu_starting opens the
	 * initial gate and allows the AP to start soft initialization.
	 */
	KASSERT(cpu_starting == NULL);
	cpu_starting = ci;
	for (i = 100000; (!(ci->ci_flags & CPUF_PRESENT)) && i > 0; i--) {
#ifdef MPDEBUG
		extern int cpu_trace[3];
		static int otrace[3];
		if (memcmp(otrace, cpu_trace, sizeof(otrace)) != 0) {
			aprint_debug_dev(ci->ci_dev, "trace %02x %02x %02x\n",
			    cpu_trace[0], cpu_trace[1], cpu_trace[2]);
			memcpy(otrace, cpu_trace, sizeof(otrace));
		}
#endif
		i8254_delay(10);
	}

	if ((ci->ci_flags & CPUF_PRESENT) == 0) {
		aprint_error_dev(ci->ci_dev, "failed to become ready\n");
#if defined(MPDEBUG) && defined(DDB)
		printf("dropping into debugger; continue from here to resume boot\n");
		Debugger();
#endif
	} else {
		/*
		 * Synchronize time stamp counters.  Invalidate cache and do twice
		 * to try and minimize possible cache effects.  Disable interrupts
		 * to try and rule out any external interference.
		 */
		psl = x86_read_psl();
		x86_disable_intr();
		wbinvd();
		tsc_sync_bp(ci);
		x86_write_psl(psl);
	}

	CPU_START_CLEANUP(ci);
	cpu_starting = NULL;
}

void
cpu_boot_secondary(struct cpu_info *ci)
{
	int64_t drift;
	u_long psl;
	int i;

	atomic_or_32(&ci->ci_flags, CPUF_GO);
	for (i = 100000; (!(ci->ci_flags & CPUF_RUNNING)) && i > 0; i--) {
		i8254_delay(10);
	}
	if ((ci->ci_flags & CPUF_RUNNING) == 0) {
		aprint_error_dev(ci->ci_dev, "failed to start\n");
#if defined(MPDEBUG) && defined(DDB)
		printf("dropping into debugger; continue from here to resume boot\n");
		Debugger();
#endif
	} else {
		/* Synchronize TSC again, check for drift. */
		drift = ci->ci_data.cpu_cc_skew;
		psl = x86_read_psl();
		x86_disable_intr();
		wbinvd();
		tsc_sync_bp(ci);
		x86_write_psl(psl);
		drift -= ci->ci_data.cpu_cc_skew;
		aprint_debug_dev(ci->ci_dev, "TSC skew=%lld drift=%lld\n",
		    (long long)ci->ci_data.cpu_cc_skew, (long long)drift);
		tsc_sync_drift(drift);
	}
}

/*
 * The CPU ends up here when its ready to run
 * This is called from code in mptramp.s; at this point, we are running
 * in the idle pcb/idle stack of the new CPU.  When this function returns,
 * this processor will enter the idle loop and start looking for work.
 */
void
cpu_hatch(void *v)
{
	struct cpu_info *ci = (struct cpu_info *)v;
	int s, i;

#ifdef __x86_64__
	cpu_init_msrs(ci, true);
#endif
	cpu_probe(ci);

	ci->ci_data.cpu_cc_freq = cpu_info_primary.ci_data.cpu_cc_freq;
	/* cpu_get_tsc_freq(ci); */ 

	KDASSERT((ci->ci_flags & CPUF_PRESENT) == 0);

	/*
	 * Synchronize time stamp counters.  Invalidate cache and do twice
	 * to try and minimize possible cache effects.  Note that interrupts
	 * are off at this point.
	 */
	wbinvd();
	atomic_or_32(&ci->ci_flags, CPUF_PRESENT);
	tsc_sync_ap(ci);

	/*
	 * Wait to be brought online.  Use 'monitor/mwait' if available,
	 * in order to make the TSC drift as much as possible. so that
	 * we can detect it later.  If not available, try 'pause'. 
	 * We'd like to use 'hlt', but we have interrupts off.
	 */
	while ((ci->ci_flags & CPUF_GO) == 0) {
		if ((ci->ci_feature2_flags & CPUID2_MONITOR) != 0) {
			x86_monitor(&ci->ci_flags, 0, 0);
			if ((ci->ci_flags & CPUF_GO) != 0) {
				continue;
			}
			x86_mwait(0, 0);
		} else {
			for (i = 10000; i != 0; i--) {
				x86_pause();
			}
		}
	}

	/* Because the text may have been patched in x86_patch(). */
	wbinvd();
	x86_flush();

	KASSERT((ci->ci_flags & CPUF_RUNNING) == 0);

	lcr3(pmap_kernel()->pm_pdirpa);
	curlwp->l_addr->u_pcb.pcb_cr3 = pmap_kernel()->pm_pdirpa;
	lcr0(ci->ci_data.cpu_idlelwp->l_addr->u_pcb.pcb_cr0);
	cpu_init_idt();
	gdt_init_cpu(ci);
	lapic_enable();
	lapic_set_lvt();
	lapic_initclocks();

#ifdef i386
#if NNPX > 0
	npxinit(ci);
#endif
#else
	fpuinit(ci);
#endif
	lldt(GSYSSEL(GLDT_SEL, SEL_KPL));
	ltr(ci->ci_tss_sel);

	cpu_init(ci);
	cpu_get_tsc_freq(ci);

	s = splhigh();
#ifdef i386
	lapic_tpr = 0;
#else
	lcr8(0);
#endif
	x86_enable_intr();
	splx(s);
	x86_errata();

	aprint_debug_dev(ci->ci_dev, "running\n");
}

#if defined(DDB)

#include <ddb/db_output.h>
#include <machine/db_machdep.h>

/*
 * Dump CPU information from ddb.
 */
void
cpu_debug_dump(void)
{
	struct cpu_info *ci;
	CPU_INFO_ITERATOR cii;

	db_printf("addr		dev	id	flags	ipis	curlwp 		fpcurlwp\n");
	for (CPU_INFO_FOREACH(cii, ci)) {
		db_printf("%p	%s	%ld	%x	%x	%10p	%10p\n",
		    ci,
		    ci->ci_dev == NULL ? "BOOT" : device_xname(ci->ci_dev),
		    (long)ci->ci_cpuid,
		    ci->ci_flags, ci->ci_ipis,
		    ci->ci_curlwp,
		    ci->ci_fpcurlwp);
	}
}
#endif

static void
cpu_copy_trampoline(void)
{
	/*
	 * Copy boot code.
	 */
	extern u_char cpu_spinup_trampoline[];
	extern u_char cpu_spinup_trampoline_end[];
	
	vaddr_t mp_trampoline_vaddr;

	mp_trampoline_vaddr = uvm_km_alloc(kernel_map, PAGE_SIZE, 0,
	    UVM_KMF_VAONLY);

	pmap_kenter_pa(mp_trampoline_vaddr, mp_trampoline_paddr,
	    VM_PROT_READ | VM_PROT_WRITE);
	pmap_update(pmap_kernel());
	memcpy((void *)mp_trampoline_vaddr,
	    cpu_spinup_trampoline,
	    cpu_spinup_trampoline_end - cpu_spinup_trampoline);

	pmap_kremove(mp_trampoline_vaddr, PAGE_SIZE);
	pmap_update(pmap_kernel());
	uvm_km_free(kernel_map, mp_trampoline_vaddr, PAGE_SIZE, UVM_KMF_VAONLY);
}

#ifdef i386
static void
tss_init(struct i386tss *tss, void *stack, void *func)
{
	memset(tss, 0, sizeof *tss);
	tss->tss_esp0 = tss->tss_esp = (int)((char *)stack + USPACE - 16);
	tss->tss_ss0 = GSEL(GDATA_SEL, SEL_KPL);
	tss->__tss_cs = GSEL(GCODE_SEL, SEL_KPL);
	tss->tss_fs = GSEL(GCPU_SEL, SEL_KPL);
	tss->tss_gs = tss->__tss_es = tss->__tss_ds =
	    tss->__tss_ss = GSEL(GDATA_SEL, SEL_KPL);
	tss->tss_cr3 = pmap_kernel()->pm_pdirpa;
	tss->tss_esp = (int)((char *)stack + USPACE - 16);
	tss->tss_ldt = GSEL(GLDT_SEL, SEL_KPL);
	tss->__tss_eflags = PSL_MBO | PSL_NT;	/* XXX not needed? */
	tss->__tss_eip = (int)func;
}

/* XXX */
#define IDTVEC(name)	__CONCAT(X, name)
typedef void (vector)(void);
extern vector IDTVEC(tss_trap08);
#ifdef DDB
extern vector Xintrddbipi;
extern int ddb_vec;
#endif

static void
cpu_set_tss_gates(struct cpu_info *ci)
{
	struct segment_descriptor sd;

	ci->ci_doubleflt_stack = (char *)uvm_km_alloc(kernel_map, USPACE, 0,
	    UVM_KMF_WIRED);
	tss_init(&ci->ci_doubleflt_tss, ci->ci_doubleflt_stack,
	    IDTVEC(tss_trap08));
	setsegment(&sd, &ci->ci_doubleflt_tss, sizeof(struct i386tss) - 1,
	    SDT_SYS386TSS, SEL_KPL, 0, 0);
	ci->ci_gdt[GTRAPTSS_SEL].sd = sd;
	setgate(&idt[8], NULL, 0, SDT_SYSTASKGT, SEL_KPL,
	    GSEL(GTRAPTSS_SEL, SEL_KPL));

#if defined(DDB)
	/*
	 * Set up separate handler for the DDB IPI, so that it doesn't
	 * stomp on a possibly corrupted stack.
	 *
	 * XXX overwriting the gate set in db_machine_init.
	 * Should rearrange the code so that it's set only once.
	 */
	ci->ci_ddbipi_stack = (char *)uvm_km_alloc(kernel_map, USPACE, 0,
	    UVM_KMF_WIRED);
	tss_init(&ci->ci_ddbipi_tss, ci->ci_ddbipi_stack, Xintrddbipi);

	setsegment(&sd, &ci->ci_ddbipi_tss, sizeof(struct i386tss) - 1,
	    SDT_SYS386TSS, SEL_KPL, 0, 0);
	ci->ci_gdt[GIPITSS_SEL].sd = sd;

	setgate(&idt[ddb_vec], NULL, 0, SDT_SYSTASKGT, SEL_KPL,
	    GSEL(GIPITSS_SEL, SEL_KPL));
#endif
}
#else
static void
cpu_set_tss_gates(struct cpu_info *ci)
{

}
#endif	/* i386 */

int
mp_cpu_start(struct cpu_info *ci, paddr_t target)
{
	unsigned short dwordptr[2];
	int error;

	/*
	 * Bootstrap code must be addressable in real mode
	 * and it must be page aligned.
	 */
	KASSERT(target < 0x10000 && target % PAGE_SIZE == 0);

	/*
	 * "The BSP must initialize CMOS shutdown code to 0Ah ..."
	 */

	outb(IO_RTC, NVRAM_RESET);
	outb(IO_RTC+1, NVRAM_RESET_JUMP);

	/*
	 * "and the warm reset vector (DWORD based at 40:67) to point
	 * to the AP startup code ..."
	 */

	dwordptr[0] = 0;
	dwordptr[1] = target >> 4;

	memcpy((uint8_t *)cmos_data_mapping + 0x467, dwordptr, 4);

	if ((cpu_feature & CPUID_APIC) == 0) {
		aprint_error("mp_cpu_start: CPU does not have APIC\n");
		return ENODEV;
	}

	/*
	 * ... prior to executing the following sequence:".  We'll also add in
	 * local cache flush, in case the BIOS has left the AP with its cache
	 * disabled.  It may not be able to cope with MP coherency.
	 */
	wbinvd();

	if (ci->ci_flags & CPUF_AP) {
		error = x86_ipi_init(ci->ci_cpuid);
		if (error != 0) {
			aprint_error_dev(ci->ci_dev, "%s: IPI not taken (1)\n",
			    __func__);
			return error;
		}
		i8254_delay(10000);

		error = x86_ipi_startup(ci->ci_cpuid, target / PAGE_SIZE);
		if (error != 0) {
			aprint_error_dev(ci->ci_dev, "%s: IPI not taken (2)\n",
			    __func__);
			return error;
		}
		i8254_delay(200);

		error = x86_ipi_startup(ci->ci_cpuid, target / PAGE_SIZE);
		if (error != 0) {
			aprint_error_dev(ci->ci_dev, "%s: IPI not taken (3)\n",
			    __func__);
			return error;
		}
		i8254_delay(200);
	}

	return 0;
}

void
mp_cpu_start_cleanup(struct cpu_info *ci)
{
	/*
	 * Ensure the NVRAM reset byte contains something vaguely sane.
	 */

	outb(IO_RTC, NVRAM_RESET);
	outb(IO_RTC+1, NVRAM_RESET_RST);
}

#ifdef __x86_64__
typedef void (vector)(void);
extern vector Xsyscall, Xsyscall32;

void
cpu_init_msrs(struct cpu_info *ci, bool full)
{
	wrmsr(MSR_STAR,
	    ((uint64_t)GSEL(GCODE_SEL, SEL_KPL) << 32) |
	    ((uint64_t)LSEL(LSYSRETBASE_SEL, SEL_UPL) << 48));
	wrmsr(MSR_LSTAR, (uint64_t)Xsyscall);
	wrmsr(MSR_CSTAR, (uint64_t)Xsyscall32);
	wrmsr(MSR_SFMASK, PSL_NT|PSL_T|PSL_I|PSL_C);

	if (full) {
		wrmsr(MSR_FSBASE, 0);
		wrmsr(MSR_GSBASE, (uint64_t)ci);
		wrmsr(MSR_KERNELGSBASE, 0);
	}

	if (cpu_feature & CPUID_NOX)
		wrmsr(MSR_EFER, rdmsr(MSR_EFER) | EFER_NXE);
}
#endif	/* __x86_64__ */

void
cpu_offline_md(void)
{
	int s;

	s = splhigh();
#ifdef i386
#if NNPX > 0
	npxsave_cpu(true);
#endif
#else
	fpusave_cpu(true);
#endif
	splx(s);
}

/* XXX joerg restructure and restart CPUs individually */
static bool
cpu_suspend(device_t dv PMF_FN_ARGS)
{
	struct cpu_softc *sc = device_private(dv);
	struct cpu_info *ci = sc->sc_info;
	int err;

	if (ci->ci_flags & CPUF_PRIMARY)
		return true;
	if (ci->ci_data.cpu_idlelwp == NULL)
		return true;
	if ((ci->ci_flags & CPUF_PRESENT) == 0)
		return true;

	sc->sc_wasonline = !(ci->ci_schedstate.spc_flags & SPCF_OFFLINE);

	if (sc->sc_wasonline) {
		mutex_enter(&cpu_lock);
		err = cpu_setstate(ci, false);
		mutex_exit(&cpu_lock);
	
		if (err)
			return false;
	}

	return true;
}

static bool
cpu_resume(device_t dv PMF_FN_ARGS)
{
	struct cpu_softc *sc = device_private(dv);
	struct cpu_info *ci = sc->sc_info;
	int err = 0;

	if (ci->ci_flags & CPUF_PRIMARY)
		return true;
	if (ci->ci_data.cpu_idlelwp == NULL)
		return true;
	if ((ci->ci_flags & CPUF_PRESENT) == 0)
		return true;

	if (sc->sc_wasonline) {
		mutex_enter(&cpu_lock);
		err = cpu_setstate(ci, true);
		mutex_exit(&cpu_lock);
	}

	return err == 0;
}

void
cpu_get_tsc_freq(struct cpu_info *ci)
{
	uint64_t last_tsc;

	if (ci->ci_feature_flags & CPUID_TSC) {
		last_tsc = rdmsr(MSR_TSC);
		i8254_delay(100000);
		ci->ci_data.cpu_cc_freq = (rdmsr(MSR_TSC) - last_tsc) * 10;
	}
}

void
x86_cpu_idle_mwait(void)
{
	struct cpu_info *ci = curcpu();

	KASSERT(ci->ci_ilevel == IPL_NONE);

	x86_monitor(&ci->ci_want_resched, 0, 0);
	if (__predict_false(ci->ci_want_resched)) {
		return;
	}
	x86_mwait(0, 0);
}

void
x86_cpu_idle_halt(void)
{
	struct cpu_info *ci = curcpu();

	KASSERT(ci->ci_ilevel == IPL_NONE);

	x86_disable_intr();
	if (!__predict_false(ci->ci_want_resched)) {
		x86_stihlt();
	} else {
		x86_enable_intr();
	}
}
