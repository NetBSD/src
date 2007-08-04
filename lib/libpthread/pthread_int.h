/*	$NetBSD: pthread_int.h,v 1.44.2.2 2007/08/04 18:54:14 ad Exp $	*/

/*-
 * Copyright (c) 2001, 2002, 2003, 2006, 2007 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Nathan J. Williams and Andrew Doran.
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

#ifndef _LIB_PTHREAD_INT_H
#define _LIB_PTHREAD_INT_H

#define PTHREAD__DEBUG
#define ERRORCHECK

#include "pthread_types.h"
#include "pthread_queue.h"
#include "pthread_debug.h"
#include "pthread_md.h"

#include <lwp.h>
#include <signal.h>

#define PTHREAD_KEYS_MAX 256
/*
 * The size of this structure needs to be no larger than struct
 * __pthread_cleanup_store, defined in pthread.h.
 */
struct pt_clean_t {
	PTQ_ENTRY(pt_clean_t)	ptc_next;
	void	(*ptc_cleanup)(void *);
	void	*ptc_arg;
};

/* Private data for pthread_attr_t */
struct pthread_attr_private {
	char ptap_name[PTHREAD_MAX_NAMELEN_NP];
	void *ptap_namearg;
	void *ptap_stackaddr;
	size_t ptap_stacksize;
	size_t ptap_guardsize;
};

struct	__pthread_st {
	unsigned int	pt_magic;	/* Magic number */
	int		pt_num;		/* ID XXX should die */
	int		pt_state;	/* running, blocked, etc. */
	pthread_spin_t	pt_lock;	/* lock on state */
	int		pt_flags;	/* see PT_FLAG_* below */
	int		pt_cancel;	/* Deferred cancellation */
	int		pt_spinlocks;	/* Number of spinlocks held. */
	void		*pt_mutexhint;	/* Last mutex held. */
	int		pt_errno;	/* Thread-specific errno. */
	stack_t		pt_stack;	/* Our stack */
	ucontext_t	*pt_uc;		/* Saved context when we're stopped */
	void		*pt_exitval;	/* Read by pthread_join() */
	char		*pt_name;	/* Thread's name, set by the app. */

	/* Stack of cancellation cleanup handlers and their arguments */
	PTQ_HEAD(, pt_clean_t)	pt_cleanup_stack;

	/* Thread-specific data */
	void		*pt_specific[PTHREAD_KEYS_MAX];

	/* For debugger: LWPs waiting to join. */
	pthread_queue_t	pt_joiners;
	PTQ_ENTRY(__pthread_st) pt_joinq;

	/* LWP ID and entry on the list of all threads. */
	lwpid_t		pt_lid;
	PTQ_ENTRY(__pthread_st)	pt_allq;

	/*
	 * General synchronization data.  We try to align, as threads
	 * on other CPUs will access this data frequently.
	 */
	int		pt_dummy1 __aligned(128);
	int		pt_sleeponq;	/* on a sleep queue */
	int		pt_signalled;	/* Received pthread_cond_signal() */
	void		*pt_sleepobj;	/* object slept on */
	pthread_queue_t	*pt_sleepq;	/* sleep queue */
	PTQ_ENTRY(__pthread_st)	pt_sleep;
	int		pt_dummy2 __aligned(128);
};

/* Thread states */
#define PT_STATE_RUNNING	1
#define PT_STATE_ZOMBIE		5
#define PT_STATE_DEAD		6

/* Flag values */

#define PT_FLAG_DETACHED	0x0001
#define PT_FLAG_CS_DISABLED	0x0004	/* Cancellation disabled */
#define PT_FLAG_CS_ASYNC	0x0008  /* Cancellation is async */
#define PT_FLAG_CS_PENDING	0x0010
#define PT_FLAG_SCOPE_SYSTEM	0x0040
#define PT_FLAG_EXPLICIT_SCHED	0x0080
#define PT_FLAG_SUSPENDED	0x0100	/* In the suspended queue */

#define PT_MAGIC	0x11110001
#define PT_DEAD		0xDEAD0001

#define PT_ATTR_MAGIC	0x22220002
#define PT_ATTR_DEAD	0xDEAD0002

#ifdef PT_FIXEDSTACKSIZE_LG

#define	PT_STACKSIZE_LG	PT_FIXEDSTACKSIZE_LG 
#define	PT_STACKSIZE	(1<<(PT_STACKSIZE_LG)) 
#define	PT_STACKMASK	(PT_STACKSIZE-1)

