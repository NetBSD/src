/*	$NetBSD: intr.c,v 1.10 2008/12/18 00:12:00 pooka Exp $	*/

/*
 * Copyright (c) 2008 Antti Kantee.  All Rights Reserved.
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

#include <sys/param.h>
#include <sys/cpu.h>
#include <sys/kthread.h>
#include <sys/intr.h>

#include <rump/rumpuser.h>

#include "rump_private.h"

/*
 * Interrupt simulator.  It executes hardclock() and softintrs.
 */

time_t time_uptime = 1;

struct softint {
	void (*si_func)(void *);
	void *si_arg;
	bool si_onlist;
	bool si_mpsafe;

	LIST_ENTRY(softint) si_entries;
};
static LIST_HEAD(, softint) si_pending = LIST_HEAD_INITIALIZER(si_pending);
static kmutex_t si_mtx;
static kcondvar_t si_cv;

#define INTRTHREAD_DEFAULT	2
#define INTRTHREAD_MAX		20
static int wrkidle, wrktotal;

static void sithread(void *);

static void
makeworker(bool bootstrap)
{
	int rv;

	if (wrktotal > INTRTHREAD_MAX) {
		/* XXX: ratecheck */
		printf("maximum interrupt threads (%d) reached\n",
		    INTRTHREAD_MAX);
		return;
	}
	rv = kthread_create(PRI_NONE, 0, NULL, sithread,
	    NULL, NULL, "rumpsi");
	if (rv) {
		if (bootstrap)
			panic("intr thread creation failed %d", rv);
		else
			printf("intr thread creation failed %d\n", rv);
	} else {
		wrkidle++;
		wrktotal++;
	}
}

/*
 * clock "interrupt"
 */
static void
doclock(void *noarg)
{
	static int ticks = 0;
	extern int hz;

	for (;;) {
		callout_hardclock();

		/* XXX: will drift */
		if (++ticks == hz) {
			time_uptime++;
			ticks = 0;
		}
		kpause("tickw8", false, 1, NULL);
	}
}

/*
 * run a scheduled soft interrupt
 */
static void
sithread(void *arg)
{
	struct softint *si;
	void (*func)(void *) = NULL;
	void *funarg;
	bool mpsafe;

	mutex_enter(&si_mtx);
	for (;;) {
		if (!LIST_EMPTY(&si_pending)) {
			si = LIST_FIRST(&si_pending);
			func = si->si_func;
			funarg = si->si_arg;
			mpsafe = si->si_mpsafe;

			si->si_onlist = false;
			LIST_REMOVE(si, si_entries);
		} else {
			cv_wait(&si_cv, &si_mtx);
			continue;
		}
		wrkidle--;
		if (__predict_false(wrkidle == 0))
			makeworker(false);
		mutex_exit(&si_mtx);

		if (!mpsafe)
			KERNEL_LOCK(1, curlwp);
		func(funarg);
		if (!mpsafe)
			KERNEL_UNLOCK_ONE(curlwp);

		mutex_enter(&si_mtx);
		wrkidle++;
	}
}

void
softint_init(struct cpu_info *ci)
{
	int rv;

	mutex_init(&si_mtx, MUTEX_DEFAULT, IPL_NONE);
	cv_init(&si_cv, "intrw8"); /* cv of temporary w8ness */

	/* XXX: should have separate "wanttimer" control */
	if (rump_threads) {
		rv = kthread_create(PRI_NONE, 0, NULL, doclock,
		    NULL, NULL, "rumpclk");
		if (rv)
			panic("clock thread creation failed: %d", rv);
		mutex_enter(&si_mtx);
		while (wrktotal < INTRTHREAD_DEFAULT) {
			makeworker(true);
		}
		mutex_exit(&si_mtx);
	}
}

/*
 * Soft interrupts bring two choices.  If we are running with thread
 * support enabled, defer execution, otherwise execute in place.
 * See softint_schedule().
 * 
 * As there is currently no clear concept of when a thread finishes
 * work (although rump_clear_curlwp() is close), simply execute all
 * softints in the timer thread.  This is probably not the most
 * efficient method, but good enough for now.
 */
void *
softint_establish(u_int flags, void (*func)(void *), void *arg)
{
	struct softint *si;

	si = kmem_alloc(sizeof(*si), KM_SLEEP);
	si->si_func = func;
	si->si_arg = arg;
	si->si_onlist = false;
	si->si_mpsafe = flags & SOFTINT_MPSAFE;

	return si;
}

void
softint_schedule(void *arg)
{
	struct softint *si = arg;

	if (!rump_threads) {
		si->si_func(si->si_arg);
	} else {
		mutex_enter(&si_mtx);
		if (!si->si_onlist) {
			LIST_INSERT_HEAD(&si_pending, si, si_entries);
			si->si_onlist = true;
		}
		cv_signal(&si_cv);
		mutex_exit(&si_mtx);
	}
}

bool
cpu_intr_p(void)
{

	return false;
}
