/*	$NetBSD: mach_exception.c,v 1.9.8.1 2007/07/11 20:04:25 mjf Exp $ */

/*-
 * Copyright (c) 2003 The NetBSD Foundation, Inc.
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
__KERNEL_RCSID(0, "$NetBSD: mach_exception.c,v 1.9.8.1 2007/07/11 20:04:25 mjf Exp $");

#include "opt_compat_darwin.h"

#include <sys/types.h>
#include <sys/param.h>
#include <sys/signal.h>
#include <sys/proc.h>
#include <sys/malloc.h>

#ifdef COMPAT_DARWIN
#include <compat/darwin/darwin_exec.h>
#endif

#include <compat/mach/mach_types.h>
#include <compat/mach/mach_exec.h>
#include <compat/mach/mach_errno.h>
#include <compat/mach/mach_thread.h>
#include <compat/mach/mach_exception.h>
#include <compat/mach/mach_message.h>
#include <compat/mach/mach_services.h>
#include <compat/mach/mach_sysctl.h>

#include <machine/mach_machdep.h>

static void mach_siginfo_to_exception(const struct ksiginfo *, int *);

/*
 * Exception handler
 * Mach does not use signals. But systems based on Mach (e.g.: Darwin),
 * can use both Mach exceptions and UNIX signals. In order to allow the
 * Mach layer to intercept the exception and inhibit UNIX signals, we have
 * mach_trapsignal1 returning an error. If it returns 0, then the
 * exception was intercepted at the Mach level, and no signal should
 * be produced. Else, a signal might be sent. darwin_trapinfo calls
 * mach_trapinfo1 and handle signals if it gets a non zero return value.
 */
void
mach_trapsignal(struct lwp *l, struct ksiginfo *ksi)
{
	if (mach_trapsignal1(l, ksi) != 0)
		trapsignal(l, ksi);
	return;
}

int
mach_trapsignal1(struct lwp *l, struct ksiginfo *ksi)
{
	struct proc *p = l->l_proc;
	struct mach_emuldata *med;
	int exc_no;
	int code[2];

	/* Don't inhinbit non maskable signals */
	if (sigprop[ksi->ksi_signo] & SA_CANTMASK)
		return EINVAL;

	med = (struct mach_emuldata *)p->p_emuldata;

	switch (ksi->ksi_signo) {
	case SIGILL:
		exc_no = MACH_EXC_BAD_INSTRUCTION;
		break;
	case SIGFPE:
		exc_no = MACH_EXC_ARITHMETIC;
		break;
	case SIGSEGV:
	case SIGBUS:
		exc_no = MACH_EXC_BAD_ACCESS;
		break;
	case SIGTRAP:
		exc_no = MACH_EXC_BREAKPOINT;
		break;
	default: /* SIGCHLD, SIGPOLL */
		return EINVAL;
		break;
	}

	mach_siginfo_to_exception(ksi, code);

	return mach_exception(l, exc_no, code);
}

