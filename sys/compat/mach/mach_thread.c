/*	$NetBSD: mach_thread.c,v 1.4 2002/12/12 00:29:24 manu Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: mach_thread.c,v 1.4 2002/12/12 00:29:24 manu Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/signal.h>
#include <sys/proc.h>

#include <compat/mach/mach_types.h>
#include <compat/mach/mach_message.h>
#include <compat/mach/mach_clock.h>
#include <compat/mach/mach_thread.h>
#include <compat/mach/mach_errno.h>
#include <compat/mach/mach_syscallargs.h>

int 
mach_thread_policy(p, msgh, maxlen, dst)
	struct proc *p;
	mach_msg_header_t *msgh;
	size_t maxlen;
	mach_msg_header_t *dst;
{
	mach_thread_policy_request_t req;
	mach_thread_policy_reply_t rep;
	int error;

	if ((error = copyin(msgh, &req, sizeof(req))) != 0)
		return error;

	bzero(&rep, sizeof(rep));

	rep.rep_msgh.msgh_bits =
	    MACH_MSGH_REPLY_LOCAL_BITS(MACH_MSG_TYPE_MOVE_SEND_ONCE);
	rep.rep_msgh.msgh_size = sizeof(rep) - sizeof(rep.rep_trailer);
	rep.rep_msgh.msgh_local_port = req.req_msgh.msgh_local_port;
	rep.rep_msgh.msgh_id = req.req_msgh.msgh_id + 100;
	rep.rep_trailer.msgh_trailer_size = 8;

	return MACH_MSG_RETURN(p, &rep, msgh, sizeof(rep), maxlen, dst);
}

int 
mach_thread_create_running(p, msgh, maxlen, dst)
	struct proc *p;
	mach_msg_header_t *msgh;
	size_t maxlen;
	mach_msg_header_t *dst;
{
	mach_thread_create_running_request_t req;
	mach_thread_create_running_reply_t rep;
	struct mach_create_thread_child_args mctc;
	register_t retval;
	struct proc *child;
	int flags;
	int error;

	if ((error = copyin(msgh, &req, sizeof(req))) != 0)
		return error;
	
	/* 
	 * Prepare the data we want to transmit to the child
	 */
	mctc.mctc_proc = &child;
	mctc.mctc_flavor = req.req_flavor;
	mctc.mctc_child_done = 0;
	mctc.mctc_state = req.req_state;

	flags = (FORK_SHAREVM | FORK_SHARECWD | 
	    FORK_SHAREFILES | FORK_SHARESIGS);
	error = fork1(p, flags, SIGCHLD, NULL, 0, 
	    mach_create_thread_child, (void *)&mctc, &retval, &child);
	if (error != 0)
		return MACH_MSG_ERROR(p, msgh, &req, &rep, error, maxlen, dst);
		
	/* 
	 * The child relies on some values in mctc, so we should not
	 * exit until it is finished with it. We loop to avoid
	 * spurious wakeups due to signals.
	 */
	while(mctc.mctc_child_done == 0)
		(void)tsleep(&mctc.mctc_child_done, PZERO, "mach_thread", 0);

	bzero(&rep, sizeof(rep));

	rep.rep_msgh.msgh_bits =
	    MACH_MSGH_REPLY_LOCAL_BITS(MACH_MSG_TYPE_MOVE_SEND_ONCE);
	rep.rep_msgh.msgh_size = sizeof(rep) - sizeof(rep.rep_trailer);
	rep.rep_msgh.msgh_local_port = req.req_msgh.msgh_local_port;
	rep.rep_msgh.msgh_id = req.req_msgh.msgh_id + 100;
	/* XXX do something for rep.rep_child_act */
	rep.rep_trailer.msgh_trailer_size = 8;

	return MACH_MSG_RETURN(p, &rep, msgh, sizeof(rep), maxlen, dst);
}

