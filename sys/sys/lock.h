/*	$NetBSD: lock.h,v 1.72.6.2 2008/01/09 01:58:10 matt Exp $	*/

/*-
 * Copyright (c) 1999, 2000, 2006, 2007 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe of the Numerical Aerospace Simulation Facility,
 * NASA Ames Research Center, by Ross Harvey, and by Andrew Doran.
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
 *	@(#)lock.h	8.12 (Berkeley) 5/19/95
 */

#ifndef	_SYS_LOCK_H_
#define	_SYS_LOCK_H_

#include <sys/stdint.h>
#include <sys/mutex.h>

#include <machine/lock.h>

/*
 * The general lock structure.
 */
struct lock {
	u_int	lk_flags;		/* see below */
	int	lk_sharecount;		/* # of accepted shared locks */
	kmutex_t lk_interlock;		/* lock on structure */
	short	lk_exclusivecount;	/* # of recursive exclusive locks */
	short	lk_recurselevel;	/* lvl above which recursion ok */
	int	lk_waitcount;		/* # of sleepers */
	const char *lk_wmesg;		/* sleep wait channel */
	pid_t	lk_lockholder;		/* pid of exclusive holder */
	lwpid_t	lk_locklwp;		/* lid of exclusive holder */
	pri_t	lk_prio;		/* priority at which to sleep */
	int	lk_timo;		/* max sleep time */
	uintptr_t lk_lock_addr;
	uintptr_t lk_unlock_addr;
};

/*
 * Lock request types:
 *   LK_SHARED - get one of many possible shared locks. If a process
 *	holding an exclusive lock requests a shared lock, the exclusive
 *	lock(s) will be downgraded to shared locks.
 *   LK_EXCLUSIVE - stop further shared locks, when they are cleared,
 *	grant then grant an exclusive lock.
 *	Only one exclusive lock may exist at a time, except that
 *	a process holding an exclusive lock may get additional exclusive
 *	locks if it explicitly sets the LK_CANRECURSE flag in the lock
 *	request, or if the LK_CANRECURSE flag was set when the lock was
 *	initialized.
 *   LK_DOWNGRADE - the process must hold an exclusive lock that it wants
 *	to have downgraded to a shared lock. If the process holds multiple
 *	(recursive) exclusive locks, they will all be downgraded to shared
 *	locks.
 *   LK_RELEASE - release one instance of a lock.
 *   LK_DRAIN - wait for all activity on the lock to end, then mark it
 *	decommissioned. This feature is used before freeing a lock that
 *	is part of a piece of memory that is about to be freed.
 *
 * These are flags that are passed to the lockmgr routine.
 */
#define	LK_TYPE_MASK	0x0000000f	/* type of lock sought */
#define	LK_SHARED	0x00000001	/* shared lock */
#define	LK_EXCLUSIVE	0x00000002	/* exclusive lock */
#define	LK_DOWNGRADE	0x00000005	/* exclusive-to-shared downgrade */
#define	LK_RELEASE	0x00000006	/* release any type of lock */
#define	LK_DRAIN	0x00000007	/* wait for all lock activity to end */
#define	LK_EXCLOTHER	0x00000008	/* other process holds lock */
/*
 * External lock flags.
 *
 * The first three flags may be set in lock_init to set their mode permanently,
 * or passed in as arguments to the lock manager. The LK_REENABLE flag may be
 * set only at the release of a lock obtained by drain.
 */
#define	LK_EXTFLG_MASK	0x00f00070	/* mask of external flags */
#define	LK_NOWAIT	0x00000010	/* do not sleep to await lock */
#define	LK_SLEEPFAIL	0x00000020	/* sleep, then return failure */
#define	LK_CANRECURSE	0x00000040	/* this may be recursive lock attempt */
#define	LK_REENABLE	0x00000080	/* lock is be reenabled after drain */
#define	LK_RECURSEFAIL  0x00200000	/* attempt at recursive lock fails */
#define	LK_RESURRECT	0x00800000	/* immediately reenable drained lock */
/*
 * Internal lock flags.
 *
 * These flags are used internally to the lock manager.
 */
