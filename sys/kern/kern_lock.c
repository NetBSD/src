/*	$NetBSD: kern_lock.c,v 1.33 2000/07/14 07:16:44 thorpej Exp $	*/

/*-
 * Copyright (c) 1999, 2000 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe of the Numerical Aerospace Simulation Facility,
 * NASA Ames Research Center.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Ross Harvey.
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
 * Copyright (c) 1995
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code contains ideas from software contributed to Berkeley by
 * Avadis Tevanian, Jr., Michael Wayne Young, and the Mach Operating
 * System project at Carnegie-Mellon University.
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
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)kern_lock.c	8.18 (Berkeley) 5/21/95
 */

#include "opt_multiprocessor.h"
#include "opt_lockdebug.h"
#include "opt_ddb.h"

#include <sys/param.h>
#include <sys/proc.h>
#include <sys/lock.h>
#include <sys/systm.h>
#include <machine/cpu.h>

#if defined(__HAVE_ATOMIC_OPERATIONS)
#include <machine/atomic.h>
#endif

#if defined(LOCKDEBUG)
#include <sys/syslog.h>
/*
 * note that stdarg.h and the ansi style va_start macro is used for both
 * ansi and traditional c compiles.
 * XXX: this requires that stdarg.h define: va_alist and va_dcl
 */
#include <machine/stdarg.h>

void	lock_printf(const char *fmt, ...);

int	lock_debug_syslog = 0;	/* defaults to printf, but can be patched */
#endif

/*
 * Locking primitives implementation.
 * Locks provide shared/exclusive sychronization.
 */

#if defined(LOCKDEBUG) || defined(DIAGNOSTIC) /* { */
#if defined(MULTIPROCESSOR) /* { */
#if defined(__HAVE_ATOMIC_OPERATIONS) /* { */
#define	COUNT_CPU(cpu_id, x)						\
	atomic_add_ulong(&curcpu()->ci_spin_locks, (x))
#else
#define	COUNT_CPU(cpu_id, x)	/* not safe */
#endif /* __HAVE_ATOMIC_OPERATIONS */ /* } */
#else
u_long	spin_locks;
#define	COUNT_CPU(cpu_id, x)	spin_locks += (x)
#endif /* MULTIPROCESSOR */ /* } */

#define	COUNT(lkp, p, cpu_id, x)					\
do {									\
	if ((lkp)->lk_flags & LK_SPIN)					\
		COUNT_CPU((cpu_id), (x));				\
	else								\
		(p)->p_locks += (x);					\
} while (/*CONSTCOND*/0)
#else
#define COUNT(lkp, p, cpu_id, x)
#endif /* LOCKDEBUG || DIAGNOSTIC */ /* } */

/*
 * Acquire a resource.
 */
#define ACQUIRE(lkp, error, extflags, drain, wanted)			\
	if ((extflags) & LK_SPIN) {					\
		int interlocked;					\
									\
		if ((drain) == 0)					\
			(lkp)->lk_waitcount++;				\
		for (interlocked = 1;;) {				\
			if (wanted) {					\
				if (interlocked) {			\
					simple_unlock(&(lkp)->lk_interlock); \
					interlocked = 0;		\
				}					\
			} else if (interlocked) {			\
				break;					\
			} else {					\
				simple_lock(&(lkp)->lk_interlock);	\
				interlocked = 1;			\
			}						\
		}							\
		if ((drain) == 0)					\
			(lkp)->lk_waitcount--;				\
		KASSERT((wanted) == 0);					\
		error = 0;	/* sanity */				\
	} else {							\
		for (error = 0; wanted; ) {				\
			if ((drain))					\
				(lkp)->lk_flags |= LK_WAITDRAIN;	\
			else						\
				(lkp)->lk_waitcount++;			\
			/* XXX Cast away volatile. */			\
			error = ltsleep((drain) ? &(lkp)->lk_flags :	\
			    (void *)(lkp), (lkp)->lk_prio,		\
			    (lkp)->lk_wmesg, (lkp)->lk_timo,		\
			    &(lkp)->lk_interlock);			\
			if ((drain) == 0)				\
				(lkp)->lk_waitcount--;			\
			if (error)					\
				break;					\
			if ((extflags) & LK_SLEEPFAIL) {		\
				error = ENOLCK;				\
				break;					\
			}						\
		}							\
	}

