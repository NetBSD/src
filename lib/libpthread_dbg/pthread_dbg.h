/*	$NetBSD: pthread_dbg.h,v 1.3 2003/02/27 00:54:08 nathanw Exp $	*/

/*-
 * Copyright (c) 2002 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Nathan J. Williams for Wasabi Systems, Inc.
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
 *      This product includes software developed for the NetBSD Project by 
 *      Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/types.h>
#include <signal.h>

struct td_proc_st;
struct td_thread_st;
struct td_sync_st;

typedef struct td_proc_st td_proc_t;
typedef struct td_thread_st td_thread_t;
typedef struct td_sync_st td_sync_t;

struct td_proc_callbacks_t {
	int (*proc_read)(void *arg, caddr_t addr, void *buf, size_t size);
	int (*proc_write)(void *arg, caddr_t addr, void *buf, size_t size);
	int (*proc_lookup)(void *arg, char *sym, caddr_t *addr);
	int (*proc_regsize)(void *arg, int regset, size_t *size);
	int (*proc_getregs)(void *arg, int regset, int lwp, void *buf);
	int (*proc_setregs)(void *arg, int regset, int lwp, void *buf);
};


typedef struct td_thread_info_st {
	caddr_t	thread_addr;		/* Address of data structure */
	int	thread_state;		/* TD_STATE_*; see below */
	int	thread_type;		/* TD_TYPE_*; see below */
	int	thread_id;
	stack_t	thread_stack;
	int	thread_hasjoiners;	/* 1 if threads are waiting */
	caddr_t	thread_tls;
	int	thread_errno;
	sigset_t thread_sigmask;
	sigset_t thread_sigpending;
	long	pad[32];
} td_thread_info_t;

#define	TD_STATE_UNKNOWN	0
#define TD_STATE_RUNNING	1	/* On a processor */
#define	TD_STATE_RUNNABLE	2	/* On a run queue */
#define	TD_STATE_BLOCKED	3	/* Blocked in the kernel */
#define	TD_STATE_SLEEPING	4	/* Blocked on a sync object */
#define	TD_STATE_ZOMBIE		5

#define	TD_TYPE_UNKNOWN	0
#define	TD_TYPE_USER	1
#define	TD_TYPE_SYSTEM	2

typedef struct {
	caddr_t	sync_addr;	/* Address within the process */
	int	sync_type;	/* TD_SYNC_*; see below */
	size_t	sync_size;
	int	sync_haswaiters;  /* 1 if threads are known to be waiting */
	union {
		struct {
			int	locked;
			td_thread_t *owner;
		} mutex;
		struct {
			int	locked;
		} spin;
		struct {
			td_thread_t *thread;
		} join;
		struct {
			int	locked;
			int	readlocks;
			td_thread_t *writeowner;
		} rwlock;
		long pad[8];
	} sync_data;
} td_sync_info_t;

#define	TD_SYNC_UNKNOWN	0
#define	TD_SYNC_MUTEX	1	/* pthread_mutex_t */
#define TD_SYNC_COND	2	/* pthread_cond_t */
#define TD_SYNC_SPIN	3	/* pthread_spinlock_t */
#define TD_SYNC_JOIN	4	/* thread being joined */
#define TD_SYNC_RWLOCK	5	/* pthread_rwlock_t */

/* Error return codes */
#define TD_ERR_OK		0
#define TD_ERR_ERR		1  /* Generic error */
#define TD_ERR_NOSYM		2  /* Symbol not found (proc_lookup) */
#define TD_ERR_NOOBJ		3  /* No object matched the request */
#define TD_ERR_BADTHREAD	4  /* Request is not meaningful for that thread */
#define TD_ERR_INUSE		5  /* The process is already being debugged */
#define TD_ERR_NOLIB		6  /* The process is not using libpthread */
#define TD_ERR_NOMEM		7  /* malloc() failed */
#define TD_ERR_IO		8  /* A callback failed to read or write */
#define TD_ERR_INVAL		9  /* Invalid parameter */

/* Make a connection to a threaded process */
int td_open(struct td_proc_callbacks_t *, void *arg, td_proc_t **);
int td_close(td_proc_t *);

/* Iterate over the threads in the process */
int td_thr_iter(td_proc_t *, int (*)(td_thread_t *, void *), void *);

/* Get information on a thread */
int td_thr_info(td_thread_t *, td_thread_info_t *);

/* Get the user-assigned name of a thread */
int td_thr_getname(td_thread_t *, char *, int);

/* Get register state of a thread */
int td_thr_getregs(td_thread_t *, int, void *);

/* Set register state of a thread */
int td_thr_setregs(td_thread_t *, int, void *);

/* Iterate over the set of threads that are joining with the given thread */
int td_thr_join_iter(td_thread_t *, int (*)(td_thread_t *, void *), void *);

/* Get the synchronization object that the thread is sleeping on */
int td_thr_sleepinfo(td_thread_t *, td_sync_t **);

/* Get information on a synchronization object */
int td_sync_info(td_sync_t *, td_sync_info_t *);

/* Iterate over the set of threads waiting on a synchronization object */
int td_sync_waiters_iter(td_sync_t *, int (*)(td_thread_t *, void *), void *);

/* Convert the process address to a synchronization handle, if possible */
int td_map_addr2sync(td_proc_t *, caddr_t addr, td_sync_t **);

/* Convert the pthread_t to a thread handle, if possible */
int td_map_pth2thr(td_proc_t *, pthread_t, td_thread_t **);

/* Convert the thread ID to a thread handle, if possible */
int td_map_id2thr(td_proc_t *, int, td_thread_t **);

/* Return the thread handle of the thread running on the given LWP */
int td_map_lwp2thr(td_proc_t *, int, td_thread_t **);

/*
 * Establish a mapping between threads and LWPs. Must be called
 * every time a live process runs before calling td_thr_getregs() or
 * td_thr_setregs().
 */
int td_map_lwps(td_proc_t *);

/* Iterate over the set of TSD keys in the process */
int td_tsd_iter(td_proc_t *, int (*)(pthread_key_t, void (*)(void *), void *), 
    void *);

/* Get a TSD value from a thread */
int td_thr_tsd(td_thread_t *, pthread_key_t, void **);
