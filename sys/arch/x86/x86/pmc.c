/*	$NetBSD: pmc.c,v 1.11.4.2 2017/12/03 11:36:50 jdolecek Exp $	*/

/*
 * Copyright (c) 2017 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Maxime Villard.
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
 * Copyright (c) 2000 Zembu Labs, Inc.
 * All rights reserved.
 *
 * Author: Jason R. Thorpe <thorpej@zembu.com>
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
 *	This product includes software developed by Zembu Labs, Inc.
 * 4. Neither the name of Zembu Labs nor the names of its employees may
 *    be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY ZEMBU LABS, INC. ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WAR-
 * RANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DIS-
 * CLAIMED.  IN NO EVENT SHALL ZEMBU LABS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Interface to x86 CPU Performance Counters. System-wide only, for now.
 *
 * For each PMC on each CPU, two pieces of information are returned to userland:
 * the number of overflows, and the current value of the PMC. It means that the
 * total number of events for the given PMC on the given CPU is computable the
 * following way:
 *     tot_n_events = NEVENTS_SAMPLE * overfl + ctrval
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: pmc.c,v 1.11.4.2 2017/12/03 11:36:50 jdolecek Exp $");

#include "opt_pmc.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/cpu.h>
#include <sys/xcall.h>
#include <sys/kauth.h>

#include <machine/cpufunc.h>
#include <machine/cpuvar.h>
#include <machine/specialreg.h>
#include <machine/sysarch.h>
#include <machine/pmc.h>
#include <machine/cputypes.h>
#include <machine/i82489reg.h>
#include <machine/i82489var.h>

#include <x86/nmi.h>

#define NEVENTS_SAMPLE	500000

/*
 * Structure describing a PMC.
 */
typedef struct {
	bool running;
	size_t n;		/* pmc number */
	uint32_t evtmsr;	/* event selector MSR */
	uint64_t evtval;	/* event selector value */
	uint32_t ctrmsr;	/* counter MSR */
	uint64_t ctrinitval;	/* initial counter value */
	uint64_t ctrmaxval;	/* maximal counter value */
	uint64_t ctrmask;
} pmc_state_t;

/*
 * Per-CPU structure that describes the values of each PMC, plus the state
 * of the LAPIC before enabling PMCs.
 */
typedef struct {
	x86_pmc_cpuval_t val[PMC_NCOUNTERS];	/* values returned to user */ 
	uint64_t nmioverfl[PMC_NCOUNTERS];	/* incremented by NMI intr */
	uint32_t lapic_image;			/* saved content of LAPIC */
} pmc_cpu_t;

static pmc_state_t pmc_state[PMC_NCOUNTERS];
static pmc_cpu_t pmc_cpu[MAXCPUS];

static nmi_handler_t *pmc_nmi_handle;
static kmutex_t pmc_lock;

static uint32_t pmc_ncounters __read_mostly;
static int pmc_type __read_mostly;

/*
 * Handle PMC overflows. Called from NMI interrupt context, with interrupts
 * disabled.
 */
static int
pmc_nmi(const struct trapframe *tf, void *dummy)
{
	struct cpu_info *ci = curcpu();
	pmc_state_t *pmc = NULL;
	pmc_cpu_t *cpu;
	uint64_t ctr;
	size_t i;

	if (pmc_type == PMC_TYPE_NONE) {
		return 0;
	}
	for (i = 0; i < pmc_ncounters; i++) {
		pmc = &pmc_state[i];
		if (!pmc->running) {
			continue;
		}
		ctr = rdmsr(pmc->ctrmsr);
		/* If the highest bit is zero, then it's this PMC */
		if ((ctr & ((pmc->ctrmask + 1) >> 1)) != 0) {
			continue;
		}
		break;
	}
	if (i == pmc_ncounters) {
		return 0;
	}

	/* Count the overflow, and restart the counter */
	cpu = &pmc_cpu[cpu_index(ci)];
	cpu->nmioverfl[i]++;
	wrmsr(pmc->ctrmsr, pmc->ctrinitval);

	return 1;
}

