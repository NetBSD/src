/*	$NetBSD: pthread_types.h,v 1.3 2003/01/25 00:47:05 nathanw Exp $	*/

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

#ifndef _LIB_PTHREAD_TYPES_H
#define _LIB_PTHREAD_TYPES_H

#include <machine/lock.h>

/* We use the "pthread_spin_t" name internally; "pthread_spinlock_t" is the
 * POSIX spinlock object. 
 */
typedef __cpu_simple_lock_t	pthread_spin_t;

/*
 * Copied from PTQ_HEAD in pthread_queue.h
 */
#define _PTQ_HEAD(name, type)	       				\
struct name {								\
	struct type *ptqh_first;/* first element */			\
	struct type **ptqh_last;/* addr of last next element */		\
}

_PTQ_HEAD(pthread_queue_t, pthread_st);

struct	pthread_st;
struct	pthread_attr_st;
struct	pthread_mutex_st;
struct	pthread_mutexattr_st;
struct	pthread_cond_st;
struct	pthread_condattr_st;
struct	pthread_spin_st;
struct	pthread_rwlock_st;
struct	pthread_rwlockattr_st;
struct	pthread_barrier_st;
struct	pthread_barrierattr_st;

typedef struct pthread_st *pthread_t;
typedef struct pthread_attr_st pthread_attr_t;
typedef struct pthread_mutex_st pthread_mutex_t;
typedef struct pthread_mutexattr_st pthread_mutexattr_t;
typedef struct pthread_cond_st pthread_cond_t;
typedef struct pthread_condattr_st pthread_condattr_t;
typedef struct pthread_once_st pthread_once_t;
typedef struct pthread_spinlock_st pthread_spinlock_t;
typedef struct pthread_rwlock_st pthread_rwlock_t;
typedef struct pthread_rwlockattr_st pthread_rwlockattr_t;
typedef struct pthread_barrier_st pthread_barrier_t;
typedef struct pthread_barrierattr_st pthread_barrierattr_t;
typedef int pthread_key_t;

struct	pthread_attr_st {
	unsigned int	pta_magic;

	int	pta_flags;
	void	*pta_private;
};

struct	pthread_mutex_st {
	unsigned int	ptm_magic;

	/* Not a real spinlock; will never be spun on. Locked with
	 * pthread__simple_lock_try() or not at all. Therefore, not
	 * subject to preempted-spinlock-continuation.
	 * 
	 * Open research question: Would it help threaded applications if
	 * preempted-lock-continuation were applied to mutexes?
	 */
	pthread_spin_t	ptm_lock; 

	/* Protects the owner and blocked queue */
	pthread_spin_t	ptm_interlock;
	pthread_t	ptm_owner;
	struct pthread_queue_t	ptm_blocked;
	void	*ptm_private;
};

#define	_PT_MUTEX_MAGIC	0x33330003
#define	_PT_MUTEX_DEAD	0xDEAD0003

#define PTHREAD_MUTEX_INITIALIZER { _PT_MUTEX_MAGIC, 			\
				    __SIMPLELOCK_UNLOCKED,		\
				    __SIMPLELOCK_UNLOCKED,		\
				    NULL,				\
				    {NULL, NULL},			\
				    NULL				\
				  }
	

struct	pthread_mutexattr_st {
	unsigned int	ptma_magic;
	void	*ptma_private;
};

#define _PT_MUTEXATTR_MAGIC	0x44440004
#define _PT_MUTEXATTR_DEAD	0xDEAD0004


struct	pthread_cond_st {
	unsigned int	ptc_magic;

	/* Protects the queue of waiters */
	pthread_spin_t	ptc_lock;
	struct pthread_queue_t	ptc_waiters;

	pthread_mutex_t	*ptc_mutex;	/* Current mutex */
	void	*ptc_private;
};

#define	_PT_COND_MAGIC	0x55550005
#define	_PT_COND_DEAD	0xDEAD0005

#define PTHREAD_COND_INITIALIZER { _PT_COND_MAGIC,			\
				   __SIMPLELOCK_UNLOCKED,		\
				   {NULL, NULL},			\
				   NULL,				\
				   NULL  				\
				 }

struct	pthread_condattr_st {
	unsigned int	ptca_magic;
	void	*ptca_private;
};

#define	_PT_CONDATTR_MAGIC	0x66660006
#define	_PT_CONDATTR_DEAD	0xDEAD0006

struct	pthread_once_st {
	pthread_mutex_t	pto_mutex;
	int	pto_done;
};

#define PTHREAD_ONCE_INIT	{ PTHREAD_MUTEX_INITIALIZER, 0 }

struct	pthread_spinlock_st {
	unsigned int	pts_magic;
	pthread_spin_t	pts_spin;
	int		pts_flags;
};
	
#define	_PT_SPINLOCK_MAGIC	0x77770007
#define	_PT_SPINLOCK_DEAD	0xDEAD0007
#define _PT_SPINLOCK_PSHARED	0x00000001

/* PTHREAD_SPINLOCK_INITIALIZER is an extension not specified by POSIX. */
#define PTHREAD_SPINLOCK_INITIALIZER { _PT_SPINLOCK_MAGIC,		\
				       __SIMPLELOCK_UNLOCKED,		\
				       0				\
				     }

struct	pthread_rwlock_st {
	unsigned int	ptr_magic;

	/* Protects data below */
	pthread_spin_t	ptr_interlock;

	struct pthread_queue_t	ptr_rblocked;
	struct pthread_queue_t	ptr_wblocked;
	unsigned int	ptr_nreaders;
	pthread_t	ptr_writer;
	void	*ptr_private;
};

#define	_PT_RWLOCK_MAGIC	0x99990009
#define	_PT_RWLOCK_DEAD		0xDEAD0009

#define PTHREAD_RWLOCK_INITIALIZER { _PT_RWLOCK_MAGIC,			\
				     __SIMPLELOCK_UNLOCKED,		\
				     {NULL, NULL},			\
				     {NULL, NULL},			\
				     0,					\
				     NULL,				\
				     NULL,				\
				   }

struct	pthread_rwlockattr_st {
	unsigned int	ptra_magic;
	void *ptra_private;
};

#define _PT_RWLOCKATTR_MAGIC	0x99990909
#define _PT_RWLOCKATTR_DEAD	0xDEAD0909

struct	pthread_barrier_st {
	unsigned int	ptb_magic;

	/* Protects data below */
	pthread_spin_t	ptb_lock;

	struct pthread_queue_t	ptb_waiters;
	unsigned int	ptb_initcount;
	unsigned int	ptb_curcount;
	unsigned int	ptb_generation;

	void		*ptb_private;
};

#define	_PT_BARRIER_MAGIC	0x88880008
#define	_PT_BARRIER_DEAD	0xDEAD0008

struct	pthread_barrierattr_st {
	unsigned int	ptba_magic;
	void		*ptba_private;
};

#define	_PT_BARRIERATTR_MAGIC	0x88880808
#define	_PT_BARRIERATTR_DEAD	0xDEAD0808

#endif	/* _LIB_PTHREAD_TYPES_H */
