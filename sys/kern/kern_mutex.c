/*	$NetBSD: kern_mutex.c,v 1.1.2.2 2002/03/11 17:08:35 thorpej Exp $	*/

/*-
 * Copyright (c) 2002 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe.
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
 * Kernel mutex implementation, modeled after those found in Solaris,
 * a description of which can be found in:
 *
 *	Solaris Internals: Core Kernel Architecture, Jim Mauro and
 *	    Richard McDougall.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: kern_mutex.c,v 1.1.2.2 2002/03/11 17:08:35 thorpej Exp $");

#include <sys/param.h>
#include <sys/proc.h>
#include <sys/mutex.h>
#include <sys/sched.h>
#include <sys/systm.h>

#include <machine/intr.h>

/*
 * mutex_init:
 *
 *	Initialize a mutex for use.
 */
void
mutex_init(kmutex_t *mtx, kmutex_type_t type, int ipl)
{

	MUTEX_INIT(mtx, type);

	if (type == MUTEX_SPIN)
		mtx->m_minspl = ipl;
}

/*
 * mutex_destroy:
 *
 *	Tear down a mutex.
 */
void
mutex_destroy(kmutex_t *mtx)
{

	/* XXX IMPLEMENT ME XXX */
}

static __inline int
mutex_owner_is_running(struct proc *p)
{

	/*
	 * XXXSMP Proc could exit.  Should traverse the CPUs
	 * XXXSMP to see if the curproc for that CPU is the
	 * XXXSMP owner.
	 */
	return (p->p_stat == SONPROC);
}

/*
 * mutex_vector_enter:
 *
 *	Support routine for mutex_enter(); handles situations where
 *	an adaptive mutex was not acquired on the first try.
 *
 *	Pseudo-code for this routine can be found in SI, p. 77.
 */
void
mutex_vector_enter(kmutex_t *mtx)
{
	struct turnstile *ts;
	struct proc *p;

	if (MUTEX_SPIN_P(mtx)) {
		/*
		 * Spin mutex; raise interrupt priority level to that
		 * indicated in the mutex, and spin waiting for the lock.
		 */
		int s = splraiseipl(mtx->m_minspl);
		while (__cpu_simple_lock_try(&mtx->m_spinlock) == 0) {
			/* XXXJRT insert spinout detection here */ ;
		}
		mtx->m_oldspl = s;
		return;
	}

	/*
	 * Adaptive mutex; spin trying to acquire the mutex.  If we
	 * determine that the owner is not running on a processor,
	 * then we stop spinning, and block instead.
	 */
	for (;;) {
		if ((p = MUTEX_OWNER(mtx)) == NULL) {
			/*
			 * Mutex owner clear could mean two things:
			 *	* The mutex has been released.
			 *	* The owner field hasn't been set yet.
			 * Try to acquire it again.  If that fails,
			 * we'll just loop again.
			 */
			if (mutex_tryenter(mtx))
				break;
			continue;
		}

		if (p == curproc)
			panic("mutex_vector_enter: locking against myself");

		/*
		 * Check to see if the owner is running on a processor.
		 * If so, then we should just spin, as the owner will
		 * likely release the lock very soon.  If not, then we
		 * have to pay a context switch penalty anyway (for the
		 * owner to run), so we might as well block ourselves.
		 */
		if (mutex_owner_is_running(p))
			continue;

		ts = turnstile_lookup(mtx);

		/*
		 * Mark the mutex has having waiters.  After we do
		 * this, we need to check one more time if we should
		 * spin.
		 *
		 * We should spin if:
		 *	* The owner is running again.
		 *	* The owner released the lock.
		 *
		 * We need the check because the owner may release
		 * the lock before we mark it has having waiters
		 * (which would mean it wouldn't try to acquire the
		 * interlock on the sleep queue).
		 */
		MUTEX_SET_WAITERS(mtx);
		if (mutex_owner_is_running(p) ||
		    (MUTEX_OWNER(mtx) == p && MUTEX_HAS_WAITERS(mtx)) == 0) {
			turnstile_exit(mtx);
			continue;
		}
		/* XXXJRT p->p_priority */
		(void) turnstile_block(ts, TS_WRITER_Q, p->p_priority, mtx);
	}

	LOCK_ASSERT(MUTEX_OWNER(mtx) == curproc);
}

/*
 * mutex_vector_tryenter:
 *
 *	Support routine for mutex_tryenter(); handles the case
 *	where we're trying a spin mutex.
 */
int
mutex_vector_tryenter(kmutex_t *mtx)
{
	int s;

	/*
	 * If this is an adaptive mutex, just return "didn't succeed."
	 */
	if (MUTEX_ADAPTIVE_P(mtx))
		return (0);

	s = splraiseipl(mtx->m_minspl);
	if (__cpu_simple_lock_try(&mtx->m_spinlock)) {
		mtx->m_oldspl = s;
		return (1);
	}
	splx(s);
	return (0);
}

/*
 * mutex_vector_exit:
 *
 *	Support routine for mutex_exit(); handles spin mutexes
 *	or adaptive mutexes with waiters.
 */
void
mutex_vector_exit(kmutex_t *mtx)
{
	struct turnstile *ts;

	if (MUTEX_SPIN_P(mtx)) {
		/*
		 * Spin mutex; just release the lock and drop spl.
		 */
		int s = mtx->m_oldspl;
		__cpu_simple_unlock(&mtx->m_spinlock);
		splx(s);
		return;
	}

	if (MUTEX_OWNER(mtx) != curproc)
		panic("mutex_vector_exit: not owner");

	/*
	 * Get this lock's turnstile.  This gets the interlock on
	 * the sleep queue.  Once we have that, we can clear the
	 * lock.  If there was no turnstile for the lock, there
	 * were no waiters remaining (e.g. they were interrupted
	 * and went away, or something).
	 */
	ts = turnstile_lookup(mtx);
	MUTEX_RELEASE(mtx);
	if (ts == NULL)
		turnstile_exit(mtx);
	else
		turnstile_wakeup(ts, TS_WRITER_Q, ts->ts_waiters);
}

/*
 * mutex_owner:
 *
 *	Return the owner of the mutex.
 */
struct proc *
mutex_owner(kmutex_t *mtx)
{

	/*
	 * Only adaptive mutexes are owned by threads.
	 */
	return (MUTEX_ADAPTIVE_P(mtx) ? MUTEX_OWNER(mtx) : NULL);
}
