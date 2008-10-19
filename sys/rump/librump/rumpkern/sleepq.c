/*	$NetBSD: sleepq.c,v 1.1.4.2 2008/10/19 22:18:07 haad Exp $	*/

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
#include <sys/condvar.h>
#include <sys/mutex.h>
#include <sys/queue.h>
#include <sys/sleepq.h>
#include <sys/syncobj.h>

/*
 * Flimsy and minimalistic sleepq implementation.  This is implemented
 * only for the use of callouts in kern_timeout.c.  locking etc is
 * completely incorrect, horrible, etc etc etc.
 */

syncobj_t sleep_syncobj;
static kcondvar_t sq_cv;
static kmutex_t sq_mtx;

void
sleepq_init(sleepq_t *sq)
{

	TAILQ_INIT(sq);

	cv_init(&sq_cv, "sleepq"); /* XXX */
	mutex_init(&sq_mtx, MUTEX_DEFAULT, IPL_NONE); /* multi-XXX */
}

void
sleepq_enqueue(sleepq_t *sq, wchan_t wc, const char *wmsg, syncobj_t *sob)
{
	struct lwp *l = curlwp;

	if (__predict_false(sob != &sleep_syncobj || strcmp(wmsg, "callout"))) {
		panic("sleepq: unsupported enqueue");
	}

	l->l_wchan = wc;
	TAILQ_INSERT_TAIL(sq, l, l_sleepchain);
}

int
sleepq_block(int timo, bool hatch)
{
	struct lwp *l = curlwp;

	KASSERT(timo == 0 && !hatch);

	mutex_enter(&sq_mtx);
	while (l->l_wchan)
		cv_wait(&sq_cv, &sq_mtx);
	mutex_exit(&sq_mtx);

	return 0;
}

lwp_t *
sleepq_wake(sleepq_t *sq, wchan_t wchan, u_int expected, kmutex_t *mp)
{
	struct lwp *l;
	bool found = false;

	TAILQ_FOREACH(l, sq, l_sleepchain) {
		if (l->l_wchan == wchan) {
			found = true;
			l->l_wchan = NULL;
		}
	}
	if (found)
		cv_broadcast(&sq_cv);

	mutex_spin_exit(mp);
	return NULL;
}

/*
 * XXX: used only by callout, therefore here
 *
 * We don't fudge around with the lwp mutex at all, therefore
 * this is enough.
 */
kmutex_t *
lwp_lock_retry(struct lwp *l, kmutex_t *old)
{

	return old;
}