#define	SETHOLDER(lkp, pid, cpu_id)					\
do {									\
	if ((lkp)->lk_flags & LK_SPIN)					\
		(lkp)->lk_cpu = cpu_id;					\
	else								\
		(lkp)->lk_lockholder = pid;				\
} while (/*CONSTCOND*/0)

#define	WEHOLDIT(lkp, pid, cpu_id)					\
	(((lkp)->lk_flags & LK_SPIN) != 0 ?				\
	 ((lkp)->lk_cpu == (cpu_id)) : ((lkp)->lk_lockholder == (pid)))

#define	WAKEUP_WAITER(lkp)						\
do {									\
	if (((lkp)->lk_flags & LK_SPIN) == 0 && (lkp)->lk_waitcount) {	\
		/* XXX Cast away volatile. */				\
		wakeup_one((void *)(lkp));				\
	}								\
} while (/*CONSTCOND*/0)

#if defined(LOCKDEBUG) /* { */
#if defined(MULTIPROCESSOR) /* { */
struct simplelock spinlock_list_slock = SIMPLELOCK_INITIALIZER;

#define	SPINLOCK_LIST_LOCK()						\
	__cpu_simple_lock(&spinlock_list_slock.lock_data)

#define	SPINLOCK_LIST_UNLOCK()						\
	__cpu_simple_unlock(&spinlock_list_slock.lock_data)
#else
#define	SPINLOCK_LIST_LOCK()	/* nothing */

#define	SPINLOCK_LIST_UNLOCK()	/* nothing */
#endif /* MULTIPROCESSOR */ /* } */

TAILQ_HEAD(, lock) spinlock_list =
    TAILQ_HEAD_INITIALIZER(spinlock_list);

#define	HAVEIT(lkp)							\
do {									\
	if ((lkp)->lk_flags & LK_SPIN) {				\
		int s = splhigh();					\
		SPINLOCK_LIST_LOCK();					\
		/* XXX Cast away volatile. */				\
		TAILQ_INSERT_TAIL(&spinlock_list, (struct lock *)(lkp),	\
		    lk_list);						\
		SPINLOCK_LIST_UNLOCK();					\
		splx(s);						\
	}								\
} while (/*CONSTCOND*/0)

#define	DONTHAVEIT(lkp)							\
do {									\
	if ((lkp)->lk_flags & LK_SPIN) {				\
		int s = splhigh();					\
		SPINLOCK_LIST_LOCK();					\
		/* XXX Cast away volatile. */				\
		TAILQ_REMOVE(&spinlock_list, (struct lock *)(lkp),	\
		    lk_list);						\
		SPINLOCK_LIST_UNLOCK();					\
		splx(s);						\
	}								\
} while (/*CONSTCOND*/0)
#else
#define	HAVEIT(lkp)		/* nothing */

#define	DONTHAVEIT(lkp)		/* nothing */
#endif /* LOCKDEBUG */ /* } */

#if defined(LOCKDEBUG)
/*
 * Lock debug printing routine; can be configured to print to console
 * or log to syslog.
 */
void
lock_printf(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	if (lock_debug_syslog)
		vlog(LOG_DEBUG, fmt, ap);
	else
		vprintf(fmt, ap);
	va_end(ap);
}
#endif /* LOCKDEBUG */

/*
 * Initialize a lock; required before use.
 */
