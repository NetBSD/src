/*	$NetBSD: pthread_dbg.c,v 1.4 2003/02/27 00:54:07 nathanw Exp $	*/

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

#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>

#include <pthread.h>
#include <pthread_int.h>
#include <pthread_dbg.h>
#include <pthread_dbg_int.h>
#include <machine/reg.h>

#define MIN(a,b)	((a)<(b) ? (a) : (b))

static int td__getthread(td_proc_t *proc, caddr_t addr, td_thread_t **threadp);
static int td__getsync(td_proc_t *proc, caddr_t addr, td_sync_t **syncp);

int
td_open(struct td_proc_callbacks_t *cb, void *arg, td_proc_t **procp)
{
	td_proc_t *proc;
	caddr_t dbgaddr;
	int dbg;
	int val;

	proc = malloc(sizeof(*proc));
	if (proc == NULL)
		return TD_ERR_NOMEM;

	proc->cb = cb;
	proc->arg = arg;

	val = LOOKUP(proc, "pthread__dbg", &dbgaddr);
	if (val != 0) {
		if (val == TD_ERR_NOSYM)
			val = TD_ERR_NOLIB;
		goto error;
	}

	val = READ(proc, dbgaddr, &dbg, sizeof(int));
	if (val != 0)
		goto error;

	if (dbg != 0) {
		/* Another instance of libpthread_dbg is already attached. */
		val = TD_ERR_INUSE;
		goto error;
	}

	dbg = getpid();
	/*
	 * If this fails it probably means we're debugging a core file and
	 * can't write to it.
	 * If it's something else we'll lose the next time we hit WRITE,
	 * but not before, and that's OK.
	 */
	WRITE(proc, dbgaddr, &dbg, sizeof(int));

	proc->allqueue = 0;
	PTQ_INIT(&proc->threads);
	PTQ_INIT(&proc->syncs);
	
	*procp = proc;

	return 0;

 error:
	free(proc);
	return val;
}

int
td_close(td_proc_t *proc)
{
	caddr_t dbgaddr;
	int dbg;
	int val;
	td_thread_t *t, *next;
	td_sync_t *s, *nexts;

	val = LOOKUP(proc, "pthread__dbg", &dbgaddr);
	if (val != 0)
		return val;

	dbg = 0;
	/*
	 * Error returns from this write are mot really a problem;
	 * the process doesn't exist any more.
	 */
	WRITE(proc, dbgaddr, &dbg, sizeof(int));

	/* Deallocate the list of thread structures */
	for (t = PTQ_FIRST(&proc->threads); t; t = next) {
		next = PTQ_NEXT(t, list);
		PTQ_REMOVE(&proc->threads, t, list);
		free(t);
	}
	/* Deallocate the list of sync objects */
	for (s = PTQ_FIRST(&proc->syncs); s; s = nexts) {
		nexts = PTQ_NEXT(s, list);
		PTQ_REMOVE(&proc->syncs, s, list);
		free(s);
	}
	free(proc);
	return 0;
}


int
td_thr_iter(td_proc_t *proc, int (*call)(td_thread_t *, void *), void *callarg)
{
	int val;
	caddr_t allqaddr, next;
	struct pthread_queue_t allq;
	td_thread_t *thread;

	if (proc->allqueue == 0) {
		val = LOOKUP(proc, "pthread__allqueue", &allqaddr);
		if (val != 0)
			return val;
		proc->allqueue = allqaddr;
	} else {
		allqaddr = proc->allqueue;
	}

	val = READ(proc, allqaddr, &allq, sizeof(allq));
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
		    next + offsetof(struct pthread_st, pt_allq.ptqe_next), 
		    &next, sizeof(next));
		if (val != 0)
			return val;
	}
	return 0;
}

