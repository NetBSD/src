/*	$NetBSD: sysmon_taskq.c,v 1.3 2003/09/06 23:28:30 christos Exp $	*/

/*
 * Copyright (c) 2001, 2003 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Jason R. Thorpe for Wasabi Systems, Inc.
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
 *	This product includes software developed for the NetBSD Project by
 *	Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
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

/*
 * General purpose task queue for sysmon back-ends.  This can be
 * used to run callbacks that require thread context.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: sysmon_taskq.c,v 1.3 2003/09/06 23:28:30 christos Exp $");

#include <sys/param.h>
#include <sys/malloc.h>
#include <sys/lock.h>
#include <sys/queue.h>
#include <sys/proc.h>
#include <sys/kthread.h>
#include <sys/systm.h>

#include <dev/sysmon/sysmon_taskq.h>

struct sysmon_task {
	TAILQ_ENTRY(sysmon_task) st_list;
	void (*st_func)(void *);
	void *st_arg;
	u_int st_pri;
};

static TAILQ_HEAD(, sysmon_task) sysmon_task_queue =
    TAILQ_HEAD_INITIALIZER(sysmon_task_queue);
struct simplelock sysmon_task_queue_slock = SIMPLELOCK_INITIALIZER;

#define	SYSMON_TASK_QUEUE_LOCK(s)					\
do {									\
	s = splsched();							\
	simple_lock(&sysmon_task_queue_slock);				\
} while (/*CONSTCOND*/0)

#define	SYSMON_TASK_QUEUE_UNLOCK(s)					\
do {									\
	simple_unlock(&sysmon_task_queue_slock);			\
	splx((s));							\
} while (/*CONSTCOND*/0)

static __volatile int sysmon_task_queue_cleanup_sem;

static struct proc *sysmon_task_queue_proc;

static void sysmon_task_queue_create_thread(void *);
static void sysmon_task_queue_thread(void *);

static struct simplelock sysmon_task_queue_initialized_slock =
    SIMPLELOCK_INITIALIZER;
static int sysmon_task_queue_initialized;

/*
 * sysmon_task_queue_init:
 *
 *	Initialize the sysmon task queue.
 */
void
sysmon_task_queue_init(void)
{

	simple_lock(&sysmon_task_queue_initialized_slock);
	if (sysmon_task_queue_initialized) {
		simple_unlock(&sysmon_task_queue_initialized_slock);
		return;
	}

	sysmon_task_queue_initialized = 1;
	simple_unlock(&sysmon_task_queue_initialized_slock);

	kthread_create(sysmon_task_queue_create_thread, NULL);
}

/*
 * sysmon_task_queue_fini:
 *
 *	Tear town the sysmon task queue.
 */
void
sysmon_task_queue_fini(void)
{
	int s;

	SYSMON_TASK_QUEUE_LOCK(s);

	sysmon_task_queue_cleanup_sem = 1;
	wakeup(&sysmon_task_queue);

	while (sysmon_task_queue_cleanup_sem != 0) {
		(void) ltsleep((void *) &sysmon_task_queue_cleanup_sem,
		    PVM, "stfini", 0, &sysmon_task_queue_slock);
	}

	SYSMON_TASK_QUEUE_UNLOCK(s);
}

/*
 * sysmon_task_queue_create_thread:
 *
 *	Create the sysmon task queue execution thread.
 */
static void
sysmon_task_queue_create_thread(void *arg)
{
	int error;

	error = kthread_create1(sysmon_task_queue_thread, NULL,
	    &sysmon_task_queue_proc, "sysmon");
	if (error) {
		printf("Unable to create sysmon task queue thread, "
		    "error = %d\n", error);
		panic("sysmon_task_queue_create_thread");
	}
}

/*
 * sysmon_task_queue_thread:
 *
 *	The sysmon task queue execution thread.  We execute callbacks that
 *	have been queued for us.
 */
static void
sysmon_task_queue_thread(void *arg)
{
	struct sysmon_task *st;
	int s;

	/*
	 * Run through all the tasks before we check for the exit
	 * condition; it's probably more important to actually run
	 * all the tasks before we exit.
	 */
	for (;;) {
		SYSMON_TASK_QUEUE_LOCK(s);
		st = TAILQ_FIRST(&sysmon_task_queue);
		if (st == NULL) {
			/* Check for the exit condition. */
			if (sysmon_task_queue_cleanup_sem != 0) {
				/* Time to die. */
				sysmon_task_queue_cleanup_sem = 0;
				wakeup((void *) &sysmon_task_queue_cleanup_sem);
				SYSMON_TASK_QUEUE_UNLOCK(s);
				kthread_exit(0);
			}
			(void) ltsleep(&sysmon_task_queue, PVM,
			    "smtaskq", 0, &sysmon_task_queue_slock);
			SYSMON_TASK_QUEUE_UNLOCK(s);
			continue;
		}
		TAILQ_REMOVE(&sysmon_task_queue, st, st_list);
		SYSMON_TASK_QUEUE_UNLOCK(s);

		(*st->st_func)(st->st_arg);
		free(st, M_TEMP);
	}
	panic("sysmon_task_queue_thread: impossible");
}

/*
 * sysmon_task_queue_sched:
 *
 *	Schedule a task for deferred execution.
 */
int
sysmon_task_queue_sched(u_int pri, void (*func)(void *), void *arg)
{
	struct sysmon_task *st, *lst;
	int s;

	if (sysmon_task_queue_proc == NULL)
		printf("WARNING: Callback scheduled before sysmon task queue "
		    "thread present.\n");

	if (func == NULL)
		return (EINVAL);

	st = malloc(sizeof(*st), M_TEMP, M_NOWAIT);
	if (st == NULL)
		return (ENOMEM);

	st->st_func = func;
	st->st_arg = arg;
	st->st_pri = pri;

	SYSMON_TASK_QUEUE_LOCK(s);
	TAILQ_FOREACH(lst, &sysmon_task_queue, st_list) {
		if (st->st_pri > lst->st_pri) {
			TAILQ_INSERT_BEFORE(lst, st, st_list);
			break;
		}
	}
	if (lst == NULL)
		TAILQ_INSERT_TAIL(&sysmon_task_queue, st, st_list);
	wakeup(&sysmon_task_queue);
	SYSMON_TASK_QUEUE_UNLOCK(s);

	return (0);
}
