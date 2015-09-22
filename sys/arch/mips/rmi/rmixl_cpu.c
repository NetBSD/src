/*	$NetBSD: rmixl_cpu.c,v 1.6.6.2 2015/09/22 12:05:47 skrll Exp $	*/

/*
 * Copyright 2002 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Simon Burge for Wasabi Systems, Inc.
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
 *      This product includes software developed for the NetBSD Project by
 *      Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "locators.h"

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: rmixl_cpu.c,v 1.6.6.2 2015/09/22 12:05:47 skrll Exp $");

#include "opt_multiprocessor.h"
#include "opt_ddb.h"

#include <sys/param.h>
#include <sys/device.h>
#include <sys/systm.h>
#include <sys/cpu.h>
#include <sys/lock.h>
#include <sys/lwp.h>
#include <sys/cpu.h>
#include <sys/malloc.h>
#include <uvm/uvm_pglist.h>
#include <uvm/uvm_extern.h>
#include <mips/regnum.h>
#include <mips/asm.h>
#include <mips/pmap.h>
#include <mips/rmi/rmixlreg.h>
#include <mips/rmi/rmixlvar.h>
#include <mips/rmi/rmixl_cpucorevar.h>
#include <mips/rmi/rmixl_cpuvar.h>
#include <mips/rmi/rmixl_intr.h>
#include <mips/rmi/rmixl_fmnvar.h>
#ifdef DDB
#include <mips/db_machdep.h>
#endif


static int	cpu_rmixl_match(device_t, cfdata_t, void *);
static void	cpu_rmixl_attach(device_t, device_t, void *);
static void	cpu_rmixl_attach_primary(struct rmixl_cpu_softc * const);
#ifdef NOTYET
static int	cpu_fmn_intr(void *, rmixl_fmn_rxmsg_t *);
#endif

#ifdef MULTIPROCESSOR
void		cpu_rmixl_hatch(struct cpu_info *);
void		cpu_rmixl_run(struct cpu_info *);
static int	cpu_setup_trampoline_common(struct cpu_info *, struct rmixl_cpu_trampoline_args *);
static void	cpu_setup_trampoline_callback(struct cpu_info *);
#endif	/* MULTIPROCESSOR */

#ifdef DEBUG
void		rmixl_cpu_data_print(struct cpu_data *);
struct cpu_info *
		rmixl_cpuinfo_print(u_int);
#endif	/* DEBUG */

CFATTACH_DECL_NEW(cpu_rmixl, sizeof(struct rmixl_cpu_softc),
	cpu_rmixl_match, cpu_rmixl_attach, NULL, NULL);

#ifdef MULTIPROCESSOR
static struct rmixl_cpu_trampoline_args rmixl_cpu_trampoline_args;
#endif

/*
 * cpu_rmixl_watchpoint_init - initialize COP0 watchpoint stuff
 *
 * clear IEU_DEFEATURE[DBE] to ensure T_WATCH on watchpoint exception
 * set COP0 watchhi and watchlo
 *
 * disable all watchpoints
 */
static void
cpu_rmixl_watchpoint_init(void)
{
	uint32_t r;

	r = rmixl_mfcr(RMIXL_PCR_IEU_DEFEATURE);
	r &= ~__BIT(7);		/* DBE */
	rmixl_mtcr(RMIXL_PCR_IEU_DEFEATURE, r);

	cpuwatch_clr_all();
}

/*
 * cpu_xls616_erratum
 *
 * on the XLS616, COUNT/COMPARE clock regs seem to interact between
 * threads on a core
 *
 * the symptom of the error is retarded clock interrupts
 * and very slow apparent system performance
 *
 * other XLS chips may have the same problem.
 * we may need to add other PID checks.
 */