int
td_thr_info(td_thread_t *thread, td_thread_info_t *info)
{
	int val, tmp;
	struct pthread_queue_t queue;

	val = READ(thread->proc, thread->addr, &tmp, sizeof(tmp));
	if (val != 0)
		return val;

	if (tmp != PT_MAGIC)
		return TD_ERR_BADTHREAD;

	info->thread_addr = thread->addr;
	if ((val = READ(thread->proc, 
	    thread->addr + offsetof(struct pthread_st, pt_state), 
	    &tmp, sizeof(int))) != 0)
		return val;
	switch (tmp) {
	case PT_STATE_RUNNING:
		info->thread_state = TD_STATE_RUNNING;
		break;
	case PT_STATE_RUNNABLE:
		info->thread_state = TD_STATE_RUNNABLE;
		break;
	case PT_STATE_BLOCKED_SYS:
		info->thread_state = TD_STATE_BLOCKED;
		break;
	case PT_STATE_BLOCKED_QUEUE:
		info->thread_state = TD_STATE_SLEEPING;
		break;
	case PT_STATE_ZOMBIE:
		info->thread_state = TD_STATE_ZOMBIE;
		break;
	default:
		info->thread_state = TD_STATE_UNKNOWN;
	}

	if ((val = READ(thread->proc, 
	    thread->addr + offsetof(struct pthread_st, pt_type), 
	    &tmp, sizeof(int))) != 0)
		return val;
	switch (tmp) {
	case PT_THREAD_NORMAL:
		info->thread_type = TD_TYPE_USER;
		break;
	case PT_THREAD_UPCALL:
	case PT_THREAD_IDLE:
		info->thread_type = TD_TYPE_SYSTEM;
		break;
	default:
		info->thread_type = TD_TYPE_UNKNOWN;
	}

	if ((val = READ(thread->proc, 
	    thread->addr + offsetof(struct pthread_st, pt_stack),
	    &info->thread_stack, sizeof(stack_t))) != 0)
		return val;

	if ((val = READ(thread->proc, 
	    thread->addr + offsetof(struct pthread_st, pt_joiners),
	    &queue, sizeof(struct pthread_queue_t))) != 0)
		return val;

	if (PTQ_EMPTY(&queue))
		info->thread_hasjoiners = 0;
	else
		info->thread_hasjoiners = 1;

	if ((val = READ(thread->proc, 
	    thread->addr + offsetof(struct pthread_st, pt_errno),
	    &info->thread_errno, sizeof(info->thread_errno))) != 0)
		return val;

	if ((val = READ(thread->proc, 
	    thread->addr + offsetof(struct pthread_st, pt_num),
	    &info->thread_id, sizeof(info->thread_errno))) != 0)
		return val;

	if ((val = READ(thread->proc, 
	    thread->addr + offsetof(struct pthread_st, pt_sigmask),
	    &info->thread_sigmask, sizeof(info->thread_sigmask))) != 0)
		return val;

	if ((val = READ(thread->proc, 
	    thread->addr + offsetof(struct pthread_st, pt_siglist),
	    &info->thread_sigpending, sizeof(info->thread_sigpending))) != 0)
		return val;
	
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
	    thread->addr + offsetof(struct pthread_st, pt_name),
	    &nameaddr, sizeof(nameaddr))) != 0)
		return val;

	if (nameaddr == 0)
		name[0] = '\0';
	else if ((val = READ(thread->proc, nameaddr,
	    name, MIN(PTHREAD_MAX_NAMELEN_NP, len))) != 0)
		return val;

	return 0;
}