void
lockinit(struct lock *lkp, int prio, const char *wmesg, int timo, int flags)
{

	memset(lkp, 0, sizeof(struct lock));
	simple_lock_init(&lkp->lk_interlock);
	lkp->lk_flags = flags & LK_EXTFLG_MASK;
	if (flags & LK_SPIN)
		lkp->lk_cpu = LK_NOCPU;
	else {
		lkp->lk_lockholder = LK_NOPROC;
		lkp->lk_prio = prio;
		lkp->lk_timo = timo;
	}
	lkp->lk_wmesg = wmesg;	/* just a name for spin locks */
}

/*
 * Determine the status of a lock.
 */
int
lockstatus(struct lock *lkp)
{
	int lock_type = 0;

	simple_lock(&lkp->lk_interlock);
	if (lkp->lk_exclusivecount != 0)
		lock_type = LK_EXCLUSIVE;
	else if (lkp->lk_sharecount != 0)
		lock_type = LK_SHARED;
	simple_unlock(&lkp->lk_interlock);
	return (lock_type);
}

/*
 * XXX XXX kludge around another kludge..
 *
 * vfs_shutdown() may be called from interrupt context, either as a result
 * of a panic, or from the debugger.   It proceeds to call
 * sys_sync(&proc0, ...), pretending its running on behalf of proc0
 *
 * We would like to make an attempt to sync the filesystems in this case, so
 * if this happens, we treat attempts to acquire locks specially.
 * All locks are acquired on behalf of proc0.
 *
 * If we've already paniced, we don't block waiting for locks, but
 * just barge right ahead since we're already going down in flames.
 */

/*
 * Set, change, or release a lock.
 *
 * Shared requests increment the shared count. Exclusive requests set the
 * LK_WANT_EXCL flag (preventing further shared locks), and wait for already
 * accepted shared locks and shared-to-exclusive upgrades to go away.
 */
int
lockmgr(__volatile struct lock *lkp, u_int flags,
    struct simplelock *interlkp)
{
	int error;
	pid_t pid;
	int extflags;
	cpuid_t cpu_id;
	struct proc *p = curproc;
	int lock_shutdown_noblock = 0;

	error = 0;

	simple_lock(&lkp->lk_interlock);
	if (flags & LK_INTERLOCK)
		simple_unlock(interlkp);
	extflags = (flags | lkp->lk_flags) & LK_EXTFLG_MASK;

#ifdef DIAGNOSTIC /* { */
	/*
	 * Don't allow spins on sleep locks and don't allow sleeps
	 * on spin locks.
	 */
	if ((flags ^ lkp->lk_flags) & LK_SPIN)
		panic("lockmgr: sleep/spin mismatch\n");
#endif /* } */

	if (extflags & LK_SPIN)
		pid = LK_KERNPROC;
	else {
		if (p == NULL) {
			if (!doing_shutdown) {
#ifdef DIAGNOSTIC
				panic("lockmgr: no context");
#endif
			} else {
				p = &proc0;
				if (panicstr && (!(flags & LK_NOWAIT))) {
					flags |= LK_NOWAIT;
					lock_shutdown_noblock = 1;
				}
			}
		}
		pid = p->p_pid;
	}
	cpu_id = cpu_number();

	/*
	 * Once a lock has drained, the LK_DRAINING flag is set and an
	 * exclusive lock is returned. The only valid operation thereafter
	 * is a single release of that exclusive lock. This final release
	 * clears the LK_DRAINING flag and sets the LK_DRAINED flag. Any
	 * further requests of any sort will result in a panic. The bits
	 * selected for these two flags are chosen so that they will be set
	 * in memory that is freed (freed memory is filled with 0xdeadbeef).
	 * The final release is permitted to give a new lease on life to
	 * the lock by specifying LK_REENABLE.
	 */
	if (lkp->lk_flags & (LK_DRAINING|LK_DRAINED)) {
#ifdef DIAGNOSTIC /* { */
		if (lkp->lk_flags & LK_DRAINED)
			panic("lockmgr: using decommissioned lock");
		if ((flags & LK_TYPE_MASK) != LK_RELEASE ||
		    WEHOLDIT(lkp, pid, cpu_id) == 0)
			panic("lockmgr: non-release on draining lock: %d\n",
			    flags & LK_TYPE_MASK);
#endif /* DIAGNOSTIC */ /* } */
		lkp->lk_flags &= ~LK_DRAINING;
		if ((flags & LK_REENABLE) == 0)
			lkp->lk_flags |= LK_DRAINED;
	}

