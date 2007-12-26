/*	$NetBSD: mach_notify.c,v 1.18.48.1 2007/12/26 19:49:27 ad Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: mach_notify.c,v 1.18.48.1 2007/12/26 19:49:27 ad Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/signal.h>
#include <sys/proc.h>
#include <sys/malloc.h>

#include <compat/mach/mach_types.h>
#include <compat/mach/mach_exec.h>
#include <compat/mach/mach_thread.h>
#include <compat/mach/mach_notify.h>
#include <compat/mach/mach_message.h>
#include <compat/mach/mach_services.h>

void
mach_notify_port_destroyed(struct lwp *l, struct mach_right *mr)
{
	struct mach_port *mp;
	mach_notify_port_destroyed_request_t *req;

	if (mr->mr_notify_destroyed == NULL)
		return;

	mp = mr->mr_notify_destroyed->mr_port;

#ifdef DIAGNOSTIC
	if ((mp == NULL) || (mp->mp_recv == NULL)) {
		printf("mach_notify_port_destroyed: bad port or receiver\n");
		return;
	}
#endif

	MACH_PORT_REF(mp);

	req = malloc(sizeof(*req), M_EMULDATA, M_WAITOK | M_ZERO);

	req->req_msgh.msgh_bits =
	    MACH_MSGH_REPLY_LOCAL_BITS(MACH_MSG_TYPE_MOVE_SEND_ONCE);
	req->req_msgh.msgh_size = sizeof(*req) - sizeof(req->req_trailer);
	req->req_msgh.msgh_local_port = mr->mr_notify_destroyed->mr_name;
	req->req_msgh.msgh_id = MACH_NOTIFY_DESTROYED_MSGID;
	req->req_body.msgh_descriptor_count = 1;
	req->req_rights.name = mr->mr_name;

	mach_set_trailer(req, sizeof(*req));

	(void)mach_message_get((mach_msg_header_t *)req, sizeof(*req), mp, l);
#ifdef DEBUG_MACH_MSG
	printf("pid %d: message queued on port %p (%d) [%p]\n",
	    l->l_proc->p_pid, mp, req->req_msgh.msgh_id,
	    mp->mp_recv->mr_sethead);
#endif
	wakeup(mp->mp_recv->mr_sethead);

	MACH_PORT_UNREF(mp);

	return;
}

void
mach_notify_port_no_senders(struct lwp *l, struct mach_right *mr)
{
	struct mach_port *mp;
	mach_notify_port_no_senders_request_t *req;

	if ((mr->mr_notify_no_senders == NULL) ||
	    (mr->mr_notify_no_senders->mr_port == NULL))
		return;
	mp = mr->mr_notify_no_senders->mr_port;

#ifdef DIAGNOSTIC
	if ((mp == NULL) ||
	    (mp->mp_recv == NULL) ||
	    (mp->mp_datatype != MACH_MP_NOTIFY_SYNC)) {
		printf("mach_notify_port_no_senders: bad port or reciever\n");
		return;
	}
#endif
	MACH_PORT_REF(mp);
	if ((int)mp->mp_data >= mr->mr_refcount)
		goto out;

	req = malloc(sizeof(*req), M_EMULDATA, M_WAITOK | M_ZERO);

	req->req_msgh.msgh_bits =
	    MACH_MSGH_REPLY_LOCAL_BITS(MACH_MSG_TYPE_MOVE_SEND_ONCE);
	req->req_msgh.msgh_size = sizeof(*req) - sizeof(req->req_trailer);
	req->req_msgh.msgh_local_port = mr->mr_notify_no_senders->mr_name;
	req->req_msgh.msgh_id = MACH_NOTIFY_NO_SENDERS_MSGID;
	req->req_mscount = mr->mr_refcount;

	mach_set_trailer(req, sizeof(*req));

	(void)mach_message_get((mach_msg_header_t *)req, sizeof(*req), mp, l);
#ifdef DEBUG_MACH_MSG
	printf("pid %d: message queued on port %p (%d) [%p]\n",
	    l->l_proc->p_pid, mp, req->req_msgh.msgh_id,
	    mp->mp_recv->mr_sethead);
#endif
	wakeup(mp->mp_recv->mr_sethead);

out:
	MACH_PORT_UNREF(mp);
	return;
}

void
mach_notify_port_dead_name(struct lwp *l, struct mach_right *mr)
{
	struct mach_port *mp;
	mach_notify_port_dead_name_request_t *req;

	if ((mr->mr_notify_dead_name == NULL) ||
	    (mr->mr_notify_dead_name->mr_port == NULL))
		return;
	mp = mr->mr_notify_dead_name->mr_port;

#ifdef DIAGNOSTIC
	if ((mp == NULL) || (mp->mp_recv)) {
		printf("mach_notify_port_dead_name: bad port or reciever\n");
		return;
	}
#endif
	MACH_PORT_REF(mp);

	req = malloc(sizeof(*req), M_EMULDATA, M_WAITOK | M_ZERO);

	req->req_msgh.msgh_bits =
	    MACH_MSGH_REPLY_LOCAL_BITS(MACH_MSG_TYPE_MOVE_SEND_ONCE);
	req->req_msgh.msgh_size = sizeof(*req) - sizeof(req->req_trailer);
	req->req_msgh.msgh_local_port = mr->mr_notify_dead_name->mr_name;
	req->req_msgh.msgh_id = MACH_NOTIFY_DEAD_NAME_MSGID;
	req->req_name = mr->mr_name;

	mach_set_trailer(req, sizeof(*req));

	mr->mr_refcount++;

	(void)mach_message_get((mach_msg_header_t *)req, sizeof(*req), mp, l);
#ifdef DEBUG_MACH_MSG
	printf("pid %d: message queued on port %p (%d) [%p]\n",
	    l->l_proc->p_pid, mp, req->req_msgh.msgh_id,
	    mp->mp_recv->mr_sethead);
#endif
	wakeup(mp->mp_recv->mr_sethead);
	MACH_PORT_UNREF(mp);

	return;
}