int
td_thr_getregs(td_thread_t *thread, int regset, void *buf)
{
	int tmp, val;
	caddr_t addr;
	ucontext_t uc;

	val = READ(thread->proc, 
	    thread->addr + offsetof(struct pthread_st, pt_state), 
	    &tmp, sizeof(int));
	if (val != 0)
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
	case PT_STATE_RUNNABLE:
	case PT_STATE_BLOCKED_SYS:
	case PT_STATE_BLOCKED_QUEUE:
		/*
		 * The register state of the thread is in the ucontext_t 
		 * of the thread structure.
		 */
		val = READ(thread->proc, 
		    thread->addr + offsetof(struct pthread_st, pt_uc),
		    &addr, sizeof(addr));
		if (val != 0)
			return val;
		val = READ(thread->proc,
		    addr, &uc, sizeof(uc));
		if (val != 0)
			return val;

		switch (regset) {
		case 0:
			PTHREAD_UCONTEXT_TO_REG((struct reg *)buf, &uc);
			break;
		case 1:
			PTHREAD_UCONTEXT_TO_FPREG((struct fpreg *)buf, &uc);
			break;
		case 2:
			return TD_ERR_INVAL;
		}
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

	int tmp, val;
	caddr_t addr;
	ucontext_t uc;

	val = READ(thread->proc, 
	    thread->addr + offsetof(struct pthread_st, pt_state), 
	    &tmp, sizeof(int));
	if (val != 0)
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
	case PT_STATE_RUNNABLE:
	case PT_STATE_BLOCKED_SYS:
	case PT_STATE_BLOCKED_QUEUE:
		/*
		 * The register state of the thread is in the ucontext_t 
		 * of the thread structure.
		 *
		 * Fetch the uc first, since there is state in it
		 * besides the registers that should be preserved.
		 */
		val = READ(thread->proc, 
		    thread->addr + offsetof(struct pthread_st, pt_uc),
		    &addr, sizeof(addr));
		if (val != 0)
			return val;
		val = READ(thread->proc,
		    addr, &uc, sizeof(uc));
		if (val != 0)
			return val;

		switch (regset) {
		case 0:
			PTHREAD_REG_TO_UCONTEXT(&uc,
			    (struct reg *)(void *)buf);
			break;
		case 1:
			PTHREAD_FPREG_TO_UCONTEXT(&uc,
			    (struct fpreg *)(void *)buf);
			break;
		case 2:
			return TD_ERR_INVAL;
		}

		val = WRITE(thread->proc,
		    addr, &uc, sizeof(uc));
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
td_thr_join_iter(td_thread_t *thread, int (*call)(td_thread_t *, void *),
    void *arg)
{
	int val;
	caddr_t next;
	td_thread_t *thread2;
	struct pthread_queue_t queue;

	if ((val = READ(thread->proc, 
	    thread->addr + offsetof(struct pthread_st, pt_joiners),
	    &queue, sizeof(struct pthread_queue_t))) != 0)
		return val;

	next = (void *)queue.ptqh_first;
	while (next != NULL) {
		val = td__getthread(thread->proc, next, &thread2);
		if (val != 0)
			return val;
		val = (*call)(thread2, arg);
		if (val != 0)
			return 0;

		val = READ(thread->proc, 
		    next + offsetof(struct pthread_st, pt_sleep.ptqe_next), 
		    &next, sizeof(next));
		if (val != 0)
			return val;
	}

	return 0;
}

int
td_sync_info(td_sync_t *s, td_sync_info_t *info)
{
	int val, magic, n;
	struct pthread_queue_t queue;
	pthread_spin_t slock;
	pthread_t taddr;
	
	val = READ(s->proc, s->addr, &magic, sizeof(magic));
	if (val != 0)
		return val;

	info->sync_type = TD_SYNC_UNKNOWN;
	info->sync_size = 0;
	info->sync_haswaiters = 0;
	switch (magic) {
	case _PT_MUTEX_MAGIC:
		info->sync_type = TD_SYNC_MUTEX;
		info->sync_size = sizeof(struct pthread_mutex_st);
		if ((val = READ(s->proc, 
		    s->addr + offsetof(struct pthread_mutex_st, ptm_blocked),
		    &queue, sizeof(struct pthread_queue_t))) != 0)
		return val;

		if (!PTQ_EMPTY(&queue))
			info->sync_haswaiters = 1;
		/*
		 * The cast to (void *) is to explicitly throw away the
		 * volatile qualifier on pthread_spin_t, 
		 * from __cpu_simple_lock_t.
		 */
		if ((val = READ(s->proc, 
		    s->addr + offsetof(struct pthread_mutex_st, ptm_lock),
		    (void *)&slock, sizeof(struct pthread_spinlock_st))) != 0)
			return val;
		if (slock == __SIMPLELOCK_LOCKED) {
			info->sync_data.mutex.locked = 1;
			if ((val = READ(s->proc, 
			    s->addr + offsetof(struct pthread_mutex_st, 
				ptm_owner),
			    &taddr, sizeof(pthread_t))) != 0)
				return val;
			taddr = pthread__id(taddr);
			td__getthread(s->proc, (void *)taddr, 
			    &info->sync_data.mutex.owner);
		} else
			info->sync_data.mutex.locked = 0;
		break;
	case _PT_COND_MAGIC:
		info->sync_type = TD_SYNC_COND;
		info->sync_size = sizeof(struct pthread_cond_st);
		if ((val = READ(s->proc, 
		    s->addr + offsetof(struct pthread_cond_st, ptc_waiters),
		    &queue, sizeof(struct pthread_queue_t))) != 0)
			return val;
		if (!PTQ_EMPTY(&queue))
			info->sync_haswaiters = 1;
		break;
	case _PT_SPINLOCK_MAGIC:
		info->sync_type = TD_SYNC_SPIN;
		info->sync_size = sizeof(struct pthread_spinlock_st);
		if ((val = READ(s->proc, 
		    s->addr + offsetof(struct pthread_spinlock_st, pts_spin),
		    (void *)&slock, sizeof(struct pthread_spinlock_st))) != 0)
			return val;
		if (slock == __SIMPLELOCK_LOCKED)
			info->sync_data.spin.locked = 1;
		break;
	case PT_MAGIC:
		info->sync_type = TD_SYNC_JOIN;
		info->sync_size = sizeof(struct pthread_st);
		td__getthread(s->proc, s->addr,
		    &info->sync_data.join.thread);
		if ((val = READ(s->proc, 
		    s->addr + offsetof(struct pthread_st, pt_joiners),
		    &queue, sizeof(struct pthread_queue_t))) != 0)
			return val;

		if (!PTQ_EMPTY(&queue))
			info->sync_haswaiters = 1;
		break;
	case (int)_PT_RWLOCK_MAGIC:
		info->sync_type = TD_SYNC_RWLOCK;
		info->sync_size = sizeof(struct pthread_rwlock_st);
		if ((val = READ(s->proc, 
		    s->addr + offsetof(struct pthread_rwlock_st, ptr_rblocked),
		    &queue, sizeof(struct pthread_queue_t))) != 0)
			return val;
		if (!PTQ_EMPTY(&queue))
			info->sync_haswaiters = 1;

		if ((val = READ(s->proc, 
		    s->addr + offsetof(struct pthread_rwlock_st, ptr_wblocked),
		    &queue, sizeof(struct pthread_queue_t))) != 0)
			return val;
		if (!PTQ_EMPTY(&queue))
			info->sync_haswaiters = 1;


		info->sync_data.rwlock.locked = 0;
		if ((val = READ(s->proc, 
		    s->addr + offsetof(struct pthread_rwlock_st, ptr_nreaders),
		    &n, sizeof(int))) != 0)
			return val;
		info->sync_data.rwlock.readlocks = n;
		if (n > 0)
			info->sync_data.rwlock.locked = 1;
	
		if ((val = READ(s->proc, 
		    s->addr + offsetof(struct pthread_rwlock_st, ptr_writer),
		    &taddr, sizeof(pthread_t))) != 0)
			return val;
		if (taddr != 0) {
			info->sync_data.rwlock.locked = 1;
			td__getthread(s->proc, (void *)taddr, 
			    &info->sync_data.rwlock.writeowner);
		}
		/*FALLTHROUGH*/
	default:
		return (0);
	}

	info->sync_addr = s->addr;

	return 0;
}


int
td_sync_waiters_iter(td_sync_t *s, int (*call)(td_thread_t *, void *),
    void *arg)
{
	int val, magic;
	caddr_t next;
	struct pthread_queue_t queue;
	td_thread_t *thread;

	val = READ(s->proc, s->addr, &magic, sizeof(magic));
	if (val != 0)
		return val;

	switch (magic) {
	case _PT_MUTEX_MAGIC:
		if ((val = READ(s->proc, 
		    s->addr + offsetof(struct pthread_mutex_st, ptm_blocked),
		    &queue, sizeof(struct pthread_queue_t))) != 0)
			return val;
		break;
	case _PT_COND_MAGIC:
		if ((val = READ(s->proc, 
		    s->addr + offsetof(struct pthread_cond_st, ptc_waiters),
		    &queue, sizeof(struct pthread_queue_t))) != 0)
			return val;
		break;
	case PT_MAGIC:
		/* Redundant with join_iter, but what the hell... */
		if ((val = READ(s->proc, 
		    s->addr + offsetof(struct pthread_st, pt_joiners),
		    &queue, sizeof(struct pthread_queue_t))) != 0)
			return val;
		break;
	default:
		return (0);
	}
	
	next = (void *)queue.ptqh_first;
	while (next != NULL) {
		val = td__getthread(s->proc, next, &thread);
		if (val != 0)
			return val;
		val = (*call)(thread, arg);
		if (val != 0)
			return 0;

		val = READ(s->proc, 
		    next + offsetof(struct pthread_st, pt_sleep.ptqe_next), 
		    &next, sizeof(next));
		if (val != 0)
			return val;
	}
	return 0;
}


int
td_map_addr2sync(td_proc_t *proc, caddr_t addr, td_sync_t **syncp)
{
	int magic, val;

	val = READ(proc, addr, &magic, sizeof(magic));
	if (val != 0)
		return val;

	if ((magic != _PT_MUTEX_MAGIC) &&
	    (magic != _PT_COND_MAGIC) &&
	    (magic != _PT_SPINLOCK_MAGIC))
		return TD_ERR_NOOBJ;

	val = td__getsync(proc, addr, syncp);
	if (val != 0)
		return val;

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
	caddr_t allqaddr, next;
	struct pthread_queue_t allq;
	td_thread_t *thread;


	if (proc->allqueue == 0) {
		val = LOOKUP(proc, "pthread__allqueue", &allqaddr);
		if (val != 0)
			return val;
		proc->allqueue = allqaddr;
	} else {
		allqaddr = proc->allqueue;
	}

	val = READ(proc, allqaddr, &allq, sizeof(allq));
	if (val != 0)
		return val;
	
	next = (void *)allq.ptqh_first;
	while (next != NULL) {
		val = READ(proc, 
		    next + offsetof(struct pthread_st, pt_num), 
		    &num, sizeof(num));

		if (num == threadid)
			break;

		val = READ(proc, 
		    next + offsetof(struct pthread_st, pt_allq.ptqe_next), 
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

/* Return the thread handle of the thread running on the given LWP */
int
td_map_lwp2thr(td_proc_t *proc, int lwp, td_thread_t **threadp)
{
	int val, magic;
	struct reg gregs;
	ucontext_t uc;
	void *th;

	val = GETREGS(proc, 0, lwp, &gregs);
	if (val != 0)
		return val;

	PTHREAD_REG_TO_UCONTEXT(&uc, &gregs);

	th = pthread__id(pthread__uc_sp(&uc));

	val = READ(proc, th, &magic, sizeof(magic));
	if (val != 0)
		return val;

	if (magic != PT_MAGIC)
		return TD_ERR_NOOBJ;

	val = td__getthread(proc, th, threadp);
	if (val != 0)
		return val;

	(*threadp)->lwp = lwp;

	return 0;
}

int
td_map_lwps(td_proc_t *proc)
{
	int i, val, nlwps;
	caddr_t addr;
	td_thread_t *thread;

	val = LOOKUP(proc, "pthread__maxlwps", &addr);
	if (val != 0)
		return val;

	val = READ(proc, addr, &nlwps, sizeof(int));
	if (val != 0)
		return val;

	for (i = 1; i <= nlwps; i++) {
		/*
		 * Errors are deliberately ignored for the call to
		 * td_map_lwp2thr(); it is entirely likely that not
		 * all LWPs in the range 1..nlwps exist, and that's
		 * not a problem.
		 */
		td_map_lwp2thr(proc, i, &thread);
	}
	return 0;
}

int
td_tsd_iter(td_proc_t *proc,
    int (*call)(pthread_key_t, void (*)(void *), void *), void *arg)
{
	caddr_t desaddr, allocaddr;
	int val;
	int i, allocated;
	void (*destructor)(void *);

	val = LOOKUP(proc, "pthread__tsd_alloc", &allocaddr);
	if (val != 0)
		return val;
	val = LOOKUP(proc, "pthread__tsd_destructors", &desaddr);
	if (val != 0)
		return val;
	
	for (i = 0; i < PTHREAD_KEYS_MAX; i++) {
		val = READ(proc, allocaddr + i * sizeof(int),
		    &allocated, sizeof(int));
		if (val != 0)
			return val;

		if (allocated) {
			val = READ(proc,  desaddr + i * sizeof(destructor),
			    &destructor, sizeof(destructor));
			if (val != 0)
				return val;
			
			val = (call)(i, destructor, arg);
			if (val != 0)
				return val;
		}
	}

	return 0;
}

/* Get the synchronization object that the thread is sleeping on */
int
td_thr_sleepinfo(td_thread_t *thread, td_sync_t **s)
{
	int val;
	caddr_t addr;

	if ((val = READ(thread->proc, 
	    thread->addr + offsetof(struct pthread_st, pt_sleepobj), 
	    &addr, sizeof(caddr_t))) != 0)
		return val;

	td__getsync(thread->proc, addr, s);

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


static int
td__getsync(td_proc_t *proc, caddr_t addr, td_sync_t **syncp)
{
	td_sync_t *s;

	/* Check if we've allocated a descriptor for this object. */
	PTQ_FOREACH(s, &proc->syncs, list) {
		if (s->addr == addr)
			break;
	}
	/* Allocate a fresh one */
	if (s == NULL) {
		s = malloc(sizeof(*s));
		if (s == NULL)
			return TD_ERR_NOMEM;
		s->proc = proc;
		s->addr = addr;
		PTQ_INSERT_HEAD(&proc->syncs, s, list);
	}

	*syncp = s;
	return 0;
}


int
td_thr_tsd(td_thread_t *thread, pthread_key_t key, void **value)
{
	int val;

	val = READ(thread->proc, thread->addr + 
	    offsetof(struct pthread_st, pt_specific) +
	    key * sizeof(void *), &value, sizeof(void *));

	return val;
}