	switch (flags & LK_TYPE_MASK) {

	case LK_SHARED:
		if (WEHOLDIT(lkp, pid, cpu_id) == 0) {
			/*
			 * If just polling, check to see if we will block.
			 */
			if ((extflags & LK_NOWAIT) && (lkp->lk_flags &
			    (LK_HAVE_EXCL | LK_WANT_EXCL | LK_WANT_UPGRADE))) {
				error = EBUSY;
				break;
			}
			/*
			 * Wait for exclusive locks and upgrades to clear.
			 */
			ACQUIRE(lkp, error, extflags, 0, lkp->lk_flags &
			    (LK_HAVE_EXCL | LK_WANT_EXCL | LK_WANT_UPGRADE));
			if (error)
				break;
			lkp->lk_sharecount++;
			COUNT(lkp, p, cpu_id, 1);
			break;
		}
		/*
		 * We hold an exclusive lock, so downgrade it to shared.
		 * An alternative would be to fail with EDEADLK.
		 */
		lkp->lk_sharecount++;
		COUNT(lkp, p, cpu_id, 1);
		/* fall into downgrade */

	case LK_DOWNGRADE:
		if (WEHOLDIT(lkp, pid, cpu_id) == 0 ||
		    lkp->lk_exclusivecount == 0)
			panic("lockmgr: not holding exclusive lock");
		lkp->lk_sharecount += lkp->lk_exclusivecount;
		lkp->lk_exclusivecount = 0;
		lkp->lk_recurselevel = 0;
		lkp->lk_flags &= ~LK_HAVE_EXCL;
		SETHOLDER(lkp, LK_NOPROC, LK_NOCPU);
		DONTHAVEIT(lkp);
		WAKEUP_WAITER(lkp);
		break;

	case LK_EXCLUPGRADE:
		/*
		 * If another process is ahead of us to get an upgrade,
		 * then we want to fail rather than have an intervening
		 * exclusive access.
		 */
		if (lkp->lk_flags & LK_WANT_UPGRADE) {
			lkp->lk_sharecount--;
			COUNT(lkp, p, cpu_id, -1);
			error = EBUSY;
			break;
		}
		/* fall into normal upgrade */

	case LK_UPGRADE:
		/*
		 * Upgrade a shared lock to an exclusive one. If another
		 * shared lock has already requested an upgrade to an
		 * exclusive lock, our shared lock is released and an
		 * exclusive lock is requested (which will be granted
		 * after the upgrade). If we return an error, the file
		 * will always be unlocked.
		 */
		if (WEHOLDIT(lkp, pid, cpu_id) || lkp->lk_sharecount <= 0)
			panic("lockmgr: upgrade exclusive lock");
		lkp->lk_sharecount--;
		COUNT(lkp, p, cpu_id, -1);
		/*
		 * If we are just polling, check to see if we will block.
		 */
		if ((extflags & LK_NOWAIT) &&
		    ((lkp->lk_flags & LK_WANT_UPGRADE) ||
		     lkp->lk_sharecount > 1)) {
			error = EBUSY;
			break;
		}
		if ((lkp->lk_flags & LK_WANT_UPGRADE) == 0) {
			/*
			 * We are first shared lock to request an upgrade, so
			 * request upgrade and wait for the shared count to
			 * drop to zero, then take exclusive lock.
			 */
			lkp->lk_flags |= LK_WANT_UPGRADE;
			ACQUIRE(lkp, error, extflags, 0, lkp->lk_sharecount);
			lkp->lk_flags &= ~LK_WANT_UPGRADE;
			if (error)
				break;
			lkp->lk_flags |= LK_HAVE_EXCL;
			SETHOLDER(lkp, pid, cpu_id);
			HAVEIT(lkp);
			if (lkp->lk_exclusivecount != 0)
				panic("lockmgr: non-zero exclusive count");
			lkp->lk_exclusivecount = 1;
			if (extflags & LK_SETRECURSE)
				lkp->lk_recurselevel = 1;
			COUNT(lkp, p, cpu_id, 1);
			break;
		}
		/*
		 * Someone else has requested upgrade. Release our shared
		 * lock, awaken upgrade requestor if we are the last shared
		 * lock, then request an exclusive lock.
		 */
		if (lkp->lk_sharecount == 0)
			WAKEUP_WAITER(lkp);
		/* fall into exclusive request */

	case LK_EXCLUSIVE:
		if (WEHOLDIT(lkp, pid, cpu_id)) {
			/*
			 * Recursive lock.
			 */
			if ((extflags & LK_CANRECURSE) == 0 &&
			     lkp->lk_recurselevel == 0) {
				if (extflags & LK_RECURSEFAIL) {
					error = EDEADLK;
					break;
				} else
					panic("lockmgr: locking against myself");
			}
			lkp->lk_exclusivecount++;
			if (extflags & LK_SETRECURSE &&
			    lkp->lk_recurselevel == 0)
				lkp->lk_recurselevel = lkp->lk_exclusivecount;
			COUNT(lkp, p, cpu_id, 1);
			break;
		}
		/*
		 * If we are just polling, check to see if we will sleep.
		 */
		if ((extflags & LK_NOWAIT) && ((lkp->lk_flags &
		     (LK_HAVE_EXCL | LK_WANT_EXCL | LK_WANT_UPGRADE)) ||
		     lkp->lk_sharecount != 0)) {
			error = EBUSY;
			break;
		}
		/*
		 * Try to acquire the want_exclusive flag.
		 */
		ACQUIRE(lkp, error, extflags, 0, lkp->lk_flags &
		    (LK_HAVE_EXCL | LK_WANT_EXCL));
		if (error)
			break;
		lkp->lk_flags |= LK_WANT_EXCL;
		/*
		 * Wait for shared locks and upgrades to finish.
		 */
		ACQUIRE(lkp, error, extflags, 0, lkp->lk_sharecount != 0 ||
		       (lkp->lk_flags & LK_WANT_UPGRADE));
		lkp->lk_flags &= ~LK_WANT_EXCL;
		if (error)
			break;
		lkp->lk_flags |= LK_HAVE_EXCL;
		SETHOLDER(lkp, pid, cpu_id);
		HAVEIT(lkp);
		if (lkp->lk_exclusivecount != 0)
			panic("lockmgr: non-zero exclusive count");
		lkp->lk_exclusivecount = 1;
		if (extflags & LK_SETRECURSE)
			lkp->lk_recurselevel = 1;
		COUNT(lkp, p, cpu_id, 1);
		break;

	case LK_RELEASE:
		if (lkp->lk_exclusivecount != 0) {
			if (WEHOLDIT(lkp, pid, cpu_id) == 0) {
				if (lkp->lk_flags & LK_SPIN) {
					panic("lockmgr: processor %lu, not "
					    "exclusive lock holder %lu "
					    "unlocking", cpu_id, lkp->lk_cpu);
				} else {
					panic("lockmgr: pid %d, not "
					    "exclusive lock holder %d "
					    "unlocking", pid,
					    lkp->lk_lockholder);
				}
			}
			if (lkp->lk_exclusivecount == lkp->lk_recurselevel)
				lkp->lk_recurselevel = 0;
			lkp->lk_exclusivecount--;
			COUNT(lkp, p, cpu_id, -1);
			if (lkp->lk_exclusivecount == 0) {
				lkp->lk_flags &= ~LK_HAVE_EXCL;
				SETHOLDER(lkp, LK_NOPROC, LK_NOCPU);
				DONTHAVEIT(lkp);
			}
		} else if (lkp->lk_sharecount != 0) {
			lkp->lk_sharecount--;
			COUNT(lkp, p, cpu_id, -1);
		}
		WAKEUP_WAITER(lkp);
		break;

	case LK_DRAIN:
		/*
		 * Check that we do not already hold the lock, as it can 
		 * never drain if we do. Unfortunately, we have no way to
		 * check for holding a shared lock, but at least we can
		 * check for an exclusive one.
		 */
		if (WEHOLDIT(lkp, pid, cpu_id))
			panic("lockmgr: draining against myself");
		/*
		 * If we are just polling, check to see if we will sleep.
		 */
		if ((extflags & LK_NOWAIT) && ((lkp->lk_flags &
		     (LK_HAVE_EXCL | LK_WANT_EXCL | LK_WANT_UPGRADE)) ||
		     lkp->lk_sharecount != 0 || lkp->lk_waitcount != 0)) {
			error = EBUSY;
			break;
		}
		ACQUIRE(lkp, error, extflags, 1,
		    ((lkp->lk_flags &
		     (LK_HAVE_EXCL | LK_WANT_EXCL | LK_WANT_UPGRADE)) ||
		     lkp->lk_sharecount != 0 ||
		     lkp->lk_waitcount != 0));
		if (error)
			break;
		lkp->lk_flags |= LK_DRAINING | LK_HAVE_EXCL;
		SETHOLDER(lkp, pid, cpu_id);
		HAVEIT(lkp);
		lkp->lk_exclusivecount = 1;
		/* XXX unlikely that we'd want this */
		if (extflags & LK_SETRECURSE)
			lkp->lk_recurselevel = 1;
		COUNT(lkp, p, cpu_id, 1);
		break;

	default:
		simple_unlock(&lkp->lk_interlock);
		panic("lockmgr: unknown locktype request %d",
		    flags & LK_TYPE_MASK);
		/* NOTREACHED */
	}
	if ((lkp->lk_flags & (LK_WAITDRAIN|LK_SPIN)) == LK_WAITDRAIN &&
	    ((lkp->lk_flags &
	      (LK_HAVE_EXCL | LK_WANT_EXCL | LK_WANT_UPGRADE)) == 0 &&
	     lkp->lk_sharecount == 0 && lkp->lk_waitcount == 0)) {
		lkp->lk_flags &= ~LK_WAITDRAIN;
		wakeup_one((void *)&lkp->lk_flags);
	}
	/*
	 * Note that this panic will be a recursive panic, since
	 * we only set lock_shutdown_noblock above if panicstr != NULL.
	 */
	if (error && lock_shutdown_noblock)
		panic("lockmgr: deadlock (see previous panic)");
	
