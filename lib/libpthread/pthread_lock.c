/*	$NetBSD: pthread_lock.c,v 1.4 2003/01/22 13:52:03 scw Exp $	*/

/*-
 * Copyright (c) 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Nathan J. Williams.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
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

#include <sys/param.h>
#include <sys/ras.h>
#include <sys/sysctl.h>

#include <assert.h>
#include <errno.h>
#include <unistd.h>

#include "pthread.h"
#include "pthread_int.h"

#undef PTHREAD_SPIN_DEBUG

#ifdef PTHREAD_SPIN_DEBUG
#define SDPRINTF(x) DPRINTF(x)
#else
#define SDPRINTF(x)
#endif

/* How many times to try before checking whether we've been continued. */
#define NSPINS 1	/* no point in actually spinning until MP works */

static int nspins = NSPINS;

extern void pthread__lock_ras_start(void), pthread__lock_ras_end(void);

static void
pthread__ras_simple_lock_init(__cpu_simple_lock_t *alp)
{

	*alp = __SIMPLELOCK_UNLOCKED;
}

static int
pthread__ras_simple_lock_try(__cpu_simple_lock_t *alp)
{
	__cpu_simple_lock_t old;

	/* This is the atomic sequence. */
	__asm __volatile("pthread__lock_ras_start:");
	old = *alp;
	*alp = __SIMPLELOCK_LOCKED;
	__asm __volatile("pthread__lock_ras_end:");

	return (old == __SIMPLELOCK_UNLOCKED);
}

static void
pthread__ras_simple_unlock(__cpu_simple_lock_t *alp)
{

	*alp = __SIMPLELOCK_UNLOCKED;
}

static const struct pthread_lock_ops pthread__lock_ops_ras = {
	pthread__ras_simple_lock_init,
	pthread__ras_simple_lock_try,
	pthread__ras_simple_unlock,
};

static void
pthread__atomic_simple_lock_init(__cpu_simple_lock_t *alp)
{

	__cpu_simple_lock_init(alp);
}

static int
pthread__atomic_simple_lock_try(__cpu_simple_lock_t *alp)
{

	return (__cpu_simple_lock_try(alp));
}

static void
pthread__atomic_simple_unlock(__cpu_simple_lock_t *alp)
{

	__cpu_simple_unlock(alp);
}

static const struct pthread_lock_ops pthread__lock_ops_atomic = {
	pthread__atomic_simple_lock_init,
	pthread__atomic_simple_lock_try,
	pthread__atomic_simple_unlock,
};

/*
 * We default to pointing to the RAS primitives; we might need to use
 * locks early, but before main() starts.  This is safe, since no other
 * threads will be active for the process, so atomicity will not be
 * required.
 */
const struct pthread_lock_ops *pthread__lock_ops = &pthread__lock_ops_ras;

/*
 * Initialize the locking primitives.  On uniprocessors, we always
 * use Restartable Atomic Sequences if they are available.  Otherwise,
 * we fall back onto machine-dependent atomic lock primitives.
 */
void
pthread__lockprim_init(void)
{
	int mib[2];
	size_t len; 
	int ncpu;
 
	mib[0] = CTL_HW;
	mib[1] = HW_NCPU; 
 
	len = sizeof(ncpu);
	sysctl(mib, 2, &ncpu, &len, NULL, 0);

	if (ncpu == 1 && rasctl((void *)pthread__lock_ras_start,
	    (size_t)((uintptr_t)pthread__lock_ras_end -
	             (uintptr_t)pthread__lock_ras_start),
	    RAS_INSTALL) == 0) {
		pthread__lock_ops = &pthread__lock_ops_ras;
		return;
	}

	pthread__lock_ops = &pthread__lock_ops_atomic;
}

void
pthread_lockinit(pthread_spin_t *lock)
{

	pthread__simple_lock_init(lock);
}

void
pthread_spinlock(pthread_t thread, pthread_spin_t *lock)
{
	int count, ret;

	count = nspins;
	SDPRINTF(("(pthread_spinlock %p) incrementing spinlock %p (count %d)\n",
		thread, lock, thread->pt_spinlocks));
#ifdef PTHREAD_SPIN_DEBUG
	if(!(thread->pt_spinlocks >= 0)) {
		(void)kill(getpid(), SIGABRT);
		_exit(1);
	}
#endif
	++thread->pt_spinlocks;

	do {
		while (((ret = pthread__simple_lock_try(lock)) == 0) && --count)
			;

		if (ret == 1)
			break;

	SDPRINTF(("(pthread_spinlock %p) decrementing spinlock %p (count %d)\n",
		thread, lock, thread->pt_spinlocks));
		--thread->pt_spinlocks;
			
		/*
		 * We may be preempted while spinning. If so, we will
		 * be restarted here if thread->pt_spinlocks is
		 * nonzero, which can happen if:
		 * a) we just got the lock
		 * b) we haven't yet decremented the lock count.
		 * If we're at this point, (b) applies. Therefore,
		 * check if we're being continued, and if so, bail.
		 * (in case (a), we should let the code finish and 
		 * we will bail out in pthread_spinunlock()).
		 */
		if (thread->pt_next != NULL) {
			PTHREADD_ADD(PTHREADD_SPINPREEMPT);
			pthread__switch(thread, thread->pt_next);
		}
		/* try again */
		count = nspins;
	SDPRINTF(("(pthread_spinlock %p) incrementing spinlock from %d\n",
		thread, thread->pt_spinlocks));
		++thread->pt_spinlocks;
	} while (/*CONSTCOND*/1);

	PTHREADD_ADD(PTHREADD_SPINLOCKS);
	/* Got it! We're out of here. */
}


