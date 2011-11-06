/*	$NetBSD: cpu.c,v 1.70 2011/11/06 15:18:19 cherry Exp $	*/
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
__KERNEL_RCSID(0, "$NetBSD: cpu.c,v 1.70 2011/11/06 15:18:19 cherry Exp $");

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
#include <sys/cpufreq.h>
#include <sys/atomic.h>
#include <sys/reboot.h>
#include <sys/idle.h>

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

#ifdef i386
#include <machine/npx.h>
#else
#include <machine/fpu.h>
#endif

#include <xen/xen.h>
#include <xen/xen3-public/vcpu.h>
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

int mp_cpu_start(struct cpu_info *, vaddr_t);
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
	.ci_cpumask = 1,
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

uint32_t cpus_attached = 1;
uint32_t cpus_running = 1;

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
	struct vcpu_runstate_info vcr;
	int error;

	if (strcmp(vcaa->vcaa_name, match->cf_name) == 0) {
		error = HYPERVISOR_vcpu_op(VCPUOP_get_runstate_info,
					   vcaa->vcaa_caa.cpu_number,
					   &vcr);
		switch (error) {
		case 0:
			return 1;
		case -ENOENT:
			return 0;
		default:
			panic("Unknown hypervisor error %d returned on vcpu runstate probe\n", error);
		}
	}

	return 0;
}

static void
vcpu_attach(device_t parent, device_t self, void *aux)
{
	struct vcpu_attach_args *vcaa = aux;

	KASSERT(vcaa->vcaa_caa.cpu_func == NULL);
	vcaa->vcaa_caa.cpu_func = &mp_cpu_funcs;
	cpu_attach_common(parent, self, &vcaa->vcaa_caa);

	if (!pmf_device_register(self, NULL, NULL))
		aprint_error_dev(self, "couldn't establish power handler\n");
}

static int
vcpu_is_up(struct cpu_info *ci)
{
	KASSERT(ci != NULL);
	return HYPERVISOR_vcpu_op(VCPUOP_is_up, ci->ci_cpuid, NULL);
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
	 * Knowing the size of the largest cache on this CPU, potentially
	 * re-color our pages.
	 */
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
	}

	ci->ci_self = ci;
	sc->sc_info = ci;
	ci->ci_dev = self;
	ci->ci_cpuid = cpunum;

	KASSERT(HYPERVISOR_shared_info != NULL);
	ci->ci_vcpu = &HYPERVISOR_shared_info->vcpu_info[cpunum];

	KASSERT(ci->ci_func == 0);
	ci->ci_func = caa->cpu_func;

	/* Must be called before mi_cpu_attach(). */
	cpu_vm_init(ci);

	if (caa->cpu_role == CPU_ROLE_AP) {
		int error;

		error = mi_cpu_attach(ci);

		KASSERT(ci->ci_data.cpu_idlelwp != NULL);
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
		pmap_cpu_init_late(ci); /* XXX: cosmetic */

		/* Every processor needs to init it's own ipi h/w (similar to lapic) */
		xen_ipi_init();
		/* XXX: clock_init() */

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
		atomic_or_32(&ci->ci_flags, CPUF_AP);

		/*
		 * report on an AP
		 */

#if defined(MULTIPROCESSOR)
		/* interrupt handler stack */
		cpu_intr_init(ci);

		/* Setup per-cpu memory for gdt */
		gdt_alloc_cpu(ci);

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
#else
		aprint_error(": not started\n");
#endif
		break;

	default:
		aprint_normal("\n");
		panic("unknown processor type??\n");
	}

	pat_init(ci);
	atomic_or_32(&cpus_attached, ci->ci_cpumask);

#ifdef MPVERBOSE
	if (mp_verbose) {
		struct lwp *l = ci->ci_data.cpu_idlelwp;
		struct pcb *pcb = lwp_getpcb(l);

		aprint_verbose_dev(self,
		    "idle lwp at %p, idle sp at 0x%p\n",
		    l,
#ifdef i386
		    (void *)pcb->pcb_esp
#else /* i386 */
		    (void *)pcb->pcb_rsp
#endif /* i386 */
		);
		
	}
#endif /* MPVERBOSE */
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

	/* XXX: register vcpu_register_runstate_memory_area, and figure out how to make sure this VCPU is running ? */
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

