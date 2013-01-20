/*	$NetBSD: pthread_dbg.c,v 1.42 2013/01/20 18:45:19 christos Exp $	*/

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

#include <sys/cdefs.h>
__RCSID("$NetBSD: pthread_dbg.c,v 1.42 2013/01/20 18:45:19 christos Exp $");

#define __EXPOSE_STACK 1

#include <sys/param.h>
#include <sys/types.h>
#include <sys/lock.h>

#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <lwp.h>

#include <machine/reg.h>

#include <pthread.h>
#include <pthread_int.h>
#include <pthread_dbg.h>
#include <pthread_dbg_int.h>

#define PT_STACKMASK (proc->stackmask)

/* Compensate for debuggers that want a zero ID to be a sentinel */
#define TN_OFFSET 1

static int td__getthread(td_proc_t *proc, caddr_t addr, td_thread_t **threadp);

int
td_open(struct td_proc_callbacks_t *cb, void *arg, td_proc_t **procp)
{
	td_proc_t *proc;
	caddr_t addr;
	int dbg;
	int val;

	proc = malloc(sizeof(*proc));
	if (proc == NULL)
		return TD_ERR_NOMEM;
	proc->cb = cb;
	proc->arg = arg;

	val = LOOKUP(proc, "pthread__dbg", &addr);
	if (val != 0) {
		if (val == TD_ERR_NOSYM)
			val = TD_ERR_NOLIB;
		goto error;
	}
	proc->dbgaddr = addr;

	val = LOOKUP(proc, "pthread__allqueue", &addr);
	if (val != 0)
		goto error;
	proc->allqaddr = addr;

	val = LOOKUP(proc, "pthread__tsd_list", &addr);
	if (val != 0)
		goto error;
	proc->tsdlistaddr = addr;

	val = LOOKUP(proc, "pthread__tsd_destructors", &addr);
	if (val != 0)
		goto error;
	proc->tsddestaddr = addr;

	val = READ(proc, proc->dbgaddr, &dbg, sizeof(int));
	if (val != 0)
		goto error;

	if (dbg != 0) {
		/* Another instance of libpthread_dbg is already attached. */
		val = TD_ERR_INUSE;
		goto error;
	}

	val = LOOKUP(proc, "pthread__stacksize_lg", &addr);
	if (val == 0)
		proc->stacksizeaddr = addr;
	else
		proc->stacksizeaddr = NULL;
	proc->stacksizelg = -1;
	proc->stacksize = 0;
	proc->stackmask = 0;

	proc->regbuf = NULL;
	proc->fpregbuf = NULL;
	
	dbg = getpid();
	/*
	 * If this fails it probably means we're debugging a core file and
	 * can't write to it.
	 * If it's something else we'll lose the next time we hit WRITE,
	 * but not before, and that's OK.
	 */
	WRITE(proc, proc->dbgaddr, &dbg, sizeof(int));

	PTQ_INIT(&proc->threads);
	
	*procp = proc;

	return 0;

 error:
	free(proc);
	return val;
}

int
td_close(td_proc_t *proc)
{
	int dbg;
	td_thread_t *t, *next;

	dbg = 0;
	/*
	 * Error returns from this write are mot really a problem;
	 * the process doesn't exist any more.
	 */
	WRITE(proc, proc->dbgaddr, &dbg, sizeof(int));

	/* Deallocate the list of thread structures */
	for (t = PTQ_FIRST(&proc->threads); t; t = next) {
		next = PTQ_NEXT(t, list);
		PTQ_REMOVE(&proc->threads, t, list);
		free(t);
	}
	if (proc->regbuf != NULL) {
		free(proc->regbuf);
		free(proc->fpregbuf);
	}

	free(proc);
	return 0;
}


int
td_thr_iter(td_proc_t *proc, int (*call)(td_thread_t *, void *), void *callarg)
{
	int val;
	caddr_t next;
	pthread_queue_t allq;
	td_thread_t *thread;

	val = READ(proc, proc->allqaddr, &allq, sizeof(allq));
	if (val != 0)
		return val;
	
	next = (void *)allq.ptqh_first;
	while (next != NULL) {
		val = td__getthread(proc, next, &thread);
		if (val != 0)
			return val;
		val = (*call)(thread, callarg);
		if (val != 0)
			return 0;

		val = READ(proc, 
		    next + offsetof(struct __pthread_st, pt_allq.ptqe_next), 
		    &next, sizeof(next));
		if (val != 0)
			return val;
	}
	return 0;
}

