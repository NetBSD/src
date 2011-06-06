/*	$NetBSD: cpu.c,v 1.52.2.1 2011/06/06 09:07:11 jruoho Exp $	*/
/* NetBSD: cpu.c,v 1.18 2004/02/20 17:35:01 yamt Exp  */

/*-
 * Copyright (c) 2000 The NetBSD Foundation, Inc.
 * Copyright (c) 2002, 2006, 2007 YAMAMOTO Takashi,
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by RedBack Networks Inc.
 *
 * Author: Bill Sommerfeld
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
__KERNEL_RCSID(0, "$NetBSD: cpu.c,v 1.52.2.1 2011/06/06 09:07:11 jruoho Exp $");

#include "opt_ddb.h"
#include "opt_multiprocessor.h"
#include "opt_mpbios.h"		/* for MPDEBUG */
#include "opt_mtrr.h"
#include "opt_xen.h"

#include "lapic.h"
#include "ioapic.h"

#include <sys/param.h>
#include <sys/proc.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/kmem.h>
#include <sys/cpu.h>
#include <sys/atomic.h>
#include <sys/reboot.h>

#include <uvm/uvm.h>

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

#include <xen/vcpuvar.h>

#if NLAPIC > 0
#include <machine/apicvar.h>
#include <machine/i82489reg.h>
#include <machine/i82489var.h>
#endif

#include <dev/ic/mc146818reg.h>
#include <dev/isa/isareg.h>

#if MAXCPUS > 32
#error cpu_info contains 32bit bitmasks
#endif

static int	cpu_match(device_t, cfdata_t, void *);
static void	cpu_attach(device_t, device_t, void *);
static void	cpu_defer(device_t);
static int	cpu_rescan(device_t, const char *, const int *);
static void	cpu_childdetached(device_t, device_t);
static int	vcpu_match(device_t, cfdata_t, void *);
static void	vcpu_attach(device_t, device_t, void *);
static void	cpu_attach_common(device_t, device_t, void *);
void		cpu_offline_md(void);

struct cpu_softc {
	device_t sc_dev;		/* device tree glue */
	struct cpu_info *sc_info;	/* pointer to CPU info */
	bool sc_wasonline;
};

int mp_cpu_start(struct cpu_info *, paddr_t);
void mp_cpu_start_cleanup(struct cpu_info *);
const struct cpu_functions mp_cpu_funcs = { mp_cpu_start, NULL,
				      mp_cpu_start_cleanup };

CFATTACH_DECL2_NEW(cpu, sizeof(struct cpu_softc),
    cpu_match, cpu_attach, NULL, NULL, cpu_rescan, cpu_childdetached);

CFATTACH_DECL_NEW(vcpu, sizeof(struct cpu_softc),
    vcpu_match, vcpu_attach, NULL, NULL);

/*
 * Statically-allocated CPU info for the primary CPU (or the only
 * CPU, on uniprocessors).  The CPU info list is initialized to
 * point at it.
 */
#ifdef TRAPLOG
#include <machine/tlog.h>
struct tlog tlog_primary;
#endif
struct cpu_info cpu_info_primary __aligned(CACHE_LINE_SIZE) = {
	.ci_dev = 0,
	.ci_self = &cpu_info_primary,
	.ci_idepth = -1,
	.ci_curlwp = &lwp0,
	.ci_curldt = -1,
#ifdef TRAPLOG
	.ci_tlog = &tlog_primary,
#endif

};
struct cpu_info phycpu_info_primary __aligned(CACHE_LINE_SIZE) = {
	.ci_dev = 0,
	.ci_self = &phycpu_info_primary,
};

struct cpu_info *cpu_info_list = &cpu_info_primary;
struct cpu_info *phycpu_info_list = &phycpu_info_primary;

static void	cpu_set_tss_gates(struct cpu_info *ci);

uint32_t cpus_attached = 0;
uint32_t cpus_running = 0;

uint32_t phycpus_attached = 0;
uint32_t phycpus_running = 0;

uint32_t cpu_feature[5]; /* X86 CPUID feature bits
			  *	[0] basic features %edx
			  *	[1] basic features %ecx
			  *	[2] extended features %edx
			  *	[3] extended features %ecx
			  *	[4] VIA padlock features
			  */

