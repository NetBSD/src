/*	$NetBSD: ltsleep.c,v 1.23 2009/12/05 22:44:08 pooka Exp $	*/

/*
 * Copyright (c) 2007 Antti Kantee.  All Rights Reserved.
 *
 * Development of this software was supported by the
 * Finnish Cultural Foundation.
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

/*
 * Emulate the prehistoric tsleep-style kernel interfaces.  We assume
 * only code under the biglock will be using this type of synchronization
 * and use the biglock as the cv interlock.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: ltsleep.c,v 1.23 2009/12/05 22:44:08 pooka Exp $");

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/queue.h>
#include <sys/simplelock.h>

#include <rump/rumpuser.h>

#include "rump_private.h"

struct ltsleeper {
	wchan_t id;
	struct rumpuser_cv *cv;
	LIST_ENTRY(ltsleeper) entries;
};

static LIST_HEAD(, ltsleeper) sleepers = LIST_HEAD_INITIALIZER(sleepers);

static int
sleeper(struct ltsleeper *ltsp, int timo)
{
	int rv, nlocks;

	LIST_INSERT_HEAD(&sleepers, ltsp, entries);
	kernel_unlock_allbutone(&nlocks);

	/* protected by biglock */
	if (timo) {
		if (rumpuser_cv_timedwait(ltsp->cv, rump_giantlock,
		    timo / hz, (timo % hz) * (1000000000/hz)) == 0)
			rv = 0;
		else
			rv = EWOULDBLOCK;
	} else {
		rumpuser_cv_wait(ltsp->cv, rump_giantlock);
		rv = 0;
	}

	LIST_REMOVE(ltsp, entries);
	rumpuser_cv_destroy(ltsp->cv);
	kernel_ununlock_allbutone(nlocks);

	return rv;
}

int
ltsleep(wchan_t ident, pri_t prio, const char *wmesg, int timo,
	volatile struct simplelock *slock)
{
	struct ltsleeper lts;
	int rv;

	lts.id = ident;
	rumpuser_cv_init(&lts.cv);

	if (slock)
		simple_unlock(slock);

	rv = sleeper(&lts, timo);

	if (slock && (prio & PNORELOCK) == 0)
		simple_lock(slock);

	return rv;
}

int
mtsleep(wchan_t ident, pri_t prio, const char *wmesg, int timo,
	kmutex_t *lock)
{
	struct ltsleeper lts;
	int rv;

	lts.id = ident;
	rumpuser_cv_init(&lts.cv);

	mutex_exit(lock);

	rv = sleeper(&lts, timo);

	if ((prio & PNORELOCK) == 0)
		mutex_enter(lock);

	return rv;
}

static void
do_wakeup(wchan_t ident, void (*wakeupfn)(struct rumpuser_cv *))
{
	struct ltsleeper *ltsp;

	KASSERT(kernel_biglocked());
	LIST_FOREACH(ltsp, &sleepers, entries) {
		if (ltsp->id == ident) {
			wakeupfn(ltsp->cv);
		}
	}
}

void
wakeup(wchan_t ident)
{

	do_wakeup(ident, rumpuser_cv_broadcast);
}

void
wakeup_one(wchan_t ident)
{

	do_wakeup(ident, rumpuser_cv_signal);
}