int
td_thr_info(td_thread_t *thread, td_thread_info_t *info)
{
	int tmp, val;

	val = READ(thread->proc, thread->addr, &tmp, sizeof(tmp));
	if (val != 0)
		return val;

	if (tmp != PT_MAGIC)
		return TD_ERR_BADTHREAD;

	info->thread_addr = thread->addr;
	if ((val = READ(thread->proc, 
		      thread->addr + offsetof(struct __pthread_st, pt_state), 
		      &tmp, sizeof(tmp))) != 0)
		return val;
	switch (tmp) {
	case PT_STATE_RUNNING:
		info->thread_state = TD_STATE_RUNNING;
		break;
#ifdef XXXLWP
	case PT_STATE_SUSPENDED:
		info->thread_state = TD_STATE_SUSPENDED;
		break;
#endif
	case PT_STATE_ZOMBIE:
		info->thread_state = TD_STATE_ZOMBIE;
		break;
	default:
		info->thread_state = TD_STATE_UNKNOWN;
	}

	info->thread_type = TD_TYPE_USER;

	if ((val = READ(thread->proc, 
	    thread->addr + offsetof(struct __pthread_st, pt_stack),
	    &info->thread_stack, sizeof(stack_t))) != 0)
		return val;

	if ((val = READ(thread->proc, 
	    thread->addr + offsetof(struct __pthread_st, pt_errno),
	    &info->thread_errno, sizeof(info->thread_errno))) != 0)
		return val;

	if ((val = READ(thread->proc, 
	    thread->addr + offsetof(struct __pthread_st, pt_lid),
	    &info->thread_id, sizeof(info->thread_id))) != 0)
		return val;

	info->thread_id += TN_OFFSET;
	
	return 0;
}

int
td_thr_getname(td_thread_t *thread, char *name, int len)
{
	int val, tmp;
	caddr_t nameaddr;
	

	val = READ(thread->proc, thread->addr, &tmp, sizeof(tmp));
	if (val != 0)
		return val;

	if (tmp != PT_MAGIC)
		return TD_ERR_BADTHREAD;

	if ((val = READ(thread->proc,
	    thread->addr + offsetof(struct __pthread_st, pt_name),
	    &nameaddr, sizeof(nameaddr))) != 0)
		return val;

	if (nameaddr == 0)
		name[0] = '\0';
	else if ((val = READ(thread->proc, nameaddr,
	    name, (size_t)MIN(PTHREAD_MAX_NAMELEN_NP, len))) != 0)
		return val;

	return 0;
}

int
td_thr_getregs(td_thread_t *thread, int regset, void *buf)
{
	int tmp, val;

	if ((val = READ(thread->proc, 
		      thread->addr + offsetof(struct __pthread_st, pt_state), 
		      &tmp, sizeof(tmp))) != 0)
		return val;

	switch (tmp) {
	case PT_STATE_RUNNING:
		/*
		 * The register state of the thread is live in the
		 * inferior process's register state.
		 */
		val = GETREGS(thread->proc, regset, thread->lwp, buf);
		if (val != 0)
			return val;
		break;
	case PT_STATE_ZOMBIE:
	default:
		return TD_ERR_BADTHREAD;
	}

	return 0;
}

int
td_thr_setregs(td_thread_t *thread, int regset, void *buf)
{
	int val, tmp;

	if ((val = READ(thread->proc, 
		      thread->addr + offsetof(struct __pthread_st, pt_state), 
		      &tmp, sizeof(tmp))) != 0)
		return val;

	switch (tmp) {
	case PT_STATE_RUNNING:
		/*
		 * The register state of the thread is live in the
		 * inferior process's register state.  
		 */
		val = SETREGS(thread->proc, regset, thread->lwp, buf);
		if (val != 0)
			return val;
		break;
	case PT_STATE_ZOMBIE:
	default:
		return TD_ERR_BADTHREAD;
	}

	return 0;
}

int
td_map_pth2thr(td_proc_t *proc, pthread_t thread, td_thread_t **threadp)
{
	int magic, val;

	val = READ(proc, (void *)thread, &magic, sizeof(magic));
	if (val != 0)
		return val;

	if (magic != PT_MAGIC)
		return TD_ERR_NOOBJ;

	val = td__getthread(proc, (void *)thread, threadp);
	if (val != 0)
		return val;

	return 0;
}

