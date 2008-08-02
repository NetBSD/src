/*	$NetBSD: pthread_types.h,v 1.13.6.2 2008/08/02 19:46:31 matt Exp $	*/

/*-
 * Copyright (c) 2001, 2008 The NetBSD Foundation, Inc.
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

/*
 * We use the "pthread_spin_t" name internally; "pthread_spinlock_t" is the
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

_PTQ_HEAD(pthread_queue_struct_t, __pthread_st);
typedef struct pthread_queue_struct_t pthread_queue_t;

struct	__pthread_st;
struct	__pthread_attr_st;
struct	__pthread_mutex_st;
struct	__pthread_mutexattr_st;
struct	__pthread_cond_st;
struct	__pthread_condattr_st;
struct	__pthread_spin_st;
struct	__pthread_rwlock_st;
struct	__pthread_rwlockattr_st;
struct	__pthread_barrier_st;
struct	__pthread_barrierattr_st;

typedef struct __pthread_st *pthread_t;
typedef struct __pthread_attr_st pthread_attr_t;
typedef struct __pthread_mutex_st pthread_mutex_t;
typedef struct __pthread_mutexattr_st pthread_mutexattr_t;
typedef struct __pthread_cond_st pthread_cond_t;
typedef struct __pthread_condattr_st pthread_condattr_t;
typedef struct __pthread_once_st pthread_once_t;
typedef struct __pthread_spinlock_st pthread_spinlock_t;
typedef struct __pthread_rwlock_st pthread_rwlock_t;
typedef struct __pthread_rwlockattr_st pthread_rwlockattr_t;
typedef struct __pthread_barrier_st pthread_barrier_t;
typedef struct __pthread_barrierattr_st pthread_barrierattr_t;
typedef int pthread_key_t;

struct	__pthread_attr_st {
	unsigned int	pta_magic;

	int	pta_flags;
	void	*pta_private;
};

/*
 * ptm_owner is the actual lock field which is locked via CAS operation.
 * This structure's layout is designed to compatible with the previous
 * version used in SA pthreads.
 */
#ifdef __CPU_SIMPLE_LOCK_PAD
/*
 * If __SIMPLE_UNLOCKED != 0 and we have to pad, we have to worry about
 * endianness.  Currently that isn't an issue but put in a check in case
 * something changes in the future.
 */
#if __SIMPLELOCK_UNLOCKED != 0
#error __CPU_SIMPLE_LOCK_PAD incompatible with __SIMPLELOCK_UNLOCKED == 0
#endif
#endif
struct	__pthread_mutex_st {
	unsigned int	ptm_magic;
	pthread_spin_t	ptm_errorcheck;
#ifdef __CPU_SIMPLE_LOCK_PAD
	uint8_t		ptm_pad1[3];
#endif
	pthread_spin_t	ptm_interlock;	/* unused - backwards compat */
#ifdef __CPU_SIMPLE_LOCK_PAD
	uint8_t		ptm_pad2[3];
#endif
	volatile pthread_t ptm_owner;
	pthread_t * volatile ptm_waiters;
	unsigned int	ptm_recursed;
	void		*ptm_spare2;	/* unused - backwards compat */
};

#define	_PT_MUTEX_MAGIC	0x33330003
#define	_PT_MUTEX_DEAD	0xDEAD0003

#ifdef __CPU_SIMPLE_LOCK_PAD
#define _PTHREAD_MUTEX_INITIALIZER { _PT_MUTEX_MAGIC, 			\
				    __SIMPLELOCK_UNLOCKED, { 0, 0, 0 },	\
				    __SIMPLELOCK_UNLOCKED, { 0, 0, 0 },	\
				    NULL, NULL, 0, NULL			\
				  }
#else
#define _PTHREAD_MUTEX_INITIALIZER { _PT_MUTEX_MAGIC, 			\
				    __SIMPLELOCK_UNLOCKED,		\
				    __SIMPLELOCK_UNLOCKED,		\
				    NULL, NULL, 0, NULL			\
				  }