static inline bool
cpu_xls616_erratum(device_t parent, struct cpucore_attach_args *ca)
{
#if 0
	if (mips_options.mips_cpu->cpu_pid == MIPS_XLS616) {
		if (ca->ca_thread > 0) {
			aprint_error_dev(parent, "XLS616 CLOCK ERRATUM: "
				"deconfigure cpu%d\n", ca->ca_thread);
			return true;
		}
	}
#endif
	return false;
}

static bool
cpu_rmixl_erratum(device_t parent, struct cpucore_attach_args *ca)
{
	return cpu_xls616_erratum(parent, ca);
}

static int
cpu_rmixl_match(device_t parent, cfdata_t cf, void *aux)
{
	struct cpucore_attach_args *ca = aux;
	int thread = cf->cf_loc[CPUCORECF_THREAD];

	if (!cpu_rmixl(mips_options.mips_cpu))
		return 0;

	if (strncmp(ca->ca_name, cf->cf_name, strlen(cf->cf_name)) == 0
#ifndef MULTIPROCESSOR
	    && ca->ca_thread == 0
#endif
	    && (thread == CPUCORECF_THREAD_DEFAULT || thread == ca->ca_thread)
	    && (!cpu_rmixl_erratum(parent, ca)))
			return 1;

	return 0;
}

static void
cpu_rmixl_attach(device_t parent, device_t self, void *aux)
{
	struct rmixl_cpu_softc * const sc = device_private(self);
	struct cpu_info *ci = NULL;
	static bool once = false;
	extern void rmixl_spl_init_cpu(void);

	if (once == false) {
		/* first attach is the primary cpu */
		once = true;
		ci = curcpu();
		sc->sc_dev = self;
		sc->sc_ci = ci;
		ci->ci_softc = (void *)sc;

		rmixl_spl_init_cpu();	/* spl initialization for CPU#0 */
		cpu_rmixl_attach_primary(sc);

#ifdef MULTIPROCESSOR
		mips_locoresw.lsw_cpu_init = cpu_rmixl_hatch;
		mips_locoresw.lsw_cpu_run = cpu_rmixl_run;
	} else {
		struct cpucore_attach_args * const ca = aux;
		struct cpucore_softc * const ccsc = device_private(parent);
		rmixlfw_psb_type_t psb_type = rmixl_configuration.rc_psb_type;
		cpuid_t cpuid;

		KASSERT(ca->ca_core < 8);
		KASSERT(ca->ca_thread < 4);
		cpuid = (ca->ca_core << 2) | ca->ca_thread;
		ci = cpu_info_alloc(ccsc->sc_tlbinfo, cpuid,
		    /* XXX */ 0, ca->ca_core, ca->ca_thread);
		KASSERT(ci != NULL);
		if (ccsc->sc_tlbinfo == NULL)
			ccsc->sc_tlbinfo = ci->ci_tlb_info;
		sc->sc_dev = self;
		sc->sc_ci = ci;
		ci->ci_softc = (void *)sc;

		switch (psb_type) {
		case PSB_TYPE_RMI:
		case PSB_TYPE_DELL:
			cpu_setup_trampoline_callback(ci);
			break;
		default:
			aprint_error(": psb type=%s cpu_wakeup unsupported\n",
				rmixlfw_psb_type_name(psb_type));
			return;
		}

		for (size_t i=0; i < 10000; i++) {
			if (!kcpuset_isset(cpus_hatched, cpu_index(ci)))
				 break;
			DELAY(100);
		}
		if (!kcpuset_isset(cpus_hatched, cpu_index(ci))) {
			aprint_error(": failed to hatch\n");
			return;
		}
#endif	/* MULTIPROCESSOR */
	}

	/*
	 * do per-cpu interrupt initialization
	 */
	rmixl_intr_init_cpu(ci);

	aprint_normal("\n");

        cpu_attach_common(self, ci);
}

/*
 * attach the primary processor
 */
