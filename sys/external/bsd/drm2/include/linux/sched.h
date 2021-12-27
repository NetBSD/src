/*	$NetBSD: sched.h,v 1.23 2021/12/27 13:28:41 riastradh Exp $	*/

/*-
 * Copyright (c) 2013 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Taylor R. Campbell.
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

#ifndef _LINUX_SCHED_H_
#define _LINUX_SCHED_H_

#include <sys/param.h>

#include <sys/cdefs.h>
#include <sys/kernel.h>
#include <sys/lwp.h>
#include <sys/proc.h>
#include <sys/sched.h>

#include <asm/barrier.h>
#include <asm/param.h>
#include <asm/processor.h>

#include <linux/device.h>
#include <linux/errno.h>

struct seq_file;

#define	TASK_COMM_LEN	MAXCOMLEN

#define	MAX_SCHEDULE_TIMEOUT	(INT_MAX/2)	/* paranoia */

#define	TASK_UNINTERRUPTIBLE	__BIT(0)
#define	TASK_INTERRUPTIBLE	__BIT(1)

#define	current	curproc

static inline pid_t
task_pid_nr(struct proc *p)
{
	return p->p_pid;
}

static inline long
schedule_timeout_uninterruptible(long timeout)
{
	unsigned start, end;

	KASSERT(timeout >= 0);
	KASSERT(timeout < MAX_SCHEDULE_TIMEOUT);

	if (cold) {
		unsigned us;
		if (hz <= 1000) {
			unsigned ms = hztoms(MIN((unsigned long)timeout,
			    mstohz(INT_MAX/2)));
			us = MIN(ms, INT_MAX/1000)*1000;
		} else if (hz <= 1000000) {
			us = MIN(timeout, (INT_MAX/1000000)/hz)*hz*1000000;
		} else {
			us = timeout/(1000000/hz);
		}
		DELAY(us);
		return 0;
	}

	start = getticks();
	/* Caller is expected to loop anyway, so no harm in truncating.  */
	(void)kpause("loonix", /*intr*/false, MIN(timeout, INT_MAX/2), NULL);
	end = getticks();

	return timeout - MIN(timeout, (end - start));
}

static inline bool
need_resched(void)
{

	return preempt_needed();
}

static inline void
cond_resched(void)
{

	preempt_point();
}

static inline bool
signal_pending_state(int state, struct proc *p)
{

	KASSERT(p == current);
	KASSERT(state & (TASK_INTERRUPTIBLE|TASK_UNINTERRUPTIBLE));
	if (state & TASK_UNINTERRUPTIBLE)
		return false;
	return sigispending(curlwp, 0);
}

static inline void
sched_setscheduler(struct proc *p, int class, struct sched_param *param)
{

	KASSERT(p == curproc);
	KASSERT(class == SCHED_FIFO);
	KASSERT(param->sched_priority == 1);

	lwp_lock(curlwp);
	curlwp->l_class = class;
	lwp_changepri(curlwp, PRI_KERNEL_RT);
	lwp_unlock(curlwp);
}

#endif  /* _LINUX_SCHED_H_ */
