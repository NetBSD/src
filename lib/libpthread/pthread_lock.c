/*	$NetBSD: pthread_lock.c,v 1.1.2.7 2002/02/08 22:26:55 nathanw Exp $	*/

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

#include <assert.h>

#include "pthread.h"
#include "pthread_int.h"

#undef PTHREAD_SA_DEBUG

#ifdef PTHREAD_SA_DEBUG
#define SDPRINTF(x) DPRINTF(x)
#else
#define SDPRINTF(x)
#endif

/* How many times to try before checking whether we've been continued. */
#define NSPINS 20	/* XXX arbitrary */

static int nspins = NSPINS;

void
pthread_lockinit(pthread_spin_t *lock)
{

	__cpu_simple_lock_init(lock);
}

void
pthread_spinlock(pthread_t thread, pthread_spin_t *lock)
{
	int count, ret;

	count = nspins;
	SDPRINTF(("(pthread_spinlock %p) incrementing spinlock from %d\n",
		thread, thread->pt_spinlocks));
	++thread->pt_spinlocks;

	do {
		while (((ret = __cpu_simple_lock_try(lock)) == 0) && --count)
			;

		if (ret == 1)
			break;

		/* As long as this is uniprocessor, encountering a
		 * locked spinlock is a bug. 
		 */
		assert (ret == 1);

	SDPRINTF(("(pthread_spinlock %p) decrementing spinlock from %d\n",
		thread, thread->pt_spinlocks));
		--thread->pt_spinlocks;
			
		/* We may be preempted while spinning. If so, we will
		 * be restarted here if thread->pt_spinlocks is
		 * nonzero, which can happen if:
		 * a) we just got the lock
		 * b) we haven't yet decremented the lock count.
		 * If we're at this point, (b) applies. Therefore,
		 * check if we're being continued, and if so, bail.
		 * (in case (a), we should let the code finish and 
		 * we will bail out in pthread_spinunlock()).
		 */
		if ((thread->pt_next != NULL) &&
		    (thread->pt_type != PT_THREAD_UPCALL)) {
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

	ret = __cpu_simple_lock_try(lock);
	
	if (ret == 0) {
	SDPRINTF(("(pthread_spintrylock %p) decrementing spinlock from %d\n",
		thread, thread->pt_spinlocks));
		--thread->pt_spinlocks;
		/* See above. */
		if ((thread->pt_next != NULL) &&
		    (thread->pt_type != PT_THREAD_UPCALL)) {
			PTHREADD_ADD(PTHREADD_SPINPREEMPT);
			pthread__switch(thread, thread->pt_next);
		}
	}

	return ret;
}


void
pthread_spinunlock(pthread_t thread, pthread_spin_t *lock)
{
	__cpu_simple_unlock(lock);
	SDPRINTF(("(pthread_spinunlock %p) decrementing spinlock from %d\n",
		thread, thread->pt_spinlocks));
	--thread->pt_spinlocks;

	PTHREADD_ADD(PTHREADD_SPINUNLOCKS);

	/* If we were preempted while holding a spinlock, the
	 * scheduler will notice this and continue us. To be good
	 * citzens, we must now get out of here if that was our
	 * last spinlock.
	 * XXX when will we ever have more than one?
	 */
	
	if ((thread->pt_spinlocks == 0) && (thread->pt_next != NULL) 
		&& (thread->pt_type != PT_THREAD_UPCALL)) {
		PTHREADD_ADD(PTHREADD_SPINPREEMPT);
		pthread__switch(thread, thread->pt_next);
	}
}