bool x86_mp_online;
paddr_t mp_trampoline_paddr = MP_TRAMPOLINE;

#if defined(MULTIPROCESSOR)
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
}
#endif	/* MULTIPROCESSOR */

static int
cpu_match(device_t parent, cfdata_t match, void *aux)
{

	return 1;
}

static void
cpu_attach(device_t parent, device_t self, void *aux)
{
	struct cpu_softc *sc = device_private(self);
	struct cpu_attach_args *caa = aux;
	struct cpu_info *ci;
	uintptr_t ptr;
	static int nphycpu = 0;

	sc->sc_dev = self;

	if (phycpus_attached == ~0) {
		aprint_error(": increase MAXCPUS\n");
		return;
	}

	/*
	 * If we're an Application Processor, allocate a cpu_info
	 * If we're the first attached CPU use the primary cpu_info,
	 * otherwise allocate a new one
	 */
	aprint_naive("\n");
	aprint_normal("\n");
	if (nphycpu > 0) {
		struct cpu_info *tmp;
		ptr = (uintptr_t)kmem_zalloc(sizeof(*ci) + CACHE_LINE_SIZE - 1,
		    KM_SLEEP);
		ci = (struct cpu_info *)roundup2(ptr, CACHE_LINE_SIZE);
		ci->ci_curldt = -1;

		tmp = phycpu_info_list;
		while (tmp->ci_next)
			tmp = tmp->ci_next;

		tmp->ci_next = ci;
	} else {
		ci = &phycpu_info_primary;
	}

	ci->ci_self = ci;
	sc->sc_info = ci;

	ci->ci_dev = self;
	ci->ci_acpiid = caa->cpu_id;
	ci->ci_cpuid = caa->cpu_number;
	ci->ci_vcpu = NULL;
	ci->ci_index = nphycpu++;
	ci->ci_cpumask = (1 << cpu_index(ci));

	atomic_or_32(&phycpus_attached, ci->ci_cpumask);

	if (!pmf_device_register(self, NULL, NULL))
		aprint_error_dev(self, "couldn't establish power handler\n");

	(void)config_defer(self, cpu_defer);
}

static void
cpu_defer(device_t self)
{
	cpu_rescan(self, NULL, NULL);
}

static int
cpu_rescan(device_t self, const char *ifattr, const int *locators)
{
	struct cpu_softc *sc = device_private(self);
	struct cpufeature_attach_args cfaa;
	struct cpu_info *ci = sc->sc_info;

	memset(&cfaa, 0, sizeof(cfaa));
	cfaa.ci = ci;

	if (ifattr_match(ifattr, "cpufeaturebus")) {

		if (ci->ci_frequency == NULL) {
			cfaa.name = "frequency";
			ci->ci_frequency = config_found_ia(self,
			    "cpufeaturebus", &cfaa, NULL);
		}
	}

	return 0;
}

static void
cpu_childdetached(device_t self, device_t child)
{
	struct cpu_softc *sc = device_private(self);
	struct cpu_info *ci = sc->sc_info;

	if (ci->ci_frequency == child)
		ci->ci_frequency = NULL;
}

static int
vcpu_match(device_t parent, cfdata_t match, void *aux)
{
	struct vcpu_attach_args *vcaa = aux;

	if (strcmp(vcaa->vcaa_name, match->cf_name) == 0)
		return 1;
	return 0;
}

