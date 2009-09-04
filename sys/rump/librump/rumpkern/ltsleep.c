/*	$NetBSD: ltsleep.c,v 1.16 2009/09/04 16:41:39 pooka Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: ltsleep.c,v 1.16 2009/09/04 16:41:39 pooka Exp $");

#include <sys/param.h>
#include <sys/proc.h>
#include <sys/queue.h>
#include <sys/simplelock.h>

#include <rump/rumpuser.h>

#include "rump_private.h"

struct ltsleeper {
	wchan_t id;
	kcondvar_t cv;
	LIST_ENTRY(ltsleeper) entries;
};

static LIST_HEAD(, ltsleeper) sleepers = LIST_HEAD_INITIALIZER(sleepers);
static kmutex_t sleepermtx;

kcondvar_t lbolt; /* Oh Kath Ra */

int
ltsleep(wchan_t ident, pri_t prio, const char *wmesg, int timo,
	volatile struct simplelock *slock)
{
	struct ltsleeper lts;
	int nlocks;

	lts.id = ident;
	cv_init(&lts.cv, NULL);

	while (!mutex_tryenter(&sleepermtx))
		continue;
	KERNEL_UNLOCK_ALL(curlwp, &nlocks);
	if (slock)
		simple_unlock(slock);
	LIST_INSERT_HEAD(&sleepers, &lts, entries);

	/* protected by sleepermtx */
	cv_wait(&lts.cv, &sleepermtx);

	LIST_REMOVE(&lts, entries);
	mutex_exit(&sleepermtx);

	cv_destroy(&lts.cv);

	KERNEL_LOCK(nlocks, curlwp);

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
	cv_init(&lts.cv, NULL);

	while (!mutex_tryenter(&sleepermtx))
		continue;
	KERNEL_UNLOCK_ALL(curlwp, &nlocks);
	LIST_INSERT_HEAD(&sleepers, &lts, entries);

	/* protected by sleepermtx */
	mutex_exit(lock);
	cv_wait(&lts.cv, &sleepermtx);

	LIST_REMOVE(&lts, entries);
	mutex_exit(&sleepermtx);

	cv_destroy(&lts.cv);

	KERNEL_LOCK(nlocks, curlwp);

	if ((prio & PNORELOCK) == 0)
		mutex_enter(lock);

	return 0;
}

void
wakeup(wchan_t ident)
{
	struct ltsleeper *ltsp;

	mutex_enter(&sleepermtx);
	LIST_FOREACH(ltsp, &sleepers, entries)
		if (ltsp->id == ident)
			cv_broadcast(&ltsp->cv);
	mutex_exit(&sleepermtx);
}

void
wakeup_one(wchan_t ident)
{
	struct ltsleeper *ltsp;

	mutex_enter(&sleepermtx);
	LIST_FOREACH(ltsp, &sleepers, entries) {
		if (ltsp->id == ident) {
			cv_signal(&ltsp->cv);
			break;
		}
	}
	mutex_exit(&sleepermtx);
}

void
rump_sleepers_init(void)
{

	mutex_init(&sleepermtx, MUTEX_DEFAULT, 0);
}
