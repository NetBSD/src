/*	$NetBSD: mach_thread.c,v 1.17 2003/01/26 19:32:04 manu Exp $ */

/*-
 * Copyright (c) 2002 The NetBSD Foundation, Inc.
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
__KERNEL_RCSID(0, "$NetBSD: mach_thread.c,v 1.17 2003/01/26 19:32:04 manu Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/systm.h>
#include <sys/signal.h>
#include <sys/lock.h>
#include <sys/queue.h>
#include <sys/proc.h>

#include <compat/mach/mach_types.h>
#include <compat/mach/mach_message.h>
#include <compat/mach/mach_exec.h>
#include <compat/mach/mach_clock.h>
#include <compat/mach/mach_port.h>
#include <compat/mach/mach_thread.h>
#include <compat/mach/mach_errno.h>
#include <compat/mach/mach_syscallargs.h>

int
mach_sys_syscall_thread_switch(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct mach_sys_syscall_thread_switch_args /* {
		syscallarg(mach_port_name_t) thread_name;
		syscallarg(int) option;
		syscallarg(mach_msg_timeout_t) option_time;
	} */ *uap = v;
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
mach_thread_policy(args)
	struct mach_trap_args *args;
{
	mach_thread_policy_request_t *req = args->smsg;
	mach_thread_policy_reply_t *rep = args->rmsg;
	size_t *msglen = args->rsize;

	rep->rep_msgh.msgh_bits =
	    MACH_MSGH_REPLY_LOCAL_BITS(MACH_MSG_TYPE_MOVE_SEND_ONCE);
	rep->rep_msgh.msgh_size = sizeof(*rep) - sizeof(rep->rep_trailer);
	rep->rep_msgh.msgh_local_port = req->req_msgh.msgh_local_port;
	rep->rep_msgh.msgh_id = req->req_msgh.msgh_id + 100;
	rep->rep_trailer.msgh_trailer_size = 8;

	*msglen = sizeof(*rep);
	return 0;
}

int 
mach_thread_create_running(args)
	struct mach_trap_args *args;
{
	mach_thread_create_running_request_t *req = args->smsg;
	mach_thread_create_running_reply_t *rep = args->rmsg;
	size_t *msglen = args->rsize;
	struct lwp *l = args->l;
	struct proc *p = l->l_proc;
	struct mach_create_thread_child_args mctc;
	vaddr_t uaddr;
	int flags;
	int error;
	int inmem;
	int s;

	/* 
	 * Prepare the data we want to transmit to the child
	 */
	mctc.mctc_flavor = req->req_flavor;
	mctc.mctc_oldlwp = l;
	mctc.mctc_child_done = 0;
	mctc.mctc_state = req->req_state;

        inmem = uvm_uarea_alloc(&uaddr);
        if (__predict_false(uaddr == 0))
                return (ENOMEM);

	flags = 0;
	if ((error = newlwp(l, p, uaddr, inmem, flags, NULL, 0,
	    mach_create_thread_child, (void *)&mctc, &mctc.mctc_lwp)) != 0)
		return mach_msg_error(args, error);
		
	/*
	 * Make the child runnable
	 */
	SCHED_LOCK(s);
	mctc.mctc_lwp->l_stat = LSRUN;
	setrunqueue(mctc.mctc_lwp);
	SCHED_UNLOCK(s);
	simple_lock(&p->p_lwplock);
	p->p_nrlwps++;
	simple_unlock(&p->p_lwplock);

	/* 
	 * The child relies on some values in mctc, so we should not
	 * exit until it is finished with it. We catch signals so that 
	 * the process can be killed with kill -9, but we loop to avoid
	 * spurious wakeups due to other signals.
	 */
	while(mctc.mctc_child_done == 0)
		(void)tsleep(&mctc.mctc_child_done, 
		    PZERO|PCATCH, "mach_thread", 0);

	rep->rep_msgh.msgh_bits =
	    MACH_MSGH_REPLY_LOCAL_BITS(MACH_MSG_TYPE_MOVE_SEND_ONCE);
	rep->rep_msgh.msgh_size = sizeof(*rep) - sizeof(rep->rep_trailer);
	rep->rep_msgh.msgh_local_port = req->req_msgh.msgh_local_port;
	rep->rep_msgh.msgh_id = req->req_msgh.msgh_id + 100;
	/* XXX do something for rep->rep_child_act */
	rep->rep_trailer.msgh_trailer_size = 8;

	*msglen = sizeof(*rep);
	return 0;
}