static void
vcpu_attach(device_t parent, device_t self, void *aux)
{
	struct vcpu_attach_args *vcaa = aux;

	cpu_attach_common(parent, self, &vcaa->vcaa_caa);
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

static void
cpu_attach_common(device_t parent, device_t self, void *aux)
{
	struct cpu_softc *sc = device_private(self);
	struct cpu_attach_args *caa = aux;
	struct cpu_info *ci;
	uintptr_t ptr;
	int cpunum = caa->cpu_number;
	static bool again = false;

	sc->sc_dev = self;

	/*
	 * If we're an Application Processor, allocate a cpu_info
	 * structure, otherwise use the primary's.
	 */
	if (caa->cpu_role == CPU_ROLE_AP) {
		aprint_naive(": Application Processor\n");
		ptr = (uintptr_t)kmem_alloc(sizeof(*ci) + CACHE_LINE_SIZE - 1,
		    KM_SLEEP);
		ci = (struct cpu_info *)roundup2(ptr, CACHE_LINE_SIZE);
		memset(ci, 0, sizeof(*ci));
#ifdef TRAPLOG
		ci->ci_tlog_base = kmem_zalloc(sizeof(struct tlog), KM_SLEEP);
#endif
	} else {
		aprint_naive(": %s Processor\n",
		    caa->cpu_role == CPU_ROLE_SP ? "Single" : "Boot");
		ci = &cpu_info_primary;
#if NLAPIC > 0
		if (cpunum != lapic_cpu_number()) {
			/* XXX should be done earlier */
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
#endif
	}

	ci->ci_self = ci;
	sc->sc_info = ci;
	ci->ci_dev = self;
	ci->ci_cpuid = cpunum;

	KASSERT(HYPERVISOR_shared_info != NULL);
	ci->ci_vcpu = &HYPERVISOR_shared_info->vcpu_info[cpunum];

	ci->ci_func = caa->cpu_func;

	/* Must be called before mi_cpu_attach(). */
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
#if NLAPIC > 0
		if (caa->cpu_role != CPU_ROLE_SP) {
			/* Enable lapic. */
			lapic_enable();
			lapic_set_lvt();
			lapic_calibrate_timer();
		}
#endif
		/* Make sure DELAY() is initialized. */
		DELAY(1);
		again = true;
	}

	/* further PCB init done later. */

	switch (caa->cpu_role) {
	case CPU_ROLE_SP:
		atomic_or_32(&ci->ci_flags, CPUF_SP);
		cpu_identify(ci);
#if 0
		x86_errata();
#endif
		x86_cpu_idle_init();
		break;

	case CPU_ROLE_BP:
		atomic_or_32(&ci->ci_flags, CPUF_BSP);
		cpu_identify(ci);
		cpu_init(ci);
#if 0
		x86_errata();
#endif
		x86_cpu_idle_init();
		break;

	case CPU_ROLE_AP:
		/*
		 * report on an AP
		 */

#if defined(MULTIPROCESSOR)
		cpu_intr_init(ci);
		gdt_alloc_cpu(ci);
		cpu_set_tss_gates(ci);
		pmap_cpu_init_early(ci);
		pmap_cpu_init_late(ci);
		cpu_start_secondary(ci);
		if (ci->ci_flags & CPUF_PRESENT) {
			struct cpu_info *tmp;

			identifycpu(ci);
			tmp = cpu_info_list;
			while (tmp->ci_next)
				tmp = tmp->ci_next;

			tmp->ci_next = ci;
		}
#else
		aprint_error_dev(self, "not started\n");
#endif
		break;

	default:
		aprint_normal("\n");
		panic("unknown processor type??\n");
	}

	pat_init(ci);
	atomic_or_32(&cpus_attached, ci->ci_cpumask);

#if 0
	if (!pmf_device_register(self, cpu_suspend, cpu_resume))
		aprint_error_dev(self, "couldn't establish power handler\n");
#endif

#if defined(MULTIPROCESSOR)
	if (mp_verbose) {
		struct lwp *l = ci->ci_data.cpu_idlelwp;
		struct pcb *pcb = lwp_getpcb(l);

		aprint_verbose_dev(self,
		    "idle lwp at %p, idle sp at 0x%p\n",
		    l,
#ifdef i386
		    (void *)pcb->pcb_esp
#else
		    (void *)pcb->pcb_rsp
#endif
		);
		
	}
#endif
}

/*
 * Initialize the processor appropriately.
 */

void
cpu_init(struct cpu_info *ci)
{

	/*
	 * On a P6 or above, enable global TLB caching if the
	 * hardware supports it.
	 */
	if (cpu_feature[0] & CPUID_PGE)
		lcr4(rcr4() | CR4_PGE);	/* enable global TLB caching */

#ifdef XXXMTRR
	/*
	 * On a P6 or above, initialize MTRR's if the hardware supports them.
	 */
	if (cpu_feature[0] & CPUID_MTRR) {
		if ((ci->ci_flags & CPUF_AP) == 0)
			i686_mtrr_init_first();
		mtrr_init_cpu(ci);
	}
#endif
	/*
	 * If we have FXSAVE/FXRESTOR, use them.
	 */
	if (cpu_feature[0] & CPUID_FXSR) {
		lcr4(rcr4() | CR4_OSFXSR);

		/*
		 * If we have SSE/SSE2, enable XMM exceptions.
		 */
		if (cpu_feature[0] & (CPUID_SSE|CPUID_SSE2))
			lcr4(rcr4() | CR4_OSXMMEXCPT);
	}

#ifdef __x86_64__
	/* No user PGD mapped for this CPU yet */
	ci->ci_xen_current_user_pgd = 0;
#endif

	atomic_or_32(&cpus_running, ci->ci_cpumask);
	atomic_or_32(&ci->ci_flags, CPUF_RUNNING);
}


#ifdef MULTIPROCESSOR
void
cpu_boot_secondary_processors(void)
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
		if (ci->ci_flags & (CPUF_BSP|CPUF_SP|CPUF_PRIMARY))
			continue;
		cpu_boot_secondary(ci);
	}

	x86_mp_online = true;
}

