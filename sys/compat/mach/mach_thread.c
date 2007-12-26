/*	$NetBSD: mach_thread.c,v 1.42.2.2 2007/12/26 21:39:05 ad Exp $ */

/*-
 * Copyright (c) 2002-2003 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Emmanuel Dreyfus
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: mach_thread.c,v 1.42.2.2 2007/12/26 21:39:05 ad Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/systm.h>
#include <sys/signal.h>
#include <sys/rwlock.h>
#include <sys/queue.h>
#include <sys/proc.h>
#include <sys/resource.h>
#include <sys/resourcevar.h>

#include <compat/mach/mach_types.h>
#include <compat/mach/mach_message.h>
#include <compat/mach/mach_exec.h>
#include <compat/mach/mach_clock.h>
#include <compat/mach/mach_port.h>
#include <compat/mach/mach_task.h>
#include <compat/mach/mach_thread.h>
#include <compat/mach/mach_errno.h>
#include <compat/mach/mach_services.h>
#include <compat/mach/mach_syscallargs.h>

int
mach_sys_syscall_thread_switch(struct lwp *l, const struct mach_sys_syscall_thread_switch_args *uap, register_t *retval)
{
	/* {
		syscallarg(mach_port_name_t) thread_name;
		syscallarg(int) option;
		syscallarg(mach_msg_timeout_t) option_time;
	} */
	int timeout;
	struct mach_emuldata *med;

	med = (struct mach_emuldata *)l->l_proc->p_emuldata;
	timeout = SCARG(uap, option_time) * hz / 1000;

	/*
	 * The day we will be able to find out the struct proc from
	 * the port number, try to use preempt() to call the right thread.
	 * [- but preempt() is for _involuntary_ context switches.]
	 */
	switch(SCARG(uap, option)) {
	case MACH_SWITCH_OPTION_NONE:
		yield();
		break;

	case MACH_SWITCH_OPTION_WAIT:
		med->med_thpri = 1;
		while (med->med_thpri != 0)
			(void)tsleep(&med->med_thpri, PZERO|PCATCH,
			    "thread_switch", timeout);
		break;

	case MACH_SWITCH_OPTION_DEPRESS:
	case MACH_SWITCH_OPTION_IDLE:
		/* Use a callout to restore the priority after depression? */
		med->med_thpri = l->l_priority;
		l->l_priority = MAXPRI;
		break;

	default:
		uprintf("mach_sys_syscall_thread_switch(): unknown option %d\n",		    SCARG(uap, option));
		break;
	}
	return 0;
}

int
mach_sys_swtch_pri(struct lwp *l, const struct mach_sys_swtch_pri_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) pri;
	} */

	/*
	 * Copied from preempt(9). We cannot just call preempt
	 * because we want to return mi_switch(9) return value.
	 */
	KERNEL_UNLOCK_ALL(l, &l->l_biglocks);
	lwp_lock(l);
	if (l->l_stat == LSONPROC)
		l->l_proc->p_stats->p_ru.ru_nivcsw++;	/* XXXSMP */
	*retval = mi_switch(l);
	KERNEL_LOCK(l->l_biglocks, l);

	return 0;
}

int
mach_sys_swtch(struct lwp *l, const void *v, register_t *retval)
{
	struct mach_sys_swtch_pri_args cup;

	SCARG(&cup, pri) = 0;

	return mach_sys_swtch_pri(l, &cup, retval);
}


int
mach_thread_policy(struct mach_trap_args *args)
{
	mach_thread_policy_request_t *req = args->smsg;
	mach_thread_policy_reply_t *rep = args->rmsg;
	size_t *msglen = args->rsize;
	int end_offset;

	/* Sanity check req_count */
	end_offset = req->req_count +
		     (sizeof(req->req_setlimit) / sizeof(req->req_base[0]));
	if (MACH_REQMSG_OVERFLOW(args, req->req_base[end_offset]))
		return mach_msg_error(args, EINVAL);

	uprintf("Unimplemented mach_thread_policy\n");

	*msglen = sizeof(*rep);
	mach_set_header(rep, req, *msglen);

	rep->rep_retval = 0;

	mach_set_trailer(rep, *msglen);

	return 0;
}

