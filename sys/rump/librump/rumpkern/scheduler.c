/*      $NetBSD: scheduler.c,v 1.5 2009/11/04 18:11:11 pooka Exp $	*/

/*
 * Copyright (c) 2009 Antti Kantee.  All Rights Reserved.
 *
 * Development of this software was supported by
 * The Finnish Cultural Foundation.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: scheduler.c,v 1.5 2009/11/04 18:11:11 pooka Exp $");

#include <sys/param.h>
#include <sys/cpu.h>
#include <sys/kmem.h>
#include <sys/mutex.h>
#include <sys/queue.h>
#include <sys/select.h>

#include <rump/rumpuser.h>

#include "rump_private.h"

/* should go for MAXCPUS at some point */
static struct cpu_info rump_cpus[1];
static struct rumpcpu {
	struct cpu_info *rcpu_ci;
	SLIST_ENTRY(rumpcpu) rcpu_entries;
} rcpu_storage[1];
struct cpu_info *rump_cpu = &rump_cpus[0];
int ncpu = 1;

static SLIST_HEAD(,rumpcpu) cpu_freelist = SLIST_HEAD_INITIALIZER(cpu_freelist);
static struct rumpuser_mtx *schedmtx;
static struct rumpuser_cv *schedcv, *lwp0cv;

static bool lwp0busy = false;

struct cpu_info *
cpu_lookup(u_int index)
{

	return &rump_cpus[index];
}

void
rump_scheduler_init()
{
	struct rumpcpu *rcpu;
	struct cpu_info *ci;
	int i;

	rumpuser_mutex_init(&schedmtx);
	rumpuser_cv_init(&schedcv);
	rumpuser_cv_init(&lwp0cv);
	for (i = 0; i < ncpu; i++) {
		rcpu = &rcpu_storage[i];
		ci = &rump_cpus[i];
		rump_cpu_bootstrap(ci);
		ci->ci_schedstate.spc_mutex =
		    mutex_obj_alloc(MUTEX_DEFAULT, IPL_NONE);
		rcpu->rcpu_ci = ci;
		SLIST_INSERT_HEAD(&cpu_freelist, rcpu, rcpu_entries);
	}
}

void
rump_schedule()
{
	struct lwp *l;

	/*
	 * If there is no dedicated lwp, allocate a temp one and
	 * set it to be free'd upon unschedule().  Use lwp0 context
	 * for reserving the necessary resources.
	 */
	l = rumpuser_get_curlwp();
	if (l == NULL) {
		/* busy lwp0 */
		rumpuser_mutex_enter_nowrap(schedmtx);
		while (lwp0busy)
			rumpuser_cv_wait_nowrap(lwp0cv, schedmtx);
		lwp0busy = true;
		rumpuser_mutex_exit(schedmtx);

		/* schedule cpu and use lwp0 */
		rump_schedule_cpu(&lwp0);
		rumpuser_set_curlwp(&lwp0);
		l = rump_lwp_alloc(0, rump_nextlid());

		/* release lwp0 */
		rump_lwp_switch(l);
		rumpuser_mutex_enter_nowrap(schedmtx);
		lwp0busy = false;
		rumpuser_cv_signal(lwp0cv);
		rumpuser_mutex_exit(schedmtx);

		/* mark new lwp as dead-on-exit */
		rump_lwp_release(l);
	} else {
		rump_schedule_cpu(l);
	}
}

void
rump_schedule_cpu(struct lwp *l)
{
	struct rumpcpu *rcpu;

	rumpuser_mutex_enter_nowrap(schedmtx);
	while ((rcpu = SLIST_FIRST(&cpu_freelist)) == NULL)
		rumpuser_cv_wait_nowrap(schedcv, schedmtx);
	SLIST_REMOVE_HEAD(&cpu_freelist, rcpu_entries);
	rumpuser_mutex_exit(schedmtx);
	KASSERT(l->l_cpu == NULL);
	l->l_cpu = rcpu->rcpu_ci;
	l->l_mutex = rcpu->rcpu_ci->ci_schedstate.spc_mutex;
}

void
rump_unschedule()
{
	struct lwp *l;

	l = rumpuser_get_curlwp();
	KASSERT(l->l_mutex == l->l_cpu->ci_schedstate.spc_mutex);
	rump_unschedule_cpu(l);
	l->l_mutex = NULL;
	if (l->l_flag & LW_WEXIT) {
		kmem_free(l, sizeof(*l));
		rumpuser_set_curlwp(NULL);
	}
}

void
rump_unschedule_cpu(struct lwp *l)
{
	struct rumpcpu *rcpu;
	struct cpu_info *ci;

	ci = l->l_cpu;
	l->l_cpu = NULL;
	rcpu = &rcpu_storage[ci-&rump_cpus[0]];
	KASSERT(rcpu->rcpu_ci == ci);

	rumpuser_mutex_enter_nowrap(schedmtx);
	SLIST_INSERT_HEAD(&cpu_freelist, rcpu, rcpu_entries);
	rumpuser_cv_signal(schedcv);
	rumpuser_mutex_exit(schedmtx);
}

/* Give up and retake CPU (perhaps a different one) */
void
yield()
{
	struct lwp *l = curlwp;
	int nlocks;

	KERNEL_UNLOCK_ALL(l, &nlocks);
	rump_unschedule_cpu(l);
	rump_schedule_cpu(l);
	KERNEL_LOCK(nlocks, l);
}

void
preempt()
{

	yield();
}
