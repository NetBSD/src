/*	$NetBSD: sched.h,v 1.7 2018/08/27 06:17:30 riastradh Exp $	*/

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
#include <sys/kernel.h>
#include <sys/proc.h>

#include <asm/param.h>
#include <asm/barrier.h>
#include <linux/errno.h>

#define	TASK_COMM_LEN	MAXCOMLEN

#define	current	curproc

static inline pid_t
task_pid_nr(struct proc *p)
{
	return p->p_pid;
}

static inline long
schedule_timeout_uninterruptible(long timeout)
{
	long remain;
	int start, end;

	if (cold) {
		DELAY(timeout);
		return 0;
	}

	start = hardclock_ticks;
	/* XXX Integer truncation...not likely to matter here.  */
	(void)kpause("loonix", false /*!intr*/, timeout, NULL);
	end = hardclock_ticks;

	remain = timeout - (end - start);
	return remain > 0 ? remain : 0;
}

#endif  /* _LINUX_SCHED_H_ */