static void
cpu_rmixl_attach_primary(struct rmixl_cpu_softc * const sc)
{
	struct cpu_info *ci = sc->sc_ci;
	uint32_t ebase;

	KASSERT(CPU_IS_PRIMARY(ci));

	/*
	 * obtain and set cpuid of the primary processor
	 */
	asm volatile("dmfc0 %0, $15, 1;" : "=r"(ebase));
	ci->ci_cpuid = ebase & __BITS(9,0);

	cpu_rmixl_watchpoint_init();

	rmixl_fmn_init();

	rmixl_intr_init_clk();
#ifdef MULTIPROCESSOR
	rmixl_intr_init_ipi();
#endif

#ifdef NOTYET
	void *ih = rmixl_fmn_intr_establish(RMIXL_FMN_STID_CORE0,
		cpu_fmn_intr, ci);
	if (ih == NULL)
		panic("%s: rmixl_fmn_intr_establish failed",
			__func__);
	sc->sc_ih_fmn = ih;
#endif
}

#ifdef NOTYET
static int
cpu_fmn_intr(void *arg, rmixl_fmn_rxmsg_t *rxmsg)
{
	if (CPU_IS_PRIMARY(curcpu())) {
		printf("%s: cpu%ld: rxsid=%#x, code=%d, size=%d\n",
			__func__, cpu_number(),
			rxmsg->rxsid, rxmsg->code, rxmsg->size);
		for (int i=0; i < rxmsg->size; i++)
			printf("\t%#"PRIx64"\n", rxmsg->msg.data[i]);
	}

	return 1;
}
#endif

#ifdef MULTIPROCESSOR
/*
 * cpu_rmixl_run
 *
 * - chip-specific post-running code called from cpu_hatch via lsw_cpu_run
 */
void
cpu_rmixl_run(struct cpu_info *ci)
{
	struct rmixl_cpu_softc * const sc = (void *)ci->ci_softc;
	cpucore_rmixl_run(device_parent(sc->sc_dev));
}

/*
 * cpu_rmixl_hatch
 *
 * - chip-specific hatch code called from cpu_hatch via lsw_cpu_init
 */
void
cpu_rmixl_hatch(struct cpu_info *ci)
{
	struct rmixl_cpu_softc * const sc = (void *)ci->ci_softc;
	extern void rmixl_spl_init_cpu(void);

	rmixl_spl_init_cpu();	/* spl initialization for this CPU */

	(void)splhigh();

#ifdef DIAGNOSTIC
	uint32_t ebase = mipsNN_cp0_ebase_read();
	KASSERT((ebase & MIPS_EBASE_CPUNUM) == ci->ci_cpuid);
	KASSERT(curcpu() == ci);
#endif

	cpucore_rmixl_hatch(device_parent(sc->sc_dev));

	cpu_rmixl_watchpoint_init();
}

static int
cpu_setup_trampoline_common(struct cpu_info *ci, struct rmixl_cpu_trampoline_args *ta)
{
	struct lwp *l = ci->ci_data.cpu_idlelwp;
	uintptr_t stacktop;

#ifdef DIAGNOSTIC
	/* Ensure our current stack can be used by the firmware */
	uint64_t sp;
	__asm__ volatile("move	%0, $sp\n" : "=r"(sp));
#ifdef _LP64
	/* can be made into a KSEG0 addr */
	KASSERT(MIPS_XKPHYS_P(sp));
	KASSERT((MIPS_XKPHYS_TO_PHYS(sp) >> 32) == 0);
#else
	/* is a KSEG0 addr */
	KASSERT(MIPS_KSEG0_P(sp));
#endif	/* _LP64 */
#endif	/* DIAGNOSTIC */

#ifndef _LP64
	/*
	 * Ensure 'ci' is a KSEG0 address for trampoline args
	 * to avoid TLB fault in cpu_trampoline() when loading ci_idlelwp
	 */
	KASSERT(MIPS_KSEG0_P(ci));
#endif

	/*
	 * Ensure 'ta' is a KSEG0 address for trampoline args
	 * to avoid TLB fault in trampoline when loading args.
	 *
	 * Note:
	 *   RMI firmware only passes the lower 32-bit half of 'ta'
	 *   to rmixl_cpu_trampoline (the upper half is clear)
	 *   so rmixl_cpu_trampoline must reconstruct the missing upper half
	 *   rmixl_cpu_trampoline "knows" 'ta' is a KSEG0 address
	 *   and sign-extends to make an LP64 KSEG0 address.
	 */
	KASSERT(MIPS_KSEG0_P(ta));

	/*
	 * marshal args for rmixl_cpu_trampoline;
	 * note for non-LP64 kernel, use of intptr_t
	 * forces sign extension of 32 bit pointers
	 */
	stacktop = (uintptr_t)l->l_md.md_utf - CALLFRAME_SIZ;
	ta->ta_sp = (uint64_t)(intptr_t)stacktop;
	ta->ta_lwp = (uint64_t)(intptr_t)l;
	ta->ta_cpuinfo = (uint64_t)(intptr_t)ci;

	return 0;
}