int
mach_exception(exc_l, exc, code)
	struct lwp *exc_l;	/* currently running lwp */
	int exc;
	int *code;
{
	int behavior, flavor;
	mach_msg_header_t *msgh;
	size_t msglen;
	struct mach_right *exc_mr;
	struct mach_emuldata *exc_med;
	struct mach_lwp_emuldata *exc_mle;
	struct mach_emuldata *catcher_med;
	struct mach_right *kernel_mr;
	struct lwp *catcher_l;	/* The lwp catching the exception */
	struct mach_right *exc_task;
	struct mach_right *exc_thread;
	struct mach_port *exc_port;
	struct mach_exc_info *mei;
	int error = 0;

#ifdef DIAGNOSTIC
	if (exc_l == NULL) {
		printf("mach_exception: exc_l = %p\n", exc_l);
		return ESRCH;
	}
#endif
#ifdef DEBUG_MACH
	printf("mach_exception: %d.%d, exc %d, code (%d, %d)\n",
	    exc_l->l_proc->p_pid, exc_l->l_lid, exc, code[0], code[1]);
#endif

	/*
	 * It's extremely useful to have the ability of catching
	 * the process at the time it dies.
	 */
	if (mach_exception_hang) {
		struct proc *p = exc_l->l_proc;

		sigminusset(&contsigmask, &exc_l->l_sigpendset->sp_set);
		lwp_lock(exc_l);
		p->p_pptr->p_nstopchild++;
		p->p_stat = SSTOP;
		exc_l->l_stat = LSSTOP;
		p->p_nrlwps--;
		KERNEL_UNLOCK_ALL(exc_l, &exc_l->l_biglocks);
		mi_switch(exc_l);
		KERNEL_LOCK(exc_l->l_biglocks, exc_l);
	}

	/*
	 * No exception if there is no exception port or if it has no receiver
	 */
	exc_mle = exc_l->l_emuldata;
	exc_med = exc_l->l_proc->p_emuldata;
	if ((exc_port = exc_med->med_exc[exc]) == NULL)
		return EINVAL;

	MACH_PORT_REF(exc_port);
	if (exc_port->mp_recv == NULL) {
		error = EINVAL;
		goto out;
	}

#ifdef DEBUG_MACH
	printf("catcher is %d.%d, state %d\n",
	    exc_port->mp_recv->mr_lwp->l_proc->p_pid,
	    exc_port->mp_recv->mr_lwp->l_lid,
	    exc_port->mp_recv->mr_lwp->l_proc->p_stat);
#endif
	/*
	 * Don't send exceptions to dying processes
	 */
	if (P_ZOMBIE(exc_port->mp_recv->mr_lwp->l_proc)) {
		error = ESRCH;
		goto out;
	}

	/*
	 * XXX Avoid a nasty deadlock because process in TX state
	 * (traced and suspended) are invulnerable to kill -9.
	 *
	 * The scenario:
	 * - the parent gets Child's signals though Mach exceptions
	 * - the parent is killed. Before calling the emulation hook
	 *   mach_exit(), it will wait for the child
	 * - the child receives SIGHUP, which is turned into a Mach
	 *   exception. The child sleeps awaiting for the parent
	 *   to tell it to continue.
	 *   For some reason I do not understand, it goes in the
	 *   suspended state instead of the sleeping state.
	 * - Parents waits for the child, child is suspended, we
	 *   are stuck.
	 *
	 * By preventing exception to traced processes with
	 * a dying parent, a signal is sent instead of the
	 * notification, this fixes the problem.
	 */
	if ((exc_l->l_proc->p_slflag & PSL_TRACED) &&
	    (exc_l->l_proc->p_pptr->p_sflag & PS_WEXIT)) {
#ifdef DEBUG_MACH
		printf("mach_exception: deadlock avoided\n");
#endif
		error = EINVAL;
		goto out;
	}

	if (exc_port->mp_datatype != MACH_MP_EXC_INFO) {
#ifdef DIAGNOSTIC
		printf("mach_exception: unexpected datatype");
#endif
		error = EINVAL;
		goto out;
	}
	mei = exc_port->mp_data;
	behavior = mei->mei_behavior;
	flavor = mei->mei_flavor;

	/*
	 * We want the port names in the target process, that is,
	 * the process with receive right for exc_port.
	 */
	catcher_l = exc_port->mp_recv->mr_lwp;
	catcher_med = catcher_l->l_proc->p_emuldata;
	exc_mr = mach_right_get(exc_port, catcher_l, MACH_PORT_TYPE_SEND, 0);
	kernel_mr = mach_right_get(catcher_med->med_kernel,
	    catcher_l, MACH_PORT_TYPE_SEND, 0);

	exc_task = mach_right_get(exc_med->med_kernel,
	    catcher_l, MACH_PORT_TYPE_SEND, 0);
	exc_thread = mach_right_get(exc_mle->mle_kernel,
	    catcher_l, MACH_PORT_TYPE_SEND, 0);

	switch (behavior) {
	case MACH_EXCEPTION_DEFAULT: {
		mach_exception_raise_request_t *req;

		req = malloc(sizeof(*req), M_EMULDATA, M_WAITOK | M_ZERO);
		msglen = sizeof(*req);
		msgh = (mach_msg_header_t *)req;

		req->req_msgh.msgh_bits =
		    MACH_MSGH_REPLY_LOCAL_BITS(MACH_MSG_TYPE_MOVE_SEND) |
		    MACH_MSGH_REMOTE_BITS(MACH_MSG_TYPE_MOVE_SEND_ONCE);
		req->req_msgh.msgh_size =
		    sizeof(*req) - sizeof(req->req_trailer);
		req->req_msgh.msgh_remote_port = kernel_mr->mr_name;
		req->req_msgh.msgh_local_port = exc_mr->mr_name;
		req->req_msgh.msgh_id = MACH_EXC_RAISE_MSGID;

		mach_add_port_desc(req, exc_thread->mr_name);
		mach_add_port_desc(req, exc_task->mr_name);

		req->req_exc = exc;
		req->req_codecount = 2;
		memcpy(&req->req_code[0], code, sizeof(req->req_code));

		mach_set_trailer(req, msglen);

		break;
	}

	case MACH_EXCEPTION_STATE: {
		mach_exception_raise_state_request_t *req;
		int dc;

		req = malloc(sizeof(*req), M_EMULDATA, M_WAITOK | M_ZERO);
		msglen = sizeof(*req);
		msgh = (mach_msg_header_t *)req;

		req->req_msgh.msgh_bits =
		    MACH_MSGH_REPLY_LOCAL_BITS(MACH_MSG_TYPE_MOVE_SEND) |
		    MACH_MSGH_REMOTE_BITS(MACH_MSG_TYPE_MOVE_SEND_ONCE);
		req->req_msgh.msgh_size =
		    sizeof(*req) - sizeof(req->req_trailer);
		req->req_msgh.msgh_remote_port = kernel_mr->mr_name;
		req->req_msgh.msgh_local_port = exc_mr->mr_name;
		req->req_msgh.msgh_id = MACH_EXCEPTION_STATE;
		req->req_exc = exc;
		req->req_codecount = 2;
		memcpy(&req->req_code[0], code, sizeof(req->req_code));
		req->req_flavor = flavor;
		mach_thread_get_state_machdep(exc_l,
		    flavor, req->req_state, &dc);

		msglen = msglen -
			 sizeof(req->req_state) +
			 (dc * sizeof(req->req_state[0]));
		mach_set_trailer(req, msglen);

		break;
	}

	case MACH_EXCEPTION_STATE_IDENTITY: {
		mach_exception_raise_state_identity_request_t *req;
		int dc;

		req = malloc(sizeof(*req), M_EMULDATA, M_WAITOK | M_ZERO);
		msglen = sizeof(*req);
		msgh = (mach_msg_header_t *)req;

		req->req_msgh.msgh_bits =
		    MACH_MSGH_REPLY_LOCAL_BITS(MACH_MSG_TYPE_MOVE_SEND) |
		    MACH_MSGH_REMOTE_BITS(MACH_MSG_TYPE_MOVE_SEND_ONCE);
		req->req_msgh.msgh_size =
		    sizeof(*req) - sizeof(req->req_trailer);
		req->req_msgh.msgh_remote_port = kernel_mr->mr_name;
		req->req_msgh.msgh_local_port = exc_mr->mr_name;
		req->req_msgh.msgh_id = MACH_EXC_RAISE_STATE_IDENTITY_MSGID;
		req->req_body.msgh_descriptor_count = 2;

		mach_add_port_desc(req, exc_thread->mr_name);
		mach_add_port_desc(req, exc_task->mr_name);

		req->req_exc = exc;
		req->req_codecount = 2;
		memcpy(&req->req_code[0], code, sizeof(req->req_code));
		req->req_flavor = flavor;
		mach_thread_get_state_machdep(exc_l,
		    flavor, req->req_state, &dc);

		msglen = msglen -
			 sizeof(req->req_state) +
			 (dc * sizeof(req->req_state[0]));

		mach_set_trailer(req, msglen);

		break;
	}

	default:
		printf("unknown exception bevahior %d\n", behavior);
		error = EINVAL;
		goto out;
		break;
	}

	mach_set_trailer(msgh, msglen);

	/*
	 * Once an exception is sent on the exception port,
	 * no new exception will be taken until the catcher
	 * acknowledge the first one.
	 */
	lockmgr(&catcher_med->med_exclock, LK_EXCLUSIVE, NULL);

	/*
	 * If the catcher died, we are done.
	 */
	if (((exc_port = exc_med->med_exc[exc]) == NULL) ||
	    (exc_port->mp_recv == NULL) ||
	    (P_ZOMBIE(exc_port->mp_recv->mr_lwp->l_proc))) {
		error = ESRCH;
		goto out;
	}

	(void)mach_message_get(msgh, msglen, exc_port, NULL);
	wakeup(exc_port->mp_recv->mr_sethead);

	/*
	 * The thread that caused the exception is now
	 * supposed to wait for a reply to its message.
	 */
#ifdef DEBUG_MACH
	printf("mach_exception: %d.%d sleep on catcher_med->med_exclock = %p\n",
	    exc_l->l_proc->p_pid, exc_l->l_lid, &catcher_med->med_exclock);
#endif
	error = tsleep(&catcher_med->med_exclock, PZERO, "mach_exc", 0);
#ifdef DEBUG_MACH
	printf("mach_exception: %d.%d resumed, error = %d\n",
	    exc_l->l_proc->p_pid, exc_l->l_lid, error);
#endif

	/*
	 * Unlock the catcher's exception handler
	 */
	lockmgr(&catcher_med->med_exclock, LK_RELEASE, NULL);

out:
	MACH_PORT_UNREF(exc_port);
	return error;
}

