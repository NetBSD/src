/*	$NetBSD: mach_notify.c,v 1.3 2003/11/17 01:52:14 manu Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: mach_notify.c,v 1.3 2003/11/17 01:52:14 manu Exp $");

#include "opt_ktrace.h"
#include "opt_compat_mach.h" /* For COMPAT_MACH in <sys/ktrace.h> */

#include <sys/types.h>
#include <sys/param.h>
#include <sys/signal.h>
#include <sys/proc.h>
#include <sys/malloc.h>
#ifdef KTRACE
#include <sys/ktrace.h>
#endif

#include <compat/mach/mach_types.h>
#include <compat/mach/mach_exec.h>
#include <compat/mach/mach_thread.h>
#include <compat/mach/mach_notify.h>

static void mach_siginfo_to_exception(const struct ksiginfo *, int *);

void
mach_notify_port_destroyed(l, mr)
	struct lwp *l;
	struct mach_right *mr;
{
	struct mach_port *mp;
	mach_notify_port_destroyed_request_t *req;

	if (mr->mr_notify_destroyed == NULL)
		return;
	mp = mr->mr_notify_destroyed->mr_port;

#ifdef DEBUG_MACH
	if (mp == NULL) {
		printf("Warning: notification right without a port\n");
		return;
	}
#endif

	req = malloc(sizeof(*req), M_EMULDATA, M_WAITOK | M_ZERO);

	req->req_msgh.msgh_bits =
	    MACH_MSGH_REPLY_LOCAL_BITS(MACH_MSG_TYPE_MOVE_SEND_ONCE);
	req->req_msgh.msgh_size = sizeof(*req) - sizeof(req->req_trailer);
	req->req_msgh.msgh_local_port = mr->mr_notify_destroyed->mr_name;
	req->req_msgh.msgh_id = MACH_NOTIFY_DESTROYED_MSGID;
	req->req_body.msgh_descriptor_count = 1;
	req->req_rights.name = mr->mr_name;
	req->req_trailer.msgh_trailer_size = 8;

	(void)mach_message_get((mach_msg_header_t *)req, sizeof(*req), mp, l);
#ifdef DEBUG_MACH_MSG
	printf("pid %d: message queued on port %p (%d) [%p]\n",
	    l->l_proc->p_pid, mp, req->req_msgh.msgh_id, 
	    mp->mp_recv->mr_sethead);
#endif
	wakeup(mp->mp_recv->mr_sethead);

	return;
}

void
mach_notify_port_no_senders(l, mr)
	struct lwp *l;
	struct mach_right *mr;
{
	struct mach_port *mp;
	mach_notify_port_no_senders_request_t *req;

	if ((mr->mr_notify_no_senders == NULL) || 
	    (mr->mr_notify_no_senders->mr_port == NULL))
		return;
	mp = mr->mr_notify_no_senders->mr_port;

#ifdef DEBUG_MACH
	if ((mp == NULL) || (mp->mp_datatype != MACH_MP_NOTIFY_SYNC)) {
		printf("Warning: notification right without a port\n");
		return;
	}
#endif
	if ((int)mp->mp_data >= mr->mr_refcount)
		return;

	req = malloc(sizeof(*req), M_EMULDATA, M_WAITOK | M_ZERO);

	req->req_msgh.msgh_bits =
	    MACH_MSGH_REPLY_LOCAL_BITS(MACH_MSG_TYPE_MOVE_SEND_ONCE);
	req->req_msgh.msgh_size = sizeof(*req) - sizeof(req->req_trailer);
	req->req_msgh.msgh_local_port = mr->mr_notify_no_senders->mr_name;
	req->req_msgh.msgh_id = MACH_NOTIFY_NO_SENDERS_MSGID;
	req->req_mscount = mr->mr_refcount;
	req->req_trailer.msgh_trailer_size = 8;

	(void)mach_message_get((mach_msg_header_t *)req, sizeof(*req), mp, l);
#ifdef DEBUG_MACH_MSG
	printf("pid %d: message queued on port %p (%d) [%p]\n",
	    l->l_proc->p_pid, mp, req->req_msgh.msgh_id, 
	    mp->mp_recv->mr_sethead);
#endif
	wakeup(mp->mp_recv->mr_sethead);

	return;
}

void
mach_notify_port_dead_name(l, mr)
	struct lwp *l;
	struct mach_right *mr;
{
	struct mach_port *mp;
	mach_notify_port_dead_name_request_t *req;

	if ((mr->mr_notify_dead_name == NULL) || 
	    (mr->mr_notify_dead_name->mr_port == NULL))
		return;
	mp = mr->mr_notify_no_senders->mr_port;

#ifdef DEBUG_MACH
	if (mp == NULL) {
		printf("Warning: notification right without a port\n");
		return;
	}
#endif