static void
cpu_setup_trampoline_callback(struct cpu_info *ci)
{
	void (*wakeup_cpu)(void *, void *, unsigned int);
	struct rmixl_cpu_trampoline_args *ta = &rmixl_cpu_trampoline_args;
	extern void rmixl_cpu_trampoline(void *);
	extern void rmixlfw_wakeup_cpu(void *, void *, u_int64_t, void *);

	cpu_setup_trampoline_common(ci, ta);

#if _LP64
	wakeup_cpu = (void *)rmixl_configuration.rc_psb_info.wakeup;
#else
	wakeup_cpu = (void *)(intptr_t)
		(rmixl_configuration.rc_psb_info.wakeup & 0xffffffff);
#endif

	rmixlfw_wakeup_cpu(rmixl_cpu_trampoline, (void *)ta,
		(uint64_t)1 << ci->ci_cpuid, wakeup_cpu);
}
#endif	/* MULTIPROCESSOR */


#ifdef DEBUG
void
rmixl_cpu_data_print(struct cpu_data *dp)
{
	printf("cpu_biglock_wanted %p\n", dp->cpu_biglock_wanted);
	printf("cpu_callout %p\n", dp->cpu_callout);
	printf("cpu_unused1 %p\n", dp->cpu_unused1);
	printf("cpu_unused2 %d\n", dp->cpu_unused2);
	printf("&cpu_schedstate %p\n", &dp->cpu_schedstate);	/* TBD */
	printf("&cpu_xcall %p\n", &dp->cpu_xcall);		/* TBD */
	printf("cpu_xcall_pending %d\n", dp->cpu_xcall_pending);
	printf("cpu_onproc %p\n", dp->cpu_onproc);
	printf("cpu_idlelwp %p\n", dp->cpu_idlelwp);
	printf("cpu_lockstat %p\n", dp->cpu_lockstat);
	printf("cpu_index %d\n", dp->cpu_index);
	printf("cpu_biglock_count %d\n", dp->cpu_biglock_count);
	printf("cpu_spin_locks %d\n", dp->cpu_spin_locks);
	printf("cpu_simple_locks %d\n", dp->cpu_simple_locks);
	printf("cpu_spin_locks2 %d\n", dp->cpu_spin_locks2);
	printf("cpu_lkdebug_recurse %d\n", dp->cpu_lkdebug_recurse);
	printf("cpu_softints %d\n", dp->cpu_softints);
	printf("cpu_nsyscall %"PRIu64"\n", dp->cpu_nsyscall);
	printf("cpu_ntrap %"PRIu64"\n", dp->cpu_ntrap);
	printf("cpu_nfault %"PRIu64"\n", dp->cpu_nfault);
	printf("cpu_nintr %"PRIu64"\n", dp->cpu_nintr);
	printf("cpu_nsoft %"PRIu64"\n", dp->cpu_nsoft);
	printf("cpu_nswtch %"PRIu64"\n", dp->cpu_nswtch);
	printf("cpu_uvm %p\n", dp->cpu_uvm);
	printf("cpu_softcpu %p\n", dp->cpu_softcpu);
	printf("&cpu_biodone %p\n", &dp->cpu_biodone);		/* TBD */
	printf("&cpu_percpu %p\n", &dp->cpu_percpu);		/* TBD */
	printf("cpu_selcluster %p\n", dp->cpu_selcluster);
	printf("cpu_nch %p\n", dp->cpu_nch);
	printf("&cpu_ld_locks %p\n", &dp->cpu_ld_locks);	/* TBD */
	printf("&cpu_ld_lock %p\n", &dp->cpu_ld_lock);		/* TBD */
	printf("cpu_cc_freq %#"PRIx64"\n", dp->cpu_cc_freq);
	printf("cpu_cc_skew %#"PRIx64"\n", dp->cpu_cc_skew);
}

