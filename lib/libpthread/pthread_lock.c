/* Copyright */

#include <assert.h>
#include <errno.h>
#include <signal.h>
#include <stdlib.h>
#include <ucontext.h>
#include <sys/queue.h>

#include "pthread.h"
#include "pthread_int.h"

/* How many times to try before checking whether we've been continued. */
#define NSPINS 20	/* XXX arbitrary */

static int nspins = NSPINS;

void
pthread_lockinit(pt_spin_t *lock)
{

	__cpu_simple_lock_init(lock);
}

void
pthread_spinlock(pthread_t thread, pt_spin_t *lock)
{
	int count, ret;

	count = nspins;
	++thread->pt_spinlocks;

	do {
		while (((ret = __cpu_simple_lock_try(lock)) == 0) && --count)
			;

		if (ret == 1)
			break;
		
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
		if (thread->pt_next != NULL) {
			PTHREADD_ADD(PTHREADD_SPINPREEMPT);
			pthread__switch(thread, thread->pt_next, 0);
		}
		/* try again */
		count = nspins;
		++thread->pt_spinlocks;
	} while (/*CONSTCOND*/1);

	PTHREADD_ADD(PTHREADD_SPINLOCKS);
	/* Got it! We're out of here. */
}


int
pthread_spintrylock(pthread_t thread, pt_spin_t *lock)
{
	int ret;

	++thread->pt_spinlocks;

	ret = __cpu_simple_lock_try(lock);
	
	if (ret == 0) {
		--thread->pt_spinlocks;
		/* See above. */
		if (thread->pt_next != NULL) {
			PTHREADD_ADD(PTHREADD_SPINPREEMPT);
			pthread__switch(thread, thread->pt_next, 0);
		}
	}

	return ret;
}


void
pthread_spinunlock(pthread_t thread, pt_spin_t *lock)
{
	__cpu_simple_unlock(lock);
	--thread->pt_spinlocks;

	PTHREADD_ADD(PTHREADD_SPINUNLOCKS);

	/* If we were preempted while holding a spinlock, the
	 * scheduler will notice this and continue us. To be good
	 * citzens, we must now get out of here if that was our
	 * last spinlock.
	 * XXX when will we ever have more than one?
	 */
	
	if ((thread->pt_spinlocks == 0) && (thread->pt_next != NULL)) {
		PTHREADD_ADD(PTHREADD_SPINPREEMPT);
		pthread__switch(thread, thread->pt_next, 0);
	}
}

