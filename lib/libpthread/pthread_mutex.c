/* Copyright */

#include <assert.h>
#include <errno.h>
#include <signal.h>
#include <stdlib.h>
#include <ucontext.h>
#include <sys/queue.h>

#include "pthread.h"
#include "pthread_int.h"


int
pthread_mutex_init(pthread_mutex_t *mutex, const pthread_mutexattr_t *attr)
{

	assert(mutex != NULL);

	/* XXX No mutex attr support yet. */
	if (attr != NULL)
		return EINVAL;

	/* Allocate. */

	mutex->ptm_magic = PT_MUTEX_MAGIC;
	mutex->ptm_owner = NULL;
	pthread_lockinit(&mutex->ptm_lock);
	pthread_lockinit(&mutex->ptm_interlock);
	PTQ_INIT(&mutex->ptm_blocked);

	return 0;
}


int
pthread_mutex_destroy(pthread_mutex_t *mutex)
{

	assert(mutex != NULL);
	assert(mutex->ptm_lock == __SIMPLELOCK_UNLOCKED);

	mutex->ptm_magic = PT_MUTEX_DEAD;

	return 0;
}


/*
 * Note regarding memory visibility: Pthreads has rules about memory
 * visibility and mutexes. Very roughly: Memory a thread can see when
 * it unlocks a mutex can be seen by another thread that locks the
 * same mutex.
 * 
 * A memory barrier after a lock and before an unlock will provide
 * this behavior. This code relies on __cpu_simple_lock_try() to issue
 * a barrier after obtaining a lock, and on __cpu_simple_unlock() to
 * issue a barrier before releasing a lock.
 */

int
pthread_mutex_lock(pthread_mutex_t *mutex)
{
	pthread_t self;
#ifdef ERRORCHECK
	if ((mutex == NULL) || (mutex->ptm_magic != PT_MUTEX_MAGIC))
		return EINVAL;
#endif

	while (/*CONSTCOND*/1) {
		if (__cpu_simple_lock_try(&mutex->ptm_lock))
		    break; /* got it! */
		
		self = pthread__self();
		/* Okay, didn't look free. Get the interlock... */
		pthread_spinlock(self, &mutex->ptm_interlock);
		/* The mutex_unlock routine will get the interlock
		 * before looking at the list of sleepers, so if the
		 * lock is held we can safely put ourselves on the
		 * sleep queue. If it's not held, we can try taking it
		 * again.
		 */
		if (mutex->ptm_lock == __SIMPLELOCK_LOCKED) {
			PTQ_INSERT_TAIL(&mutex->ptm_blocked, self, pt_sleep);
			self->pt_state = PT_STATE_BLOCKED;
			pthread__block(self, &mutex->ptm_interlock);
			/* interlock is not held when we return */
		} else {
			pthread_spinunlock(self, &mutex->ptm_interlock);
		}
		/* Go around for another try. */
	}

	/* We have the lock! */
#ifdef ERRORCHECK
	mutex->ptm_owner = self;
#endif	
	return 0;
}


int
pthread_mutex_trylock(pthread_mutex_t *mutex)
{

#ifdef ERRORCHECK
	if ((mutex == NULL) || (mutex->ptm_magic != PT_MUTEX_MAGIC))
		return EINVAL;
#endif

	if (__cpu_simple_lock_try(&mutex->ptm_lock) == 0)
		return EBUSY;

#ifdef ERRORCHECK
	mutex->ptm_owner = pthread__self();
#endif
	return 0;
}


int
pthread_mutex_unlock(pthread_mutex_t *mutex)
{
	pthread_t self, blocked; 
	struct pt_queue_t blockedq, nullq = PTQ_HEAD_INITIALIZER;

#ifdef ERRORCHECK
	if ((mutex == NULL) || (mutex->ptm_magic != PT_MUTEX_MAGIC))
		return EINVAL;

	if (mutex->ptm_lock != __SIMPLELOCK_LOCKED)
		return EPERM; /* Not exactly the right error. */

	/* One is only permitted to unlock one's own mutexes. */
	if (mutex->ptm_owner != self)
		return EPERM; 
#endif

	self = pthread__self();
	pthread_spinlock(self, &mutex->ptm_interlock);
       	blockedq = mutex->ptm_blocked;
	mutex->ptm_blocked = nullq;
#ifdef ERRORCHECK
	mutex->ptm_owner = NULL;
#endif
	__cpu_simple_unlock(&mutex->ptm_lock);
	pthread_spinunlock(self, &mutex->ptm_interlock);

	/* Give everyone on the sleep queue another chance at the lock. */
	PTQ_FOREACH(blocked, &blockedq, pt_sleep) 
		pthread__sched(self, blocked);

	return 0;
}