int
td_map_id2thr(td_proc_t *proc, int threadid, td_thread_t **threadp)
{
	int val, num;
	caddr_t next;
	pthread_queue_t allq;
	td_thread_t *thread;


	val = READ(proc, proc->allqaddr, &allq, sizeof(allq));
	if (val != 0)
		return val;
	
	/* Correct for offset */
	threadid -= TN_OFFSET;
	next = (void *)allq.ptqh_first;
	while (next != NULL) {
		val = READ(proc, 
		    next + offsetof(struct __pthread_st, pt_lid), 
		    &num, sizeof(num));

		if (num == threadid)
			break;

		val = READ(proc, 
		    next + offsetof(struct __pthread_st, pt_allq.ptqe_next), 
		    &next, sizeof(next));
		if (val != 0)
			return val;
	}

	if (next == 0) {
		/* A matching thread was not found. */
		return TD_ERR_NOOBJ;
	}

	val = td__getthread(proc, next, &thread);
	if (val != 0)
		return val;
	*threadp = thread;

	return 0;
}

int
td_tsd_iter(td_proc_t *proc,
    int (*call)(pthread_key_t, void (*)(void *), void *), void *arg)
{
#ifdef notyet
	int val;
	int i;
	void *allocated;
	void (*destructor)(void *);

	for (i = 0; i < PTHREAD_KEYS_MAX; i++) {
		val = READ(proc, proc->tsdlistaddr + i * sizeof(allocated),
		    &allocated, sizeof(allocated));
		if (val != 0)
			return val;

		if ((uintptr_t)allocated) {
			val = READ(proc,  proc->tsddestaddr +
			    i * sizeof(destructor),
			    &destructor, sizeof(destructor));
			if (val != 0)
				return val;
			
			val = (call)(i, destructor, arg);
			if (val != 0)
				return val;
		}
	}
#else
	abort();
#endif

	return 0;
}
		
/* Suspend a thread from running */
int
td_thr_suspend(td_thread_t *thread)
{
	int tmp, val;

	/* validate the thread */
	val = READ(thread->proc, thread->addr, &tmp, sizeof(tmp));
	if (val != 0)
		return val;
	if (tmp != PT_MAGIC)
		return TD_ERR_BADTHREAD;

	val = READ(thread->proc, thread->addr +
	    offsetof(struct __pthread_st, pt_lid),
	    &tmp, sizeof(tmp));
	if (val != 0)
		return val;

	/* XXXLWP continue the sucker */;

	return 0;
}

/* Restore a suspended thread to its previous state */
int
td_thr_resume(td_thread_t *thread)
{
	int tmp, val;

	/* validate the thread */
	val = READ(thread->proc, thread->addr, &tmp, sizeof(tmp));
	if (val != 0)
		return val;
	if (tmp != PT_MAGIC)
		return TD_ERR_BADTHREAD;

	val = READ(thread->proc, thread->addr +
	    offsetof(struct __pthread_st, pt_lid),
	    &tmp, sizeof(tmp));
	if (val != 0)
		return val;

	/* XXXLWP continue the sucker */;

	return 0;
}

static int
td__getthread(td_proc_t *proc, caddr_t addr, td_thread_t **threadp)
{
	td_thread_t *thread;
	
	/*
	 * Check if we've allocated a descriptor for this thread. 
	 * Sadly, this makes iterating over a set of threads O(N^2)
	 * in the number of threads. More sophisticated data structures
	 * can wait.
       	 */
	PTQ_FOREACH(thread, &proc->threads, list) {
		if (thread->addr == addr)
			break;
	}
	if (thread == NULL) {
		thread = malloc(sizeof(*thread));
		if (thread == NULL)
			return TD_ERR_NOMEM;
		thread->proc = proc;
		thread->addr = addr;
		thread->lwp  = 0;
		PTQ_INSERT_HEAD(&proc->threads, thread, list);
	}

	*threadp = thread;
	return 0;
}

int
td_thr_tsd(td_thread_t *thread, pthread_key_t key, void **value)
{
	int val;

	val = READ(thread->proc, thread->addr + 
	    offsetof(struct __pthread_st, pt_specific) +
	    key * sizeof(void *), value, sizeof(*value));

	return val;
}
