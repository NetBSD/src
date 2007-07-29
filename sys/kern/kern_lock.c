/*	$NetBSD: kern_lock.c,v 1.117 2007/07/29 11:45:21 ad Exp $	*/

/*-
 * Copyright (c) 1999, 2000, 2006, 2007 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe of the Numerical Aerospace Simulation Facility,
 * NASA Ames Research Center, and by Andrew Doran.
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
 * 3. Neither the name of the University nor the names of its contributors
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: kern_lock.c,v 1.117 2007/07/29 11:45:21 ad Exp $");

#include "opt_multiprocessor.h"
#include "opt_ddb.h"

#define	__MUTEX_PRIVATE

#include <sys/param.h>
#include <sys/proc.h>
#include <sys/lock.h>
#include <sys/systm.h>
#include <sys/lockdebug.h>

#include <machine/cpu.h>
#include <machine/stdarg.h>

#include <dev/lockstat.h>

#if defined(LOCKDEBUG)
#include <sys/syslog.h>
/*
 * note that stdarg.h and the ansi style va_start macro is used for both
 * ansi and traditional c compiles.
 * XXX: this requires that stdarg.h define: va_alist and va_dcl
 */
#include <machine/stdarg.h>

void	lock_printf(const char *fmt, ...)
    __attribute__((__format__(__printf__,1,2)));

static int acquire(volatile struct lock **, int *, int, int, int, uintptr_t);

int	lock_debug_syslog = 0;	/* defaults to printf, but can be patched */

#ifdef DDB
#include <ddb/ddbvar.h>
#include <machine/db_machdep.h>
#include <ddb/db_command.h>
#include <ddb/db_interface.h>
#endif
#endif /* defined(LOCKDEBUG) */

/*
 * Locking primitives implementation.
 * Locks provide shared/exclusive synchronization.
 */

#if defined(LOCKDEBUG) || defined(DIAGNOSTIC) /* { */
#if defined(MULTIPROCESSOR) /* { */
#define	COUNT_CPU(cpu_id, x)						\
	curcpu()->ci_spin_locks += (x)
#else
u_long	spin_locks;
#define	COUNT_CPU(cpu_id, x)	spin_locks += (x)
#endif /* MULTIPROCESSOR */ /* } */

#define	COUNT(lkp, l, cpu_id, x)					\
do {									\
	if ((lkp)->lk_flags & LK_SPIN)					\
		COUNT_CPU((cpu_id), (x));				\
	else								\
		(l)->l_locks += (x);					\
} while (/*CONSTCOND*/0)
#else
#define COUNT(lkp, p, cpu_id, x)
#define COUNT_CPU(cpu_id, x)
#endif /* LOCKDEBUG || DIAGNOSTIC */ /* } */

#define	INTERLOCK_ACQUIRE(lkp, flags, s)				\
do {									\
	if ((flags) & LK_SPIN)						\
		s = splhigh();						\
	simple_lock(&(lkp)->lk_interlock);				\
} while (/*CONSTCOND*/ 0)

#define	INTERLOCK_RELEASE(lkp, flags, s)				\
do {									\
	simple_unlock(&(lkp)->lk_interlock);				\
	if ((flags) & LK_SPIN)						\
		splx(s);						\
} while (/*CONSTCOND*/ 0)

#ifdef DDB /* { */
#if defined(MULTIPROCESSOR) || defined(LOCKDEBUG)
int simple_lock_debugger = 1;	/* more serious on MP */
#else
int simple_lock_debugger = 0;
#endif
#define	SLOCK_DEBUGGER()	if (simple_lock_debugger && db_onpanic) Debugger()
#define	SLOCK_TRACE()							\
	db_stack_trace_print((db_expr_t)__builtin_frame_address(0),	\
	    true, 65535, "", lock_printf);
#else
#define	SLOCK_DEBUGGER()	/* nothing */
#define	SLOCK_TRACE()		/* nothing */
#endif /* } */

#if defined(LOCKDEBUG)
#if defined(DDB)
#define	SPINLOCK_SPINCHECK_DEBUGGER	if (db_onpanic) Debugger()
#else
#define	SPINLOCK_SPINCHECK_DEBUGGER	/* nothing */
#endif

#define	SPINLOCK_SPINCHECK_DECL						\
	/* 32-bits of count -- wrap constitutes a "spinout" */		\
	uint32_t __spinc = 0

#define	SPINLOCK_SPINCHECK						\
do {									\
	if (++__spinc == 0) {						\
		lock_printf("LK_SPIN spinout, excl %d, share %d\n",	\
		    lkp->lk_exclusivecount, lkp->lk_sharecount);	\
		if (lkp->lk_exclusivecount)				\
			lock_printf("held by CPU %lu\n",		\
			    (u_long) lkp->lk_cpu);			\
		if (lkp->lk_lock_file)					\
			lock_printf("last locked at %s:%d\n",		\
			    lkp->lk_lock_file, lkp->lk_lock_line);	\
		if (lkp->lk_unlock_file)				\
			lock_printf("last unlocked at %s:%d\n",		\
			    lkp->lk_unlock_file, lkp->lk_unlock_line);	\
		SLOCK_TRACE();						\
		SPINLOCK_SPINCHECK_DEBUGGER;				\
	}								\
} while (/*CONSTCOND*/ 0)
#else
#define	SPINLOCK_SPINCHECK_DECL			/* nothing */
#define	SPINLOCK_SPINCHECK			/* nothing */
#endif /* LOCKDEBUG && DDB */

#define	RETURN_ADDRESS		((uintptr_t)__builtin_return_address(0))

/*
 * Acquire a resource.
 */
