/* Copyright */

#include <assert.h>
#include <errno.h>
#include <signal.h>
#include <stdlib.h>
#include <ucontext.h>
#include <sys/queue.h>

#include "pthread.h"
#include "pthread_int.h"
#include "pthread_mutex.h"




int
pthread_mutex_init(pthread_mutex_t *mutex, const pthread_mutexattr_t *attr)
{
	pthread_mutex_t newmutex;

	assert(mutex != NULL);

	/* XXX No mutex attr support yet. */
	if (attr != NULL)
		return EINVAL;

	/* Allocate. */
	newmutex = pthread__malloc(sizeof(struct pthread_mutex_st));
	if (newmutex == NULL)
		return ENOMEM;

	newmutex->ptm_magic = PT_MUTEX_MAGIC;
	newmutex->ptm_owner = NULL;
	SIMPLEQ_INIT(&newmutex->ptm_blocked);

	*mutex = newmutex;

	return 0;
}


int
pthread_mutex_destroy(pthread_mutex_t *mutex)
{

	if ((mutex == NULL) || (*mutex == NULL) ||
	    (*mutex)->ptm_magic != PT_MUTEX_MAGIC)
		return EINVAL;

	(*mutex)->ptm_magic = PT_MUTEX_DEAD;
	pthread__free(*mutex);

	return 0;
}


int
pthread_mutex_lock(pthread_mutex_t *mutex)
{
	pthread_t self;

	if ((mutex == NULL) || (*mutex == NULL) ||
	    (*mutex)->ptm_magic != PT_MUTEX_MAGIC)
		return EINVAL;

	self = pthread__self();

	pthread_spinlock(self, &(*mutex)->ptm_interlock);

	while ((*mutex)->ptm_locked == PT_MUTEX_LOCKED) {
		/* Put ourselves on the queue and go to sleep. */
		SIMPLEQ_INSERT_TAIL(&(*mutex)->ptm_blocked, self, pt_sleep);

		self->pt_state = PT_STATE_BLOCKED;
		pthread__block(self, &(*mutex)->ptm_interlock);
		
		/* We're back. Try again. */
		pthread_spinlock(self, &(*mutex)->ptm_interlock);
	}

	assert((*mutex)->ptm_locked == PT_MUTEX_UNLOCKED);
	
	/* We have the interlock; go for the real thing. */
	(*mutex)->ptm_locked = PT_MUTEX_LOCKED;
	(*mutex)->ptm_owner = self;
	
	pthread_spinunlock(self, &(*mutex)->ptm_interlock);

	return 0;
}


int
pthread_mutex_trylock(pthread_mutex_t *mutex)
{
	pthread_t self;

	if ((mutex == NULL) || (*mutex == NULL) ||
	    (*mutex)->ptm_magic != PT_MUTEX_MAGIC)
		return EINVAL;

	if ((*mutex)->ptm_locked == PT_MUTEX_LOCKED)
		return EBUSY;

	self = pthread__self();

	pthread_spinlock(self, &(*mutex)->ptm_interlock);
	if ((*mutex)->ptm_locked == PT_MUTEX_LOCKED) {
		pthread_spinunlock(self, &(*mutex)->ptm_interlock);
		return EBUSY;
	}

	assert((*mutex)->ptm_locked == PT_MUTEX_UNLOCKED);
	
	/* We have the interlock; go for the real thing. */
	(*mutex)->ptm_locked = PT_MUTEX_LOCKED;
	(*mutex)->ptm_owner = self;
	
	pthread_spinunlock(self, &(*mutex)->ptm_interlock);

	return 0;
}


int
pthread_mutex_unlock(pthread_mutex_t *mutex)
{
	pthread_t self, blocked; 

	if ((mutex == NULL) || (*mutex == NULL) ||
	    (*mutex)->ptm_magic != PT_MUTEX_MAGIC)
		return EINVAL;

	if ((*mutex)->ptm_locked == PT_MUTEX_UNLOCKED)
		return EPERM; /* Not exactly the right error. */

	assert((*mutex)->ptm_locked == PT_MUTEX_LOCKED);

	/* One is only permitted to unlock one's own mutexes. */
	self = pthread__self();
	if ((*mutex)->ptm_owner != self)
		return EPERM; 
	
	pthread_spinlock(self, &(*mutex)->ptm_interlock);
	(*mutex)->ptm_locked = PT_MUTEX_UNLOCKED;
	(*mutex)->ptm_owner = NULL;
	
	blocked = SIMPLEQ_FIRST(&(*mutex)->ptm_blocked);
	if (blocked)
		SIMPLEQ_REMOVE_HEAD(&(*mutex)->ptm_blocked, blocked, pt_sleep);

	pthread_spinunlock(self, &(*mutex)->ptm_interlock);

	if (blocked)
		pthread__sched(self, blocked);

	return 0;
}