	simple_unlock(&lkp->lk_interlock);
	return (error);
}

/*
 * Print out information about state of a lock. Used by VOP_PRINT
 * routines to display ststus about contained locks.
 */
void
lockmgr_printinfo(__volatile struct lock *lkp)
{

	if (lkp->lk_sharecount)
		printf(" lock type %s: SHARED (count %d)", lkp->lk_wmesg,
		    lkp->lk_sharecount);
	else if (lkp->lk_flags & LK_HAVE_EXCL) {
		printf(" lock type %s: EXCL (count %d) by ",
		    lkp->lk_wmesg, lkp->lk_exclusivecount);
		if (lkp->lk_flags & LK_SPIN)
			printf("processor %lu", lkp->lk_cpu);
		else
			printf("pid %d", lkp->lk_lockholder);
	} else
		printf(" not locked");
	if ((lkp->lk_flags & LK_SPIN) == 0 && lkp->lk_waitcount > 0)
		printf(" with %d pending", lkp->lk_waitcount);
}

#if defined(LOCKDEBUG) /* { */
TAILQ_HEAD(, simplelock) simplelock_list =
    TAILQ_HEAD_INITIALIZER(simplelock_list);

#if defined(MULTIPROCESSOR) /* { */
struct simplelock simplelock_list_slock = SIMPLELOCK_INITIALIZER;