#else  /* PT_FIXEDSTACKSIZE_LG */

extern	int		pthread_stacksize_lg;
extern	size_t		pthread_stacksize;
extern	vaddr_t		pthread_stackmask;

#define	PT_STACKSIZE_LG	pthread_stacksize_lg
#define	PT_STACKSIZE	pthread_stacksize
#define	PT_STACKMASK	pthread_stackmask

#endif /* PT_FIXEDSTACKSIZE_LG */

/* Flag to be used in a ucontext_t's uc_flags indicating that
 * the saved register state is "user" state only, not full
 * trap state.
 */
#define _UC_USER_BIT		30
#define _UC_USER		(1LU << _UC_USER_BIT)

void	pthread_init(void)  __attribute__ ((__constructor__));

/* Utility functions */

/* Get offset from stack start to struct sa_stackinfo */
ssize_t	pthread__stackinfo_offset(void);

void	pthread__unpark_all(pthread_t self, pthread_spin_t *lock,
			    pthread_queue_t *threadq);
void	pthread__unpark(pthread_t self, pthread_spin_t *lock,
			pthread_queue_t *queue, pthread_t target);
int	pthread__park(pthread_t self, pthread_spin_t *lock,
		      pthread_queue_t *threadq,
		      const struct timespec *abs_timeout,
		      int cancelpt, const void *hint);

int	pthread__stackalloc(pthread_t *t);
void	pthread__initmain(pthread_t *t);

/* Internal locking primitives */
void	pthread__lockprim_init(int ncpu);
void	pthread_lockinit(pthread_spin_t *lock);
void	pthread_spinlock(pthread_t thread, pthread_spin_t *lock);
int	pthread_spintrylock(pthread_t thread, pthread_spin_t *lock);
void	pthread_spinunlock(pthread_t thread, pthread_spin_t *lock);

extern const struct pthread_lock_ops *pthread__lock_ops;

void	pthread__simple_lock_init(__cpu_simple_lock_t *);
int	pthread__simple_lock_try(__cpu_simple_lock_t *);
void	pthread__simple_unlock(__cpu_simple_lock_t *);

#ifndef _getcontext_u
int	_getcontext_u(ucontext_t *);
#endif
#ifndef _setcontext_u
int	_setcontext_u(const ucontext_t *);
#endif
#ifndef _swapcontext_u
int	_swapcontext_u(ucontext_t *, const ucontext_t *);
#endif

void	pthread__testcancel(pthread_t self);
int	pthread__find(pthread_t self, pthread_t target);

#ifndef PTHREAD_MD_INIT
#define PTHREAD_MD_INIT
#endif

#ifndef _INITCONTEXT_U_MD
#define _INITCONTEXT_U_MD(ucp)
#endif

#define _INITCONTEXT_U(ucp) do {					\
	(ucp)->uc_flags = _UC_CPU | _UC_STACK;				\
	_INITCONTEXT_U_MD(ucp)						\
	} while (/*CONSTCOND*/0)

#ifdef PTHREAD_MACHINE_HAS_ID_REGISTER
#define pthread__id(reg) (reg)
#else
/* Stack location of pointer to a particular thread */
#define pthread__id(sp) \
	((pthread_t) (((vaddr_t)(sp)) & ~PT_STACKMASK))

#define pthread__id_reg() pthread__sp()
#endif

#define pthread__self() (pthread__id(pthread__id_reg()))

#define pthread__abort()						\
	pthread__assertfunc(__FILE__, __LINE__, __func__, "unreachable")

#define pthread__assert(e) do {						\
	if (__predict_false(!(e)))					\
       	       pthread__assertfunc(__FILE__, __LINE__, __func__, #e);	\
        } while (/*CONSTCOND*/0)

#define pthread__error(err, msg, e) do {				\
	if (__predict_false(!(e))) {					\
       	       pthread__errorfunc(__FILE__, __LINE__, __func__, msg);	\
	       return (err);						\
	} 								\
        } while (/*CONSTCOND*/0)

void	pthread__destroy_tsd(pthread_t self);
void	pthread__assertfunc(const char *file, int line, const char *function,
		const char *expr);
void	pthread__errorfunc(const char *file, int line, const char *function,
		const char *msg);

#if defined(i386) || defined(__x86_64__)
#define	pthread__smt_pause()	__asm __volatile("rep; nop" ::: "memory")
#else
#define	pthread__smt_pause()	/* nothing */
#endif

extern int pthread__nspins;

#endif /* _LIB_PTHREAD_INT_H */
