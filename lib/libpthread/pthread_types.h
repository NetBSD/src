/*	$Id: pthread_types.h,v 1.1.2.1 2001/07/13 02:06:29 nathanw Exp $	*/

/*
 * Copyright
 */

#ifndef _LIB_PTHREAD_TYPES_H
#define _LIB_PTHREAD_TYPES_H

#include <machine/lock.h>
#include "pthread_queue.h"

typedef __cpu_simple_lock_t	pt_spin_t;

PTQ_HEAD(pt_queue_t, pthread_st);

struct	pthread_st;
struct	pthread_attr_st;
struct	pthread_mutex_st;
struct	pthread_mutexattr_st;
struct	pthread_cond_st;
struct	pthread_condattr_st;

typedef struct pthread_st *pthread_t;
typedef struct pthread_attr_st pthread_attr_t;
typedef struct pthread_mutex_st pthread_mutex_t;
typedef struct pthread_mutexattr_st pthread_mutexattr_t;
typedef struct pthread_cond_st pthread_cond_t;
typedef struct pthread_condattr_st pthread_condattr_t;


struct	pthread_attr_st {
	unsigned int	pta_magic;

	int	pta_flags;
};

struct	pthread_mutex_st {
	unsigned int	ptm_magic;

	/* Not a real spinlock; will never be spun on. Locked with
	 * __cpu_simple_lock_try() or not at all. Therefore, not
	 * subject to preempted-spinlock-continuation.
	 * 
	 * Open research question: Would it help threaded applications if
	 * preempted-lock-continuation were applied to mutexes?
	 */
	pt_spin_t	ptm_lock; 

	/* Protects the owner and blocked queue */
	pt_spin_t	ptm_interlock;
	pthread_t	ptm_owner;
	struct pt_queue_t	ptm_blocked;
};

#define	PT_MUTEX_MAGIC	0xCAFEFEED
#define	PT_MUTEX_DEAD	0xFEEDDEAD

#define PTHREAD_MUTEX_INITIALIZER { PT_MUTEX_MAGIC, 			\
				    __SIMPLELOCK_UNLOCKED,		\
				    __SIMPLELOCK_UNLOCKED,		\
				    NULL,				\
				    PTQ_HEAD_INITIALIZER		\
				  }
	

struct	pthread_mutexattr_st {
	int	pad;
};

#endif	/* _LIB_PTHREAD_TYPES_H */