int
pthread_spintrylock(pthread_t thread, pthread_spin_t *lock)
{
	int ret;

	SDPRINTF(("(pthread_spinlock %p) incrementing spinlock from %d\n",
		thread, thread->pt_spinlocks));
	++thread->pt_spinlocks;

	ret = pthread__simple_lock_try(lock);
	
	if (ret == 0) {
	SDPRINTF(("(pthread_spintrylock %p) decrementing spinlock from %d\n",
		thread, thread->pt_spinlocks));
		--thread->pt_spinlocks;
		/* See above. */
		if (thread->pt_next != NULL) {
			PTHREADD_ADD(PTHREADD_SPINPREEMPT);
			pthread__switch(thread, thread->pt_next);
		}
	}

	return ret;
}


void
pthread_spinunlock(pthread_t thread, pthread_spin_t *lock)
{

	pthread__simple_unlock(lock);
	SDPRINTF(("(pthread_spinunlock %p) decrementing spinlock %p (count %d)\n",
		thread, lock, thread->pt_spinlocks));
	--thread->pt_spinlocks;
#ifdef PTHREAD_SPIN_DEBUG
	if (!(thread->pt_spinlocks >= 0)) {
		(void)kill(getpid(), SIGABRT);
		_exit(1);
	}
#endif
	PTHREADD_ADD(PTHREADD_SPINUNLOCKS);

	/*
	 * If we were preempted while holding a spinlock, the
	 * scheduler will notice this and continue us. To be good
	 * citzens, we must now get out of here if that was our
	 * last spinlock.
	 * XXX when will we ever have more than one?
	 */
	
	if ((thread->pt_spinlocks == 0) && (thread->pt_next != NULL)) {
		PTHREADD_ADD(PTHREADD_SPINPREEMPT);
		pthread__switch(thread, thread->pt_next);
	}
}


/* 
 * Public (POSIX-specified) spinlocks.
 * These don't interact with the spin-preemption code, nor do they
 * perform any adaptive sleeping.
 */

int
pthread_spin_init(pthread_spinlock_t *lock, int pshared)
{

#ifdef ERRORCHECK
	if ((lock == NULL) ||
	    ((pshared != PTHREAD_PROCESS_PRIVATE) &&
		(pshared != PTHREAD_PROCESS_SHARED)))
		return EINVAL;
#endif
	lock->pts_magic = _PT_SPINLOCK_MAGIC;
	/*
	 * We don't actually use the pshared flag for anything;
	 * cpu simple locks have all the process-shared properties 
	 * that we want anyway.
	 */
	lock->pts_flags = pshared;
	pthread_lockinit(&lock->pts_spin);

	return 0;
}

int
pthread_spin_destroy(pthread_spinlock_t *lock)
{

#ifdef ERRORCHECK
	if ((lock == NULL) || (lock->pts_magic != _PT_SPINLOCK_MAGIC))
		return EINVAL;
	
	if (lock->pts_spin != __SIMPLELOCK_UNLOCKED)
		return EBUSY;
#endif

	lock->pts_magic = _PT_SPINLOCK_DEAD;

	return 0;
}

int
pthread_spin_lock(pthread_spinlock_t *lock)
{

#ifdef ERRORCHECK
	if ((lock == NULL) || (lock->pts_magic != _PT_SPINLOCK_MAGIC))
		return EINVAL;
#endif

	while (pthread__simple_lock_try(&lock->pts_spin) == 0)
		/* spin */ ;

	return 0;
}

int
pthread_spin_trylock(pthread_spinlock_t *lock)
{

#ifdef ERRORCHECK
	if ((lock == NULL) || (lock->pts_magic != _PT_SPINLOCK_MAGIC))
		return EINVAL;
#endif

	if (pthread__simple_lock_try(&lock->pts_spin) == 0)
		return EBUSY;

	return 0;
}

int
pthread_spin_unlock(pthread_spinlock_t *lock)
{

#ifdef ERRORCHECK
	if ((lock == NULL) || (lock->pts_magic != _PT_SPINLOCK_MAGIC))
		return EINVAL;
#endif

	pthread__simple_unlock(&lock->pts_spin);

	return 0;
}