static void
pmc_read_cpu(void *arg1, void *arg2)
{
	pmc_state_t *pmc = (pmc_state_t *)arg1;
	struct cpu_info *ci = curcpu();
	pmc_cpu_t *cpu = &pmc_cpu[cpu_index(ci)];
	uint64_t evtmsr, en;

	switch (pmc_type) {
	case PMC_TYPE_I686:
		en = PMC6_EVTSEL_EN;
		break;

	case PMC_TYPE_K7:
		en = K7_EVTSEL_EN;
		break;

	case PMC_TYPE_F10H:
		en = F10H_EVTSEL_EN;
		break;
	}

	evtmsr = rdmsr(pmc->evtmsr);

	/*
	 * Quickly disable the counter, to avoid getting an NMI after setting
	 * ctrval.
	 */
	wrmsr(pmc->evtmsr, evtmsr & ~en);

	cpu->val[pmc->n].ctrval =
	    (rdmsr(pmc->ctrmsr) & pmc->ctrmask) - pmc->ctrinitval;
	cpu->val[pmc->n].overfl = cpu->nmioverfl[pmc->n];

	/* Re-enable the counter */
	wrmsr(pmc->evtmsr, evtmsr);
}

static void
pmc_apply_cpu(void *arg1, void *arg2)
{
	pmc_state_t *pmc = (pmc_state_t *)arg1;
	bool start = (bool)arg2;
	struct cpu_info *ci = curcpu();
	pmc_cpu_t *cpu = &pmc_cpu[cpu_index(ci)];

	if (start) {
		cpu->lapic_image = lapic_readreg(LAPIC_PCINT);
		cpu->val[pmc->n].ctrval = 0;
		cpu->val[pmc->n].overfl = 0;
		cpu->nmioverfl[pmc->n] = 0;

		lapic_writereg(LAPIC_PCINT, LAPIC_DLMODE_NMI);
	}

	wrmsr(pmc->ctrmsr, pmc->ctrinitval);
	switch (pmc_type) {
	case PMC_TYPE_I686:
	case PMC_TYPE_K7:
	case PMC_TYPE_F10H:
		wrmsr(pmc->evtmsr, pmc->evtval);
		break;
	}

	if (!start) {
		lapic_writereg(LAPIC_PCINT, cpu->lapic_image);
	}
}

static void
pmc_read(pmc_state_t *pmc)
{
	uint64_t xc;

	xc = xc_broadcast(0, pmc_read_cpu, pmc, NULL);
	xc_wait(xc);
}

static void
pmc_apply(pmc_state_t *pmc, bool start)
{
	uint64_t xc;

	xc = xc_broadcast(0, pmc_apply_cpu, pmc, (void *)start);
	xc_wait(xc);

	pmc->running = start;
}

