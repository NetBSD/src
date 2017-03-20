/*	$NetBSD: pmc.c,v 1.3.2.2 2017/03/20 06:57:22 pgoyette Exp $	*/

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
 * Interface to x86 CPU Performance Counters.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: pmc.c,v 1.3.2.2 2017/03/20 06:57:22 pgoyette Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/cpu.h>
#include <sys/xcall.h>

#include <machine/cpufunc.h>
#include <machine/cpuvar.h>
#include <machine/specialreg.h>
#include <machine/sysarch.h>
#include <machine/pmc.h>
#include <machine/cpu_counter.h>
#include <machine/cputypes.h>

typedef struct {
	bool running;
	uint32_t evtmsr;	/* event selector MSR */
	uint64_t evtval;	/* event selector value */
	uint32_t ctrmsr;	/* counter MSR */
	uint64_t ctrval;	/* initial counter value */
} pmc_state_t;

static x86_pmc_cpuval_t pmc_val_cpus[MAXCPUS] __aligned(CACHE_LINE_SIZE);
static kmutex_t pmc_lock;

static pmc_state_t pmc_state[PMC_NCOUNTERS];
static int pmc_ncounters __read_mostly;
static int pmc_type __read_mostly;

static void
pmc_read_cpu(void *arg1, void *arg2)
{
	pmc_state_t *pmc = (pmc_state_t *)arg1;
	struct cpu_info *ci = curcpu();

	pmc_val_cpus[cpu_index(ci)].ctrval =
	    rdmsr(pmc->ctrmsr) & 0xffffffffffULL;
	pmc_val_cpus[cpu_index(ci)].overfl = 0;
}

static void
pmc_read(pmc_state_t *pmc)
{
	uint64_t xc;

	xc = xc_broadcast(0, pmc_read_cpu, pmc, NULL);
	xc_wait(xc);
}

static void
pmc_apply_cpu(void *arg1, void *arg2)
{
	pmc_state_t *pmc = (pmc_state_t *)arg1;
	struct cpu_info *ci = curcpu();

	wrmsr(pmc->ctrmsr, pmc->ctrval);
	switch (pmc_type) {
	case PMC_TYPE_I586:
		wrmsr(MSR_CESR, pmc_state[0].evtval |
		    (pmc_state[1].evtval << 16));
		break;

	case PMC_TYPE_I686:
	case PMC_TYPE_K7:
	case PMC_TYPE_F10H:
		wrmsr(pmc->evtmsr, pmc->evtval);
		break;
	}

	pmc_val_cpus[cpu_index(ci)].ctrval = 0;
	pmc_val_cpus[cpu_index(ci)].overfl = 0;
}

static void
pmc_apply(pmc_state_t *pmc)
{
	uint64_t xc;

	xc = xc_broadcast(0, pmc_apply_cpu, pmc, NULL);
	xc_wait(xc);
}