static int
acquire(volatile struct lock **lkpp, int *s, int extflags,
    int drain, int wanted, uintptr_t ra)
{
	int error;
	volatile struct lock *lkp = *lkpp;
	LOCKSTAT_TIMER(slptime);
	LOCKSTAT_FLAG(lsflag);

	KASSERT(drain || (wanted & LK_WAIT_NONZERO) == 0);

	if (extflags & LK_SPIN) {
		int interlocked;

		SPINLOCK_SPINCHECK_DECL;

		if (!drain) {
			lkp->lk_waitcount++;
			lkp->lk_flags |= LK_WAIT_NONZERO;
		}
		for (interlocked = 1;;) {
			SPINLOCK_SPINCHECK;
			if ((lkp->lk_flags & wanted) != 0) {
				if (interlocked) {
					INTERLOCK_RELEASE(lkp, LK_SPIN, *s);
					interlocked = 0;
				}
				SPINLOCK_SPIN_HOOK;
			} else if (interlocked) {
				break;
			} else {
				INTERLOCK_ACQUIRE(lkp, LK_SPIN, *s);
				interlocked = 1;
			}
		}
		if (!drain) {
			lkp->lk_waitcount--;
			if (lkp->lk_waitcount == 0)
				lkp->lk_flags &= ~LK_WAIT_NONZERO;
		}
		KASSERT((lkp->lk_flags & wanted) == 0);
		error = 0;	/* sanity */
	} else {
		LOCKSTAT_ENTER(lsflag);

		for (error = 0; (lkp->lk_flags & wanted) != 0; ) {
			if (drain)
				lkp->lk_flags |= LK_WAITDRAIN;
			else {
				lkp->lk_waitcount++;
				lkp->lk_flags |= LK_WAIT_NONZERO;
			}
			/* XXX Cast away volatile. */
			LOCKSTAT_START_TIMER(lsflag, slptime);
			error = ltsleep(drain ?
			    (volatile const void *)&lkp->lk_flags :
			    (volatile const void *)lkp, lkp->lk_prio,
			    lkp->lk_wmesg, lkp->lk_timo, &lkp->lk_interlock);
			LOCKSTAT_STOP_TIMER(lsflag, slptime);
			LOCKSTAT_EVENT_RA(lsflag, (void *)(uintptr_t)lkp,
			    LB_LOCKMGR | LB_SLEEP1, 1, slptime, ra);
			if (!drain) {
				lkp->lk_waitcount--;
				if (lkp->lk_waitcount == 0)
					lkp->lk_flags &= ~LK_WAIT_NONZERO;
			}
			if (error)
				break;
			if (extflags & LK_SLEEPFAIL) {
				error = ENOLCK;
				break;
			}
			if (lkp->lk_newlock != NULL) {
				simple_lock(&lkp->lk_newlock->lk_interlock);
				simple_unlock(&lkp->lk_interlock);
				if (lkp->lk_waitcount == 0)
					wakeup(&lkp->lk_newlock);
				*lkpp = lkp = lkp->lk_newlock;
			}
		}

		LOCKSTAT_EXIT(lsflag);
	}

	return error;
}

#define	SETHOLDER(lkp, pid, lid, cpu_id)				\
do {									\
	if ((lkp)->lk_flags & LK_SPIN)					\
		(lkp)->lk_cpu = cpu_id;					\
	else {								\
		(lkp)->lk_lockholder = pid;				\
		(lkp)->lk_locklwp = lid;				\
	}								\
} while (/*CONSTCOND*/0)

#define	WEHOLDIT(lkp, pid, lid, cpu_id)					\
	(((lkp)->lk_flags & LK_SPIN) != 0 ?				\
	 ((lkp)->lk_cpu == (cpu_id)) :					\
	 ((lkp)->lk_lockholder == (pid) && (lkp)->lk_locklwp == (lid)))

