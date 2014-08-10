/*	$NetBSD: intr.c,v 1.42.2.1 2014/08/10 06:56:51 tls Exp $	*/

/*
 * Copyright (c) 2008-2010 Antti Kantee.  All Rights Reserved.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: intr.c,v 1.42.2.1 2014/08/10 06:56:51 tls Exp $");

#include <sys/param.h>
#include <sys/atomic.h>
#include <sys/cpu.h>
#include <sys/kernel.h>
#include <sys/kmem.h>
#include <sys/kthread.h>
#include <sys/malloc.h>
#include <sys/intr.h>
#include <sys/timetc.h>

#include <rump/rumpuser.h>

#include "rump_private.h"

/*
 * Interrupt simulator.  It executes hardclock() and softintrs.
 */

#define SI_MPSAFE 0x01
#define SI_KILLME 0x02

struct softint_percpu;
struct softint {
	void (*si_func)(void *);
	void *si_arg;
	int si_flags;
	int si_level;

	struct softint_percpu *si_entry; /* [0,ncpu-1] */
};

struct softint_percpu {
	struct softint *sip_parent;
	bool sip_onlist;

	LIST_ENTRY(softint_percpu) sip_entries;
};

struct softint_lev {
	struct rumpuser_cv *si_cv;
	LIST_HEAD(, softint_percpu) si_pending;
};

kcondvar_t lbolt; /* Oh Kath Ra */

static u_int ticks;
static int ncpu_final;

static u_int
rumptc_get(struct timecounter *tc)
{

	KASSERT(rump_threads);
	return ticks;
}

static struct timecounter rumptc = {
	.tc_get_timecount	= rumptc_get,
	.tc_poll_pps 		= NULL,
	.tc_counter_mask	= ~0,
	.tc_frequency		= 0,
	.tc_name		= "rumpclk",
	.tc_quality		= 0,
};

/*
 * clock "interrupt"
 */
static void
doclock(void *noarg)
{
	struct timespec thetick, curclock;
	int64_t sec;
	long nsec;
	int error;
	int cpuindx = curcpu()->ci_index;
	extern int hz;

	error = rumpuser_clock_gettime(RUMPUSER_CLOCK_ABSMONO, &sec, &nsec);
	if (error)
		panic("clock: cannot get monotonic time");

	curclock.tv_sec = sec;
	curclock.tv_nsec = nsec;
	thetick.tv_sec = 0;
	thetick.tv_nsec = 1000000000/hz;

	for (;;) {
		callout_hardclock();

		error = rumpuser_clock_sleep(RUMPUSER_CLOCK_ABSMONO,
		    curclock.tv_sec, curclock.tv_nsec);
		KASSERT(!error);
		timespecadd(&curclock, &thetick, &curclock);

		if (cpuindx != 0)
			continue;

		if ((++ticks % hz) == 0) {
			cv_broadcast(&lbolt);
		}
		tc_ticktock();
	}
}

/*
 * Soft interrupt execution thread.  This thread is pinned to the
 * same CPU that scheduled the interrupt, so we don't need to do
 * lock against si_lvl.
 */
static void
sithread(void *arg)
{
	struct softint_percpu *sip;
	struct softint *si;
	void (*func)(void *) = NULL;
	void *funarg;
	bool mpsafe;
	int mylevel = (uintptr_t)arg;
	struct softint_lev *si_lvlp, *si_lvl;
	struct cpu_data *cd = &curcpu()->ci_data;

	si_lvlp = cd->cpu_softcpu;
	si_lvl = &si_lvlp[mylevel];

	for (;;) {
		if (!LIST_EMPTY(&si_lvl->si_pending)) {
			sip = LIST_FIRST(&si_lvl->si_pending);
			si = sip->sip_parent;

			func = si->si_func;
			funarg = si->si_arg;
			mpsafe = si->si_flags & SI_MPSAFE;

			sip->sip_onlist = false;
			LIST_REMOVE(sip, sip_entries);
			if (si->si_flags & SI_KILLME) {
				softint_disestablish(si);
				continue;
			}
		} else {
			rump_schedlock_cv_wait(si_lvl->si_cv);
			continue;
		}

		if (!mpsafe)
			KERNEL_LOCK(1, curlwp);
		func(funarg);
		if (!mpsafe)
			KERNEL_UNLOCK_ONE(curlwp);
	}

	panic("sithread unreachable");
}

static kmutex_t sithr_emtx;
static unsigned int sithr_est;
static int sithr_canest;

/*
 * Create softint handler threads when the softint for each respective
 * level is established for the first time.  Most rump kernels don't
 * need at least half of the softint levels, so on-demand saves bootstrap
 * time and memory resources.  Note, though, that this routine may be
 * called before it's possible to call kthread_create().  Creation of
 * those softints (SOFTINT_CLOCK, as of writing this) will be deferred
 * to until softint_init() is called for the main CPU.
 */