#define	SLOCK_LIST_LOCK()						\
	__cpu_simple_lock(&simplelock_list_slock.lock_data)

#define	SLOCK_LIST_UNLOCK()						\
	__cpu_simple_unlock(&simplelock_list_slock.lock_data)

#if defined(__HAVE_ATOMIC_OPERATIONS) /* { */
#define	SLOCK_COUNT(x)							\
	atomic_add_ulong(&curcpu()->ci_simple_locks, (x))
#else
#define	SLOCK_COUNT(x)		/* not safe */
#endif /* __HAVE_ATOMIC_OPERATIONS */ /* } */
#else
u_long simple_locks;

#define	SLOCK_LIST_LOCK()	/* nothing */

#define	SLOCK_LIST_UNLOCK()	/* nothing */

#define	SLOCK_COUNT(x)		simple_locks += (x)
#endif /* MULTIPROCESSOR */ /* } */

#ifdef DDB /* { */
int simple_lock_debugger = 0;
#define	SLOCK_DEBUGGER()	if (simple_lock_debugger) Debugger()
#else
#define	SLOCK_DEBUGGER()	/* nothing */
#endif /* } */

#ifdef MULTIPROCESSOR
#define SLOCK_MP()		lock_printf("on cpu %d\n", cpu_number())
#else
#define SLOCK_MP()		/* nothing */
#endif

