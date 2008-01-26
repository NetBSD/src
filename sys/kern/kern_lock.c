/*	$NetBSD: kern_lock.c,v 1.133 2008/01/26 14:29:31 ad Exp $	*/

/*-
 * Copyright (c) 1999, 2000, 2006, 2007, 2008 The NetBSD Foundation, Inc.
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
__KERNEL_RCSID(0, "$NetBSD: kern_lock.c,v 1.133 2008/01/26 14:29:31 ad Exp $");

#include "opt_multiprocessor.h"

#include <sys/param.h>
#include <sys/proc.h>
#include <sys/lock.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/lockdebug.h>
#include <sys/cpu.h>
#include <sys/syslog.h>
#include <sys/atomic.h>

#include <machine/stdarg.h>
#include <machine/lock.h>

#include <dev/lockstat.h>

/*
 * note that stdarg.h and the ansi style va_start macro is used for both
 * ansi and traditional c compiles.
 * XXX: this requires that stdarg.h define: va_alist and va_dcl
 */
void	lock_printf(const char *fmt, ...)
    __attribute__((__format__(__printf__,1,2)));

static int acquire(struct lock **, int *, int, int, int, uintptr_t);

int	lock_debug_syslog = 0;	/* defaults to printf, but can be patched */
bool	kernel_lock_dodebug;

__cpu_simple_lock_t kernel_lock[CACHE_LINE_SIZE / sizeof(__cpu_simple_lock_t)]
    __aligned(CACHE_LINE_SIZE);

#ifdef LOCKDEBUG
static lockops_t lockmgr_lockops = {
	"lockmgr",
	1,
	(void *)nullop
};
#endif

#if defined(LOCKDEBUG) || defined(DIAGNOSTIC) /* { */
#define	COUNT(lkp, l, cpu_id, x)	(l)->l_locks += (x)
#else
#define COUNT(lkp, p, cpu_id, x)
#endif /* LOCKDEBUG || DIAGNOSTIC */ /* } */

#define	RETURN_ADDRESS		((uintptr_t)__builtin_return_address(0))

/*
 * Acquire a resource.
 */
static int
acquire(struct lock **lkpp, int *s, int extflags,
	int drain, int wanted, uintptr_t ra)
{
	int error;
	struct lock *lkp = *lkpp;

	KASSERT(drain || (wanted & LK_WAIT_NONZERO) == 0);

	for (error = 0; (lkp->lk_flags & wanted) != 0; ) {
		if (drain)
			lkp->lk_flags |= LK_WAITDRAIN;
		else {
			lkp->lk_waitcount++;
			lkp->lk_flags |= LK_WAIT_NONZERO;
		}
		error = mtsleep(drain ? (void *)&lkp->lk_flags : (void *)lkp,
		    lkp->lk_prio, lkp->lk_wmesg, lkp->lk_timo,
		    __UNVOLATILE(&lkp->lk_interlock));
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
	}

	return error;
}

#define	SETHOLDER(lkp, pid, lid, cpu_id)				\
do {									\
	(lkp)->lk_lockholder = pid;					\
	(lkp)->lk_locklwp = lid;					\
} while (/*CONSTCOND*/0)

#define	WEHOLDIT(lkp, pid, lid, cpu_id)					\
	 ((lkp)->lk_lockholder == (pid) && (lkp)->lk_locklwp == (lid))

#define	WAKEUP_WAITER(lkp)						\
do {									\
	if (((lkp)->lk_flags & LK_WAIT_NONZERO) != 0) {			\
		wakeup((lkp));						\
	}								\
} while (/*CONSTCOND*/0)

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
lockpanic(struct lock *lkp, const char *fmt, ...)
{
	char s[150], b[150];
	static const char *locktype[] = {
	    "*0*", "shared", "exclusive", "*3*", "*4*", "downgrade",
	    "*release*", "drain", "exclother", "*9*", "*10*",
	    "*11*", "*12*", "*13*", "*14*", "*15*"
	};
	va_list ap;
	va_start(ap, fmt);
	vsnprintf(s, sizeof(s), fmt, ap);
	va_end(ap);
	bitmask_snprintf(lkp->lk_flags, __LK_FLAG_BITS, b, sizeof(b));
	panic("%s ("
	    "type %s flags %s, sharecount %d, exclusivecount %d, "
	    "recurselevel %d, waitcount %d, wmesg %s"
	    ", lock_addr %p, unlock_addr %p"
	    ")\n",
	    s, locktype[lkp->lk_flags & LK_TYPE_MASK],
	    b, lkp->lk_sharecount, lkp->lk_exclusivecount,
	    lkp->lk_recurselevel, lkp->lk_waitcount, lkp->lk_wmesg,
	    (void *)lkp->lk_lock_addr, (void *)lkp->lk_unlock_addr
	);
}

