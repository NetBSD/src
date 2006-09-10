/*	$NetBSD: kern_mutex.c,v 1.1.36.1 2006/09/10 23:42:41 ad Exp $	*/

/*-
 * Copyright (c) 2002, 2006 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe and Andrew Doran.
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

#include "opt_lockdebug.h"
#include "opt_multiprocessor.h"

#define	__MUTEX_PRIVATE

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: kern_mutex.c,v 1.1.36.1 2006/09/10 23:42:41 ad Exp $");

#include <sys/param.h>
#include <sys/proc.h>
#include <sys/mutex.h>
#include <sys/sched.h>
#include <sys/turnstile.h>
#include <sys/systm.h>

#include <dev/lockstat.h>

#include <machine/intr.h>

#define	MUTEX_ABORT(mtx, msg)					\
    mutex_abort((mtx), __FUNCTION__, msg)

#if defined(LOCKDEBUG)

#undef __HAVE_MUTEX_ENTER
#undef __HAVE_MUTEX_EXIT
#undef __HAVE_MUTEX_EXIT_LINKED

#ifdef notyet
#define	MUTEX_LOCKED(mtx)					\
	(mtx)->mtx_debug.mtx_locked = (vaddr_t) __builtin_return_address(0)
#define	MUTEX_UNLOCKED(mtx)					\
	(mtx)->mtx_debug.mtx_unlocked = (vaddr_t) __builtin_return_address(0)
#else	/* notyet */
#define	MUTEX_LOCKED(mtx)		/* nothing */
#define	MUTEX_UNLOCKED(mtx)		/* nothing */
#endif	/* notyet */