	req = malloc(sizeof(*req), M_EMULDATA, M_WAITOK | M_ZERO);

	req->req_msgh.msgh_bits =
	    MACH_MSGH_REPLY_LOCAL_BITS(MACH_MSG_TYPE_MOVE_SEND_ONCE);
	req->req_msgh.msgh_size = sizeof(*req) - sizeof(req->req_trailer);
	req->req_msgh.msgh_local_port = mr->mr_notify_dead_name->mr_name;
	req->req_msgh.msgh_id = MACH_NOTIFY_DEAD_NAME_MSGID;
	req->req_name = mr->mr_name;
	req->req_trailer.msgh_trailer_size = 8;

	mr->mr_refcount++;

	mp = mr->mr_notify_dead_name->mr_port;
	(void)mach_message_get((mach_msg_header_t *)req, sizeof(*req), mp, l);
#ifdef DEBUG_MACH_MSG
	printf("pid %d: message queued on port %p (%d) [%p]\n",
	    l->l_proc->p_pid, mp, req->req_msgh.msgh_id, 
	    mp->mp_recv->mr_sethead);
#endif
	wakeup(mp->mp_recv->mr_sethead);

	return;
}


/* 
 * Exception handler 
 * Mach does not use signals, so mach_trapsignal will not try to send
 * any signal. But systems based on Mach (e.g.: Darwin), can use both 
 * Mach exceptions and UNIX signals. In order to allow the Mach layer 
 * to intercept the  exception and inhiubit UNIX signals, we have 
 * mach_trapsignal1 returning an error. If it returns 0, then the 
 * exception was intercepted at the Mach level, and no signal should 
 * be produced. Else, a signal might be sent. darwin_trapinfo calls
 * mach_trapinfo1 and handle signls if it gets a non zero return value.
 */
void
mach_trapsignal(l, ksi)
	struct lwp *l;
	const struct ksiginfo *ksi;
{
	(void)mach_trapsignal1(l, ksi);
	return;
}

int
mach_trapsignal1(l, ksi)
	struct lwp *l;
	const struct ksiginfo *ksi;
{
	struct proc *p = l->l_proc;
	struct mach_emuldata *med = (struct mach_emuldata *)p->p_emuldata;
	struct mach_port *exc_port;
	struct mach_right *task;
	struct mach_right *thread;
	int exc_no;

	/* XXX Thread and task should have different ports */
	task = mach_right_get(med->med_kernel, l, MACH_PORT_TYPE_SEND, 0);
	thread = mach_right_get(med->med_kernel, l, MACH_PORT_TYPE_SEND, 0);

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
		exc_no = MACH_EXC_SOFTWARE;
		break;
	default: /* SIGCHLD, SIGPOLL */
		return EINVAL;
		break;
	}

	if ((exc_port = med->med_exc[exc_no]) == NULL)
		return EINVAL;
	else
		return mach_exception(l, ksi, exc_port, exc_no, task, thread);
}

int
mach_exception(l, ksi, exc_port, exc, task, thread)
	struct lwp *l;
	const struct ksiginfo *ksi;
	struct mach_port *exc_port;
	int exc;
	struct mach_right *task;
	struct mach_right *thread;
{
	int behavior, flavor;
	mach_msg_header_t *msgh;
	size_t msglen;
	struct mach_right *exc_right;

#ifdef DIAGNOSTIC
	if (exc_port->mp_datatype != MACH_MP_EXC_FLAGS)
		printf("mach_exception: unexpected datatype");
#endif
	behavior = (int)exc_port->mp_data >> 16;
	flavor = (int)exc_port->mp_data & 0xffff;
	exc_right = mach_right_get(exc_port, l, MACH_PORT_TYPE_SEND, 0);