static void
mach_siginfo_to_exception(ksi, code)
	const struct ksiginfo *ksi;
	int *code;
{
	code[1] = (long)ksi->ksi_addr;
	switch (ksi->ksi_signo) {
	case SIGBUS:
		switch (ksi->ksi_code) {
		case BUS_ADRALN:
			code[0] = MACH_BUS_ADRALN;
			break;
		default:
			printf("untranslated siginfo signo %d, code %d\n",
			    ksi->ksi_signo, ksi->ksi_code);
			break;
		}
		break;

	case SIGSEGV:
		switch (ksi->ksi_code) {
		case SEGV_MAPERR:
			code[0] = MACH_SEGV_MAPERR;
			break;
		case SEGV_ACCERR:
			code[0] = MACH_SEGV_ACCERR;
			break;
		default:
			printf("untranslated siginfo signo %d, code %d\n",
			    ksi->ksi_signo, ksi->ksi_code);
			break;
		}
		break;

	case SIGTRAP:
		switch (ksi->ksi_code) {
		case TRAP_BRKPT:
			code[0] = MACH_TRAP_BRKPT;
			code[1] = (long)ksi->ksi_addr;
			break;
		default:
			printf("untranslated siginfo signo %d, code %d\n",
			    ksi->ksi_signo, ksi->ksi_code);
			break;
		}
		break;

	case SIGILL:
		switch (ksi->ksi_code) {
		case ILL_ILLOPC:
		case ILL_ILLOPN:
		case ILL_ILLADR:
			code[0] = MACH_ILL_ILLOPC;
			break;
		case ILL_PRVOPC:
		case ILL_PRVREG:
			code[0] = MACH_ILL_PRVOPC;
			break;
		case ILL_ILLTRP:
			code[0] = MACH_ILL_ILLTRP;
			break;
		default:
			printf("untranslated siginfo signo %d, code %d\n",
			    ksi->ksi_signo, ksi->ksi_code);
			break;
		}
		break;

	default:
		printf("untranslated siginfo signo %d, code %d\n",
		    ksi->ksi_signo, ksi->ksi_code);
		break;
	}
}