static void
cpu_init_idle_lwp(struct cpu_info *ci)
{
	struct lwp *l = ci->ci_data.cpu_idlelwp;
	struct pcb *pcb = lwp_getpcb(l);

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
	int i;
	struct pmap *kpm = pmap_kernel();
	extern uint32_t mp_pdirpa;

	mp_pdirpa = kpm->pm_pdirpa; /* XXX move elsewhere, not per CPU. */

	atomic_or_32(&ci->ci_flags, CPUF_AP);

	aprint_debug_dev(ci->ci_dev, "starting\n");

	ci->ci_curlwp = ci->ci_data.cpu_idlelwp;
	if (CPU_STARTUP(ci, mp_trampoline_paddr) != 0)
		return;

	/*
	 * wait for it to become ready
	 */
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
		delay(10);
	}
	if ((ci->ci_flags & CPUF_PRESENT) == 0) {
		aprint_error_dev(ci->ci_dev, "failed to become ready\n");
#if defined(MPDEBUG) && defined(DDB)
		printf("dropping into debugger; continue from here to resume boot\n");
		Debugger();
#endif
	}

	CPU_START_CLEANUP(ci);
}

void
cpu_boot_secondary(struct cpu_info *ci)
{
	int i;

	atomic_or_32(&ci->ci_flags, CPUF_GO);
	for (i = 100000; (!(ci->ci_flags & CPUF_RUNNING)) && i > 0; i--) {
		delay(10);
	}
	if ((ci->ci_flags & CPUF_RUNNING) == 0) {
		aprint_error_dev(ci->ci_dev, "CPU failed to start\n");
#if defined(MPDEBUG) && defined(DDB)
		printf("dropping into debugger; continue from here to resume boot\n");
		Debugger();
#endif
	}
}

/*
 * The CPU ends up here when its ready to run
 * This is called from code in mptramp.s; at this point, we are running
 * in the idle pcb/idle stack of the new CPU.  When this function returns,
 * this processor will enter the idle loop and start looking for work.
 *
 * XXX should share some of this with init386 in machdep.c
 */
