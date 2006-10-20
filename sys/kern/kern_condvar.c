/*	$NetBSD: kern_condvar.c,v 1.1.2.1 2006/10/20 19:40:17 ad Exp $	*/

/*-
 * Copyright (c) 2006 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Andrew Doran.
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
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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
 * Kernel condition variables implementation, modeled after those found in
 * Solaris, a description of which can be found in:
 *
 *	Solaris Internals: Core Kernel Architecture, Jim Mauro and
 *	    Richard McDougall.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: kern_condvar.c,v 1.1.2.1 2006/10/20 19:40:17 ad Exp $");

#include <sys/param.h>
#include <sys/proc.h>
#include <sys/sched.h>
#include <sys/systm.h>
#include <sys/condvar.h>
#include <sys/sleepq.h>

/*
 * cv_init:
 *
 *	Initialize a condition variable for use.
 */
void
cv_init(kcondvar_t *cv, kcondvar_type_t type, const char *wmesg)
{

	KASSERT(type == CV_DEFAULT);
	cv->cv_wmesg = wmesg;
	cv->cv_ptr = NULL;
}

/*
 * cv_destroy:
 *
 *	Tear down a condition variable.
 */
void
cv_destroy(kcondvar_t *cv)
{

	KASSERT(cv->cv_waiters == 0);
}

/*
 * cv_wait:
 *
 *	Wait non-interruptably on a condition variable until awoken.
 */
void
cv_wait(kcondvar_t *cv, kmutex_t *mtx)
{
	struct lwp *l = curlwp;
	sleepq_t *sq;

	LOCK_ASSERT(mutex_owned(mtx));

	if (sleepq_dontsleep(l)) {
		(void)sleepq_abort(mtx, 0);
		return;
	}

	sq = sleeptab_lookup(cv);
	cv->cv_waiters++;

	sleepq_enter(sq, sched_kpri(l), cv, cv->cv_wmesg, 0, 0);

	mutex_exit_linked(mtx, l->l_mutex);

	(void)sleepq_block(sq, 0);

	mutex_enter(mtx);
}

/*
 * cv_wait_sig:
 *
 *	Wait on a condition variable until a awoken or a signal is received. 
 *	Will also return early if the process is exiting.  Returns zero if
 *	awoken normallly, ERESTART if a signal was received and the system
 *	call is restartable, or EINTR otherwise.
 */
int
cv_wait_sig(kcondvar_t *cv, kmutex_t *mtx)
{
	struct lwp *l = curlwp;
	sleepq_t *sq;
	int error;

	LOCK_ASSERT(mutex_owned(mtx));

	if (sleepq_dontsleep(l))
		return sleepq_abort(mtx, 0);

	sq = sleeptab_lookup(cv);
	cv->cv_waiters++;

	sleepq_enter(sq, sched_kpri(l), cv, cv->cv_wmesg, 0, 1);

	mutex_exit_linked(mtx, l->l_mutex);

	error = sleepq_block(sq, 0);

	mutex_enter(mtx);
 
	return error;
}

/*
 * cv_timedwait:
 *
 *	Wait on a condition variable until awoken or the specified timeout
 *	expires.  Returns zero if awoken normally or EWOULDBLOCK if the
 *	timeout expired.
 */
int
cv_timedwait(kcondvar_t *cv, kmutex_t *mtx, int timo)
{
	struct lwp *l = curlwp;
	sleepq_t *sq;
	int error;

	LOCK_ASSERT(mutex_owned(mtx));

	if (sleepq_dontsleep(l))
		return sleepq_abort(mtx, 0);

	sq = sleeptab_lookup(cv);
	cv->cv_waiters++;

	sleepq_enter(sq, sched_kpri(l), cv, cv->cv_wmesg, timo, 0);

	mutex_exit_linked(mtx, l->l_mutex);

	error = sleepq_block(sq, timo);

	mutex_enter(mtx);
 
	return error;
}

/*
 * cv_timedwait_sig:
 *
 *	Wait on a condition variable until a timeout expires, awoken or a
 *	signal is received.  Will also return early if the process is
 *	exiting.  Returns zero if awoken normallly, EWOULDBLOCK if the
 *	timeout expires, ERESTART if a signal was received and the system
 *	call is restartable, or EINTR otherwise.
 */
int
cv_timedwait_sig(kcondvar_t *cv, kmutex_t *mtx, int timo)
{
	struct lwp *l = curlwp;
	sleepq_t *sq;
	int error;

	LOCK_ASSERT(mutex_owned(mtx));

	if (sleepq_dontsleep(l))
		return sleepq_abort(mtx, 0);

	sq = sleeptab_lookup(cv);
	cv->cv_waiters++;

	sleepq_enter(sq, sched_kpri(l), cv, cv->cv_wmesg, timo, 1);

	mutex_exit_linked(mtx, l->l_mutex);

	error = sleepq_block(sq, timo);

	mutex_enter(mtx);
 
	return error;
}

/*
 * cv_signal:
 *
 *	Wake the highest priority LWP waiting on a condition variable.
 */
void
cv_signal(kcondvar_t *cv)
{
	sleepq_t *sq;

	sq = sleeptab_lookup(cv);
	if (cv->cv_waiters != 0) {
		cv->cv_waiters--;
		sleepq_wakeone(sq, cv);
	} else
		sleepq_unlock(sq);
}

/*
 * cv_broadcast:
 *
 *	Wake all LWPs waiting on a condition variable.
 */
void
cv_broadcast(kcondvar_t *cv)
{
	sleepq_t *sq;

	sq = sleeptab_lookup(cv);
	if (cv->cv_waiters != 0) {
		sleepq_wakeall(sq, cv, cv->cv_waiters);
		cv->cv_waiters = 0;
	} else
		sleepq_unlock(sq);
}