static void
pmc_start(pmc_state_t *pmc, struct x86_pmc_startstop_args *args)
{
	uint64_t event, unit;

	/*
	 * Initialize the counter MSR.
	 */
	pmc->ctrinitval = pmc->ctrmaxval - NEVENTS_SAMPLE;

	/*
	 * Initialize the event MSR.
	 */
	switch (pmc_type) {
	case PMC_TYPE_I686:
		pmc->evtval = args->event | PMC6_EVTSEL_EN | PMC6_EVTSEL_INT |
		    (args->unit << PMC6_EVTSEL_UNIT_SHIFT) |
		    ((args->flags & PMC_SETUP_KERNEL) ? PMC6_EVTSEL_OS : 0) |
		    ((args->flags & PMC_SETUP_USER) ? PMC6_EVTSEL_USR : 0) |
		    ((args->flags & PMC_SETUP_EDGE) ? PMC6_EVTSEL_E : 0) |
		    ((args->flags & PMC_SETUP_INV) ? PMC6_EVTSEL_INV : 0) |
		    (args->compare << PMC6_EVTSEL_COUNTER_MASK_SHIFT);
		break;

	case PMC_TYPE_K7:
		event = (args->event & K7_EVTSEL_EVENT);
		unit = (args->unit << K7_EVTSEL_UNIT_SHIFT) &
		    K7_EVTSEL_UNIT;
		pmc->evtval = event | unit | K7_EVTSEL_EN | K7_EVTSEL_INT |
		    ((args->flags & PMC_SETUP_KERNEL) ? K7_EVTSEL_OS : 0) |
		    ((args->flags & PMC_SETUP_USER) ? K7_EVTSEL_USR : 0) |
		    ((args->flags & PMC_SETUP_EDGE) ? K7_EVTSEL_E : 0) |
		    ((args->flags & PMC_SETUP_INV) ? K7_EVTSEL_INV : 0) |
		    (args->compare << K7_EVTSEL_COUNTER_MASK_SHIFT);
		break;

	case PMC_TYPE_F10H:
		event =
		    ((uint64_t)(args->event & 0x00FF) <<
		     F10H_EVTSEL_EVENT_SHIFT_LOW) |
		    ((uint64_t)(args->event & 0x0F00) <<
		     F10H_EVTSEL_EVENT_SHIFT_HIGH);
		unit = (args->unit << F10H_EVTSEL_UNIT_SHIFT) &
		    F10H_EVTSEL_UNIT_MASK;
		pmc->evtval = event | unit | F10H_EVTSEL_EN | F10H_EVTSEL_INT |
		    ((args->flags & PMC_SETUP_KERNEL) ? F10H_EVTSEL_OS : 0) |
		    ((args->flags & PMC_SETUP_USER) ? F10H_EVTSEL_USR : 0) |
		    ((args->flags & PMC_SETUP_EDGE) ? F10H_EVTSEL_EDGE : 0) |
		    ((args->flags & PMC_SETUP_INV) ? F10H_EVTSEL_INV : 0) |
		    (args->compare << F10H_EVTSEL_COUNTER_MASK_SHIFT);
		break;
	}

	/*
	 * Apply the changes.
	 */
	pmc_apply(pmc, true);
}

static void
pmc_stop(pmc_state_t *pmc, struct x86_pmc_startstop_args *args)
{
	pmc->evtval = 0;
	pmc->ctrinitval = 0;
	pmc_apply(pmc, false);
}

void
pmc_init(void)
{
	const char *cpu_vendorstr;
	struct cpu_info *ci;
	size_t i;

	pmc_type = PMC_TYPE_NONE;

	if (cpu_class != CPUCLASS_686)
		return;

	ci = curcpu();
	cpu_vendorstr = (char *)ci->ci_vendor;

	if (strncmp(cpu_vendorstr, "GenuineIntel", 12) == 0) {
		/* Right now we're missing Pentium 4 support. */
		if (cpuid_level == -1 ||
		    CPUID_TO_FAMILY(ci->ci_signature) == CPU_FAMILY_P4)
			return;
		pmc_type = PMC_TYPE_I686;
		pmc_ncounters = 2;
		for (i = 0; i < pmc_ncounters; i++) {
			pmc_state[i].n = i;
			pmc_state[i].evtmsr = MSR_EVNTSEL0 + i;
			pmc_state[i].ctrmsr = MSR_PERFCTR0 + i;
			pmc_state[i].ctrmaxval = (UINT64_C(1) << 40) - 1;
			pmc_state[i].ctrmask = 0xFFFFFFFFFFULL;
		}
	} else if (strncmp(cpu_vendorstr, "AuthenticAMD", 12) == 0) {
		if (CPUID_TO_FAMILY(ci->ci_signature) == 0x10) {
			pmc_type = PMC_TYPE_F10H;
			pmc_ncounters = 4;
			for (i = 0; i < pmc_ncounters; i++) {
				pmc_state[i].n = i;
				pmc_state[i].evtmsr = MSR_F10H_EVNTSEL0 + i;
				pmc_state[i].ctrmsr = MSR_F10H_PERFCTR0 + i;
				pmc_state[i].ctrmaxval =
				    (UINT64_C(1) << 48) - 1;
				pmc_state[i].ctrmask = 0xFFFFFFFFFFFFULL;
			}
		} else {
			/* XXX: make sure it is at least K7 */
			pmc_type = PMC_TYPE_K7;
			pmc_ncounters = 4;
			for (i = 0; i < pmc_ncounters; i++) {
				pmc_state[i].n = i;
				pmc_state[i].evtmsr = MSR_K7_EVNTSEL0 + i;
				pmc_state[i].ctrmsr = MSR_K7_PERFCTR0 + i;
				pmc_state[i].ctrmaxval =
				    (UINT64_C(1) << 48) - 1;
				pmc_state[i].ctrmask = 0xFFFFFFFFFFFFULL;
			}
		}
	}

	pmc_nmi_handle = nmi_establish(pmc_nmi, NULL);
	mutex_init(&pmc_lock, MUTEX_DEFAULT, IPL_NONE);
}

