/*	$NetBSD: subr_xcall.c,v 1.9.14.1 2009/05/13 17:21:57 jym Exp $	*/

/*-
 * Copyright (c) 2007, 2008 The NetBSD Foundation, Inc.
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

/*
 * Cross call support
 *
 * Background
 *
 *	Sometimes it is necessary to modify hardware state that is tied
 *	directly to individual CPUs (such as a CPU's local timer), and
 *	these updates can not be done remotely by another CPU.  The LWP
 *	requesting the update may be unable to guarantee that it will be
 *	running on the CPU where the update must occur, when the update
 *	occurs.
 *
 *	Additionally, it's sometimes necessary to modify per-CPU software
 *	state from a remote CPU.  Where these update operations are so
 *	rare or the access to the per-CPU data so frequent that the cost
 *	of using locking or atomic operations to provide coherency is
 *	prohibitive, another way must be found.
 *
 *	Cross calls help to solve these types of problem by allowing
 *	any CPU in the system to request that an arbitrary function be
 *	executed on any other CPU.
 *
 * Implementation
 *
 *	A slow mechanism for making 'low priority' cross calls is
 *	provided.  The function to be executed runs on the remote CPU
 *	within a bound kthread.  No queueing is provided, and the
 *	implementation uses global state.  The function being called may
 *	block briefly on locks, but in doing so must be careful to not
 *	interfere with other cross calls in the system.  The function is
 *	called with thread context and not from a soft interrupt, so it
 *	can ensure that it is not interrupting other code running on the
 *	CPU, and so has exclusive access to the CPU.  Since this facility
 *	is heavyweight, it's expected that it will not be used often.
 *
 *	Cross calls must not allocate memory, as the pagedaemon uses
 *	them (and memory allocation may need to wait on the pagedaemon).
 *
 * Future directions
 *
 *	Add a low-overhead mechanism to run cross calls in interrupt
 *	context (XC_HIGHPRI).
 */
 
#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: subr_xcall.c,v 1.9.14.1 2009/05/13 17:21:57 jym Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/xcall.h>
#include <sys/mutex.h>
#include <sys/condvar.h>
#include <sys/evcnt.h>
#include <sys/kthread.h>
#include <sys/cpu.h>

static void	xc_thread(void *);
static uint64_t	xc_lowpri(u_int, xcfunc_t, void *, void *, struct cpu_info *);

static kmutex_t		xc_lock;
static xcfunc_t		xc_func;
static void		*xc_arg1;
static void		*xc_arg2;
static kcondvar_t	xc_busy;
static struct evcnt	xc_unicast_ev;
static struct evcnt	xc_broadcast_ev;
static uint64_t		xc_headp;
static uint64_t		xc_tailp;
static uint64_t		xc_donep;

/*
 * xc_init_cpu:
 *
 *	Initialize the cross-call subsystem.  Called once for each CPU
 *	in the system as they are attached.
 */
void
xc_init_cpu(struct cpu_info *ci)
{
	static bool again;
	int error;

	if (!again) {
		/* Autoconfiguration will prevent re-entry. */
		again = true;
		mutex_init(&xc_lock, MUTEX_DEFAULT, IPL_NONE);
		cv_init(&xc_busy, "xcallbsy");
		evcnt_attach_dynamic(&xc_unicast_ev, EVCNT_TYPE_MISC, NULL,
		   "crosscall", "unicast");
		evcnt_attach_dynamic(&xc_broadcast_ev, EVCNT_TYPE_MISC, NULL,
		   "crosscall", "broadcast");
	}

	cv_init(&ci->ci_data.cpu_xcall, "xcall");
	error = kthread_create(PRI_XCALL, KTHREAD_MPSAFE, ci, xc_thread,
	    NULL, NULL, "xcall/%u", ci->ci_index);
	if (error != 0)
		panic("xc_init_cpu: error %d", error);
}

/*
 * xc_broadcast:
 *
 *	Trigger a call on all CPUs in the system.
 */
uint64_t
xc_broadcast(u_int flags, xcfunc_t func, void *arg1, void *arg2)
{

	if ((flags & XC_HIGHPRI) != 0) {
		panic("xc_broadcast: no high priority crosscalls yet");
	} else {
		return xc_lowpri(flags, func, arg1, arg2, NULL);
	}
}

/*
 * xc_unicast:
 *
 *	Trigger a call on one CPU.
 */
uint64_t
xc_unicast(u_int flags, xcfunc_t func, void *arg1, void *arg2,
	   struct cpu_info *ci)
{

	if ((flags & XC_HIGHPRI) != 0) {
		panic("xc_unicast: no high priority crosscalls yet");
	} else {
		KASSERT(ci != NULL);
		return xc_lowpri(flags, func, arg1, arg2, ci);
	}
}

/*
 * xc_lowpri:
 *
 *	Trigger a low priority call on one or more CPUs.
 */
static uint64_t
xc_lowpri(u_int flags, xcfunc_t func, void *arg1, void *arg2,
	  struct cpu_info *ci)
{
	CPU_INFO_ITERATOR cii;
	uint64_t where;

	mutex_enter(&xc_lock);
	while (xc_headp != xc_tailp)
		cv_wait(&xc_busy, &xc_lock);
	xc_arg1 = arg1;
	xc_arg2 = arg2;
	xc_func = func;
	if (ci == NULL) {
		xc_broadcast_ev.ev_count++;
		for (CPU_INFO_FOREACH(cii, ci)) {
			if ((ci->ci_schedstate.spc_flags & SPCF_RUNNING) == 0)
				continue;
			xc_headp += 1;
			ci->ci_data.cpu_xcall_pending = true;
			cv_signal(&ci->ci_data.cpu_xcall);
		}
	} else {
		xc_unicast_ev.ev_count++;
		xc_headp += 1;
		ci->ci_data.cpu_xcall_pending = true;
		cv_signal(&ci->ci_data.cpu_xcall);
	}
	KASSERT(xc_tailp < xc_headp);
	where = xc_headp;
	mutex_exit(&xc_lock);

	return where;
}

/*
 * xc_wait:
 *
 *	Wait for a cross call to complete.
 */
void
xc_wait(uint64_t where)
{

	if (xc_donep >= where)
		return;

	mutex_enter(&xc_lock);
	while (xc_donep < where)
		cv_wait(&xc_busy, &xc_lock);
	mutex_exit(&xc_lock);
}

/*
 * xc_thread:
 *
 *	One thread per-CPU to dispatch low priority calls.
 */
static void
xc_thread(void *cookie)
{
	void *arg1, *arg2;
	struct cpu_info *ci;
	xcfunc_t func;

	ci = curcpu();

	mutex_enter(&xc_lock);
	for (;;) {
		while (!ci->ci_data.cpu_xcall_pending) {
			if (xc_headp == xc_tailp)
				cv_broadcast(&xc_busy);
			cv_wait(&ci->ci_data.cpu_xcall, &xc_lock);
			KASSERT(ci == curcpu());
		}
		ci->ci_data.cpu_xcall_pending = false;
		func = xc_func;
		arg1 = xc_arg1;
		arg2 = xc_arg2;
		xc_tailp++;
		mutex_exit(&xc_lock);

		(*func)(arg1, arg2);

		mutex_enter(&xc_lock);
		xc_donep++;
	}
	/* NOTREACHED */
}