static void
pmc_start(pmc_state_t *pmc, struct x86_pmc_startstop_args *args)
{
	uint64_t event, unit;

	pmc->running = true;

	/*
	 * Initialize the counter MSR.
	 */
	pmc->ctrval = args->val;

	/*
	 * Initialize the event MSR.
	 */
	switch (pmc_type) {
	case PMC_TYPE_I586:
		pmc->evtval = args->event |
		    ((args->flags & PMC_SETUP_KERNEL) ? PMC5_CESR_OS : 0) |
		    ((args->flags & PMC_SETUP_USER) ? PMC5_CESR_USR : 0) |
		    ((args->flags & PMC_SETUP_EDGE) ? PMC5_CESR_E : 0);
		break;

	case PMC_TYPE_I686:
		pmc->evtval = args->event | PMC6_EVTSEL_EN |
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
		pmc->evtval = event | unit | K7_EVTSEL_EN |
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
		pmc->evtval = event | unit | F10H_EVTSEL_EN |
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
	pmc_apply(pmc);
}

static void
pmc_stop(pmc_state_t *pmc, struct x86_pmc_startstop_args *args)
{
	pmc->running = false;
	pmc->evtval = 0;
	pmc->ctrval = 0;
	pmc_apply(pmc);
}

void
pmc_init(void)
{
	const char *cpu_vendorstr;
	struct cpu_info *ci;

	pmc_type = PMC_TYPE_NONE;

	ci = curcpu();
	cpu_vendorstr = (char *)ci->ci_vendor;

	switch (cpu_class) {
	case CPUCLASS_586:
		if (strncmp(cpu_vendorstr, "GenuineIntel", 12) == 0) {
			pmc_type = PMC_TYPE_I586;
			pmc_ncounters = 2;
			pmc_state[0].ctrmsr = MSR_CTR0;
			pmc_state[1].ctrmsr = MSR_CTR1;
			break;
		}

	case CPUCLASS_686:
		if (strncmp(cpu_vendorstr, "GenuineIntel", 12) == 0) {
			/* Right now we're missing Pentium 4 support. */
			if (cpuid_level == -1 ||
			    CPUID_TO_FAMILY(ci->ci_signature) == CPU_FAMILY_P4)
				break;
			pmc_type = PMC_TYPE_I686;
			pmc_ncounters = 2;
			pmc_state[0].evtmsr = MSR_EVNTSEL0;
			pmc_state[0].ctrmsr = MSR_PERFCTR0;
			pmc_state[1].evtmsr = MSR_EVNTSEL1;
			pmc_state[1].ctrmsr = MSR_PERFCTR1;
		} else if (strncmp(cpu_vendorstr, "AuthenticAMD", 12) == 0) {
			if (CPUID_TO_FAMILY(ci->ci_signature) == 0x10) {
				pmc_type = PMC_TYPE_F10H;
				pmc_ncounters = 4;
				pmc_state[0].evtmsr = MSR_F10H_EVNTSEL0;
				pmc_state[0].ctrmsr = MSR_F10H_PERFCTR0;
				pmc_state[1].evtmsr = MSR_F10H_EVNTSEL1;
				pmc_state[1].ctrmsr = MSR_F10H_PERFCTR1;
				pmc_state[2].evtmsr = MSR_F10H_EVNTSEL2;
				pmc_state[2].ctrmsr = MSR_F10H_PERFCTR2;
				pmc_state[3].evtmsr = MSR_F10H_EVNTSEL3;
				pmc_state[3].ctrmsr = MSR_F10H_PERFCTR3;
			} else {
				/* XXX: make sure it is at least K7 */
				pmc_type = PMC_TYPE_K7;
				pmc_ncounters = 4;
				pmc_state[0].evtmsr = MSR_K7_EVNTSEL0;
				pmc_state[0].ctrmsr = MSR_K7_PERFCTR0;
				pmc_state[1].evtmsr = MSR_K7_EVNTSEL1;
				pmc_state[1].ctrmsr = MSR_K7_PERFCTR1;
				pmc_state[2].evtmsr = MSR_K7_EVNTSEL2;
				pmc_state[2].ctrmsr = MSR_K7_PERFCTR2;
				pmc_state[3].evtmsr = MSR_K7_EVNTSEL3;
				pmc_state[3].ctrmsr = MSR_K7_PERFCTR3;
			}
		}
		break;
	}

	mutex_init(&pmc_lock, MUTEX_DEFAULT, IPL_NONE);
}

int
sys_pmc_info(struct lwp *l, struct x86_pmc_info_args *uargs, register_t *retval)
{
	struct x86_pmc_info_args rv;

	memset(&rv, 0, sizeof(rv));

	rv.vers = PMC_VERSION;
	rv.type = pmc_type;
	rv.nctrs = pmc_ncounters;

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
	size_t nval;
	int error;

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
		error = copyout(&pmc_val_cpus, args.values,
		    nval * sizeof(x86_pmc_cpuval_t));
		args.nval = nval;
	} else {
		error = ENOENT;
	}

	mutex_exit(&pmc_lock);

	if (error)
		return error;

	return copyout(&args, uargs, sizeof(args));
}