#define	MUTEX_DASSERT(mtx, cond)				\
do {								\
	if (!(cond))						\
		MUTEX_ABORT(mtx, "assertion failed: " #cond);	\
} while (/* CONSTCOND */ 0);
	
#else

#define	MUTEX_LOCKED(mtx)		/* nothing */
#define	MUTEX_UNLOCKED(mtx)		/* nothing */
#define	MUTEX_DASSERT(mtx, cond)	/* nothing */

#endif /* LOCKDEBUG */

#if defined(DIAGNOSTIC)

#define	MUTEX_ASSERT(mtx, cond)					\
do {								\
	if (!(cond))						\
		MUTEX_ABORT(mtx, "assertion failed: " #cond);	\
} while (/* CONSTCOND */ 0)

#else

#define	MUTEX_ASSERT(mtx, cond)	/* nothing */

#endif	/* DIAGNOSTIC */

void	mutex_dump(kmutex_t *);

/*
 * mutex_abort:
 *
 *	Dump the contents of a mutex structure and call panic().
 */
static void
mutex_abort(kmutex_t *mtx, const char *func, const char *msg)
{

	printf("%s: %s\n", func, msg);
	mutex_dump(mtx);
	panic("mutex_abort");
}

/*
 * mutex_dump:
 *
 *	Dump the contents of a mutex structure.
 */
void
mutex_dump(kmutex_t *mtx)
{

#ifdef notyet
#if defined(LOCKDEBUG)
	printf("last locked  : 0x%16lx unlocked : 0x%16lx\n",
	    (long)mtx->mtx_debug.mtx_locked,
	    (long)mtx->mtx_debug.mtx_unlocked);
#endif
#endif
	printf("curlwp       : 0x%16lx lockaddr : 0x%16lx\n"
	    "owner field  : 0x%16lx wait/spin: %d/%d\n"
	    "minspl       : 0x%16x oldspl   : 0x%16x\n",
	    (long)curlwp, (long)mtx, (long)MUTEX_OWNER(mtx),
	    MUTEX_HAS_WAITERS(mtx), MUTEX_SPIN_P(mtx),
	    (int)mtx->mtx_minspl, (int)mtx->mtx_oldspl);
}

/*
 * mutex_init:
 *
 *	Initialize a mutex for use.
 */
void
mutex_init(kmutex_t *mtx, kmutex_type_t type, int ipl)
{

	memset(mtx, 0, sizeof(*mtx));

	if (type == MUTEX_DRIVER)
		type = (ipl == IPL_NONE ? MUTEX_ADAPTIVE : MUTEX_SPIN);

	switch (type) {
	case MUTEX_ADAPTIVE:
	case MUTEX_DEFAULT:
		KASSERT(ipl == IPL_NONE);
		MUTEX_INITIALIZE_ADAPTIVE(mtx);
		break;
	case MUTEX_SPIN:
		KASSERT(ipl != IPL_NONE);
		MUTEX_INITIALIZE_SPIN(mtx, ipl);
		break;
	default:
		panic("mutex_init: impossible type");
		break;
	}
}

/*
 * mutex_destroy:
 *
 *	Tear down a mutex.
 */
void
mutex_destroy(kmutex_t *mtx)
{

	if (MUTEX_ADAPTIVE_P(mtx))
		KASSERT(MUTEX_OWNER(mtx) == 0 && !MUTEX_HAS_WAITERS(mtx));
	else
		KASSERT(!MUTEX_SPIN_HELD(mtx));
}

/*
 * mutex_vector_enter:
 *
 *	Support routine for mutex_enter(); handles situations where
 *	a mutex was not acquired on the first try.  For spin mutexes,
 *	the caller must have already raised the SPL.
 */
void
mutex_vector_enter(kmutex_t *mtx, int s)
{
	uintptr_t owner, curthread;
	struct turnstile *ts;
	int count, minspl;

	LOCKSTAT_COUNTER(spincnt);
	LOCKSTAT_COUNTER(slpcnt);
	LOCKSTAT_TIMER(spintime);
	LOCKSTAT_TIMER(slptime);

	/*
	 * Handle spin mutexes.
	 */
	if (MUTEX_SPIN_P(mtx)) {
		minspl = mtx->mtx_minspl;

		LOCKSTAT_START_TIMER(spintime);

		for (count = 0; !MUTEX_SPIN_ACQUIRE(mtx);) {
			splx(s);
			SPINLOCK_BACKOFF(count);
			s = splraiseipl(minspl);
		}

		if (count != 0) {
			LOCKSTAT_STOP_TIMER(spintime);
			LOCKSTAT_EVENT(mtx, LB_SPIN_MUTEX | LB_SPIN, 1,
			    spintime);
		}

		mtx->mtx_oldspl = s;
		MUTEX_LOCKED(mtx);
		return;
	}

	curthread = (uintptr_t)curlwp;

	MUTEX_DASSERT(mtx, MUTEX_ADAPTIVE_P(mtx));
	MUTEX_DASSERT(mtx, curthread != 0);

#ifdef LOCKDEBUG
	simple_lock_only_held(NULL, "mutex_enter");
#endif

	/*
	 * Adaptive mutex; spin trying to acquire the mutex.  If we
	 * determine that the owner is not running on a processor,
	 * then we stop spinning, and sleep instead.
	 */
	for (;;) {
		if ((owner = MUTEX_OWNER(mtx)) == 0) {
			/*
			 * Mutex owner clear could mean two things:
			 *	* The mutex has been released.
			 *	* The owner field hasn't been set yet.
			 * Try to acquire it again.  If that fails,
			 * we'll just loop again.
			 */
			if (MUTEX_ACQUIRE(mtx, curthread))
				break;
			continue;
		}

		if (panicstr != NULL)
			return;
		if (owner == curthread)
			MUTEX_ABORT(mtx, "locking against myself");

		/*
		 * Check to see if the owner is running on a processor.
		 * If so, then we should just spin, as the owner will
		 * likely release the lock very soon.  If not, then we
		 * have to pay a context switch penalty anyway (for the
		 * owner to run), so we might as well block ourselves.
		 */
		if (lock_owner_onproc(owner)) {
			LOCKSTAT_START_TIMER(spintime);

			for (count = 0;;) {
				owner = MUTEX_OWNER(mtx);
				if (owner == 0 || !lock_owner_onproc(owner))
					break;
				SPINLOCK_BACKOFF(count);
			}

			LOCKSTAT_STOP_TIMER(spintime);
			LOCKSTAT_COUNT(spincnt, 1);
			continue;
		}

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
		 * interlock on the sleep queue).  Note we must not
		 * set the waiters bit if we spin again.
		 */
		if (lock_owner_onproc(owner) || !MUTEX_SET_WAITERS(mtx)) {
			turnstile_exit(mtx);
			continue;
		}

		LOCKSTAT_START_TIMER(slptime);

		turnstile_block(ts, TS_WRITER_Q, sched_kpri(curlwp), mtx,
		    "mutex");

		LOCKSTAT_STOP_TIMER(slptime);
		LOCKSTAT_COUNT(slpcnt, 1);
	}

	LOCKSTAT_EVENT(mtx, LB_ADAPTIVE_MUTEX | LB_SLEEP, slpcnt, slptime);
	LOCKSTAT_EVENT(mtx, LB_ADAPTIVE_MUTEX | LB_SPIN, spincnt, spintime);

	MUTEX_LOCKED(mtx);
	LOCK_ASSERT(MUTEX_OWNER(mtx) == curthread);
}

/*
 * mutex_vector_exit:
 *
 *	Support routine for mutex_exit(); handles mutexes
 *	with waiters.  Does not release spin mutexes.
 */
void
mutex_vector_exit(kmutex_t *mtx)
{
	struct turnstile *ts;
	uintptr_t curthread;

	curthread = (uintptr_t)curlwp;

	if (MUTEX_SPIN_P(mtx))
		MUTEX_ABORT(mtx, "exiting unheld spin mutex");

	MUTEX_DASSERT(mtx, MUTEX_ADAPTIVE_P(mtx));
	MUTEX_DASSERT(mtx, curthread != 0);
	MUTEX_ASSERT(mtx, MUTEX_OWNER(mtx) == curthread);
	MUTEX_UNLOCKED(mtx);

	/*
	 * Get this lock's turnstile.  This gets the interlock on
	 * the sleep queue.  Once we have that, we can clear the
	 * lock.  If there was no turnstile for the lock, there
	 * were no waiters remaining.
	 */
	ts = turnstile_lookup(mtx);
	MUTEX_DASSERT(mtx, (ts == NULL && !MUTEX_HAS_WAITERS(mtx)) ||
	    (ts != NULL && MUTEX_HAS_WAITERS(mtx)));
	MUTEX_RELEASE(mtx);
	if (ts == NULL)
		turnstile_exit(mtx);
	else
		turnstile_wakeup(ts, TS_WRITER_Q,
		    TS_WAITERS(ts, TS_WRITER_Q), NULL);
}

/*
 * mutex_owned:
 *
 *	Return true if the current thread holds the mutex.
 */
int
mutex_owned(kmutex_t *mtx)
{

	if (MUTEX_ADAPTIVE_P(mtx))
		return MUTEX_OWNER(mtx) == (uintptr_t)curlwp;
#if defined(DIAGNOSTIC) || defined(MULTIPROCESSOR)
	return MUTEX_SPIN_HELD(mtx);
#else
	return 1;
#endif
}

/*
 * mutex_owner:
 *
 *	Return the current owner of an adaptive mutex.
 */
struct lwp *
mutex_owner(kmutex_t *mtx)
{

	MUTEX_ASSERT(mtx, MUTEX_ADAPTIVE_P(mtx));
	return (struct lwp *)MUTEX_OWNER(mtx);
}

/*
 * mutex_tryenter:
 *
 *	Try to acquire the mutex; return non-zero if we did.
 */
int
mutex_tryenter(kmutex_t *mtx)
{
	uintptr_t curthread;
	int s;

	/*
	 * Handle spin mutexes.
	 */
	if (MUTEX_SPIN_P(mtx)) {
		s = splraiseipl(mtx->mtx_minspl);
		if (MUTEX_SPIN_ACQUIRE(mtx)) {
			mtx->mtx_oldspl = s;
			MUTEX_LOCKED(mtx);
			return 1;
		}
		splx(s);
	} else {
		curthread = (uintptr_t)curlwp;
		MUTEX_ASSERT(mtx, curthread != 0);
		if (MUTEX_ACQUIRE(mtx, curthread)) {
			MUTEX_LOCKED(mtx);
			MUTEX_DASSERT(mtx, MUTEX_OWNER(mtx) == curthread);
			return 1;
		}
	}

	return 0;
}

/*
 * mutex_enter, mutex_exit, mutex_link, mutex_exit_linked
 *
 *	Stub for platforms that don't implement them
 */

#if defined(MULTIPROCESSOR) || defined(DIAGNOSTIC) || defined(LOCKDEBUG)

#ifndef __HAVE_MUTEX_ENTER
void
mutex_enter(kmutex_t *mtx)
{
	int s = 0;

	if (MUTEX_SPIN_P(mtx))
		s = splraiseipl(mtx->mtx_minspl);
	mutex_vector_enter(mtx, s);
}
#endif

#ifndef __HAVE_MUTEX_EXIT
void
mutex_exit(kmutex_t *mtx)
{
	int s;

	if (MUTEX_SPIN_P(mtx)) {
		if (!MUTEX_SPIN_HELD(mtx))
			mutex_vector_exit(mtx);
		s = mtx->mtx_oldspl;
		MUTEX_UNLOCKED(mtx);
		MUTEX_SPIN_RELEASE(mtx);
		splx(s);
		return;
	}

	mutex_vector_exit(mtx);
}
#endif

#ifndef __HAVE_MUTEX_EXIT_LINKED
void
mutex_exit_linked(kmutex_t *from, kmutex_t *to)
{
	to->mtx_oldspl = from->mtx_oldspl;
	if (!MUTEX_SPIN_P(from) || !MUTEX_SPIN_HELD(from))
		mutex_vector_exit(from);
	MUTEX_UNLOCKED(from);
	MUTEX_SPIN_RELEASE(from);
}
#endif

#else	/* defined(MULTIPROCESSOR)||defined(DIAGNOSTIC)||defined(LOCKDEBUG) */

#ifndef __HAVE_MUTEX_ENTER
void
mutex_enter(kmutex_t *mtx)
{
	if (MUTEX_SPIN_P(mtx)) {
		mtx->mtx_oldspl = splraiseipl(mtx->mtx_minspl);
		return;
	}
	mutex_vector_enter(mtx, 0);
}
#endif

#ifndef __HAVE_MUTEX_EXIT
void
mutex_exit(kmutex_t *mtx)
{
	if (MUTEX_SPIN_P(mtx)) {
		splx(mtx->mtx_oldspl);
		return;
	}
	mutex_vector_exit(mtx);
}
#endif

#ifndef __HAVE_MUTEX_EXIT_LINKED
void
mutex_exit_linked(kmutex_t *from, kmutex_t *to)
{
	to->mtx_oldspl = from->mtx_oldspl;
}
#endif

#endif	/* defined(MULTIPROCESSOR) || defined(LOCKDEBUG) */