void
cpu_hatch(void *v)
{
	struct cpu_info *ci = (struct cpu_info *)v;
	struct pcb *pcb;
	int s, i;

	cpu_probe(ci);

	cpu_feature[0] &= ~CPUID_FEAT_BLACKLIST;
	cpu_feature[2] &= ~CPUID_FEAT_EXT_BLACKLIST;

        cpu_init_msrs(ci, true);

	KDASSERT((ci->ci_flags & CPUF_PRESENT) == 0);
	atomic_or_32(&ci->ci_flags, CPUF_PRESENT);
	while ((ci->ci_flags & CPUF_GO) == 0) {
		/* Don't use delay, boot CPU may be patching the text. */
		for (i = 10000; i != 0; i--)
			x86_pause();
	}

	/* Because the text may have been patched in x86_patch(). */
	wbinvd();
	x86_flush();

	KASSERT((ci->ci_flags & CPUF_RUNNING) == 0);

	pcb = lwp_getpcb(curlwp);
	lcr3(pmap_kernel()->pm_pdirpa);
	pcb->pcb_cr3 = pmap_kernel()->pm_pdirpa;
	pcb = lwp_getpcb(ci->ci_data.cpu_idlelwp);
	lcr0(pcb->pcb_cr0);

	cpu_init_idt();
	gdt_init_cpu(ci);
	lapic_enable();
	lapic_set_lvt();
	lapic_initclocks();

#ifdef i386
	npxinit(ci);
#else
	fpuinit(ci);
#endif

	lldt(GSEL(GLDT_SEL, SEL_KPL));
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
#if 0
	x86_errata();
#endif

	aprint_debug_dev(ci->ci_dev, "CPU %ld running\n",
		(long)ci->ci_cpuid);
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
#endif /* DDB */

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
		VM_PROT_READ | VM_PROT_WRITE, 0);
	pmap_update(pmap_kernel());
	memcpy((void *)mp_trampoline_vaddr,
		cpu_spinup_trampoline,
		cpu_spinup_trampoline_end - cpu_spinup_trampoline);

	pmap_kremove(mp_trampoline_vaddr, PAGE_SIZE);
	pmap_update(pmap_kernel());
	uvm_km_free(kernel_map, mp_trampoline_vaddr, PAGE_SIZE, UVM_KMF_VAONLY);
}

#endif /* MULTIPROCESSOR */

#ifdef i386
#if 0
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
	tss->__tss_eflags = PSL_MBO | PSL_NT;   /* XXX not needed? */
	tss->__tss_eip = (int)func;
}
#endif

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
#if 0
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
#endif

#if defined(DDB) && defined(MULTIPROCESSOR)
	/*
	 * Set up separate handler for the DDB IPI, so that it doesn't
	 * stomp on a possibly corrupted stack.
	 *
	 * XXX overwriting the gate set in db_machine_init.
	 * Should rearrange the code so that it's set only once.
	 */
	ci->ci_ddbipi_stack = (char *)uvm_km_alloc(kernel_map, USPACE, 0,
	    UVM_KMF_WIRED);
	tss_init(&ci->ci_ddbipi_tss, ci->ci_ddbipi_stack,
	    Xintrddbipi);

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
#if 0
#if NLAPIC > 0
	int error;
#endif
	unsigned short dwordptr[2];

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

	pmap_kenter_pa (0, 0, VM_PROT_READ|VM_PROT_WRITE, 0);
	pmap_update(pmap_kernel());

	memcpy ((uint8_t *) 0x467, dwordptr, 4);

	pmap_kremove (0, PAGE_SIZE);
	pmap_update(pmap_kernel());

#if NLAPIC > 0
	/*
	 * ... prior to executing the following sequence:"
	 */

	if (ci->ci_flags & CPUF_AP) {
		if ((error = x86_ipi_init(ci->ci_cpuid)) != 0)
			return error;

		delay(10000);

		if (cpu_feature & CPUID_APIC) {
			error = x86_ipi_init(ci->ci_cpuid);
			if (error != 0) {
				aprint_error_dev(ci->ci_dev, "%s: IPI not taken (1)\n",
						__func__);
				return error;
			}

			delay(10000);

			error = x86_ipi(target / PAGE_SIZE, ci->ci_cpuid,
					LAPIC_DLMODE_STARTUP);
			if (error != 0) {
				aprint_error_dev(ci->ci_dev, "%s: IPI not taken (2)\n",
						__func__);
				return error;
			}
			delay(200);

			error = x86_ipi(target / PAGE_SIZE, ci->ci_cpuid,
					LAPIC_DLMODE_STARTUP);
			if (error != 0) {
				aprint_error_dev(ci->ci_dev, "%s: IPI not taken ((3)\n",
						__func__);
				return error;
			}
			delay(200);
		}
	}
#endif
#endif /* 0 */
	return 0;
}