#endif /* __CPU_SIMPLE_LOCK_PAD */
	

struct	__pthread_mutexattr_st {
	unsigned int	ptma_magic;
	void	*ptma_private;
};

#define _PT_MUTEXATTR_MAGIC	0x44440004
#define _PT_MUTEXATTR_DEAD	0xDEAD0004


struct	__pthread_cond_st {
	unsigned int	ptc_magic;

	/* Protects the queue of waiters */
	pthread_spin_t	ptc_lock;
	pthread_queue_t	ptc_waiters;

	pthread_mutex_t	*ptc_mutex;	/* Current mutex */
	void	*ptc_private;
};

#define	_PT_COND_MAGIC	0x55550005
#define	_PT_COND_DEAD	0xDEAD0005

#define _PTHREAD_COND_INITIALIZER { _PT_COND_MAGIC,			\
				   __SIMPLELOCK_UNLOCKED,		\
				   {NULL, NULL},			\
				   NULL,				\
				   NULL  				\
				 }

struct	__pthread_condattr_st {
	unsigned int	ptca_magic;
	void	*ptca_private;
};

#define	_PT_CONDATTR_MAGIC	0x66660006
#define	_PT_CONDATTR_DEAD	0xDEAD0006

struct	__pthread_once_st {
	pthread_mutex_t	pto_mutex;
	int	pto_done;
};

#define _PTHREAD_ONCE_INIT	{ PTHREAD_MUTEX_INITIALIZER, 0 }

struct	__pthread_spinlock_st {
	unsigned int	pts_magic;
	pthread_spin_t	pts_spin;
	int		pts_flags;
};
	
#define	_PT_SPINLOCK_MAGIC	0x77770007
#define	_PT_SPINLOCK_DEAD	0xDEAD0007
#define _PT_SPINLOCK_PSHARED	0x00000001

/* PTHREAD_SPINLOCK_INITIALIZER is an extension not specified by POSIX. */
#define _PTHREAD_SPINLOCK_INITIALIZER { _PT_SPINLOCK_MAGIC,		\
				       __SIMPLELOCK_UNLOCKED,		\
				       0				\
				     }

struct	__pthread_rwlock_st {
	unsigned int	ptr_magic;

	/* Protects data below */
	pthread_spin_t	ptr_interlock;

	pthread_queue_t	ptr_rblocked;
	pthread_queue_t	ptr_wblocked;
	unsigned int	ptr_nreaders;
	volatile pthread_t ptr_owner;
	void	*ptr_private;
};

#define	_PT_RWLOCK_MAGIC	0x99990009
#define	_PT_RWLOCK_DEAD		0xDEAD0009

#define _PTHREAD_RWLOCK_INITIALIZER { _PT_RWLOCK_MAGIC,			\
				     __SIMPLELOCK_UNLOCKED,		\
				     {NULL, NULL},			\
				     {NULL, NULL},			\
				     0,					\
				     NULL,				\
				     NULL,				\
				   }

struct	__pthread_rwlockattr_st {
	unsigned int	ptra_magic;
	void *ptra_private;
};

#define _PT_RWLOCKATTR_MAGIC	0x99990909
#define _PT_RWLOCKATTR_DEAD	0xDEAD0909

struct	__pthread_barrier_st {
	unsigned int	ptb_magic;

	/* Protects data below */
	pthread_spin_t	ptb_lock;

	pthread_queue_t	ptb_waiters;
	unsigned int	ptb_initcount;
	unsigned int	ptb_curcount;
	unsigned int	ptb_generation;

	void		*ptb_private;
};

#define	_PT_BARRIER_MAGIC	0x88880008
#define	_PT_BARRIER_DEAD	0xDEAD0008

struct	__pthread_barrierattr_st {
	unsigned int	ptba_magic;
	void		*ptba_private;
};

#define	_PT_BARRIERATTR_MAGIC	0x88880808
#define	_PT_BARRIERATTR_DEAD	0xDEAD0808

#endif	/* _LIB_PTHREAD_TYPES_H */
