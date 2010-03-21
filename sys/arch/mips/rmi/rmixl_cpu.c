/*	$NetBSD: rmixl_cpu.c,v 1.1.2.7 2010/03/21 22:03:16 cliff Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: rmixl_cpu.c,v 1.1.2.7 2010/03/21 22:03:16 cliff Exp $");

#include <sys/param.h>
#include <sys/device.h>
#include <sys/systm.h>
#include <sys/cpu.h>
#include <sys/lock.h>
#include <sys/lwp.h>
#include <sys/cpu.h>
#include <sys/user.h>
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


static int	cpu_rmixl_match(device_t, cfdata_t, void *);
static void	cpu_rmixl_attach(device_t, device_t, void *);
static void	cpu_rmixl_attach_once(struct rmixl_cpu_softc * const);
#ifdef NOTYET
static int	cpu_fmn_intr(void *, rmixl_fmn_rxmsg_t *);
#endif

#ifdef MULTIPROCESSOR
void		cpu_rmixl_hatch(struct cpu_info *);
#if 0
static void	cpu_setup_trampoline_ipi(struct device *, struct cpu_info *);
#endif
static void	cpu_setup_trampoline_callback(struct device *, struct cpu_info *);
static void	cpu_setup_trampoline_fmn(struct device *, struct cpu_info *);
#ifdef DEBUG
void		rmixl_cpu_data_print(struct cpu_data *);
struct cpu_info *
		rmixl_cpuinfo_print(cpuid_t);
#endif	/* DEBUG */
#endif	/* MULTIPROCESSOR */

CFATTACH_DECL_NEW(cpu_rmixl, sizeof(struct rmixl_cpu_softc),
	cpu_rmixl_match, cpu_rmixl_attach, NULL, NULL); 

#ifdef MULTIPROCESSOR
static struct rmixl_cpu_trampoline_args rmixl_cpu_trampoline_args;
#endif

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
	    && (thread == CPUCORECF_THREAD_DEFAULT || thread == ca->ca_thread))
			return 1;

	return 0;
}

static void
cpu_rmixl_attach(device_t parent, device_t self, void *aux)
{
	struct rmixl_cpu_softc * const sc = device_private(self);
	struct cpucore_attach_args *ca = aux;
	struct cpu_info *ci = NULL;

	if (ca->ca_thread == 0 && ca->ca_core == 0) {
		ci = curcpu();
		sc->sc_dev = self;
		sc->sc_ci = ci;
		ci->ci_softc = (void *)sc;

		cpu_rmixl_attach_once(sc);

#ifdef MULTIPROCESSOR
		mips_locoresw.lsw_cpu_init = cpu_rmixl_hatch;
	} else {
		struct cpucore_softc * const ccsc = device_private(parent);
		rmixlfw_psb_type_t psb_type = rmixl_configuration.rc_psb_type;
		cpuid_t cpuid;
		int s;

		s = splhigh();

		KASSERT(ca->ca_core < 8);
		KASSERT(ca->ca_thread < 4);
		cpuid = (ca->ca_core << 2) | ca->ca_thread;
		ci = cpu_info_alloc(ccsc->sc_tlbinfo, cpuid);
		KASSERT(ci != NULL);
		sc->sc_dev = self;
		sc->sc_ci = ci;
		ci->ci_softc = (void *)sc;

		switch (psb_type) {
		case PSB_TYPE_RMI:
			cpu_setup_trampoline_callback(self, ci);
			break;
		case PSB_TYPE_DELL:
			cpu_setup_trampoline_fmn(self, ci);
			break;
		default:
			aprint_error(": psb type=%s cpu_wakeup unsupported\n",
				rmixlfw_psb_type_name(psb_type));
			return;
		}

		const u_long cpu_mask = 1L << cpu_index(ci);
		for (size_t i=0; i < 10000; i++) {
			if ((cpus_hatched & cpu_mask) != 0)
				 break;
			DELAY(100);
		}
		if ((cpus_hatched & cpu_mask) == 0) {
			aprint_error(": failed to hatch\n");
			return;
		}

		splx(s);

#endif	/* MULTIPROCESSOR */
	}

	/*
	 * do per-cpu interrupt initialization
	 */
	rmixl_intr_init_cpu(ci);

	aprint_normal("\n");

        cpu_attach_common(self, ci);
}