static void
sithread_establish(int level)
{
	int docreate, rv;
	int lvlbit = 1<<level;
	int i;

	KASSERT((level & ~SOFTINT_LVLMASK) == 0);
	if (__predict_true(sithr_est & lvlbit))
		return;

	mutex_enter(&sithr_emtx);
	docreate = (sithr_est & lvlbit) == 0 && sithr_canest;
	sithr_est |= lvlbit;
	mutex_exit(&sithr_emtx);

	if (docreate) {
		for (i = 0; i < ncpu_final; i++) {
			if ((rv = kthread_create(PRI_NONE,
			    KTHREAD_MPSAFE | KTHREAD_INTR,
			    cpu_lookup(i), sithread, (void *)(uintptr_t)level,
			    NULL, "rsi%d/%d", i, level)) != 0)
				panic("softint thread create failed: %d", rv);
		}
	}
}

void
rump_intr_init(int numcpu)
{

	cv_init(&lbolt, "oh kath ra");
	mutex_init(&sithr_emtx, MUTEX_DEFAULT, IPL_NONE);
	ncpu_final = numcpu;
}

void
softint_init(struct cpu_info *ci)
{
	struct cpu_data *cd = &ci->ci_data;
	struct softint_lev *slev;
	int rv, i;

	if (!rump_threads)
		return;

	slev = kmem_alloc(sizeof(struct softint_lev) * SOFTINT_COUNT, KM_SLEEP);
	for (i = 0; i < SOFTINT_COUNT; i++) {
		rumpuser_cv_init(&slev[i].si_cv);
		LIST_INIT(&slev[i].si_pending);
	}
	cd->cpu_softcpu = slev;

	/* overloaded global init ... */
	/* XXX: should be done the last time we are called */
	if (ci->ci_index == 0) {
		int sithr_swap;

		rumptc.tc_frequency = hz;
		tc_init(&rumptc);

		/* create deferred softint threads */
		mutex_enter(&sithr_emtx);
		sithr_swap = sithr_est;
		sithr_est = 0;
		sithr_canest = 1;
		mutex_exit(&sithr_emtx);
		for (i = 0; i < SOFTINT_COUNT; i++) {
			if (sithr_swap & (1<<i))
				sithread_establish(i);
		}
	}

	/* well, not really a "soft" interrupt ... */
	if ((rv = kthread_create(PRI_NONE, KTHREAD_MPSAFE,
	    ci, doclock, NULL, NULL, "rumpclk%d", ci->ci_index)) != 0)
		panic("clock thread creation failed: %d", rv);
}

void *
softint_establish(u_int flags, void (*func)(void *), void *arg)
{
	struct softint *si;
	struct softint_percpu *sip;
	int level = flags & SOFTINT_LVLMASK;
	int i;

	si = malloc(sizeof(*si), M_TEMP, M_WAITOK);
	si->si_func = func;
	si->si_arg = arg;
	si->si_flags = flags & SOFTINT_MPSAFE ? SI_MPSAFE : 0;
	si->si_level = level;
	KASSERT(si->si_level < SOFTINT_COUNT);
	si->si_entry = malloc(sizeof(*si->si_entry) * ncpu_final,
	    M_TEMP, M_WAITOK | M_ZERO);
	for (i = 0; i < ncpu_final; i++) {
		sip = &si->si_entry[i];
		sip->sip_parent = si;
	}
	sithread_establish(level);

	return si;
}

/*
 * Soft interrupts bring two choices.  If we are running with thread
 * support enabled, defer execution, otherwise execute in place.
 */

void
softint_schedule(void *arg)
{
	struct softint *si = arg;
	struct cpu_info *ci = curcpu();
	struct softint_percpu *sip = &si->si_entry[ci->ci_index];
	struct cpu_data *cd = &ci->ci_data;
	struct softint_lev *si_lvl = cd->cpu_softcpu;

	if (!rump_threads) {
		si->si_func(si->si_arg);
	} else {
		if (!sip->sip_onlist) {
			LIST_INSERT_HEAD(&si_lvl[si->si_level].si_pending,
			    sip, sip_entries);
			sip->sip_onlist = true;
		}
	}
}

void
softint_schedule_cpu(void *arg, struct cpu_info *ci)
{
	/*
	 * TODO: implement this properly
	 */
	KASSERT(curcpu() == ci);
	softint_schedule(arg);
}

/*
 * flimsy disestablish: should wait for softints to finish.
 */
void
softint_disestablish(void *cook)
{
	struct softint *si = cook;
	int i;

	for (i = 0; i < ncpu_final; i++) {
		struct softint_percpu *sip;

		sip = &si->si_entry[i];
		if (sip->sip_onlist) {
			si->si_flags |= SI_KILLME;
			return;
		}
	}
	free(si->si_entry, M_TEMP);
	free(si, M_TEMP);
}

void
rump_softint_run(struct cpu_info *ci)
{
	struct cpu_data *cd = &ci->ci_data;
	struct softint_lev *si_lvl = cd->cpu_softcpu;
	int i;

	if (!rump_threads)
		return;

	for (i = 0; i < SOFTINT_COUNT; i++) {
		if (!LIST_EMPTY(&si_lvl[i].si_pending))
			rumpuser_cv_signal(si_lvl[i].si_cv);
	}
}

bool
cpu_intr_p(void)
{

	return false;
}

bool
cpu_softintr_p(void)
{

	return curlwp->l_pflag & LP_INTR;
}