void
mp_cpu_start_cleanup(struct cpu_info *ci)
{
#if 0
	/*
	 * Ensure the NVRAM reset byte contains something vaguely sane.
	 */

	outb(IO_RTC, NVRAM_RESET);
	outb(IO_RTC+1, NVRAM_RESET_RST);
#endif
}

void
cpu_init_msrs(struct cpu_info *ci, bool full)
{
#ifdef __x86_64__
	if (full) {
		HYPERVISOR_set_segment_base (SEGBASE_FS, 0);
		HYPERVISOR_set_segment_base (SEGBASE_GS_KERNEL, (uint64_t) ci);
		HYPERVISOR_set_segment_base (SEGBASE_GS_USER, 0);
	}
#endif	/* __x86_64__ */

	if (cpu_feature[2] & CPUID_NOX)
		wrmsr(MSR_EFER, rdmsr(MSR_EFER) | EFER_NXE);
}

void
cpu_offline_md(void)
{
        int s;

        s = splhigh();
#ifdef __i386__
        npxsave_cpu(true);
#else   
        fpusave_cpu(true);
#endif
        splx(s);
}

#if 0
/* XXX joerg restructure and restart CPUs individually */
static bool
cpu_suspend(device_t dv, const pmf_qual_t *qual)
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
cpu_resume(device_t dv, const pmf_qual_t *qual)
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
#endif

void    
cpu_get_tsc_freq(struct cpu_info *ci)
{
	const volatile vcpu_time_info_t *tinfo = &ci->ci_vcpu->time;
	delay(1000000);
	uint64_t freq = 1000000000ULL << 32;
	freq = freq / (uint64_t)tinfo->tsc_to_system_mul;
	if ( tinfo->tsc_shift < 0 )
		freq = freq << -tinfo->tsc_shift;
	else
		freq = freq >> tinfo->tsc_shift;
	ci->ci_data.cpu_cc_freq = freq;
}

void
x86_cpu_idle_xen(void)
{
	struct cpu_info *ci = curcpu();

	KASSERT(ci->ci_ilevel == IPL_NONE);

	x86_disable_intr();
	if (!__predict_false(ci->ci_want_resched)) {
		idle_block();
	} else {
		x86_enable_intr();
	}
}

/*
 * Loads pmap for the current CPU.
 */
void
cpu_load_pmap(struct pmap *pmap)
{
#ifdef i386
#ifdef PAE
	int i, s;
	struct cpu_info *ci;

	s = splvm(); /* just to be safe */
	ci = curcpu();
	paddr_t l3_pd = xpmap_ptom_masked(ci->ci_pae_l3_pdirpa);
	/* don't update the kernel L3 slot */
	for (i = 0 ; i < PDP_SIZE - 1; i++) {
		xpq_queue_pte_update(l3_pd + i * sizeof(pd_entry_t),
		    xpmap_ptom(pmap->pm_pdirpa[i]) | PG_V);
	}
	splx(s);
	tlbflush();
#else /* PAE */
	lcr3(pmap_pdirpa(pmap, 0));
#endif /* PAE */
#endif /* i386 */

#ifdef __x86_64__
	int i, s;
	pd_entry_t *old_pgd, *new_pgd;
	paddr_t addr;
	struct cpu_info *ci;

	/* kernel pmap always in cr3 and should never go in user cr3 */
	if (pmap_pdirpa(pmap, 0) != pmap_pdirpa(pmap_kernel(), 0)) {
		ci = curcpu();
		/*
		 * Map user space address in kernel space and load
		 * user cr3
		 */
		s = splvm();
		new_pgd = pmap->pm_pdir;
		old_pgd = pmap_kernel()->pm_pdir;
		addr = xpmap_ptom(pmap_pdirpa(pmap_kernel(), 0));
		for (i = 0; i < PDIR_SLOT_PTE;
		    i++, addr += sizeof(pd_entry_t)) {
			if ((new_pgd[i] & PG_V) || (old_pgd[i] & PG_V))
				xpq_queue_pte_update(addr, new_pgd[i]);
		}
		tlbflush();
		xen_set_user_pgd(pmap_pdirpa(pmap, 0));
		ci->ci_xen_current_user_pgd = pmap_pdirpa(pmap, 0);
		splx(s);
	}
#endif /* __x86_64__ */
}