/* XXX it might be possible to use this on another task */
int
mach_thread_create_running(struct mach_trap_args *args)
{
	mach_thread_create_running_request_t *req = args->smsg;
	mach_thread_create_running_reply_t *rep = args->rmsg;
	size_t *msglen = args->rsize;
	struct lwp *l = args->l;
	struct proc *p = l->l_proc;
	struct mach_create_thread_child_args mctc;
	struct mach_right *child_mr;
	struct mach_lwp_emuldata *mle;
	vaddr_t uaddr;
	int flags;
	int error;
	int inmem;
	int end_offset;

	/* Sanity check req_count */
	end_offset = req->req_count;
	if (MACH_REQMSG_OVERFLOW(args, req->req_state[end_offset]))
		return mach_msg_error(args, EINVAL);

	/*
	 * Prepare the data we want to transmit to the child.
	 */
	mctc.mctc_flavor = req->req_flavor;
	mctc.mctc_oldlwp = l;
	mctc.mctc_child_done = 0;
	mctc.mctc_state = req->req_state;

        inmem = uvm_uarea_alloc(&uaddr);
        if (__predict_false(uaddr == 0))
                return (ENOMEM);

	flags = 0;
	if ((error = lwp_create(l, p, uaddr, inmem, flags, NULL, 0,
	    mach_create_thread_child, (void *)&mctc, &mctc.mctc_lwp,
	    SCHED_OTHER)) != 0)
	{
		uvm_uarea_free(uaddr, curcpu());
		return mach_msg_error(args, error);
	}

	/*
	 * Make the child runnable.
	 */
	mutex_enter(&p->p_smutex);
	lwp_lock(mctc.mctc_lwp);
	mctc.mctc_lwp->l_private = 0;
	mctc.mctc_lwp->l_stat = LSRUN;
	sched_enqueue(mctc.mctc_lwp, false);
	p->p_nrlwps++;
	lwp_unlock(mctc.mctc_lwp);
	mutex_exit(&p->p_smutex);

	/*
	 * Get the child's kernel port
	 */
	mle = mctc.mctc_lwp->l_emuldata;
	child_mr = mach_right_get(mle->mle_kernel, l, MACH_PORT_TYPE_SEND, 0);

	/*
	 * The child relies on some values in mctc, so we should not
	 * exit until it is finished with it. We catch signals so that
	 * the process can be killed with kill -9, but we loop to avoid
	 * spurious wakeups due to other signals.
	 */
	while(mctc.mctc_child_done == 0)
		(void)tsleep(&mctc.mctc_child_done,
		    PZERO|PCATCH, "mach_thread", 0);

	*msglen = sizeof(*rep);
	mach_set_header(rep, req, *msglen);
	mach_add_port_desc(rep, child_mr->mr_name);
	mach_set_trailer(rep, *msglen);

	return 0;
}

int
mach_thread_info(struct mach_trap_args *args)
{
	mach_thread_info_request_t *req = args->smsg;
	mach_thread_info_reply_t *rep = args->rmsg;
	size_t *msglen = args->rsize;
	struct lwp *l = args->l;
	struct lwp *tl = args->tl;
	struct proc *tp = tl->l_proc;

	/* Sanity check req->req_count */
	if (req->req_count > 12)
		return mach_msg_error(args, EINVAL);

	rep->rep_count = req->req_count;

	*msglen = sizeof(*rep) + ((req->req_count - 12) * sizeof(int));
	mach_set_header(rep, req, *msglen);

	switch (req->req_flavor) {
	case MACH_THREAD_BASIC_INFO: {
		struct mach_thread_basic_info *tbi;

		if (req->req_count != (sizeof(*tbi) / sizeof(int))) /* 10 */
			return mach_msg_error(args, EINVAL);

		tbi = (struct mach_thread_basic_info *)rep->rep_out;
		tbi->user_time.seconds = tp->p_uticks * hz / 1000000;
		tbi->user_time.microseconds =
		    (tp->p_uticks) * hz - tbi->user_time.seconds;
		tbi->system_time.seconds = tp->p_sticks * hz / 1000000;
		tbi->system_time.microseconds =
		    (tp->p_sticks) * hz - tbi->system_time.seconds;
		tbi->cpu_usage = tp->p_pctcpu;
		tbi->policy = MACH_THREAD_STANDARD_POLICY;

		/* XXX this is not very accurate */
		tbi->run_state = MACH_TH_STATE_RUNNING;
		tbi->flags = 0;
		switch (l->l_stat) {
		case LSRUN:
			tbi->run_state = MACH_TH_STATE_RUNNING;
			break;
		case LSSTOP:
			tbi->run_state = MACH_TH_STATE_STOPPED;
			break;
		case LSSLEEP:
			tbi->run_state = MACH_TH_STATE_WAITING;
			break;
		case LSIDL:
			tbi->run_state = MACH_TH_STATE_RUNNING;
			tbi->flags = MACH_TH_FLAGS_IDLE;
			break;
		default:
			break;
		}

		tbi->suspend_count = 0;
		tbi->sleep_time = tl->l_slptime;
		break;
	}

	case MACH_THREAD_SCHED_TIMESHARE_INFO: {
		struct mach_policy_timeshare_info *pti;

		if (req->req_count != (sizeof(*pti) / sizeof(int))) /* 5 */
			return mach_msg_error(args, EINVAL);

		pti = (struct mach_policy_timeshare_info *)rep->rep_out;

		pti->max_priority = tl->l_priority;
		pti->base_priority = tl->l_priority;
		pti->cur_priority = tl->l_priority;
		pti->depressed = 0;
		pti->depress_priority = tl->l_priority;
		break;
	}

	case MACH_THREAD_SCHED_RR_INFO:
	case MACH_THREAD_SCHED_FIFO_INFO:
		uprintf("Unimplemented thread_info flavor %d\n",
		    req->req_flavor);
	default:
		return mach_msg_error(args, EINVAL);
		break;
	}

	mach_set_trailer(rep, *msglen);

	return 0;
}