#define	LK_WANT_EXCL	0x00000200	/* exclusive lock sought */
#define	LK_HAVE_EXCL	0x00000400	/* exclusive lock obtained */
#define	LK_WAITDRAIN	0x00000800	/* process waiting for lock to drain */
#define	LK_DRAINING	0x00004000	/* lock is being drained */
#define	LK_DRAINED	0x00008000	/* lock has been decommissioned */
#define	LK_DODEBUG	0x00010000	/* has lockdebug bits */
/*
 * Internal state flags corresponding to lk_sharecount, and lk_waitcount
 */
#define	LK_SHARE_NONZERO 0x00040000	/* lk_sharecount != 0 */
#define	LK_WAIT_NONZERO	0x00080000	/* lk_waitcount != 0 */
/*
 * Control flags
 *
 * Non-persistent external flags.
 */
#define	LK_INTERLOCK	0x00010000	/* unlock passed simple lock after
					   getting lk_interlock */
#define	LK_RETRY	0x00020000	/* vn_lock: retry until locked */

#define __LK_FLAG_BITS \
	"\20" \
	"\22LK_RECURSEFAIL" \
	"\20LK_WAIT_NOZERO" \
	"\19LK_SHARE_NOZERO" \
	"\18LK_RETRY" \
	"\17LK_INTERLOCK" \
	"\16LK_DRAINED" \
	"\15LK_DRAINING" \
	"\12LK_WAITDRAIN" \
	"\11LK_HAVE_EXCL" \
	"\10LK_WANT_EXCL" \
	"\08LK_REENABLE" \
	"\07LK_CANRECURSE" \
	"\06LK_SLEEPFAIL" \
	"\05LK_NOWAIT"
/*
 * Lock return status.
 *
 * Successfully obtained locks return 0. Locks will always succeed
 * unless one of the following is true:
 *	LK_NOWAIT is set and a sleep would be required (returns EBUSY).
 *	LK_SLEEPFAIL is set and a sleep was done (returns ENOLCK).
 *	PCATCH is set in lock priority and a signal arrives (returns
 *	    either EINTR or ERESTART if system calls is to be restarted).
 *	Non-null lock timeout and timeout expires (returns EWOULDBLOCK).
 * A failed lock attempt always returns a non-zero error value. No lock
 * is held after an error return.
 */


/*
 * Indicator that no process/cpu holds exclusive lock
 */
#define	LK_KERNPROC	((pid_t) -2)
#define	LK_NOPROC	((pid_t) -1)
#define	LK_NOCPU	((cpuid_t) -1)

#ifdef _KERNEL

struct proc;

void	lockinit(struct lock *, pri_t, const char *, int, int);
void	lockdestroy(struct lock *);
int	lockmgr(struct lock *, u_int flags, kmutex_t *);
void	transferlockers(struct lock *, struct lock *);
int	lockstatus(struct lock *);
void	lockmgr_printinfo(struct lock *);

/*
 * From <machine/lock.h>.
 */
#ifndef SPINLOCK_SPIN_HOOK
#define	SPINLOCK_SPIN_HOOK
#endif
#ifndef SPINLOCK_BACKOFF_HOOK
#define	SPINLOCK_BACKOFF_HOOK		nullop(NULL)
#endif
#ifndef	SPINLOCK_BACKOFF_MIN
#define	SPINLOCK_BACKOFF_MIN	4
#endif
#ifndef	SPINLOCK_BACKOFF_MAX
#define	SPINLOCK_BACKOFF_MAX	128
#endif

#define	SPINLOCK_BACKOFF(count)					\
do {								\
	int __i;						\
	for (__i = (count); __i != 0; __i--) {			\
		SPINLOCK_BACKOFF_HOOK;				\
	}							\
	if ((count) < SPINLOCK_BACKOFF_MAX)			\
		(count) += (count);				\
} while (/* CONSTCOND */ 0);

#define	SPINLOCK_RUN_HOOK(count)	((count) >= SPINLOCK_BACKOFF_MAX)
#define	SPINLOCK_SPINOUT(spins)		((spins)++ > 0x0fffffff)

extern __cpu_simple_lock_t	kernel_lock;

#endif /* _KERNEL */

#endif /* _SYS_LOCK_H_ */