static void
cpu_start_secondary(struct cpu_info *ci)
{
	int i;

	aprint_debug_dev(ci->ci_dev, "starting\n");

	ci->ci_curlwp = ci->ci_data.cpu_idlelwp;

	if (CPU_STARTUP(ci, (vaddr_t) cpu_hatch) != 0) {
		return;
	}

	/*
	 * wait for it to become ready
	 */
	for (i = 100000; (!(ci->ci_flags & CPUF_PRESENT)) && i > 0; i--) {
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
 * APs end up here immediately after initialisation and VCPUOP_up in
 * mp_cpu_start(). 
 * At this point, we are running in the idle pcb/idle stack of the new
 * CPU.  This function jumps to the idle loop and starts looking for
 * work. 
 */
extern void x86_64_tls_switch(struct lwp *);
void
cpu_hatch(void *v)
{
	struct cpu_info *ci = (struct cpu_info *)v;
	struct pcb *pcb;
	int s, i;

	/* Setup TLS and kernel GS/FS */
	cpu_init_msrs(ci, true);
	cpu_init_idt();
	gdt_init_cpu(ci);

	cpu_probe(ci);

	atomic_or_32(&ci->ci_flags, CPUF_PRESENT);

	while ((ci->ci_flags & CPUF_GO) == 0) {
		/* Don't use delay, boot CPU may be patching the text. */
		for (i = 10000; i != 0; i--)
			x86_pause();
	}

	/* Because the text may have been patched in x86_patch(). */
	x86_flush();
	tlbflushg();

	KASSERT((ci->ci_flags & CPUF_RUNNING) == 0);

	pcb = lwp_getpcb(curlwp);
	pcb->pcb_cr3 = pmap_pdirpa(pmap_kernel(), 0); /* XXX: consider using pmap_load() ? */
	pcb = lwp_getpcb(ci->ci_data.cpu_idlelwp);

	xen_ipi_init();

	xen_initclocks();
	
	/* XXX: lapic_initclocks(); */

#ifdef __x86_64__
	fpuinit(ci);
#endif

	lldt(GSEL(GLDT_SEL, SEL_KPL));

	cpu_init(ci);
	cpu_get_tsc_freq(ci);

	s = splhigh();
	x86_enable_intr();
	splx(s);
#if 0
	x86_errata();
#endif

	aprint_debug_dev(ci->ci_dev, "running\n");

	cpu_switchto(NULL, ci->ci_data.cpu_idlelwp, true);

	panic("switch to idle_loop context returned!\n");
	/* NOTREACHED */
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

#endif /* MULTIPROCESSOR */

extern void hypervisor_callback(void);
extern void failsafe_callback(void);
#ifdef __x86_64__
typedef void (vector)(void);
extern vector Xsyscall, Xsyscall32;
#endif

/*
 * Setup the "trampoline". On Xen, we setup nearly all cpu context
 * outside a trampoline, so we prototype and call targetip like so:
 * void targetip(struct cpu_info *);
 */

static void
gdt_prepframes(paddr_t *frames, vaddr_t base, uint32_t entries)
{
	int i;	
	for (i = 0; i < roundup(entries, PAGE_SIZE) >> PAGE_SHIFT; i++) {

		frames[i] = ((paddr_t) xpmap_ptetomach(
				(pt_entry_t *) (base + (i << PAGE_SHIFT))))
			>> PAGE_SHIFT;

		/* Mark Read-only */
		pmap_pte_clearbits(kvtopte(base + (i << PAGE_SHIFT)),
		    PG_RW);
	}
}

#ifdef __x86_64__
extern char *ldtstore; /* XXX: Xen MP todo */

static void
xen_init_amd64_vcpuctxt(struct cpu_info *ci,
			struct vcpu_guest_context *initctx, 
			void targetrip(struct cpu_info *))
{
	/* page frames to point at GDT */
	extern int gdt_size;
	paddr_t frames[16];
	psize_t gdt_ents;

	struct lwp *l;
	struct pcb *pcb;

	volatile struct vcpu_info *vci;

	KASSERT(ci != NULL);
	KASSERT(ci != &cpu_info_primary);
	KASSERT(initctx != NULL);
	KASSERT(targetrip != NULL);

	memset(initctx, 0, sizeof *initctx);

	gdt_ents = roundup(gdt_size, PAGE_SIZE) >> PAGE_SHIFT; /* XXX: re-investigate roundup(gdt_size... ) for gdt_ents. */
	KASSERT(gdt_ents <= 16);

	gdt_prepframes(frames, (vaddr_t) ci->ci_gdt, gdt_ents);

	/* XXX: The stuff in here is amd64 specific. move to mptramp.[Sc] ? */

	/* Initialise the vcpu context: We use idle_loop()'s pcb context. */

	l = ci->ci_data.cpu_idlelwp;

	KASSERT(l != NULL);
	pcb = lwp_getpcb(l);
	KASSERT(pcb != NULL);

	/* resume with interrupts off */
	vci = ci->ci_vcpu;
	vci->evtchn_upcall_mask = 1;
	xen_mb();

	/* resume in kernel-mode */
	initctx->flags = VGCF_in_kernel | VGCF_online;

	/* Stack and entry points:
	 * We arrange for the stack frame for cpu_hatch() to
	 * appear as a callee frame of lwp_trampoline(). Being a
	 * leaf frame prevents trampling on any of the MD stack setup
	 * that x86/vm_machdep.c:cpu_lwp_fork() does for idle_loop()
	 */

	initctx->user_regs.rdi = (uint64_t) ci; /* targetrip(ci); */
	initctx->user_regs.rip = (vaddr_t) targetrip;

	initctx->user_regs.cs = GSEL(GCODE_SEL, SEL_KPL);

	initctx->user_regs.rflags = pcb->pcb_flags;
	initctx->user_regs.rsp = pcb->pcb_rsp;

	/* Data segments */
	initctx->user_regs.ss = GSEL(GDATA_SEL, SEL_KPL);
	initctx->user_regs.es = GSEL(GDATA_SEL, SEL_KPL);
	initctx->user_regs.ds = GSEL(GDATA_SEL, SEL_KPL);

	/* GDT */
	memcpy(initctx->gdt_frames, frames, sizeof frames);
	initctx->gdt_ents = gdt_ents;

	/* LDT */
	initctx->ldt_base = (unsigned long) ldtstore;
	initctx->ldt_ents = LDT_SIZE >> 3;

	/* Kernel context state */
	initctx->kernel_ss = GSEL(GDATA_SEL, SEL_KPL);
	initctx->kernel_sp = pcb->pcb_rsp0;
	initctx->ctrlreg[0] = pcb->pcb_cr0;
	initctx->ctrlreg[1] = 0; /* "resuming" from kernel - no User cr3. */
	initctx->ctrlreg[2] = pcb->pcb_cr2; /* XXX: */
	/* 
	 * Use pmap_kernel() L4 PD directly, until we setup the
	 * per-cpu L4 PD in pmap_cpu_init_late()
	 */
	initctx->ctrlreg[3] = xen_pfn_to_cr3(x86_btop(xpmap_ptom(ci->ci_kpm_pdirpa)));
	initctx->ctrlreg[4] = CR4_PAE | CR4_OSFXSR | CR4_OSXMMEXCPT;


	/* Xen callbacks */
	initctx->event_callback_eip = (unsigned long) hypervisor_callback;
	initctx->failsafe_callback_eip = (unsigned long) failsafe_callback;
	initctx->syscall_callback_eip = (unsigned long) Xsyscall;

	return;
}
#else /* i386 */
extern union descriptor *ldt;
extern void Xsyscall(void);

static void
xen_init_i386_vcpuctxt(struct cpu_info *ci,
			struct vcpu_guest_context *initctx, 
			void targeteip(struct cpu_info *))
{
	/* page frames to point at GDT */
	extern int gdt_size;
	paddr_t frames[16];
	psize_t gdt_ents;

	struct lwp *l;
	struct pcb *pcb;

	volatile struct vcpu_info *vci;

	KASSERT(ci != NULL);
	KASSERT(ci != &cpu_info_primary);
	KASSERT(initctx != NULL);
	KASSERT(targeteip != NULL);

	memset(initctx, 0, sizeof *initctx);

	gdt_ents = roundup(gdt_size, PAGE_SIZE) >> PAGE_SHIFT; /* XXX: re-investigate roundup(gdt_size... ) for gdt_ents. */
	KASSERT(gdt_ents <= 16);

	gdt_prepframes(frames, (vaddr_t) ci->ci_gdt, gdt_ents);

	/* 
	 * Initialise the vcpu context: 
	 * We use this cpu's idle_loop() pcb context.
	 */

	l = ci->ci_data.cpu_idlelwp;

	KASSERT(l != NULL);
	pcb = lwp_getpcb(l);
	KASSERT(pcb != NULL);

	/* resume with interrupts off */
	vci = ci->ci_vcpu;
	vci->evtchn_upcall_mask = 1;
	xen_mb();

	/* resume in kernel-mode */
	initctx->flags = VGCF_in_kernel | VGCF_online;

	/* Stack frame setup for cpu_hatch():
	 * We arrange for the stack frame for cpu_hatch() to
	 * appear as a callee frame of lwp_trampoline(). Being a
	 * leaf frame prevents trampling on any of the MD stack setup
	 * that x86/vm_machdep.c:cpu_lwp_fork() does for idle_loop()
	 */

	initctx->user_regs.esp = pcb->pcb_esp - 4; /* Leave word for
						      arg1 */
	{ /* targeteip(ci); */
		uint32_t *arg = (uint32_t *) initctx->user_regs.esp;
		arg[1] = (uint32_t) ci; /* arg1 */

	}

	initctx->user_regs.eip = (vaddr_t) targeteip;
	initctx->user_regs.cs = GSEL(GCODE_SEL, SEL_KPL);
	initctx->user_regs.eflags |= pcb->pcb_iopl;

	/* Data segments */
	initctx->user_regs.ss = GSEL(GDATA_SEL, SEL_KPL);
	initctx->user_regs.es = GSEL(GDATA_SEL, SEL_KPL);
	initctx->user_regs.ds = GSEL(GDATA_SEL, SEL_KPL);
	initctx->user_regs.fs = GSEL(GDATA_SEL, SEL_KPL);

	/* GDT */
	memcpy(initctx->gdt_frames, frames, sizeof frames);
	initctx->gdt_ents = gdt_ents;

	/* LDT */
	initctx->ldt_base = (unsigned long) ldt;
	initctx->ldt_ents = NLDT;

	/* Kernel context state */
	initctx->kernel_ss = GSEL(GDATA_SEL, SEL_KPL);
	initctx->kernel_sp = pcb->pcb_esp0;
	initctx->ctrlreg[0] = pcb->pcb_cr0;
	initctx->ctrlreg[1] = 0; /* "resuming" from kernel - no User cr3. */
	initctx->ctrlreg[2] = pcb->pcb_cr2; /* XXX: */
#ifdef PAE
	initctx->ctrlreg[3] = xen_pfn_to_cr3(x86_btop(xpmap_ptom(ci->ci_pae_l3_pdirpa)));
#else /* PAE */
	initctx->ctrlreg[3] = xen_pfn_to_cr3(x86_btop(xpmap_ptom(pcb->pcb_cr3)));
#endif /* PAE */
	initctx->ctrlreg[4] = /* CR4_PAE |  */CR4_OSFXSR | CR4_OSXMMEXCPT;


	/* Xen callbacks */
	initctx->event_callback_eip = (unsigned long) hypervisor_callback;
	initctx->event_callback_cs = GSEL(GCODE_SEL, SEL_KPL);
	initctx->failsafe_callback_eip = (unsigned long) failsafe_callback;
	initctx->failsafe_callback_cs = GSEL(GCODE_SEL, SEL_KPL);

	return;
}
#endif /* __x86_64__ */

int
mp_cpu_start(struct cpu_info *ci, vaddr_t target)
{

	int hyperror;
	struct vcpu_guest_context vcpuctx;

	KASSERT(ci != NULL);
	KASSERT(ci != &cpu_info_primary);
	KASSERT(ci->ci_flags & CPUF_AP);

#ifdef __x86_64__
	xen_init_amd64_vcpuctxt(ci, &vcpuctx, (void (*)(struct cpu_info *))target);
#else  /* i386 */
	xen_init_i386_vcpuctxt(ci, &vcpuctx, (void (*)(struct cpu_info *))target);
#endif /* __x86_64__ */

	/* Initialise the given vcpu to execute cpu_hatch(ci); */
	if ((hyperror = HYPERVISOR_vcpu_op(VCPUOP_initialise, ci->ci_cpuid, &vcpuctx))) {
		aprint_error(": context initialisation failed. errno = %d\n", hyperror);
		return hyperror;
	}

	/* Start it up */

	/* First bring it down */
	if ((hyperror = HYPERVISOR_vcpu_op(VCPUOP_down, ci->ci_cpuid, NULL))) {
		aprint_error(": VCPUOP_down hypervisor command failed. errno = %d\n", hyperror);
		return hyperror;
	}

	if ((hyperror = HYPERVISOR_vcpu_op(VCPUOP_up, ci->ci_cpuid, NULL))) {
		aprint_error(": VCPUOP_up hypervisor command failed. errno = %d\n", hyperror);
		return hyperror;
	}

	if (!vcpu_is_up(ci)) {
		aprint_error(": did not come up\n");
		return -1;
	}

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
	if (vcpu_is_up(ci)) {
		aprint_debug_dev(ci->ci_dev, "is started.\n");
	}
	else {
		aprint_error_dev(ci->ci_dev, "did not start up.\n");
	}

}

/* curcpu() uses %fs - shim for until cpu_init_msrs(), below */
static struct cpu_info *cpu_primary(void)
{
	return &cpu_info_primary;
}
struct cpu_info	* (*xpq_cpu)(void) = cpu_primary;

void
cpu_init_msrs(struct cpu_info *ci, bool full)
{
#ifdef __x86_64__
	if (full) {
		HYPERVISOR_set_segment_base (SEGBASE_FS, 0);
		HYPERVISOR_set_segment_base (SEGBASE_GS_KERNEL, (uint64_t) ci);
		HYPERVISOR_set_segment_base (SEGBASE_GS_USER, 0);
		xpq_cpu = x86_curcpu;
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

void    
cpu_get_tsc_freq(struct cpu_info *ci)
{
	uint32_t vcpu_tversion;
	const volatile vcpu_time_info_t *tinfo = &ci->ci_vcpu->time;

	vcpu_tversion = tinfo->version;
	while (tinfo->version == vcpu_tversion); /* Wait for a time update. XXX: timeout ? */

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
	pd_entry_t *new_pgd;
	struct cpu_info *ci;
	paddr_t l4_pd_ma;

	ci = curcpu();
	l4_pd_ma = xpmap_ptom_masked(ci->ci_kpm_pdirpa);

	/*
	 * Map user space address in kernel space and load
	 * user cr3
	 */
	s = splvm();
	new_pgd = pmap->pm_pdir;

	/* Copy user pmap L4 PDEs (in user addr. range) to per-cpu L4 */
	for (i = 0; i < PDIR_SLOT_PTE; i++) {
		xpq_queue_pte_update(l4_pd_ma + i * sizeof(pd_entry_t), new_pgd[i]);
	}

	if (__predict_true(pmap != pmap_kernel())) {
		xen_set_user_pgd(pmap_pdirpa(pmap, 0));
		ci->ci_xen_current_user_pgd = pmap_pdirpa(pmap, 0);
	}
	else {
		xpq_queue_pt_switch(l4_pd_ma);
		ci->ci_xen_current_user_pgd = 0;
	}

	tlbflush();
	splx(s);

#endif /* __x86_64__ */
}

 /*
  * pmap_cpu_init_late: perform late per-CPU initialization.
  * Short note about percpu PDIR pages:
  * Both the PAE and __x86_64__ architectures have per-cpu PDIR
  * tables. This is to get around Xen's pagetable setup constraints for
  * PAE (multiple L3[3]s cannot point to the same L2 - Xen
  * will refuse to pin a table setup this way.) and for multiple cpus
  * to map in different user pmaps on __x86_64__ (see: cpu_load_pmap())
  *
  * What this means for us is that the PDIR of the pmap_kernel() is
  * considered to be a canonical "SHADOW" PDIR with the following
  * properties: 
  * - Its recursive mapping points to itself
  * - per-cpu recurseive mappings point to themselves
  * - per-cpu L4 pages' kernel entries are expected to be in sync with
  *   the shadow
  * - APDP_PDE_SHADOW accesses the shadow pdir
  * - APDP_PDE accesses the per-cpu pdir
  * - alternate mappings are considered per-cpu - however, x86 pmap
  *   currently partially consults the shadow - this works because the
  *   shadow PDE is updated together with the per-cpu entry (see:
  *   xen_pmap.c: pmap_map_ptes(), and the pmap is locked while the
  * alternate ptes are mapped in.
  */

void
pmap_cpu_init_late(struct cpu_info *ci)
{
#if defined(PAE) || defined(__x86_64__)
	/*
	 * The BP has already its own PD page allocated during early
	 * MD startup.
	 */

	if (ci == &cpu_info_primary)
		return;

	KASSERT(ci != NULL);

#if defined(PAE)
	ci->ci_pae_l3_pdir = (paddr_t *)uvm_km_alloc(kernel_map, PAGE_SIZE, 0,
	    UVM_KMF_WIRED | UVM_KMF_ZERO | UVM_KMF_NOWAIT);

	if (ci->ci_pae_l3_pdir == NULL) {
		panic("%s: failed to allocate L3 per-cpu PD for CPU %d\n",
		      __func__, cpu_index(ci));
	}
	ci->ci_pae_l3_pdirpa = vtophys((vaddr_t) ci->ci_pae_l3_pdir);
	KASSERT(ci->ci_pae_l3_pdirpa != 0);

	/* Initialise L2 entries 0 - 2: Point them to pmap_kernel() */
	ci->ci_pae_l3_pdir[0] =
	    xpmap_ptom_masked(pmap_kernel()->pm_pdirpa[0]) | PG_V;
	ci->ci_pae_l3_pdir[1] =
	    xpmap_ptom_masked(pmap_kernel()->pm_pdirpa[1]) | PG_V;
	ci->ci_pae_l3_pdir[2] =
	    xpmap_ptom_masked(pmap_kernel()->pm_pdirpa[2]) | PG_V;
#endif /* PAE */

	ci->ci_kpm_pdir = (pd_entry_t *)uvm_km_alloc(kernel_map, PAGE_SIZE, 0,
	    UVM_KMF_WIRED | UVM_KMF_ZERO | UVM_KMF_NOWAIT);

	if (ci->ci_kpm_pdir == NULL) {
		panic("%s: failed to allocate L4 per-cpu PD for CPU %d\n",
		      __func__, cpu_index(ci));
	}
	ci->ci_kpm_pdirpa = vtophys((vaddr_t) ci->ci_kpm_pdir);
	KASSERT(ci->ci_kpm_pdirpa != 0);

#if defined(__x86_64__)
	/*
	 * Copy over the pmap_kernel() shadow L4 entries 
	 */

	memcpy(ci->ci_kpm_pdir, pmap_kernel()->pm_pdir, PAGE_SIZE);

	/* Recursive kernel mapping */
	ci->ci_kpm_pdir[PDIR_SLOT_PTE] = xpmap_ptom_masked(ci->ci_kpm_pdirpa) | PG_k | PG_V;
#elif defined(PAE)
	/* Copy over the pmap_kernel() shadow L2 entries that map the kernel */
	memcpy(ci->ci_kpm_pdir, pmap_kernel()->pm_pdir + PDIR_SLOT_KERN, nkptp[PTP_LEVELS - 1] * sizeof(pd_entry_t));
#endif /* __x86_64__ else PAE */

	/* Xen wants R/O */
	pmap_kenter_pa((vaddr_t)ci->ci_kpm_pdir, ci->ci_kpm_pdirpa,
	    VM_PROT_READ, 0);

#if defined(PAE)
	/* Initialise L3 entry 3. This mapping is shared across all
	 * pmaps and is static, ie; loading a new pmap will not update
	 * this entry.
	 */
	
	ci->ci_pae_l3_pdir[3] = xpmap_ptom_masked(ci->ci_kpm_pdirpa) | PG_k | PG_V;

	/* Mark L3 R/O (Xen wants this) */
	pmap_kenter_pa((vaddr_t)ci->ci_pae_l3_pdir, ci->ci_pae_l3_pdirpa,
		VM_PROT_READ, 0);

	xpq_queue_pin_l3_table(xpmap_ptom_masked(ci->ci_pae_l3_pdirpa));

#elif defined(__x86_64__)	
	xpq_queue_pin_l4_table(xpmap_ptom_masked(ci->ci_kpm_pdirpa));
#endif /* PAE */
#endif /* defined(PAE) || defined(__x86_64__) */
}

/*
 * Notify all other cpus to halt.
 */

void
cpu_broadcast_halt(void)
{
	xen_broadcast_ipi(XEN_IPI_HALT);
}

/*
 * Send a dummy ipi to a cpu.
 */

void
cpu_kick(struct cpu_info *ci)
{
	(void)xen_send_ipi(ci, XEN_IPI_KICK);
}