#define	WAKEUP_WAITER(lkp)						\
do {									\
	if (((lkp)->lk_flags & (LK_SPIN | LK_WAIT_NONZERO)) ==		\
	    LK_WAIT_NONZERO) {						\
		wakeup((lkp));						\
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

_TAILQ_HEAD(, struct lock, volatile) spinlock_list =
    TAILQ_HEAD_INITIALIZER(spinlock_list);

#define	HAVEIT(lkp)							\
do {									\
	if ((lkp)->lk_flags & LK_SPIN) {				\
		int sp = splhigh();					\
		SPINLOCK_LIST_LOCK();					\
		TAILQ_INSERT_TAIL(&spinlock_list, (lkp), lk_list);	\
		SPINLOCK_LIST_UNLOCK();					\
		splx(sp);						\
	}								\
} while (/*CONSTCOND*/0)

#define	DONTHAVEIT(lkp)							\
do {									\
	if ((lkp)->lk_flags & LK_SPIN) {				\
		int sp = splhigh();					\
		SPINLOCK_LIST_LOCK();					\
		TAILQ_REMOVE(&spinlock_list, (lkp), lk_list);		\
		SPINLOCK_LIST_UNLOCK();					\
		splx(sp);						\
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
	char b[150];
	va_list ap;

	va_start(ap, fmt);
	if (lock_debug_syslog)
		vlog(LOG_DEBUG, fmt, ap);
	else {
		vsnprintf(b, sizeof(b), fmt, ap);
		printf_nolog("%s", b);
	}
	va_end(ap);
}
#endif /* LOCKDEBUG */

static void
lockpanic(volatile struct lock *lkp, const char *fmt, ...)
{
	char s[150], b[150];
#ifdef LOCKDEBUG
	static const char *locktype[] = {
	    "*0*", "shared", "exclusive", "upgrade", "exclupgrade",
	    "downgrade", "release", "drain", "exclother", "*9*",
	    "*10*", "*11*", "*12*", "*13*", "*14*", "*15*"
	};
#endif
	va_list ap;
	va_start(ap, fmt);
	vsnprintf(s, sizeof(s), fmt, ap);
	va_end(ap);
	bitmask_snprintf(lkp->lk_flags, __LK_FLAG_BITS, b, sizeof(b));
	panic("%s ("
#ifdef LOCKDEBUG
	    "type %s "
#endif
	    "flags %s, sharecount %d, exclusivecount %d, "
	    "recurselevel %d, waitcount %d, wmesg %s"
#ifdef LOCKDEBUG
	    ", lock_file %s, unlock_file %s, lock_line %d, unlock_line %d"
#endif
	    ")\n",
	    s, 
#ifdef LOCKDEBUG
	    locktype[lkp->lk_flags & LK_TYPE_MASK],
#endif
	    b, lkp->lk_sharecount, lkp->lk_exclusivecount,
	    lkp->lk_recurselevel, lkp->lk_waitcount, lkp->lk_wmesg
#ifdef LOCKDEBUG
	    , lkp->lk_lock_file, lkp->lk_unlock_file, lkp->lk_lock_line,
	    lkp->lk_unlock_line
#endif
	);
}

/*
 * Transfer any waiting processes from one lock to another.
 */
void
transferlockers(struct lock *from, struct lock *to)
{

	KASSERT(from != to);
	KASSERT((from->lk_flags & LK_WAITDRAIN) == 0);
	if (from->lk_waitcount == 0)
		return;
	from->lk_newlock = to;
	wakeup((void *)from);
	tsleep((void *)&from->lk_newlock, from->lk_prio, "lkxfer", 0);
	from->lk_newlock = NULL;
	from->lk_flags &= ~(LK_WANT_EXCL | LK_WANT_UPGRADE);
	KASSERT(from->lk_waitcount == 0);
}


/*
 * Initialize a lock; required before use.
 */
void
lockinit(struct lock *lkp, pri_t prio, const char *wmesg, int timo, int flags)
{

	memset(lkp, 0, sizeof(struct lock));
	simple_lock_init(&lkp->lk_interlock);
	lkp->lk_flags = flags & LK_EXTFLG_MASK;
	if (flags & LK_SPIN)
		lkp->lk_cpu = LK_NOCPU;
	else {
		lkp->lk_lockholder = LK_NOPROC;
		lkp->lk_newlock = NULL;
		lkp->lk_prio = prio;
		lkp->lk_timo = timo;
	}
	lkp->lk_wmesg = wmesg;	/* just a name for spin locks */
#if defined(LOCKDEBUG)
	lkp->lk_lock_file = NULL;
	lkp->lk_unlock_file = NULL;
#endif
}

/*
 * Determine the status of a lock.
 */
int
lockstatus(struct lock *lkp)
{
	int s = 0; /* XXX: gcc */
	int lock_type = 0;
	struct lwp *l = curlwp; /* XXX */
	pid_t pid;
	lwpid_t lid;
	cpuid_t cpu_num;

	if ((lkp->lk_flags & LK_SPIN) || l == NULL) {
		cpu_num = cpu_number();
		pid = LK_KERNPROC;
		lid = 0;
	} else {
		cpu_num = LK_NOCPU;
		pid = l->l_proc->p_pid;
		lid = l->l_lid;
	}

	INTERLOCK_ACQUIRE(lkp, lkp->lk_flags, s);
	if (lkp->lk_exclusivecount != 0) {
		if (WEHOLDIT(lkp, pid, lid, cpu_num))
			lock_type = LK_EXCLUSIVE;
		else
			lock_type = LK_EXCLOTHER;
	} else if (lkp->lk_sharecount != 0)
		lock_type = LK_SHARED;
	else if (lkp->lk_flags & (LK_WANT_EXCL | LK_WANT_UPGRADE))
		lock_type = LK_EXCLOTHER;
	INTERLOCK_RELEASE(lkp, lkp->lk_flags, s);
	return (lock_type);
}

#if defined(LOCKDEBUG)
/*
 * Make sure no spin locks are held by a CPU that is about
 * to context switch.
 */
void
spinlock_switchcheck(void)
{
	u_long cnt;
	int s;

	if (panicstr != NULL)
		return;

	s = splhigh();
#if defined(MULTIPROCESSOR)
	cnt = curcpu()->ci_spin_locks;
#else
	cnt = spin_locks;
#endif
	splx(s);

	if (cnt != 0)
		panic("spinlock_switchcheck: CPU %lu has %lu spin locks",
		    (u_long) cpu_number(), cnt);
}
#endif /* LOCKDEBUG */

/*
 * Locks and IPLs (interrupt priority levels):
 *
 * Locks which may be taken from interrupt context must be handled
 * very carefully; you must spl to the highest IPL where the lock
 * is needed before acquiring the lock.
 *
 * It is also important to avoid deadlock, since certain (very high
 * priority) interrupts are often needed to keep the system as a whole
 * from deadlocking, and must not be blocked while you are spinning
 * waiting for a lower-priority lock.
 *
 * In addition, the lock-debugging hooks themselves need to use locks!
 *
 * A raw __cpu_simple_lock may be used from interrupts are long as it
 * is acquired and held at a single IPL.
 */

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
#if defined(LOCKDEBUG)
_lockmgr(volatile struct lock *lkp, u_int flags,
    struct simplelock *interlkp, const char *file, int line)
#else
lockmgr(volatile struct lock *lkp, u_int flags,
    struct simplelock *interlkp)
#endif
{
	int error;
	pid_t pid;
	lwpid_t lid;
	int extflags;
	cpuid_t cpu_num;
	struct lwp *l = curlwp;
	int lock_shutdown_noblock = 0;
	int s = 0;

	error = 0;

	/* LK_RETRY is for vn_lock, not for lockmgr. */
	KASSERT((flags & LK_RETRY) == 0);

	INTERLOCK_ACQUIRE(lkp, lkp->lk_flags, s);
	if (flags & LK_INTERLOCK)
		simple_unlock(interlkp);
	extflags = (flags | lkp->lk_flags) & LK_EXTFLG_MASK;

#ifdef DIAGNOSTIC /* { */
	/*
	 * Don't allow spins on sleep locks and don't allow sleeps
	 * on spin locks.
	 */
	if ((flags ^ lkp->lk_flags) & LK_SPIN)
		lockpanic(lkp, "lockmgr: sleep/spin mismatch");
#endif /* } */

	if (extflags & LK_SPIN) {
		pid = LK_KERNPROC;
		lid = 0;
	} else {
		if (l == NULL) {
			if (!doing_shutdown) {
				panic("lockmgr: no context");
			} else {
				l = &lwp0;
				if (panicstr && (!(flags & LK_NOWAIT))) {
					flags |= LK_NOWAIT;
					lock_shutdown_noblock = 1;
				}
			}
		}
		lid = l->l_lid;
		pid = l->l_proc->p_pid;
	}
	cpu_num = cpu_number();

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
			lockpanic(lkp, "lockmgr: using decommissioned lock");
		if ((flags & LK_TYPE_MASK) != LK_RELEASE ||
		    WEHOLDIT(lkp, pid, lid, cpu_num) == 0)
			lockpanic(lkp, "lockmgr: non-release on draining lock: %d",
			    flags & LK_TYPE_MASK);
#endif /* DIAGNOSTIC */ /* } */
		lkp->lk_flags &= ~LK_DRAINING;
		if ((flags & LK_REENABLE) == 0)
			lkp->lk_flags |= LK_DRAINED;
	}

	switch (flags & LK_TYPE_MASK) {

	case LK_SHARED:
		if (WEHOLDIT(lkp, pid, lid, cpu_num) == 0) {
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
			error = acquire(&lkp, &s, extflags, 0,
			    LK_HAVE_EXCL | LK_WANT_EXCL | LK_WANT_UPGRADE,
			    RETURN_ADDRESS);
			if (error)
				break;
			lkp->lk_sharecount++;
			lkp->lk_flags |= LK_SHARE_NONZERO;
			COUNT(lkp, l, cpu_num, 1);
			break;
		}
		/*
		 * We hold an exclusive lock, so downgrade it to shared.
		 * An alternative would be to fail with EDEADLK.
		 */
		lkp->lk_sharecount++;
		lkp->lk_flags |= LK_SHARE_NONZERO;
		COUNT(lkp, l, cpu_num, 1);
		/* fall into downgrade */

	case LK_DOWNGRADE:
		if (WEHOLDIT(lkp, pid, lid, cpu_num) == 0 ||
		    lkp->lk_exclusivecount == 0)
			lockpanic(lkp, "lockmgr: not holding exclusive lock");
		lkp->lk_sharecount += lkp->lk_exclusivecount;
		lkp->lk_flags |= LK_SHARE_NONZERO;
		lkp->lk_exclusivecount = 0;
		lkp->lk_recurselevel = 0;
		lkp->lk_flags &= ~LK_HAVE_EXCL;
		SETHOLDER(lkp, LK_NOPROC, 0, LK_NOCPU);
#if defined(LOCKDEBUG)
		lkp->lk_unlock_file = file;
		lkp->lk_unlock_line = line;
#endif
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
			if (lkp->lk_sharecount == 0)
				lkp->lk_flags &= ~LK_SHARE_NONZERO;
			COUNT(lkp, l, cpu_num, -1);
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
		if (WEHOLDIT(lkp, pid, lid, cpu_num) || lkp->lk_sharecount <= 0)
			lockpanic(lkp, "lockmgr: upgrade exclusive lock");
		lkp->lk_sharecount--;
		if (lkp->lk_sharecount == 0)
			lkp->lk_flags &= ~LK_SHARE_NONZERO;
		COUNT(lkp, l, cpu_num, -1);
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
			error = acquire(&lkp, &s, extflags, 0, LK_SHARE_NONZERO,
			    RETURN_ADDRESS);
			lkp->lk_flags &= ~LK_WANT_UPGRADE;
			if (error) {
				WAKEUP_WAITER(lkp);
				break;
			}
			lkp->lk_flags |= LK_HAVE_EXCL;
			SETHOLDER(lkp, pid, lid, cpu_num);
#if defined(LOCKDEBUG)
			lkp->lk_lock_file = file;
			lkp->lk_lock_line = line;
#endif
			HAVEIT(lkp);
			if (lkp->lk_exclusivecount != 0)
				lockpanic(lkp, "lockmgr: non-zero exclusive count");
			lkp->lk_exclusivecount = 1;
			if (extflags & LK_SETRECURSE)
				lkp->lk_recurselevel = 1;
			COUNT(lkp, l, cpu_num, 1);
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
		if (WEHOLDIT(lkp, pid, lid, cpu_num)) {
			/*
			 * Recursive lock.
			 */
			if ((extflags & LK_CANRECURSE) == 0 &&
			     lkp->lk_recurselevel == 0) {
				if (extflags & LK_RECURSEFAIL) {
					error = EDEADLK;
					break;
				} else
					lockpanic(lkp, "lockmgr: locking against myself");
			}
			lkp->lk_exclusivecount++;
			if (extflags & LK_SETRECURSE &&
			    lkp->lk_recurselevel == 0)
				lkp->lk_recurselevel = lkp->lk_exclusivecount;
			COUNT(lkp, l, cpu_num, 1);
			break;
		}
		/*
		 * If we are just polling, check to see if we will sleep.
		 */
		if ((extflags & LK_NOWAIT) && (lkp->lk_flags &
		     (LK_HAVE_EXCL | LK_WANT_EXCL | LK_WANT_UPGRADE |
		     LK_SHARE_NONZERO))) {
			error = EBUSY;
			break;
		}
		/*
		 * Try to acquire the want_exclusive flag.
		 */
		error = acquire(&lkp, &s, extflags, 0,
		    LK_HAVE_EXCL | LK_WANT_EXCL, RETURN_ADDRESS);
		if (error)
			break;
		lkp->lk_flags |= LK_WANT_EXCL;
		/*
		 * Wait for shared locks and upgrades to finish.
		 */
		error = acquire(&lkp, &s, extflags, 0,
		    LK_HAVE_EXCL | LK_WANT_UPGRADE | LK_SHARE_NONZERO,
		    RETURN_ADDRESS);
		lkp->lk_flags &= ~LK_WANT_EXCL;
		if (error) {
			WAKEUP_WAITER(lkp);
			break;
		}
		lkp->lk_flags |= LK_HAVE_EXCL;
		SETHOLDER(lkp, pid, lid, cpu_num);
#if defined(LOCKDEBUG)
		lkp->lk_lock_file = file;
		lkp->lk_lock_line = line;
#endif
		HAVEIT(lkp);
		if (lkp->lk_exclusivecount != 0)
			lockpanic(lkp, "lockmgr: non-zero exclusive count");
		lkp->lk_exclusivecount = 1;
		if (extflags & LK_SETRECURSE)
			lkp->lk_recurselevel = 1;
		COUNT(lkp, l, cpu_num, 1);
		break;

	case LK_RELEASE:
		if (lkp->lk_exclusivecount != 0) {
			if (WEHOLDIT(lkp, pid, lid, cpu_num) == 0) {
				if (lkp->lk_flags & LK_SPIN) {
					lockpanic(lkp,
					    "lockmgr: processor %lu, not "
					    "exclusive lock holder %lu "
					    "unlocking", cpu_num, lkp->lk_cpu);
				} else {
					lockpanic(lkp, "lockmgr: pid %d.%d, not "
					    "exclusive lock holder %d.%d "
					    "unlocking", pid, lid,
					    lkp->lk_lockholder,
					    lkp->lk_locklwp);
				}
			}
			if (lkp->lk_exclusivecount == lkp->lk_recurselevel)
				lkp->lk_recurselevel = 0;
			lkp->lk_exclusivecount--;
			COUNT(lkp, l, cpu_num, -1);
			if (lkp->lk_exclusivecount == 0) {
				lkp->lk_flags &= ~LK_HAVE_EXCL;
				SETHOLDER(lkp, LK_NOPROC, 0, LK_NOCPU);
#if defined(LOCKDEBUG)
				lkp->lk_unlock_file = file;
				lkp->lk_unlock_line = line;
#endif
				DONTHAVEIT(lkp);
			}
		} else if (lkp->lk_sharecount != 0) {
			lkp->lk_sharecount--;
			if (lkp->lk_sharecount == 0)
				lkp->lk_flags &= ~LK_SHARE_NONZERO;
			COUNT(lkp, l, cpu_num, -1);
		}
#ifdef DIAGNOSTIC
		else
			lockpanic(lkp, "lockmgr: release of unlocked lock!");
#endif
		WAKEUP_WAITER(lkp);
		break;

	case LK_DRAIN:
		/*
		 * Check that we do not already hold the lock, as it can
		 * never drain if we do. Unfortunately, we have no way to
		 * check for holding a shared lock, but at least we can
		 * check for an exclusive one.
		 */
		if (WEHOLDIT(lkp, pid, lid, cpu_num))
			lockpanic(lkp, "lockmgr: draining against myself");
		/*
		 * If we are just polling, check to see if we will sleep.
		 */
		if ((extflags & LK_NOWAIT) && (lkp->lk_flags &
		     (LK_HAVE_EXCL | LK_WANT_EXCL | LK_WANT_UPGRADE |
		     LK_SHARE_NONZERO | LK_WAIT_NONZERO))) {
			error = EBUSY;
			break;
		}
		error = acquire(&lkp, &s, extflags, 1,
		    LK_HAVE_EXCL | LK_WANT_EXCL | LK_WANT_UPGRADE |
		    LK_SHARE_NONZERO | LK_WAIT_NONZERO,
		    RETURN_ADDRESS);
		if (error)
			break;
		lkp->lk_flags |= LK_DRAINING | LK_HAVE_EXCL;
		SETHOLDER(lkp, pid, lid, cpu_num);
#if defined(LOCKDEBUG)
		lkp->lk_lock_file = file;
		lkp->lk_lock_line = line;
#endif
		HAVEIT(lkp);
		lkp->lk_exclusivecount = 1;
		/* XXX unlikely that we'd want this */
		if (extflags & LK_SETRECURSE)
			lkp->lk_recurselevel = 1;
		COUNT(lkp, l, cpu_num, 1);
		break;

	default:
		INTERLOCK_RELEASE(lkp, lkp->lk_flags, s);
		lockpanic(lkp, "lockmgr: unknown locktype request %d",
		    flags & LK_TYPE_MASK);
		/* NOTREACHED */
	}
	if ((lkp->lk_flags & (LK_WAITDRAIN|LK_SPIN)) == LK_WAITDRAIN &&
	    ((lkp->lk_flags &
	      (LK_HAVE_EXCL | LK_WANT_EXCL | LK_WANT_UPGRADE |
	      LK_SHARE_NONZERO | LK_WAIT_NONZERO)) == 0)) {
		lkp->lk_flags &= ~LK_WAITDRAIN;
		wakeup(&lkp->lk_flags);
	}
	/*
	 * Note that this panic will be a recursive panic, since
	 * we only set lock_shutdown_noblock above if panicstr != NULL.
	 */
	if (error && lock_shutdown_noblock)
		lockpanic(lkp, "lockmgr: deadlock (see previous panic)");

	INTERLOCK_RELEASE(lkp, lkp->lk_flags, s);
	return (error);
}

/*
 * For a recursive spinlock held one or more times by the current CPU,
 * release all N locks, and return N.
 * Intended for use in mi_switch() shortly before context switching.
 */

int
#if defined(LOCKDEBUG)
_spinlock_release_all(volatile struct lock *lkp, const char *file, int line)
#else
spinlock_release_all(volatile struct lock *lkp)
#endif
{
	int s, count;
	cpuid_t cpu_num;

	KASSERT(lkp->lk_flags & LK_SPIN);

	INTERLOCK_ACQUIRE(lkp, LK_SPIN, s);

	cpu_num = cpu_number();
	count = lkp->lk_exclusivecount;

	if (count != 0) {
#ifdef DIAGNOSTIC
		if (WEHOLDIT(lkp, 0, 0, cpu_num) == 0) {
			lockpanic(lkp, "spinlock_release_all: processor %lu, not "
			    "exclusive lock holder %lu "
			    "unlocking", (long)cpu_num, lkp->lk_cpu);
		}
#endif
		lkp->lk_recurselevel = 0;
		lkp->lk_exclusivecount = 0;
		COUNT_CPU(cpu_num, -count);
		lkp->lk_flags &= ~LK_HAVE_EXCL;
		SETHOLDER(lkp, LK_NOPROC, 0, LK_NOCPU);
#if defined(LOCKDEBUG)
		lkp->lk_unlock_file = file;
		lkp->lk_unlock_line = line;
#endif
		DONTHAVEIT(lkp);
	}
#ifdef DIAGNOSTIC
	else if (lkp->lk_sharecount != 0)
		lockpanic(lkp, "spinlock_release_all: release of shared lock!");
	else
		lockpanic(lkp, "spinlock_release_all: release of unlocked lock!");
#endif
	INTERLOCK_RELEASE(lkp, LK_SPIN, s);

	return (count);
}

/*
 * For a recursive spinlock held one or more times by the current CPU,
 * release all N locks, and return N.
 * Intended for use in mi_switch() right after resuming execution.
 */

void
#if defined(LOCKDEBUG)
_spinlock_acquire_count(volatile struct lock *lkp, int count,
    const char *file, int line)
#else
spinlock_acquire_count(volatile struct lock *lkp, int count)
#endif
{
	int s, error;
	cpuid_t cpu_num;

	KASSERT(lkp->lk_flags & LK_SPIN);

	INTERLOCK_ACQUIRE(lkp, LK_SPIN, s);

	cpu_num = cpu_number();

#ifdef DIAGNOSTIC
	if (WEHOLDIT(lkp, LK_NOPROC, 0, cpu_num))
		lockpanic(lkp, "spinlock_acquire_count: processor %lu already holds lock", (long)cpu_num);
#endif
	/*
	 * Try to acquire the want_exclusive flag.
	 */
	error = acquire(&lkp, &s, LK_SPIN, 0, LK_HAVE_EXCL | LK_WANT_EXCL,
	    RETURN_ADDRESS);
	lkp->lk_flags |= LK_WANT_EXCL;
	/*
	 * Wait for shared locks and upgrades to finish.
	 */
	error = acquire(&lkp, &s, LK_SPIN, 0,
	    LK_HAVE_EXCL | LK_SHARE_NONZERO | LK_WANT_UPGRADE,
	    RETURN_ADDRESS);
	lkp->lk_flags &= ~LK_WANT_EXCL;
	lkp->lk_flags |= LK_HAVE_EXCL;
	SETHOLDER(lkp, LK_NOPROC, 0, cpu_num);
#if defined(LOCKDEBUG)
	lkp->lk_lock_file = file;
	lkp->lk_lock_line = line;
#endif
	HAVEIT(lkp);
	if (lkp->lk_exclusivecount != 0)
		lockpanic(lkp, "lockmgr: non-zero exclusive count");
	lkp->lk_exclusivecount = count;
	lkp->lk_recurselevel = 1;
	COUNT_CPU(cpu_num, count);

	INTERLOCK_RELEASE(lkp, lkp->lk_flags, s);
}



/*
 * Print out information about state of a lock. Used by VOP_PRINT
 * routines to display ststus about contained locks.
 */
void
lockmgr_printinfo(volatile struct lock *lkp)
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
			printf("pid %d.%d", lkp->lk_lockholder,
			    lkp->lk_locklwp);
	} else
		printf(" not locked");
	if ((lkp->lk_flags & LK_SPIN) == 0 && lkp->lk_waitcount > 0)
		printf(" with %d pending", lkp->lk_waitcount);
}

