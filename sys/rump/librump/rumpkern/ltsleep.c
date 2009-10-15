/*	$NetBSD: ltsleep.c,v 1.19 2009/10/15 00:28:46 pooka Exp $	*/

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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: ltsleep.c,v 1.19 2009/10/15 00:28:46 pooka Exp $");

#include <sys/param.h>
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

kcondvar_t lbolt; /* Oh Kath Ra */

int
ltsleep(wchan_t ident, pri_t prio, const char *wmesg, int timo,
	volatile struct simplelock *slock)
{
	struct ltsleeper lts;
	int nlocks;

	lts.id = ident;
	rumpuser_cv_init(&lts.cv);

	if (slock)
		simple_unlock(slock);
	LIST_INSERT_HEAD(&sleepers, &lts, entries);
	kernel_unlock_allbutone(&nlocks);

	/* protected by biglock */
	rumpuser_cv_wait(lts.cv, rump_giantlock);

	LIST_REMOVE(&lts, entries);
	rumpuser_cv_destroy(lts.cv);
	kernel_ununlock_allbutone(nlocks);

	if (slock && (prio & PNORELOCK) == 0)
		simple_lock(slock);

	return 0;
}

int
mtsleep(wchan_t ident, pri_t prio, const char *wmesg, int timo,
	kmutex_t *lock)
{
	struct ltsleeper lts;
	int nlocks;

	lts.id = ident;
	rumpuser_cv_init(&lts.cv);

	mutex_exit(lock);
	LIST_INSERT_HEAD(&sleepers, &lts, entries);
	kernel_unlock_allbutone(&nlocks);

	/* protected by biglock */
	rumpuser_cv_wait(lts.cv, rump_giantlock);

	LIST_REMOVE(&lts, entries);
	rumpuser_cv_destroy(lts.cv);
	kernel_ununlock_allbutone(nlocks);

	if ((prio & PNORELOCK) == 0)
		mutex_enter(lock);

	return 0;
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

void
rump_sleepers_init(void)
{

}