/*
 * Initialize a lock; required before use.
 */
void
lockinit(struct lock *lkp, pri_t prio, const char *wmesg, int timo, int flags)
{

	memset(lkp, 0, sizeof(struct lock));
	lkp->lk_flags = flags & LK_EXTFLG_MASK;
	mutex_init(&lkp->lk_interlock, MUTEX_DEFAULT, IPL_NONE);
	lkp->lk_lockholder = LK_NOPROC;
	lkp->lk_prio = prio;
	lkp->lk_timo = timo;
	lkp->lk_wmesg = wmesg;
	lkp->lk_lock_addr = 0;
	lkp->lk_unlock_addr = 0;

	if (LOCKDEBUG_ALLOC(lkp, &lockmgr_lockops,
	    (uintptr_t)__builtin_return_address(0))) {
		lkp->lk_flags |= LK_DODEBUG;
	}
}

void
lockdestroy(struct lock *lkp)
{

	LOCKDEBUG_FREE(((lkp->lk_flags & LK_DODEBUG) != 0), lkp);
	mutex_destroy(&lkp->lk_interlock);
}

/*
 * Determine the status of a lock.
 */
int
lockstatus(struct lock *lkp)
{
	int lock_type = 0;
	struct lwp *l = curlwp; /* XXX */
	pid_t pid;
	lwpid_t lid;
	cpuid_t cpu_num;

	if (l == NULL) {
		cpu_num = cpu_number();
		pid = LK_KERNPROC;
		lid = 0;
	} else {
		cpu_num = LK_NOCPU;
		pid = l->l_proc->p_pid;
		lid = l->l_lid;
	}

	mutex_enter(&lkp->lk_interlock);
	if (lkp->lk_exclusivecount != 0) {
		if (WEHOLDIT(lkp, pid, lid, cpu_num))
			lock_type = LK_EXCLUSIVE;
		else
			lock_type = LK_EXCLOTHER;
	} else if (lkp->lk_sharecount != 0)
		lock_type = LK_SHARED;
	else if (lkp->lk_flags & LK_WANT_EXCL)
		lock_type = LK_EXCLOTHER;
	mutex_exit(&lkp->lk_interlock);
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
 * accepted shared locks to go away.
 */
int
lockmgr(struct lock *lkp, u_int flags, kmutex_t *interlkp)
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
	KASSERT((l->l_pflag & LP_INTR) == 0 || panicstr != NULL);

	mutex_enter(&lkp->lk_interlock);
	if (flags & LK_INTERLOCK)
		mutex_exit(interlkp);
	extflags = (flags | lkp->lk_flags) & LK_EXTFLG_MASK;

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
			    (LK_HAVE_EXCL | LK_WANT_EXCL))) {
				error = EBUSY;
				break;
			}
			/*
			 * Wait for exclusive locks to clear.
			 */
			error = acquire(&lkp, &s, extflags, 0,
			    LK_HAVE_EXCL | LK_WANT_EXCL,
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
		lkp->lk_unlock_addr = RETURN_ADDRESS;
#endif
		WAKEUP_WAITER(lkp);
		break;

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
			COUNT(lkp, l, cpu_num, 1);
			break;
		}
		/*
		 * If we are just polling, check to see if we will sleep.
		 */
		if ((extflags & LK_NOWAIT) && (lkp->lk_flags &
		     (LK_HAVE_EXCL | LK_WANT_EXCL | LK_SHARE_NONZERO))) {
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
		 * Wait for shared locks to finish.
		 */
		error = acquire(&lkp, &s, extflags, 0,
		    LK_HAVE_EXCL | LK_SHARE_NONZERO,
		    RETURN_ADDRESS);
		lkp->lk_flags &= ~LK_WANT_EXCL;
		if (error) {
			WAKEUP_WAITER(lkp);
			break;
		}
		lkp->lk_flags |= LK_HAVE_EXCL;
		SETHOLDER(lkp, pid, lid, cpu_num);
#if defined(LOCKDEBUG)
		lkp->lk_lock_addr = RETURN_ADDRESS;
#endif
		if (lkp->lk_exclusivecount != 0)
			lockpanic(lkp, "lockmgr: non-zero exclusive count");
		lkp->lk_exclusivecount = 1;
		COUNT(lkp, l, cpu_num, 1);
		break;

	case LK_RELEASE:
		if (lkp->lk_exclusivecount != 0) {
			if (WEHOLDIT(lkp, pid, lid, cpu_num) == 0) {
				lockpanic(lkp, "lockmgr: pid %d.%d, not "
				    "exclusive lock holder %d.%d "
				    "unlocking", pid, lid,
				    lkp->lk_lockholder,
				    lkp->lk_locklwp);
			}
			if (lkp->lk_exclusivecount == lkp->lk_recurselevel)
				lkp->lk_recurselevel = 0;
			lkp->lk_exclusivecount--;
			COUNT(lkp, l, cpu_num, -1);
			if (lkp->lk_exclusivecount == 0) {
				lkp->lk_flags &= ~LK_HAVE_EXCL;
				SETHOLDER(lkp, LK_NOPROC, 0, LK_NOCPU);
#if defined(LOCKDEBUG)
				lkp->lk_unlock_addr = RETURN_ADDRESS;
#endif
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
		     (LK_HAVE_EXCL | LK_WANT_EXCL |
		     LK_SHARE_NONZERO | LK_WAIT_NONZERO))) {
			error = EBUSY;
			break;
		}
		error = acquire(&lkp, &s, extflags, 1,
		    LK_HAVE_EXCL | LK_WANT_EXCL |
		    LK_SHARE_NONZERO | LK_WAIT_NONZERO,
		    RETURN_ADDRESS);
		if (error)
			break;
		lkp->lk_flags |= LK_HAVE_EXCL;
		if ((extflags & LK_RESURRECT) == 0)
			lkp->lk_flags |= LK_DRAINING;
		SETHOLDER(lkp, pid, lid, cpu_num);
#if defined(LOCKDEBUG)
		lkp->lk_lock_addr = RETURN_ADDRESS;
#endif
		lkp->lk_exclusivecount = 1;
		COUNT(lkp, l, cpu_num, 1);
		break;

	default:
		mutex_exit(&lkp->lk_interlock);
		lockpanic(lkp, "lockmgr: unknown locktype request %d",
		    flags & LK_TYPE_MASK);
		/* NOTREACHED */
	}
	if ((lkp->lk_flags & LK_WAITDRAIN) != 0 &&
	    ((lkp->lk_flags &
	      (LK_HAVE_EXCL | LK_WANT_EXCL |
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

	mutex_exit(&lkp->lk_interlock);
	return (error);
}

/*
 * Print out information about state of a lock. Used by VOP_PRINT
 * routines to display ststus about contained locks.
 */
void
lockmgr_printinfo(struct lock *lkp)
{

	if (lkp->lk_sharecount)
		printf(" lock type %s: SHARED (count %d)", lkp->lk_wmesg,
		    lkp->lk_sharecount);
	else if (lkp->lk_flags & LK_HAVE_EXCL) {
		printf(" lock type %s: EXCL (count %d) by ",
		    lkp->lk_wmesg, lkp->lk_exclusivecount);
		printf("pid %d.%d", lkp->lk_lockholder,
		    lkp->lk_locklwp);
	} else
		printf(" not locked");
	if (lkp->lk_waitcount > 0)
		printf(" with %d pending", lkp->lk_waitcount);
}

#if defined(LOCKDEBUG)
void
assert_sleepable(struct simplelock *interlock, const char *msg)
{

	if (panicstr != NULL)
		return;
	LOCKDEBUG_BARRIER(kernel_lock, 1);
	if (CURCPU_IDLE_P() && !cold) {
		panic("assert_sleepable: idle");
	}
}
#endif

/*
 * rump doesn't need the kernel lock so force it out.  We cannot
 * currently easily include it for compilation because of
 * a) SPINLOCK_* b) membar_producer().  They are defined in different
 * places / way for each arch, so just simply do not bother to
 * fight a lot for no gain (i.e. pain but still no gain).
 */
#ifndef _RUMPKERNEL
/*
 * Functions for manipulating the kernel_lock.  We put them here
 * so that they show up in profiles.
 */

#define	_KERNEL_LOCK_ABORT(msg)						\
    LOCKDEBUG_ABORT(kernel_lock, &_kernel_lock_ops, __func__, msg)

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
kernel_lock_init(void)
{

	KASSERT(CACHE_LINE_SIZE >= sizeof(__cpu_simple_lock_t));
	__cpu_simple_lock_init(kernel_lock);
	kernel_lock_dodebug = LOCKDEBUG_ALLOC(kernel_lock, &_kernel_lock_ops,
	    RETURN_ADDRESS);
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
	u_int spins;
	int s;

	if (nlocks == 0)
		return;
	_KERNEL_LOCK_ASSERT(nlocks > 0);

	l = curlwp;

	if (ci->ci_biglock_count != 0) {
		_KERNEL_LOCK_ASSERT(__SIMPLELOCK_LOCKED_P(kernel_lock));
		ci->ci_biglock_count += nlocks;
		l->l_blcnt += nlocks;
		return;
	}

	_KERNEL_LOCK_ASSERT(l->l_blcnt == 0);
	LOCKDEBUG_WANTLOCK(kernel_lock_dodebug, kernel_lock, RETURN_ADDRESS,
	    0);

	s = splvm();
	if (__cpu_simple_lock_try(kernel_lock)) {
		ci->ci_biglock_count = nlocks;
		l->l_blcnt = nlocks;
		LOCKDEBUG_LOCKED(kernel_lock_dodebug, kernel_lock,
		    RETURN_ADDRESS, 0);
		splx(s);
		return;
	}

	/*
	 * To remove the ordering constraint between adaptive mutexes
	 * and kernel_lock we must make it appear as if this thread is
	 * blocking.  For non-interlocked mutex release, a store fence
	 * is required to ensure that the result of any mutex_exit()
	 * by the current LWP becomes visible on the bus before the set
	 * of ci->ci_biglock_wanted becomes visible.
	 */
	membar_producer();
	owant = ci->ci_biglock_wanted;
	ci->ci_biglock_wanted = l;

	/*
	 * Spin until we acquire the lock.  Once we have it, record the
	 * time spent with lockstat.
	 */
	LOCKSTAT_ENTER(lsflag);
	LOCKSTAT_START_TIMER(lsflag, spintime);

	spins = 0;
	do {
		splx(s);
		while (__SIMPLELOCK_LOCKED_P(kernel_lock)) {
			if (SPINLOCK_SPINOUT(spins)) {
				_KERNEL_LOCK_ABORT("spinout");
			}
			SPINLOCK_BACKOFF_HOOK;
			SPINLOCK_SPIN_HOOK;
		}
		s = splvm();
	} while (!__cpu_simple_lock_try(kernel_lock));

	ci->ci_biglock_count = nlocks;
	l->l_blcnt = nlocks;
	LOCKSTAT_STOP_TIMER(lsflag, spintime);
	LOCKDEBUG_LOCKED(kernel_lock_dodebug, kernel_lock, RETURN_ADDRESS, 0);
	if (owant == NULL) {
		LOCKSTAT_EVENT_RA(lsflag, kernel_lock,
		    LB_KERNEL_LOCK | LB_SPIN, 1, spintime, RETURN_ADDRESS);
	}
	LOCKSTAT_EXIT(lsflag);
	splx(s);

	/*
	 * Now that we have kernel_lock, reset ci_biglock_wanted.  This
	 * store must be unbuffered (immediately visible on the bus) in
	 * order for non-interlocked mutex release to work correctly. 
	 * It must be visible before a mutex_exit() can execute on this
	 * processor.
	 *
	 * Note: only where CAS is available in hardware will this be
	 * an unbuffered write, but non-interlocked release cannot be
	 * done on CPUs without CAS in hardware.
	 */
	(void)atomic_swap_ptr(&ci->ci_biglock_wanted, owant);

	/*
	 * Issue a memory barrier as we have acquired a lock.  This also
	 * prevents stores from a following mutex_exit() being reordered
	 * to occur before our store to ci_biglock_wanted above.
	 */
	membar_enter();
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

	l = curlwp;

	_KERNEL_LOCK_ASSERT(nlocks < 2);

	olocks = l->l_blcnt;

	if (olocks == 0) {
		_KERNEL_LOCK_ASSERT(nlocks <= 0);
		if (countp != NULL)
			*countp = 0;
		return;
	}

	_KERNEL_LOCK_ASSERT(__SIMPLELOCK_LOCKED_P(kernel_lock));

	if (nlocks == 0)
		nlocks = olocks;
	else if (nlocks == -1) {
		nlocks = 1;
		_KERNEL_LOCK_ASSERT(olocks == 1);
	}

	_KERNEL_LOCK_ASSERT(ci->ci_biglock_count >= l->l_blcnt);

	l->l_blcnt -= nlocks;
	if (ci->ci_biglock_count == nlocks) {
		s = splvm();
		LOCKDEBUG_UNLOCKED(kernel_lock_dodebug, kernel_lock,
		    RETURN_ADDRESS, 0);
		ci->ci_biglock_count = 0;
		__cpu_simple_unlock(kernel_lock);
		splx(s);
	} else
		ci->ci_biglock_count -= nlocks;

	if (countp != NULL)
		*countp = olocks;
}
#endif /* !_RUMPKERNEL */