#if defined(LOCKDEBUG) /* { */
_TAILQ_HEAD(, struct simplelock, volatile) simplelock_list =
    TAILQ_HEAD_INITIALIZER(simplelock_list);

#if defined(MULTIPROCESSOR) /* { */
struct simplelock simplelock_list_slock = SIMPLELOCK_INITIALIZER;

#define	SLOCK_LIST_LOCK()						\
	__cpu_simple_lock(&simplelock_list_slock.lock_data)

#define	SLOCK_LIST_UNLOCK()						\
	__cpu_simple_unlock(&simplelock_list_slock.lock_data)

#define	SLOCK_COUNT(x)							\
	curcpu()->ci_simple_locks += (x)
#else
u_long simple_locks;

#define	SLOCK_LIST_LOCK()	/* nothing */

#define	SLOCK_LIST_UNLOCK()	/* nothing */

#define	SLOCK_COUNT(x)		simple_locks += (x)
#endif /* MULTIPROCESSOR */ /* } */

#ifdef MULTIPROCESSOR
#define SLOCK_MP()		lock_printf("on CPU %ld\n", 		\
				    (u_long) cpu_number())
#else
#define SLOCK_MP()		/* nothing */
#endif

#define	SLOCK_WHERE(str, alp, id, l)					\
do {									\
	lock_printf("\n");						\
	lock_printf(str);						\
	lock_printf("lock: %p, currently at: %s:%d\n", (alp), (id), (l)); \
	SLOCK_MP();							\
	if ((alp)->lock_file != NULL)					\
		lock_printf("last locked: %s:%d\n", (alp)->lock_file,	\
		    (alp)->lock_line);					\
	if ((alp)->unlock_file != NULL)					\
		lock_printf("last unlocked: %s:%d\n", (alp)->unlock_file, \
		    (alp)->unlock_line);				\
	SLOCK_TRACE()							\
	SLOCK_DEBUGGER();						\
} while (/*CONSTCOND*/0)

