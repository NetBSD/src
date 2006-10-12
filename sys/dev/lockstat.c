/*	$NetBSD: lockstat.c,v 1.3 2006/10/12 06:56:47 xtraeme Exp $	*/

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
 * Lock statistics driver, providing kernel support for the lockstat(8)
 * command.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: lockstat.c,v 1.3 2006/10/12 06:56:47 xtraeme Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/lock.h>
#include <sys/proc.h> 
#include <sys/resourcevar.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/conf.h>
#include <sys/syslog.h>

#include <dev/lockstat.h>

#ifndef __HAVE_CPU_COUNTER
#error CPU counters not available
#endif

#if LONG_BIT == 64
#define	LOCKSTAT_HASH_SHIFT	3
#elif LONG_BIT == 32
#define	LOCKSTAT_HASH_SHIFT	2
#endif

#define	LOCKSTAT_MINBUFS	100
#define	LOCKSTAT_DEFBUFS	1000
#define	LOCKSTAT_MAXBUFS	10000

#define	LOCKSTAT_HASH_SIZE	64
#define	LOCKSTAT_HASH_MASK	(LOCKSTAT_HASH_SIZE - 1)
#define	LOCKSTAT_HASH(key)	\
	((key >> LOCKSTAT_HASH_SHIFT) & LOCKSTAT_HASH_MASK)

typedef struct lscpu {
	SLIST_HEAD(, lsbuf)	lc_free;
	u_int			lc_overflow;
	LIST_HEAD(lslist, lsbuf) lc_hash[LOCKSTAT_HASH_SIZE];
} lscpu_t;

typedef struct lslist lslist_t;

void	lockstatattach(int);
void	lockstat_start(lsenable_t *);
int	lockstat_alloc(lsenable_t *);
void	lockstat_init_tables(lsenable_t *);
int	lockstat_stop(lsdisable_t *);
void	lockstat_free(void);

dev_type_open(lockstat_open);
dev_type_close(lockstat_close);
dev_type_read(lockstat_read);
dev_type_ioctl(lockstat_ioctl);

/* Protected against write by lockstat_lock().  Used by lockstat_event(). */
volatile u_int	lockstat_enabled;
uintptr_t	lockstat_csstart;
uintptr_t	lockstat_csend;
uintptr_t	lockstat_csmask;
uintptr_t	lockstat_lockaddr;

/* Protected by lockstat_lock(). */
struct simplelock lockstat_slock;
lsbuf_t		*lockstat_baseb;
size_t		lockstat_sizeb;
int		lockstat_busy;
int		lockstat_devopen;
struct timespec	lockstat_stime;

const struct cdevsw lockstat_cdevsw = {
	lockstat_open, lockstat_close, lockstat_read, nowrite, lockstat_ioctl,
	nostop, notty, nopoll, nommap, nokqfilter, 0
};

MALLOC_DEFINE(M_LOCKSTAT, "lockstat", "lockstat event buffers");

/*
 * Called when the pseudo-driver is attached.
 */
void
lockstatattach(int nunits)
{

	(void)nunits;

	__cpu_simple_lock_init(&lockstat_slock.lock_data);
}

/*
 * Grab the global lock.  If busy is set, we want to block out operations on
 * the control device.
 */
static inline int
lockstat_lock(int busy)
{

	if (!__cpu_simple_lock_try(&lockstat_slock.lock_data))
		return (EBUSY);
	if (busy) {
		if (lockstat_busy) {
			__cpu_simple_unlock(&lockstat_slock.lock_data);
			return (EBUSY);
		}
		lockstat_busy = 1;
	}
	KASSERT(lockstat_busy);

	return 0;
}

/*
 * Release the global lock.  If unbusy is set, we want to allow new
 * operations on the control device.
 */
static inline void
lockstat_unlock(int unbusy)
{

	KASSERT(lockstat_busy);
	if (unbusy)
		lockstat_busy = 0;
	__cpu_simple_unlock(&lockstat_slock.lock_data);
}

/*
 * Prepare the per-CPU tables for use, or clear down tables when tracing is
 * stopped.
 */
void
lockstat_init_tables(lsenable_t *le)
{
	int i, ncpu, per, slop, cpuno;
	CPU_INFO_ITERATOR cii;
	struct cpu_info *ci;
	lscpu_t *lc;
	lsbuf_t *lb;

	KASSERT(!lockstat_enabled);

	ncpu = 0;
	for (CPU_INFO_FOREACH(cii, ci)) {
		if (ci->ci_lockstat != NULL) {
			free(ci->ci_lockstat, M_LOCKSTAT);
			ci->ci_lockstat = NULL;
		}
		ncpu++;
	}

	if (le == NULL)
		return;

	lb = lockstat_baseb;
	per = le->le_nbufs / ncpu;
	slop = le->le_nbufs - (per * ncpu);
	cpuno = 0;
	for (CPU_INFO_FOREACH(cii, ci)) {
		lc = malloc(sizeof(*lc), M_LOCKSTAT, M_WAITOK);
		lc->lc_overflow = 0;
		ci->ci_lockstat = lc;

		SLIST_INIT(&lc->lc_free);
		for (i = 0; i < LOCKSTAT_HASH_SIZE; i++)
			LIST_INIT(&lc->lc_hash[i]);

		for (i = per; i != 0; i--, lb++) {
			lb->lb_cpu = (uint16_t)cpuno;
			SLIST_INSERT_HEAD(&lc->lc_free, lb, lb_chain.slist);
		}
		if (--slop > 0) {
			lb->lb_cpu = (uint16_t)cpuno;
			SLIST_INSERT_HEAD(&lc->lc_free, lb, lb_chain.slist);
			lb++;
		}
		cpuno++;
	}
}