#define	SLOCK_WHERE(str, alp, id, l)					\
do {									\
	lock_printf(str);						\
	lock_printf("lock: %p, currently at: %s:%d\n", (alp), (id), (l)); \
	SLOCK_MP();							\
	if ((alp)->lock_file != NULL)					\
		lock_printf("last locked: %s:%d\n", (alp)->lock_file,	\
		    (alp)->lock_line);					\
	if ((alp)->unlock_file != NULL)					\
		lock_printf("last unlocked: %s:%d\n", (alp)->unlock_file, \
		    (alp)->unlock_line);				\
	SLOCK_DEBUGGER();						\
} while (/*CONSTCOND*/0)

/*
 * Simple lock functions so that the debugger can see from whence
 * they are being called.
 */
void
simple_lock_init(struct simplelock *alp)
{

#if defined(MULTIPROCESSOR) /* { */
	__cpu_simple_lock_init(&alp->lock_data);
#else
	alp->lock_data = __SIMPLELOCK_UNLOCKED;
#endif /* } */
	alp->lock_file = NULL;
	alp->lock_line = 0;
	alp->unlock_file = NULL;
	alp->unlock_line = 0;
	alp->lock_holder = 0;
}

void
_simple_lock(__volatile struct simplelock *alp, const char *id, int l)
{
	cpuid_t cpu_id = cpu_number();
	int s;

	s = splhigh();

	/*
	 * MULTIPROCESSOR case: This is `safe' since if it's not us, we
	 * don't take any action, and just fall into the normal spin case.
	 */
	if (alp->lock_data == __SIMPLELOCK_LOCKED) {
#if defined(MULTIPROCESSOR) /* { */
		if (alp->lock_holder == cpu_id) {
			SLOCK_WHERE("simple_lock: locking against myself\n",
			    alp, id, l);
			goto out;
		}
#else
		SLOCK_WHERE("simple_lock: lock held\n", alp, id, l);
		goto out;
#endif /* MULTIPROCESSOR */ /* } */
	}

#if defined(MULTIPROCESSOR) /* { */
	/* Acquire the lock before modifying any fields. */
	__cpu_simple_lock(&alp->lock_data);
#else
	alp->lock_data = __SIMPLELOCK_LOCKED;
#endif /* } */

	alp->lock_file = id;
	alp->lock_line = l;
	alp->lock_holder = cpu_id;

	SLOCK_LIST_LOCK();
	/* XXX Cast away volatile */
	TAILQ_INSERT_TAIL(&simplelock_list, (struct simplelock *)alp, list);
	SLOCK_LIST_UNLOCK();

	SLOCK_COUNT(1);

 out:
	splx(s);
}