int
mach_thread_get_state(struct mach_trap_args *args)
{
	mach_thread_get_state_request_t *req = args->smsg;
	mach_thread_get_state_reply_t *rep = args->rmsg;
	size_t *msglen = args->rsize;
	struct lwp *tl = args->tl;
	int error;
	int size;

	/* Sanity check req->req_count */
	if (req->req_count > 144)
		return mach_msg_error(args, EINVAL);

	if ((error = mach_thread_get_state_machdep(tl,
	    req->req_flavor, &rep->rep_state, &size)) != 0)
		return mach_msg_error(args, error);

	rep->rep_count = size / sizeof(int);
	*msglen = sizeof(*rep) + ((req->req_count - 144) * sizeof(int));
	mach_set_header(rep, req, *msglen);
	mach_set_trailer(rep, *msglen);

	return 0;
}

int
mach_thread_set_state(struct mach_trap_args *args)
{
	mach_thread_set_state_request_t *req = args->smsg;
	mach_thread_set_state_reply_t *rep = args->rmsg;
	size_t *msglen = args->rsize;
	struct lwp *tl = args->tl;
	int error;
	int end_offset;

	/* Sanity check req_count */
	end_offset = req->req_count;
	if (MACH_REQMSG_OVERFLOW(args, req->req_state[end_offset]))
		return mach_msg_error(args, EINVAL);

	if ((error = mach_thread_set_state_machdep(tl,
	    req->req_flavor, &req->req_state)) != 0)
		return mach_msg_error(args, error);

	*msglen = sizeof(*rep);
	mach_set_header(rep, req, *msglen);

	rep->rep_retval = 0;

	mach_set_trailer(rep, *msglen);

	return 0;
}

int
mach_thread_suspend(struct mach_trap_args *args)
{
	mach_thread_suspend_request_t *req = args->smsg;
	mach_thread_suspend_reply_t *rep = args->rmsg;
	size_t *msglen = args->rsize;
	struct lwp *l = args->l;
	struct lwp *tl = args->tl;
	struct proc *p = tl->l_proc;
	int error;

	mutex_enter(&p->p_mutex);
	lwp_lock(tl);
	error = lwp_suspend(l, tl);
	mutex_exit(&p->p_mutex);

	*msglen = sizeof(*rep);
	mach_set_header(rep, req, *msglen);
	rep->rep_retval = native_to_mach_errno[error];
	mach_set_trailer(rep, *msglen);

	return 0;
}

int
mach_thread_resume(struct mach_trap_args *args)
{
	mach_thread_resume_request_t *req = args->smsg;
	mach_thread_resume_reply_t *rep = args->rmsg;
	size_t *msglen = args->rsize;
	struct lwp *tl = args->tl;
	struct proc *p = tl->l_proc;

	mutex_enter(&p->p_mutex);
	lwp_lock(tl);
	lwp_continue(tl);
	mutex_exit(&p->p_mutex);

	*msglen = sizeof(*rep);
	mach_set_header(rep, req, *msglen);
	rep->rep_retval = 0;
	mach_set_trailer(rep, *msglen);

	return 0;
}

int
mach_thread_abort(struct mach_trap_args *args)
{
	mach_thread_abort_request_t *req = args->smsg;
	mach_thread_abort_reply_t *rep = args->rmsg;
	size_t *msglen = args->rsize;
	struct lwp *tl = args->tl;

	lwp_exit(tl);

	*msglen = sizeof(*rep);
	mach_set_header(rep, req, *msglen);
	rep->rep_retval = 0;
	mach_set_trailer(rep, *msglen);

	return 0;
}

int
mach_thread_set_policy(struct mach_trap_args *args)
{
	mach_thread_set_policy_request_t *req = args->smsg;
	mach_thread_set_policy_reply_t *rep = args->rmsg;
	size_t *msglen = args->rsize;
	struct lwp *tl = args->tl;
	mach_port_t mn;
	struct mach_right *mr;
	int limit_count_offset, limit_offset;
	int limit_count;
	int *limit;

	limit_count_offset = req->req_base_count;
	if (MACH_REQMSG_OVERFLOW(args, req->req_base[limit_count_offset]))
		return mach_msg_error(args, EINVAL);

	limit_count = req->req_base[limit_count_offset];
	limit_offset = limit_count_offset +
	    (sizeof(req->req_limit_count) / sizeof(req->req_base[0]));
	limit = &req->req_base[limit_offset];
	if (MACH_REQMSG_OVERFLOW(args, limit[limit_count]))
		return mach_msg_error(args, EINVAL);

	mn = req->req_pset.name;
	if ((mr = mach_right_check(mn, tl, MACH_PORT_TYPE_ALL_RIGHTS)) == NULL)
		return mach_msg_error(args, EINVAL);

	*msglen = sizeof(*rep);
	mach_set_header(rep, req, *msglen);
	rep->rep_retval = 0;
	mach_set_trailer(rep, *msglen);

	return 0;
}