/*
 * Start collecting lock statistics.
 */
void
lockstat_start(lsenable_t *le)
{

	KASSERT(!lockstat_enabled);

	lockstat_init_tables(le);

	if ((le->le_flags & LE_CALLSITE) != 0)
		lockstat_csmask = (uintptr_t)-1LL;
	else
		lockstat_csmask = 0;

	lockstat_csstart = le->le_csstart;
	lockstat_csend = le->le_csend;
	lockstat_lockaddr = le->le_lock;

	/*
	 * Force a write barrier.  XXX This may not be sufficient..
	 */
	lockstat_unlock(0);
	tsleep(&lockstat_start, PPAUSE, "lockstat", mstohz(10));
	(void)lockstat_lock(0);

	getnanotime(&lockstat_stime);
	lockstat_enabled = le->le_mask;
	lockstat_unlock(0);
	(void)lockstat_lock(0);
}

/*
 * Stop collecting lock statistics.
 */
int
lockstat_stop(lsdisable_t *ld)
{
	CPU_INFO_ITERATOR cii;
	struct cpu_info *ci;
	u_int cpuno, overflow;
	struct timespec ts;
	int error;

	KASSERT(lockstat_enabled);

	/*
	 * Set enabled false, force a write barrier, and wait for other CPUs
	 * to exit lockstat_event().  XXX This may not be sufficient..
	 */
	lockstat_enabled = 0;
	lockstat_unlock(0);
	getnanotime(&ts);
	tsleep(&lockstat_stop, PPAUSE, "lockstat", mstohz(10));
 	(void)lockstat_lock(0);

	/*
	 * Did we run out of buffers while tracing?
	 */
	overflow = 0;
	for (CPU_INFO_FOREACH(cii, ci))
		overflow += ((lscpu_t *)ci->ci_lockstat)->lc_overflow;

	if (overflow != 0) {
		error = EOVERFLOW;
		log(LOG_NOTICE, "lockstat: %d buffer allocations failed\n",
		    overflow);
	} else
		error = 0;

	lockstat_init_tables(NULL);

	if (ld == NULL)
		return (error);

	/*
	 * Fill out the disable struct for the caller.
	 */
	timespecsub(&ts, &lockstat_stime, &ld->ld_time);
	ld->ld_size = lockstat_sizeb;

	cpuno = 0;
	for (CPU_INFO_FOREACH(cii, ci)) {
		if (cpuno > sizeof(ld->ld_freq) / sizeof(ld->ld_freq[0])) {
			log(LOG_WARNING, "lockstat: too many CPUs\n");
			break;
		}
		ld->ld_freq[cpuno++] = cpu_frequency(ci);
	}

	return (error);
}

/*
 * Allocate buffers for lockstat_start().
 */
int
lockstat_alloc(lsenable_t *le)
{
	lsbuf_t *lb;
	size_t sz;

	KASSERT(!lockstat_enabled);
	lockstat_free();

	sz = sizeof(*lb) * le->le_nbufs;

	lockstat_unlock(0);
	lb = malloc(sz, M_LOCKSTAT, M_WAITOK | M_ZERO);
	(void)lockstat_lock(0);

	if (lb == NULL)
		return (ENOMEM);

	KASSERT(!lockstat_enabled);
	KASSERT(lockstat_baseb == NULL);
	lockstat_sizeb = sz;
	lockstat_baseb = lb;
		
	return (0);
}

/*
 * Free allocated buffers after tracing has stopped.
 */
void
lockstat_free(void)
{

	KASSERT(!lockstat_enabled);

	if (lockstat_baseb != NULL) {
		free(lockstat_baseb, M_LOCKSTAT);
		lockstat_baseb = NULL;
	}
}

/*
 * Main entry point from lock primatives.
 */