	switch (behavior) {
	case MACH_EXCEPTION_DEFAULT: {
		mach_exception_raise_request_t *req;

		req = malloc(sizeof(*req), M_EMULDATA, M_WAITOK | M_ZERO);
		msglen = sizeof(*req);
		msgh = (mach_msg_header_t *)req;

		req->req_msgh.msgh_bits = MACH_MSGH_BITS_COMPLEX |
		    MACH_MSGH_REPLY_LOCAL_BITS(MACH_MSG_TYPE_MOVE_SEND) |
		    MACH_MSGH_REMOTE_BITS(MACH_MSG_TYPE_MOVE_SEND_ONCE);
		req->req_msgh.msgh_size = 
		    sizeof(*req) - sizeof(req->req_trailer);
		req->req_msgh.msgh_local_port = exc_right->mr_name;
		req->req_msgh.msgh_id = MACH_EXC_RAISE_MSGID;
		req->req_body.msgh_descriptor_count = 2;
		req->req_thread.name = thread->mr_name;
		req->req_thread.disposition = 0x11; /* XXX */
		req->req_thread.type = 0; /* XXX */
		req->req_task.name = task->mr_name;
		req->req_task.disposition = 0x11; /* XXX */
		req->req_task.type = 0; /* XXX */
		req->req_exc = exc;
		req->req_codecount = 2;
		mach_siginfo_to_exception(ksi, &req->req_code[0]);
		req->req_trailer.msgh_trailer_size = 8;
		break;
	}
	 
	case MACH_EXCEPTION_STATE: {
		mach_exception_raise_state_request_t *req;
		int dc;

		req = malloc(sizeof(*req), M_EMULDATA, M_WAITOK | M_ZERO);
		msglen = sizeof(*req);
		msgh = (mach_msg_header_t *)req;

		req->req_msgh.msgh_bits = MACH_MSGH_BITS_COMPLEX |
		    MACH_MSGH_REPLY_LOCAL_BITS(MACH_MSG_TYPE_MOVE_SEND) |
		    MACH_MSGH_REMOTE_BITS(MACH_MSG_TYPE_MOVE_SEND_ONCE);
		req->req_msgh.msgh_size = 
		    sizeof(*req) - sizeof(req->req_trailer);
		req->req_msgh.msgh_local_port = exc_right->mr_name;
		req->req_msgh.msgh_id = MACH_EXCEPTION_STATE;
		req->req_exc = exc;
		req->req_codecount = 2;
		mach_siginfo_to_exception(ksi, &req->req_code[0]);
		req->req_flavor = flavor;
		mach_thread_get_state_machdep(l, flavor, req->req_state, &dc);
		/* Trailer */
		req->req_state[(dc / sizeof(req->req_state[0])) + 1] = 8;

		msglen = msglen - 
			 sizeof(req->req_state) +
			 (dc * sizeof(req->req_state[0]));
		break;
	}

	case MACH_EXCEPTION_STATE_IDENTITY: {
		mach_exception_raise_state_identity_request_t *req;
		int dc;

		req = malloc(sizeof(*req), M_EMULDATA, M_WAITOK | M_ZERO);
		msglen = sizeof(*req);
		msgh = (mach_msg_header_t *)req;

		req->req_msgh.msgh_bits = MACH_MSGH_BITS_COMPLEX |
		    MACH_MSGH_REPLY_LOCAL_BITS(MACH_MSG_TYPE_MOVE_SEND) |
		    MACH_MSGH_REMOTE_BITS(MACH_MSG_TYPE_MOVE_SEND_ONCE);
		req->req_msgh.msgh_size = 
		    sizeof(*req) - sizeof(req->req_trailer);
		req->req_msgh.msgh_local_port = exc_right->mr_name;
		req->req_msgh.msgh_id = MACH_EXC_RAISE_STATE_IDENTITY_MSGID;
		req->req_body.msgh_descriptor_count = 2;
		req->req_thread.name = thread->mr_name;
		req->req_thread.disposition = 0x11; /* XXX */
		req->req_thread.type = 0; /* XXX */
		req->req_task.name = task->mr_name;
		req->req_task.disposition = 0x11; /* XXX */
		req->req_task.type = 0; /* XXX */
		req->req_exc = exc;
		req->req_codecount = 2;
		mach_siginfo_to_exception(ksi, &req->req_code[0]);
		req->req_flavor = flavor;
		mach_thread_get_state_machdep(l, flavor, req->req_state, &dc);
		/* Trailer */
		req->req_state[(dc / sizeof(req->req_state[0])) + 1] = 8;

		msglen = msglen - 
			 sizeof(req->req_state) +
			 (dc * sizeof(req->req_state[0]));
		break;
	}

	default:
		printf("unknown exception bevahior %d\n", behavior);
		return EINVAL;
		break;
	}
		
	(void)mach_message_get(msgh, msglen, exc_port, l);
	wakeup(exc_port->mp_recv->mr_sethead);

	return 0;
}

static void
mach_siginfo_to_exception(ksi, code)
	const struct ksiginfo *ksi;
	int *code;
{
	switch (ksi->ksi_signo) {
	case SIGBUS:
		switch (ksi->ksi_code) {
		case BUS_ADRALN:
			code[0] = MACH_BUS_ADRALN;
			code[1] = (long)ksi->ksi_addr;
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
			code[1] = (long)ksi->ksi_addr;
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

	default:
		printf("untranslated siginfo signo %d, code %d\n", 
		    ksi->ksi_signo, ksi->ksi_code);
		break;
	}
	return;
}