/*
 * Simple lock functions so that the debugger can see from whence
 * they are being called.
 */
void
simple_lock_init(volatile struct simplelock *alp)
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
	alp->lock_holder = LK_NOCPU;
}

void
_simple_lock(volatile struct simplelock *alp, const char *id, int l)
{
	cpuid_t cpu_num = cpu_number();
	int s;

	s = splhigh();

	/*
	 * MULTIPROCESSOR case: This is `safe' since if it's not us, we
	 * don't take any action, and just fall into the normal spin case.
	 */
	if (alp->lock_data == __SIMPLELOCK_LOCKED) {
#if defined(MULTIPROCESSOR) /* { */
		if (alp->lock_holder == cpu_num) {
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
	splx(s);
	__cpu_simple_lock(&alp->lock_data);
	s = splhigh();
#else
	alp->lock_data = __SIMPLELOCK_LOCKED;
#endif /* } */

	if (alp->lock_holder != LK_NOCPU) {
		SLOCK_WHERE("simple_lock: uninitialized lock\n",
		    alp, id, l);
	}
	alp->lock_file = id;
	alp->lock_line = l;
	alp->lock_holder = cpu_num;

	SLOCK_LIST_LOCK();
	TAILQ_INSERT_TAIL(&simplelock_list, alp, list);
	SLOCK_LIST_UNLOCK();

	SLOCK_COUNT(1);

 out:
	splx(s);
}

int
_simple_lock_held(volatile struct simplelock *alp)
{
#if defined(MULTIPROCESSOR) || defined(DIAGNOSTIC)
	cpuid_t cpu_num = cpu_number();
#endif
	int s, locked = 0;

	s = splhigh();

#if defined(MULTIPROCESSOR)
	if (__cpu_simple_lock_try(&alp->lock_data) == 0)
		locked = (alp->lock_holder == cpu_num);
	else
		__cpu_simple_unlock(&alp->lock_data);
#else
	if (alp->lock_data == __SIMPLELOCK_LOCKED) {
		locked = 1;
		KASSERT(alp->lock_holder == cpu_num);
	}
#endif

	splx(s);

	return (locked);
}

int
_simple_lock_try(volatile struct simplelock *alp, const char *id, int l)
{
	cpuid_t cpu_num = cpu_number();
	int s, rv = 0;

	s = splhigh();

	/*
	 * MULTIPROCESSOR case: This is `safe' since if it's not us, we
	 * don't take any action.
	 */
#if defined(MULTIPROCESSOR) /* { */
	if ((rv = __cpu_simple_lock_try(&alp->lock_data)) == 0) {
		if (alp->lock_holder == cpu_num)
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
	alp->lock_holder = cpu_num;

	SLOCK_LIST_LOCK();
	TAILQ_INSERT_TAIL(&simplelock_list, alp, list);
	SLOCK_LIST_UNLOCK();

	SLOCK_COUNT(1);

 out:
	splx(s);
	return (rv);
}

void
_simple_unlock(volatile struct simplelock *alp, const char *id, int l)
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
	KASSERT(alp->lock_holder == cpu_number());
	alp->lock_holder = LK_NOCPU;
#endif /* } */

 out:
	splx(s);
}

void
simple_lock_dump(void)
{
	volatile struct simplelock *alp;
	int s;

	s = splhigh();
	SLOCK_LIST_LOCK();
	lock_printf("all simple locks:\n");
	TAILQ_FOREACH(alp, &simplelock_list, list) {
		lock_printf("%p CPU %lu %s:%d\n", alp, alp->lock_holder,
		    alp->lock_file, alp->lock_line);
	}
	SLOCK_LIST_UNLOCK();
	splx(s);
}

void
simple_lock_freecheck(void *start, void *end)
{
	volatile struct simplelock *alp;
	int s;

	s = splhigh();
	SLOCK_LIST_LOCK();
	TAILQ_FOREACH(alp, &simplelock_list, list) {
		if ((volatile void *)alp >= start &&
		    (volatile void *)alp < end) {
			lock_printf("freeing simple_lock %p CPU %lu %s:%d\n",
			    alp, alp->lock_holder, alp->lock_file,
			    alp->lock_line);
			SLOCK_DEBUGGER();
		}
	}
	SLOCK_LIST_UNLOCK();
	splx(s);
}

/*
 * We must be holding exactly one lock: the spc_lock.
 */

void
simple_lock_switchcheck(void)
{

	simple_lock_only_held(NULL, "switching");
}

/*
 * Drop into the debugger if lp isn't the only lock held.
 * lp may be NULL.
 */
void
simple_lock_only_held(volatile struct simplelock *lp, const char *where)
{
	volatile struct simplelock *alp;
	cpuid_t cpu_num = cpu_number();
	int s;

	if (lp) {
		LOCK_ASSERT(simple_lock_held(lp));
	}
	s = splhigh();
	SLOCK_LIST_LOCK();
	TAILQ_FOREACH(alp, &simplelock_list, list) {
		if (alp == lp)
			continue;
		if (alp->lock_holder == cpu_num)
			break;
	}
	SLOCK_LIST_UNLOCK();
	splx(s);

	if (alp != NULL) {
		lock_printf("\n%s with held simple_lock %p "
		    "CPU %lu %s:%d\n",
		    where, alp, alp->lock_holder, alp->lock_file,
		    alp->lock_line);
		SLOCK_TRACE();
		SLOCK_DEBUGGER();
	}
}

/*
 * Set to 1 by simple_lock_assert_*().
 * Can be cleared from ddb to avoid a panic.
 */
int slock_assert_will_panic;

/*
 * If the lock isn't held, print a traceback, optionally drop into the
 *  debugger, then panic.
 * The panic can be avoided by clearing slock_assert_with_panic from the
 *  debugger.
 */
void
_simple_lock_assert_locked(volatile struct simplelock *alp,
    const char *lockname, const char *id, int l)
{
	if (simple_lock_held(alp) == 0) {
		slock_assert_will_panic = 1;
		lock_printf("%s lock not held\n", lockname);
		SLOCK_WHERE("lock not held", alp, id, l);
		if (slock_assert_will_panic && panicstr == NULL)
			panic("%s: not locked", lockname);
	}
}

void
_simple_lock_assert_unlocked(volatile struct simplelock *alp,
    const char *lockname, const char *id, int l)
{
	if (simple_lock_held(alp)) {
		slock_assert_will_panic = 1;
		lock_printf("%s lock held\n", lockname);
		SLOCK_WHERE("lock held", alp, id, l);
		if (slock_assert_will_panic && panicstr == NULL)
			panic("%s: locked", lockname);
	}
}

void
assert_sleepable(struct simplelock *interlock, const char *msg)
{

	if (panicstr != NULL)
		return;
	if (CURCPU_IDLE_P()) {
		panic("assert_sleepable: idle");
	}
	simple_lock_only_held(interlock, msg);
}

#endif /* LOCKDEBUG */ /* } */

int kernel_lock_id;
__cpu_simple_lock_t kernel_lock;

#if defined(MULTIPROCESSOR)

/*
 * Functions for manipulating the kernel_lock.  We put them here
 * so that they show up in profiles.
 */

#define	_KERNEL_LOCK_ABORT(msg)						\
    LOCKDEBUG_ABORT(kernel_lock_id, &kernel_lock, &_kernel_lock_ops,	\
        __FUNCTION__, msg)

#ifdef LOCKDEBUG
#define	_KERNEL_LOCK_ASSERT(cond)					\
do {									\
	if (!(cond))							\
		_KERNEL_LOCK_ABORT("assertion failed: " #cond);		\
} while (/* CONSTCOND */ 0)
#else
#define	_KERNEL_LOCK_ASSERT(cond)	/* nothing */
#endif

void	_kernel_lock_dump(volatile void *);

lockops_t _kernel_lock_ops = {
	"Kernel lock",
	0,
	_kernel_lock_dump
};

/*
 * Initialize the kernel lock.
 */
void
_kernel_lock_init(void)
{

	__cpu_simple_lock_init(&kernel_lock);
	kernel_lock_id = LOCKDEBUG_ALLOC(&kernel_lock, &_kernel_lock_ops);
}

/*
 * Print debugging information about the kernel lock.
 */
void
_kernel_lock_dump(volatile void *junk)
{
	struct cpu_info *ci = curcpu();

	(void)junk;

	printf_nolog("curcpu holds : %18d wanted by: %#018lx\n",
	    ci->ci_biglock_count, (long)ci->ci_biglock_wanted);
}

/*
 * Acquire 'nlocks' holds on the kernel lock.  If 'l' is non-null, the
 * acquisition is from process context.
 */
void
_kernel_lock(int nlocks, struct lwp *l)
{
	struct cpu_info *ci = curcpu();
	LOCKSTAT_TIMER(spintime);
	LOCKSTAT_FLAG(lsflag);
	struct lwp *owant;
#ifdef LOCKDEBUG
	u_int spins;
#endif
	int s;

	(void)l;

	if (nlocks == 0)
		return;
	_KERNEL_LOCK_ASSERT(nlocks > 0);

	s = splsched();	/* XXX splvm() */

	if (ci->ci_biglock_count != 0) {
		_KERNEL_LOCK_ASSERT(kernel_lock == __SIMPLELOCK_LOCKED);
		ci->ci_biglock_count += nlocks;
		splx(s);
		return;
	}

	LOCKDEBUG_WANTLOCK(kernel_lock_id,
	    (uintptr_t)__builtin_return_address(0), 0);

	if (__cpu_simple_lock_try(&kernel_lock)) {
		ci->ci_biglock_count = nlocks;
		LOCKDEBUG_LOCKED(kernel_lock_id,
		    (uintptr_t)__builtin_return_address(0), 0);
		splx(s);
		return;
	}

	LOCKSTAT_ENTER(lsflag);
	LOCKSTAT_START_TIMER(lsflag, spintime);

	/*
	 * Before setting ci_biglock_wanted we must post a store
	 * fence (see kern_mutex.c).  This is accomplished by the
	 * __cpu_simple_lock_try() above.
	 */
	owant = ci->ci_biglock_wanted;
	ci->ci_biglock_wanted = curlwp;	/* XXXAD */

#ifdef LOCKDEBUG
	spins = 0;
#endif

	do {
		while (kernel_lock == __SIMPLELOCK_LOCKED) {
#ifdef LOCKDEBUG
			if (SPINLOCK_SPINOUT(spins))
				_KERNEL_LOCK_ABORT("spinout");
#endif
			splx(s);
			SPINLOCK_SPIN_HOOK;
			(void)splsched();	/* XXX splvm() */
		}
	} while (!__cpu_simple_lock_try(&kernel_lock));

	ci->ci_biglock_wanted = owant;
	ci->ci_biglock_count += nlocks;
	LOCKSTAT_STOP_TIMER(lsflag, spintime);
	LOCKDEBUG_LOCKED(kernel_lock_id,
	    (uintptr_t)__builtin_return_address(0), 0);
	splx(s);

	/*
	 * Again, another store fence is required (see kern_mutex.c).
	 */
	mb_write();
	if (owant == NULL) {
		LOCKSTAT_EVENT(lsflag, &kernel_lock, LB_KERNEL_LOCK | LB_SPIN,
		    1, spintime);
	}
	LOCKSTAT_EXIT(lsflag);
}

/*
 * Release 'nlocks' holds on the kernel lock.  If 'nlocks' is zero, release
 * all holds.  If 'l' is non-null, the release is from process context.
 */
void
_kernel_unlock(int nlocks, struct lwp *l, int *countp)
{
	struct cpu_info *ci = curcpu();
	u_int olocks;
	int s;

	(void)l;

	_KERNEL_LOCK_ASSERT(nlocks < 2);

	olocks = ci->ci_biglock_count;

	if (olocks == 0) {
		_KERNEL_LOCK_ASSERT(nlocks <= 0);
		if (countp != NULL)
			*countp = 0;
		return;
	}

	_KERNEL_LOCK_ASSERT(kernel_lock == __SIMPLELOCK_LOCKED);

	if (nlocks == 0)
		nlocks = olocks;
	else if (nlocks == -1) {
		nlocks = 1;
		_KERNEL_LOCK_ASSERT(olocks == 1);
	}

	s = splsched();	/* XXX splvm() */
	if ((ci->ci_biglock_count -= nlocks) == 0) {
		LOCKDEBUG_UNLOCKED(kernel_lock_id,
		    (uintptr_t)__builtin_return_address(0), 0);
		__cpu_simple_unlock(&kernel_lock);
	}
	splx(s);

	if (countp != NULL)
		*countp = olocks;
}

#if defined(DEBUG)
/*
 * Assert that the kernel lock is held.
 */
void
_kernel_lock_assert_locked(void)
{

	if (kernel_lock != __SIMPLELOCK_LOCKED ||
	    curcpu()->ci_biglock_count == 0)
		_KERNEL_LOCK_ABORT("not locked");
}

void
_kernel_lock_assert_unlocked()
{

	if (curcpu()->ci_biglock_count != 0)
		_KERNEL_LOCK_ABORT("locked");
}
#endif

#endif	/* MULTIPROCESSOR || LOCKDEBUG */