int
_simple_lock_try(__volatile struct simplelock *alp, const char *id, int l)
{
	cpuid_t cpu_id = cpu_number();
	int s, rv = 0;

	s = splhigh();

	/*
	 * MULTIPROCESSOR case: This is `safe' since if it's not us, we
	 * don't take any action.
	 */
#if defined(MULTIPROCESSOR) /* { */
	if ((rv = __cpu_simple_lock_try(&alp->lock_data)) == 0) {
		if (alp->lock_holder == cpu_id)
			SLOCK_WHERE("simple_lock_try: locking against myself\n",
			    alp, id, l);
		goto out;
	}
#else
	if (alp->lock_data == __SIMPLELOCK_LOCKED) {
		SLOCK_WHERE("simple_lock_try: lock held\n", alp, id, l);
		goto out;
	}
	alp->lock_data = __SIMPLELOCK_LOCKED;
#endif /* MULTIPROCESSOR */ /* } */

	/*
	 * At this point, we have acquired the lock.
	 */

	rv = 1;

	alp->lock_file = id;
	alp->lock_line = l;
	alp->lock_holder = cpu_id;

	SLOCK_LIST_LOCK();
	/* XXX Cast away volatile. */
	TAILQ_INSERT_TAIL(&simplelock_list, (struct simplelock *)alp, list);
	SLOCK_LIST_UNLOCK();

	SLOCK_COUNT(1);

 out:
	splx(s);
	return (rv);
}

void
_simple_unlock(__volatile struct simplelock *alp, const char *id, int l)
{
	int s;

	s = splhigh();

	/*
	 * MULTIPROCESSOR case: This is `safe' because we think we hold
	 * the lock, and if we don't, we don't take any action.
	 */
	if (alp->lock_data == __SIMPLELOCK_UNLOCKED) {
		SLOCK_WHERE("simple_unlock: lock not held\n",
		    alp, id, l);
		goto out;
	}

	SLOCK_LIST_LOCK();
	TAILQ_REMOVE(&simplelock_list, alp, list);
	SLOCK_LIST_UNLOCK();

	SLOCK_COUNT(-1);

	alp->list.tqe_next = NULL;	/* sanity */
	alp->list.tqe_prev = NULL;	/* sanity */

	alp->unlock_file = id;
	alp->unlock_line = l;

#if defined(MULTIPROCESSOR) /* { */
	alp->lock_holder = LK_NOCPU;
	/* Now that we've modified all fields, release the lock. */
	__cpu_simple_unlock(&alp->lock_data);
#else
	alp->lock_data = __SIMPLELOCK_UNLOCKED;
#endif /* } */

 out:
	splx(s);
}

void
simple_lock_dump(void)
{
	struct simplelock *alp;
	int s;

	s = splhigh();
	SLOCK_LIST_LOCK();
	lock_printf("all simple locks:\n");
	for (alp = TAILQ_FIRST(&simplelock_list); alp != NULL;
	     alp = TAILQ_NEXT(alp, list)) {
		lock_printf("%p CPU %lu %s:%d\n", alp, alp->lock_holder,
		    alp->lock_file, alp->lock_line);
	}
	SLOCK_LIST_UNLOCK();
	splx(s);
}

void
simple_lock_freecheck(void *start, void *end)
{
	struct simplelock *alp;
	int s;

	s = splhigh();
	SLOCK_LIST_LOCK();
	for (alp = TAILQ_FIRST(&simplelock_list); alp != NULL;
	     alp = TAILQ_NEXT(alp, list)) {
		if ((void *)alp >= start && (void *)alp < end) {
			lock_printf("freeing simple_lock %p CPU %lu %s:%d\n",
			    alp, alp->lock_holder, alp->lock_file,
			    alp->lock_line);
			SLOCK_DEBUGGER();
		}
	}
	SLOCK_LIST_UNLOCK();
	splx(s);
}
#endif /* LOCKDEBUG */ /* } */
