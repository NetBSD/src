/*	$NetBSD: test_priority_inheritance1.c,v 1.2 2008/04/28 20:23:07 martin Exp $	*/

/*-
 * Copyright (c) 2007 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Andrew Doran.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: test_priority_inheritance1.c,v 1.2 2008/04/28 20:23:07 martin Exp $");

#include <sys/param.h>
#include <sys/ioctl.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/kthread.h>
#include <sys/kernel.h>

#define	SECONDS		5

int	testcall(struct lwp *, void *, register_t *);
void	dummy(void *);
void	lowprio(void *);
void	highprio(void *);
void	thread_exit(int, int);
void	thread_enter(int *, int);

kmutex_t	test_mutex;
kcondvar_t	test_cv;
bool		test_exit;

int		started;
kcondvar_t	start_cv;
int		exited;
kcondvar_t	exit_cv;

void
changepri(int prio)
{
	struct lwp *l = curlwp;

	lwp_lock(l);
	lwp_changepri(l, prio);
	lwp_unlock(l);
}

void
printpri(const char *msg)
{
	struct lwp *l = curlwp;

#if 0
	printf("%s: %s: eprio=%d l_priority=%d l_usrpri=%d\n",
	    l->l_proc->p_comm, msg, lwp_eprio(l), l->l_priority, l->l_usrpri);
#else
	printf("%s: %s: l_priority=%d l_usrpri=%d\n",
	    l->l_proc->p_comm, msg, l->l_priority, l->l_usrpri);
#endif
}

void
thread_enter(int *nlocks, int prio)
{
	struct lwp *l = curlwp;

	KERNEL_UNLOCK_ALL(l, nlocks);
	changepri(prio);
	yield(); /* XXX */
	printpri("enter");
}

void
thread_exit(int nlocks, int prio)
{
	struct lwp *l = curlwp;

	mutex_enter(&test_mutex);
	exited++;
	cv_signal(&exit_cv);
	mutex_exit(&test_mutex);

#if 0
	if (lwp_eprio(l) != prio)
#else
	if (l->l_priority != prio || l->l_usrpri != prio)
#endif
		printpri("ERROR: priority changed");

	printpri("exit");

	KERNEL_LOCK(nlocks, curlwp);
	kthread_exit(0);
}

void
dummy(void *cookie)
{
	int count, nlocks;
	int prio = (int)cookie;

	thread_enter(&nlocks, prio);

	mutex_enter(&test_mutex);
	started++;
	cv_signal(&start_cv);
	mutex_exit(&test_mutex);
	while (!test_exit)
		yield();

	thread_exit(nlocks, prio);
}

void
lowprio(void *cookie)
{
	struct lwp *l = curlwp;
	int count, nlocks;
	int prio = (int)cookie;

	thread_enter(&nlocks, prio);

	mutex_enter(&test_mutex);
	started++;
	cv_signal(&start_cv);

	changepri(PUSER-1);
	kpause("test", false, hz, NULL);

	printpri("have mutex");
	mutex_exit(&test_mutex);
	printpri("released mutex");

	thread_exit(nlocks, PUSER-1);
}

void
highprio(void *cookie)
{
	int count, nlocks;
	int prio = (int)cookie;

	thread_enter(&nlocks, prio);

	printf("highprio start\n");
	mutex_enter(&test_mutex);
	while (started < ncpu + 1)
		cv_wait(&start_cv, &test_mutex);
	mutex_exit(&test_mutex);
	printf("highprio done\n");

	test_exit = true;

	thread_exit(nlocks, prio);
}

int
testcall(struct lwp *l, void *uap, register_t *retval)
{
	void (*func)(void *);
	int i;

	mutex_init(&test_mutex, MUTEX_DEFAULT, IPL_NONE);
	cv_init(&start_cv, "startcv");
	cv_init(&exit_cv, "exitcv");
	cv_init(&test_cv, "testcv");

	printf("test: creating threads\n");

	test_exit = false;
	started = exited = 0;

	changepri(PUSER-30);

	for (i = 0; i < ncpu; i++)
		kthread_create1(dummy, (void *)(PUSER-10), NULL, "dummy-%d", i);
	mutex_enter(&test_mutex);
	while (started < ncpu)
		cv_wait(&start_cv, &test_mutex);
	mutex_exit(&test_mutex);

	kthread_create1(lowprio, (void *)(PUSER-20), NULL, "lowprio");
	i++;
	kthread_create1(highprio, (void *)(PUSER-20), NULL, "highprio");
	i++;

	printf("test: sleeping\n");
	kpause("test", false, hz * SECONDS, NULL);
	printf("test: woken\n");

	if (!test_exit)
		printf("test: FAIL\n");
	test_exit = true;
	mutex_enter(&test_mutex);
	while (exited != i) {
		cv_wait(&exit_cv, &test_mutex);
	}
	mutex_exit(&test_mutex);

	printf("test: finished\n");

	cv_destroy(&test_cv);
	mutex_destroy(&test_mutex);

	return 0;
}