static void
cpu_rmixl_attach_once(struct rmixl_cpu_softc * const sc)
{
	static bool once = false;

	KASSERT("once != true");
	if (once == true)
		return;
	once = true;

	rmixl_fmn_init();

	sc->sc_ih_clk = rmixl_intr_init_clk();
#ifdef MULTIPROCESSOR
	sc->sc_ih_ipi = rmixl_intr_init_ipi();
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
	if (cpu_number() == 0) {
		printf("%s: cpu %ld: rxsid=%#x, code=%d, size=%d\n",
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
 * cpu_rmixl_hatch
 *
 * - chip-specific hatch code called from cpu_hatch via lsw_cpu_init
 */
void
cpu_rmixl_hatch(struct cpu_info *ci)
{
	(void)splhigh();

#ifdef DEBUG
	uint32_t ebase;
	asm volatile("dmfc0 %0, $15, 1;" : "=r"(ebase));
	KASSERT((ebase & __BITS(9,0)) == ci->ci_cpuid);
	KASSERT(curcpu() == ci);
#endif

	if (RMIXL_CPU_THREAD(ci->ci_cpuid) == 0)
		rmixl_fmn_init_core();
}

#ifdef NOTYET
static void
cpu_setup_trampoline_ipi(struct device *self, struct cpu_info *ci)
{
	volatile struct rmixlfw_cpu_wakeup_info *wip;
	u_int cpu, core, thread;
	uint32_t ipi;
	int32_t addr;
	uint64_t gp;
	uint64_t sp;
	uint32_t mask;
	volatile uint32_t *maskp;
	__cpu_simple_lock_t *llk;
	volatile uint32_t *xflag;			/* ??? */
	extern void rmixl_cpu_trampoline(void *);

	cpu = ci->ci_cpuid;
	core = cpu >> 2;
	thread = cpu & __BITS(1,0);
printf("\n%s: cpu %d, core %d, thread %d\n", __func__, cpu, core, thread);

	wip = &rmixl_configuration.rc_cpu_wakeup_info[cpu];
printf("%s: wip %p\n", __func__, wip);

	llk = (__cpu_simple_lock_t *)(intptr_t)wip->loader_lock;
printf("%s: llk %p: %#x\n", __func__, llk, *llk);

	/* XXX WTF */
	xflag = (volatile uint32_t *)(intptr_t)(wip->loader_lock + 0x2c);
printf("%s: xflag %p, %#x\n", __func__, xflag, *xflag);

	ipi = (thread << RMIXL_PIC_IPIBASE_ID_THREAD_SHIFT)
	    | (core << RMIXL_PIC_IPIBASE_ID_CORE_SHIFT)
	    | RMIXLFW_IPI_WAKEUP;
printf("%s: ipi %#x\n", __func__, ipi);

	/* entry addr must be uncached, use KSEG1 */
	addr = (int32_t)MIPS_PHYS_TO_KSEG1(
			MIPS_KSEG0_TO_PHYS(rmixl_cpu_trampoline));
printf("%s: addr %#x\n", __func__, addr);

	__asm__ volatile("move	%0, $gp\n" : "=r"(gp));
printf("%s: gp %#"PRIx64"\n", __func__, gp);

	sp = (256 * 1024) - 32;			/* XXX TMP FIXME */
	sp = MIPS_PHYS_TO_KSEG1(sp);
printf("%s: sp %#"PRIx64"\n", __func__, sp);

	maskp = (uint32_t *)(intptr_t)wip->global_wakeup_mask;
printf("%s: maskp %p\n", __func__, maskp);

	__cpu_simple_lock(llk);
printf("%s: llk %p: %#x\n", __func__, llk, *llk);

	wip->entry.addr = addr;
	wip->entry.args = 0;
if (0) {
	wip->entry.sp = sp;
	wip->entry.gp = gp;
}

	mask = *maskp;
	mask |= 1 << cpu;
	*maskp = mask;

#if 0
	*xflag = mask;	/* XXX */
#endif

	RMIXL_IOREG_WRITE(RMIXL_PIC_IPIBASE, ipi);

	__cpu_simple_unlock(llk);
printf("%s: llk %p: %#x\n", __func__, llk, *llk);

	Debugger();
}
#endif	/* NOTYET */

static uint64_t argv[4] = { 0x1234,  0x2345, 0x3456, 0x4567 };	/* XXX TMP */

static void
cpu_setup_trampoline_callback(struct device *self, struct cpu_info *ci)
{
	void (*wakeup_cpu)(void *, void *, unsigned int);
	extern void rmixl_cpu_trampoline(void *);
	extern void rmixlfw_wakeup_cpu(void *, void *, u_int64_t, void *);
	struct lwp *l = ci->ci_data.cpu_idlelwp;
	struct rmixl_cpu_trampoline_args *ta = &rmixl_cpu_trampoline_args;
	uintptr_t stacktop;
 
	/*
	 * Ensure 'ta' is a KSEG0 address for trampoline args
	 * to avoid TLB fault in trampoline when loading args.
	 *
	 * Note:
	 *   RMI firmware only passes the lower half of 'ta'
	 *   to rmixl_cpu_trampoline (the upper half is clear)
	 *   so rmixl_cpu_trampoline must reconstruct the missing upper half
	 *   rmixl_cpu_trampoline "knows" to use MIPS_KSEG0_START
	 *   to reconstruct upper half of 'ta'.
	 */
	KASSERT(MIPS_KSEG0_P(ta));

	/*
	 * marshall args for rmixl_cpu_trampoline,
	 */
	stacktop = (uintptr_t)l->l_addr + USPACE
			 - sizeof(struct trapframe) - CALLFRAME_SIZ;
	ta->ta_sp = (uint64_t)(uintptr_t)stacktop;
	ta->ta_lwp = (uint64_t)(uintptr_t)l;
	ta->ta_cpuinfo = (uint64_t)(uintptr_t)ci;

#if _LP64
	wakeup_cpu = (void *)rmixl_configuration.rc_psb_info.wakeup;
#else
	wakeup_cpu = (void *)(intptr_t)
		(rmixl_configuration.rc_psb_info.wakeup & 0xffffffff);
#endif

	rmixlfw_wakeup_cpu(rmixl_cpu_trampoline, (void *)ta,
		1 << ci->ci_cpuid, wakeup_cpu);
}

static void
cpu_setup_trampoline_fmn(struct device *self, struct cpu_info *ci)
{
	rmixl_fmn_msg_t msg;
	intptr_t sp;
	extern void rmixl_cpu_trampoline(void *);

	sp = (intptr_t)malloc(4096, M_DEVBUF, M_NOWAIT);
	if (sp == 0)
		panic("%s: cannot malloc size 4096", __func__);

	msg.data[0] = (uint64_t)sp + 4096 - 32;
	msg.data[1] = (uint64_t)sp;
	msg.data[2] = (uint64_t)rmixl_cpu_trampoline;
	msg.data[3] = (uint64_t)argv;		/* XXX TMP DEBUG */

	msg.data[0] |= 0xffffffff00000000ULL;
	msg.data[1] |= 0xffffffff00000000ULL;
	msg.data[2] |= 0xffffffff00000000ULL;
	msg.data[3] |= 0xffffffff00000000ULL;

	rmixl_fmn_msg_send(4, RMIXL_FMN_CODE_PSB_WAKEUP,
		RMIXL_FMN_CORE_DESTID(ci->ci_cpuid, 0), &msg);	/* XXX FIXME */
}

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
	printf("&cpu_qchain %p\n", &dp->cpu_qchain);		/* TBD */
	printf("cpu_idlelwp %p\n", dp->cpu_idlelwp);
	printf("cpu_lockstat %p\n", dp->cpu_lockstat);
	printf("cpu_index %d\n", dp->cpu_index);
	printf("cpu_biglock_count %d\n", dp->cpu_biglock_count);
	printf("cpu_spin_locks %d\n", dp->cpu_spin_locks);
	printf("cpu_simple_locks %d\n", dp->cpu_simple_locks);
	printf("cpu_spin_locks2 %d\n", dp->cpu_spin_locks2);
	printf("cpu_lkdebug_recurse %d\n", dp->cpu_lkdebug_recurse);
	printf("cpu_softints %d\n", dp->cpu_softints);
	printf("cpu_nsyscall %d\n", dp->cpu_nsyscall);
	printf("cpu_ntrap %d\n", dp->cpu_ntrap);
	printf("cpu_nswtch %d\n", dp->cpu_nswtch);
	printf("cpu_uvm %p\n", dp->cpu_uvm);
	printf("cpu_softcpu %p\n", dp->cpu_softcpu);
	printf("&cpu_biodone %p\n", &dp->cpu_biodone);		/* TBD */
	printf("&cpu_percpu %p\n", &dp->cpu_percpu);		/* TBD */
	printf("cpu_selcpu %p\n", dp->cpu_selcpu);
	printf("cpu_nch %p\n", dp->cpu_nch);
	printf("&cpu_ld_locks %p\n", &dp->cpu_ld_locks);	/* TBD */
	printf("&cpu_ld_lock %p\n", &dp->cpu_ld_lock);		/* TBD */
	printf("cpu_cc_freq %#"PRIx64"\n", dp->cpu_cc_freq);
	printf("cpu_cc_skew %#"PRIx64"\n", dp->cpu_cc_skew);
}

struct cpu_info *
rmixl_cpuinfo_print(cpuid_t cpuid)
{
	struct cpu_info * const ci = cpu_lookup(cpuid);

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
		printf("ci_tlb_info %p\n", ci->ci_tlb_info);
		printf("ci_pmap_segbase %p\n", ci->ci_pmap_segbase);
		printf("ci_pmap_srcbase %#"PRIxVADDR"\n", ci->ci_pmap_srcbase);
		printf("ci_pmap_dstbase %#"PRIxVADDR"\n", ci->ci_pmap_dstbase);
#ifdef MULTIPROCESSOR
		printf("ci_flags %#lx\n", ci->ci_flags);
		printf("ci_request_ipis %#"PRIx64"\n", ci->ci_request_ipis);
		printf("ci_active_ipis %#"PRIx64"\n", ci->ci_active_ipis);
		printf("ci_ksp_tlb_slot %d\n", ci->ci_ksp_tlb_slot);
		printf("ci_fpsave_si %p\n", ci->ci_fpsave_si);
#endif
	}

	return ci;
}

#endif	/* DEBUG */
#endif	/* MULTIPROCESSOR */