void
lockstat_event(uintptr_t lock, uintptr_t callsite, u_int flags, u_int count,
	       uint64_t time)
{
	lslist_t *ll;
	lscpu_t *lc;
	lsbuf_t *lb;
	u_int event;
	int s;

	if ((flags & lockstat_enabled) != flags || count == 0)
		return;
	if (lockstat_lockaddr != 0 && lock != lockstat_lockaddr)
		return;
	if (callsite < lockstat_csstart || callsite > lockstat_csend)
		return;

	callsite &= lockstat_csmask;

	/*
	 * Find the table for this lock+callsite pair, and try to locate a
	 * buffer with the same key.
	 */
	lc = curcpu()->ci_lockstat;
	ll = &lc->lc_hash[LOCKSTAT_HASH(lock ^ callsite)];
	event = (flags & LB_EVENT_MASK) - 1;
	s = spllock();

	LIST_FOREACH(lb, ll, lb_chain.list) {
		if (lb->lb_lock == lock && lb->lb_callsite == callsite)
			break;
	}

	if (lb != NULL) {
		/*
		 * We found a record.  Move it to the front of the list, as
		 * we're likely to hit it again soon.
		 */
		if (lb != LIST_FIRST(ll)) {
			LIST_REMOVE(lb, lb_chain.list);
			LIST_INSERT_HEAD(ll, lb, lb_chain.list);
		}
		lb->lb_counts[event] += count;
		lb->lb_times[event] += time;
	} else if ((lb = SLIST_FIRST(&lc->lc_free)) != NULL) {
		/*
		 * Pinch a new buffer and fill it out.
		 */
		SLIST_REMOVE_HEAD(&lc->lc_free, lb_chain.slist);
		LIST_INSERT_HEAD(ll, lb, lb_chain.list);
		lb->lb_flags = (uint16_t)flags;
		lb->lb_lock = lock;
		lb->lb_callsite = callsite;
		lb->lb_counts[event] = count;
		lb->lb_times[event] = time;
	} else {
		/*
		 * We didn't find a buffer and there were none free.
		 * lockstat_stop() will notice later on and report the
		 * error.
		 */
		 lc->lc_overflow++;
	}

	splx(s);
}

/*
 * Accept an open() on /dev/lockstat.
 */
int
lockstat_open(dev_t dev __unused, int flag __unused, int mode __unused,
	struct lwp *l __unused)
{
	int error;

	if ((error = lockstat_lock(1)) != 0)
		return error;

	if (lockstat_devopen)
		error = EBUSY;
	else {
		lockstat_devopen = 1;
		error = 0;
	}

	lockstat_unlock(1);

	return error;
}

/*
 * Accept the last close() on /dev/lockstat.
 */
int
lockstat_close(dev_t dev __unused, int flag __unused, int mode __unused,
	struct lwp *l __unused)
{
	int error;

	if ((error = lockstat_lock(1)) == 0) {
		if (lockstat_enabled)
			(void)lockstat_stop(NULL);
		lockstat_free();
		lockstat_devopen = 0;
		lockstat_unlock(1);
	}

	return error;
}

/*
 * Handle control operations.
 */
int
lockstat_ioctl(dev_t dev __unused, u_long cmd, caddr_t data,
	int flag __unused, struct lwp *l __unused)
{
	lsenable_t *le;
	int error;

	if ((error = lockstat_lock(1)) != 0)
		return error;

	switch (cmd) {
	case IOC_LOCKSTAT_GVERSION:
		*(int *)data = LS_VERSION;
		error = 0;
		break;

	case IOC_LOCKSTAT_ENABLE:
		le = (lsenable_t *)data;

		if (!cpu_hascounter()) {
			error = ENODEV;
			break;
		}
		if (lockstat_enabled) {
			error = EBUSY;
			break;
		}

		/*
		 * Sanitize the arguments passed in and set up filtering.
		 */
		if (le->le_nbufs == 0)
			le->le_nbufs = LOCKSTAT_DEFBUFS;
		else if (le->le_nbufs > LOCKSTAT_MAXBUFS ||
		    le->le_nbufs < LOCKSTAT_MINBUFS) {
			error = EINVAL;
			break;
		}
		if ((le->le_flags & LE_ONE_CALLSITE) == 0) {
			le->le_csstart = 0;
			le->le_csend = le->le_csstart - 1;
		}
		if ((le->le_flags & LE_ONE_LOCK) == 0)
			le->le_lock = 0;
		if ((le->le_mask & LB_EVENT_MASK) == 0)
			return (EINVAL);
		if ((le->le_mask & LB_LOCK_MASK) == 0)
			return (EINVAL);

		/*
		 * Start tracing.
		 */
		if ((error = lockstat_alloc(le)) == 0)
			lockstat_start(le);
		break;

	case IOC_LOCKSTAT_DISABLE:
		if (!lockstat_enabled)
			error = EINVAL;
		else
			error = lockstat_stop((lsdisable_t *)data);
		break;

	default:
		error = ENOTTY;
		break;
	}

	lockstat_unlock(1);
	return error;
}

/*
 * Copy buffers out to user-space.
 */
int
lockstat_read(dev_t dev __unused, struct uio *uio, int flag __unused)
{
	int error;

	if ((error = lockstat_lock(1)) != 0)
		return (error);

	if (lockstat_enabled) {
		lockstat_unlock(1);
		return (EBUSY);
	}

	lockstat_unlock(0);
	error = uiomove(lockstat_baseb, lockstat_sizeb, uio);
	lockstat_lock(0);

	lockstat_unlock(1);

	return (error);
}
