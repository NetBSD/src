/*	$NetBSD: intr.c,v 1.18 2009/09/19 14:18:01 pooka Exp $	*/

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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: intr.c,v 1.18 2009/09/19 14:18:01 pooka Exp $");

#include <sys/param.h>
#include <sys/cpu.h>
#include <sys/kmem.h>
#include <sys/kthread.h>
#include <sys/intr.h>

#include <rump/rumpuser.h>

#include "rump_private.h"

/*
 * Interrupt simulator.  It executes hardclock() and softintrs.
 */

time_t time_uptime = 0;

#define SI_MPSAFE 0x01
#define SI_ONLIST 0x02
#define SI_KILLME 0x04
struct softint {
	void (*si_func)(void *);
	void *si_arg;
	int si_flags;

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
	rv = kthread_create(PRI_NONE, KTHREAD_MPSAFE | KTHREAD_INTR, NULL,
	    sithread, NULL, NULL, "rumpsi");
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

/* rumpuser structures since we call rumpuser interfaces directly */
static struct rumpuser_cv *clockcv;
static struct rumpuser_mtx *clockmtx;
static struct timespec clockbase, clockup;
static unsigned clkgen;

void
rump_getuptime(struct timespec *ts)
{
	int startgen, i = 0;

	do {
		startgen = clkgen;
		if (__predict_false(i++ > 10)) {
			yield();
			i = 0;
		}
		*ts = clockup;
	} while (startgen != clkgen || clkgen % 2 != 0);
}

void
rump_gettime(struct timespec *ts)
{
	struct timespec ts_up;

	rump_getuptime(&ts_up);
	timespecadd(&clockbase, &ts_up, ts);
}

/*
 * clock "interrupt"
 */
static void
doclock(void *noarg)
{
	struct timespec tick, curtime;
	uint64_t sec, nsec;
	int ticks = 0, error;
	extern int hz;

	rumpuser_gettime(&sec, &nsec, &error);
	clockbase.tv_sec = sec;
	clockbase.tv_nsec = nsec;
	curtime = clockbase;
	tick.tv_sec = 0;
	tick.tv_nsec = 1000000000/hz;

	rumpuser_mutex_enter(clockmtx);
	rumpuser_cv_signal(clockcv);

	for (;;) {
		callout_hardclock();

		if (++ticks == hz) {
			time_uptime++;
			ticks = 0;
		}

		/* wait until the next tick. XXX: what if the clock changes? */
		while (rumpuser_cv_timedwait(clockcv, clockmtx,
		    &curtime) != EWOULDBLOCK)
			continue;

		clkgen++;
		timespecadd(&clockup, &tick, &clockup);
		clkgen++;
		timespecadd(&clockup, &clockbase, &curtime);
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
			mpsafe = si->si_flags & SI_MPSAFE;

			si->si_flags &= ~SI_ONLIST;
			LIST_REMOVE(si, si_entries);
			if (si->si_flags & SI_KILLME)
				softint_disestablish(si);
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

	rumpuser_cv_init(&clockcv);
	rumpuser_mutex_init(&clockmtx);

	/* XXX: should have separate "wanttimer" control */
	if (rump_threads) {
		rumpuser_mutex_enter(clockmtx);
		rv = kthread_create(PRI_NONE, KTHREAD_MPSAFE, NULL, doclock,
		    NULL, NULL, "rumpclk");
		if (rv)
			panic("clock thread creation failed: %d", rv);
		mutex_enter(&si_mtx);
		while (wrktotal < INTRTHREAD_DEFAULT) {
			makeworker(true);
		}
		mutex_exit(&si_mtx);

		/* make sure we have a clocktime before returning */
		rumpuser_cv_wait(clockcv, clockmtx);
		rumpuser_mutex_exit(clockmtx);
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
	si->si_flags = flags & SOFTINT_MPSAFE ? SI_MPSAFE : 0;

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
		if (!(si->si_flags & SI_ONLIST)) {
			LIST_INSERT_HEAD(&si_pending, si, si_entries);
			si->si_flags |= SI_ONLIST;
		}
		cv_signal(&si_cv);
		mutex_exit(&si_mtx);
	}
}

/* flimsy disestablish: should wait for softints to finish */
void
softint_disestablish(void *cook)
{
	struct softint *si = cook;

	if (si->si_flags & SI_ONLIST) {
		si->si_flags |= SI_KILLME;
		return;
	}
	kmem_free(si, sizeof(*si));
}

bool
cpu_intr_p(void)
{

	return false;
}