int
sys_pmc_info(struct lwp *l, struct x86_pmc_info_args *uargs, register_t *retval)
{
	struct x86_pmc_info_args rv;
	int error;

	error = kauth_authorize_machdep(l->l_cred, KAUTH_MACHDEP_X86PMC,
	    NULL, NULL, NULL, NULL);
	if (error)
		return error;

	memset(&rv, 0, sizeof(rv));

	rv.vers = PMC_VERSION;
	rv.type = pmc_type;
	rv.nctrs = pmc_ncounters;
	rv.nsamp = NEVENTS_SAMPLE;

	return copyout(&rv, uargs, sizeof(rv));
}

int
sys_pmc_startstop(struct lwp *l, struct x86_pmc_startstop_args *uargs,
    register_t *retval)
{
	struct x86_pmc_startstop_args args;
	pmc_state_t *pmc;
	bool start;
	int error;

	error = kauth_authorize_machdep(l->l_cred, KAUTH_MACHDEP_X86PMC,
	    NULL, NULL, NULL, NULL);
	if (error)
		return error;

	if (pmc_type == PMC_TYPE_NONE)
		return ENODEV;

	error = copyin(uargs, &args, sizeof(args));
	if (error)
		return error;

	if (args.counter >= pmc_ncounters)
		return EINVAL;

	start = (args.flags & (PMC_SETUP_KERNEL|PMC_SETUP_USER)) != 0;
	pmc = &pmc_state[args.counter];

	mutex_enter(&pmc_lock);

	if (start && pmc->running) {
		mutex_exit(&pmc_lock);
		return EBUSY;
	} else if (!start && !pmc->running) {
		mutex_exit(&pmc_lock);
		return 0;
	}

	if (start) {
		pmc_start(pmc, &args);
	} else {
		pmc_stop(pmc, &args);
	}

	mutex_exit(&pmc_lock);

	return 0;
}

int
sys_pmc_read(struct lwp *l, struct x86_pmc_read_args *uargs, register_t *retval)
{
	struct x86_pmc_read_args args;
	pmc_state_t *pmc;
	pmc_cpu_t *cpu;
	size_t i, nval;
	int error;

	error = kauth_authorize_machdep(l->l_cred, KAUTH_MACHDEP_X86PMC,
	    NULL, NULL, NULL, NULL);
	if (error)
		return error;

	if (pmc_type == PMC_TYPE_NONE)
		return ENODEV;

	error = copyin(uargs, &args, sizeof(args));
	if (error)
		return error;

	if (args.counter >= pmc_ncounters)
		return EINVAL;
	if (args.values == NULL)
		return EINVAL;
	nval = MIN(ncpu, args.nval);

	pmc = &pmc_state[args.counter];

	mutex_enter(&pmc_lock);

	if (pmc->running) {
		pmc_read(pmc);

		for (i = 0; i < nval; i++) {
			cpu = &pmc_cpu[i];

			error = copyout(&cpu->val[pmc->n], args.values + i,
			    sizeof(x86_pmc_cpuval_t));
			if (error)
				break;
		}

		args.nval = nval;
	} else {
		error = ENOENT;
	}

	mutex_exit(&pmc_lock);

	if (error)
		return error;

	return copyout(&args, uargs, sizeof(args));
}