int
mach_exception_raise(args)
	struct mach_trap_args *args;
{
	struct lwp *l = args->l;
	mach_exception_raise_reply_t *rep;
	struct mach_emuldata *med;

	/*
	 * No typo here: the reply is in the sent message.
	 * The kernel is acting as a client that gets the
	 * reply message to its exception message.
	 */
	rep = args->smsg;

	/*
	 * This message is sent by the process catching the
	 * exception to release the process that raised the exception.
	 * We wake it up if the return value is 0 (no error), else
	 * we should ignore this message.
	 */
#ifdef DEBUG_MACH
	printf("mach_excpetion_raise: retval = %ld\n", (long)rep->rep_retval);
#endif
	if (rep->rep_retval != 0)
		return 0;

	med = l->l_proc->p_emuldata;

	/*
	 * Check for unexpected exception acknowledge, whereas
	 * the kernel sent no exception message.
	 */
	if (lockstatus(&med->med_exclock) == 0) {
#ifdef DEBUG_MACH
		printf("spurious mach_exception_raise\n");
#endif
		return mach_msg_error(args, EINVAL);
	}

	/*
	 * Wakeup the thread that raised the exception.
	 */
#ifdef DEBUG_MACH
	printf("mach_exception_raise: wakeup at %p\n", &med->med_exclock);
#endif
	wakeup(&med->med_exclock);

	return 0;
}

int
mach_exception_raise_state(args)
	struct mach_trap_args *args;
{
	return mach_exception_raise(args);
}

int
mach_exception_raise_state_identity(args)
	struct mach_trap_args *args;
{
	return mach_exception_raise(args);
}
