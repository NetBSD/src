/*	$NetBSD: sleepq.c,v 1.6 2009/11/17 15:23:42 pooka Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: sleepq.c,v 1.6 2009/11/17 15:23:42 pooka Exp $");

#include <sys/param.h>
#include <sys/condvar.h>
#include <sys/mutex.h>
#include <sys/once.h>
#include <sys/queue.h>
#include <sys/sleepq.h>
#include <sys/syncobj.h>

#include "rump_private.h"

/*
 * Flimsy and minimalistic sleepq implementation.  This is implemented
 * only for the use of callouts in kern_timeout.c.  locking etc is
 * completely incorrect, horrible, etc etc etc.
 */

syncobj_t sleep_syncobj;
static kcondvar_t sq_cv;

static int
sqinit1(void)
{

	cv_init(&sq_cv, "sleepq");

	return 0;
}

void
sleepq_init(sleepq_t *sq)
{
	ONCE_DECL(sqctl);

	RUN_ONCE(&sqctl, sqinit1);

	TAILQ_INIT(sq);
}

void
sleepq_enqueue(sleepq_t *sq, wchan_t wc, const char *wmsg, syncobj_t *sob)
{
	struct lwp *l = curlwp;

	l->l_wchan = wc;
	l->l_sleepq = sq;
	TAILQ_INSERT_TAIL(sq, l, l_sleepchain);
}

int
sleepq_block(int timo, bool catch)
{
	struct lwp *l = curlwp;
	int error = 0;
	kmutex_t *mp = l->l_mutex;
	int biglocks = l->l_biglocks;

	while (l->l_wchan) {
		if ((error=cv_timedwait(&sq_cv, mp, timo)) == EWOULDBLOCK) {
			TAILQ_REMOVE(l->l_sleepq, l, l_sleepchain);
			l->l_wchan = NULL;
		}
	}
	mutex_spin_exit(mp);

	if (biglocks)
		KERNEL_LOCK(biglocks, curlwp);

	return error;
}

lwp_t *
sleepq_wake(sleepq_t *sq, wchan_t wchan, u_int expected, kmutex_t *mp)
{
	struct lwp *l, *l_next;
	bool found = false;

	if (__predict_false(expected != -1))
		panic("sleepq_wake: \"expected\" not supported");

	for (l = TAILQ_FIRST(sq); l; l = l_next) {
		l_next = TAILQ_NEXT(l, l_sleepchain);
		if (l->l_wchan == wchan) {
			found = true;
			l->l_wchan = NULL;
			TAILQ_REMOVE(sq, l, l_sleepchain);
		}
	}
	if (found)
		cv_broadcast(&sq_cv);

	mutex_spin_exit(mp);
	return NULL;
}

void
sleepq_unsleep(struct lwp *l, bool cleanup)
{

	l->l_wchan = NULL;
	TAILQ_REMOVE(l->l_sleepq, l, l_sleepchain);
	cv_broadcast(&sq_cv);

	if (cleanup) {
		mutex_spin_exit(l->l_mutex);
	}
}

/*
 * Thread scheduler handles priorities.  Therefore no action here.
 * (maybe do something if we're deperate?)
 */
void
sleepq_changepri(struct lwp *l, pri_t pri)
{

}

void
sleepq_lendpri(struct lwp *l, pri_t pri)
{

}

struct lwp *
syncobj_noowner(wchan_t wc)
{

	return NULL;
}

/*
 * XXX: used only by callout, therefore here.  should try to use
 * one in kern_lwp directly.
 */
kmutex_t *
lwp_lock_retry(struct lwp *l, kmutex_t *old)
{

	while (l->l_mutex != old) {
		mutex_spin_exit(old);
		old = l->l_mutex;
		mutex_spin_enter(old);
	}
	return old;
}