struct cpu_info *
rmixl_cpuinfo_print(u_int cpuindex)
{
	struct cpu_info * const ci = cpu_lookup(cpuindex);

	if (ci != NULL) {
		rmixl_cpu_data_print(&ci->ci_data);
		printf("ci_dev %p\n", ci->ci_dev);
		printf("ci_cpuid %ld\n", ci->ci_cpuid);
		printf("ci_cctr_freq %ld\n", ci->ci_cctr_freq);
		printf("ci_cpu_freq %ld\n", ci->ci_cpu_freq);
		printf("ci_cycles_per_hz %ld\n", ci->ci_cycles_per_hz);
		printf("ci_divisor_delay %ld\n", ci->ci_divisor_delay);
		printf("ci_divisor_recip %ld\n", ci->ci_divisor_recip);
		printf("ci_curlwp %p\n", ci->ci_curlwp);
		printf("ci_want_resched %d\n", ci->ci_want_resched);
		printf("ci_mtx_count %d\n", ci->ci_mtx_count);
		printf("ci_mtx_oldspl %d\n", ci->ci_mtx_oldspl);
		printf("ci_idepth %d\n", ci->ci_idepth);
		printf("ci_cpl %d\n", ci->ci_cpl);
		printf("&ci_cpl %p\n", &ci->ci_cpl);	/* XXX */
		printf("ci_next_cp0_clk_intr %#x\n", ci->ci_next_cp0_clk_intr);
		for (int i=0; i < SOFTINT_COUNT; i++)
			printf("ci_softlwps[%d] %p\n", i, ci->ci_softlwps[i]);
		printf("ci_tlb_slot %d\n", ci->ci_tlb_slot);
		printf("ci_pmap_asid_cur %d\n", ci->ci_pmap_asid_cur);
		printf("ci_tlb_info %p\n", ci->ci_tlb_info);
		printf("ci_pmap_kern_segtab %p\n", ci->ci_pmap_kern_segtab);
		printf("ci_pmap_user_segtab %p\n", ci->ci_pmap_user_segtab);
#ifdef _LP64
		printf("ci_pmap_kern_seg0tab %p\n", ci->ci_pmap_kern_seg0tab);
		printf("ci_pmap_user_seg0tab %p\n", ci->ci_pmap_user_seg0tab);
#else
		printf("ci_pmap_srcbase %#"PRIxVADDR"\n", ci->ci_pmap_srcbase);
		printf("ci_pmap_dstbase %#"PRIxVADDR"\n", ci->ci_pmap_dstbase);
#endif
#ifdef MULTIPROCESSOR
		printf("ci_flags %#lx\n", ci->ci_flags);
		printf("ci_request_ipis %#"PRIx64"\n", ci->ci_request_ipis);
		printf("ci_active_ipis %#"PRIx64"\n", ci->ci_active_ipis);
		printf("ci_ksp_tlb_slot %d\n", ci->ci_ksp_tlb_slot);
#endif
	}

	return ci;
}
#endif	/* DEBUG */
