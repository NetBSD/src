/*	$NetBSD: mach_port.c,v 1.15 2002/12/15 00:40:25 manu Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: mach_port.c,v 1.15 2002/12/15 00:40:25 manu Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/signal.h>
#include <sys/pool.h>
#include <sys/queue.h>
#include <sys/malloc.h>
#include <sys/proc.h>

#include <compat/mach/mach_types.h>
#include <compat/mach/mach_message.h>
#include <compat/mach/mach_port.h>
#include <compat/mach/mach_clock.h>
#include <compat/mach/mach_exec.h>
#include <compat/mach/mach_errno.h>
#include <compat/mach/mach_syscallargs.h>

/* Port list, lock, pool */
static LIST_HEAD(mach_port_list, mach_port)  mach_port_list;
static struct lock mach_port_list_lock;
static struct pool mach_port_pool;
/* Rights pool */
static struct pool mach_right_pool;

int
mach_sys_reply_port(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval; 
{
	struct mach_right *mr;

	mr = mach_right_get(mach_port_get(p), p, MACH_PORT_RIGHT_RECEIVE);
	*retval = (register_t)mr;

	return 0;
}

int
mach_sys_thread_self_trap(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct mach_emuldata *med;
	struct mach_right *mr;

	/* 
	 * XXX for now thread kernel port and task kernel port are the same 
	 * awaiting for struct lwp ...
	 */
	med = (struct mach_emuldata *)p->p_emuldata;
	mr = mach_right_get(med->med_kernel, p, MACH_PORT_RIGHT_SEND);
	*retval = (register_t)mr;

	return 0;
}


int
mach_sys_task_self_trap(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct mach_emuldata *med;
	struct mach_right *mr;

	med = (struct mach_emuldata *)p->p_emuldata;
	mr = mach_right_get(med->med_kernel, p, MACH_PORT_RIGHT_SEND);
	*retval = (register_t)mr;

	return 0;
}


int
mach_sys_host_self_trap(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct mach_emuldata *med;
	struct mach_right *mr;

	med = (struct mach_emuldata *)p->p_emuldata;
	mr = mach_right_get(med->med_host, p, MACH_PORT_RIGHT_SEND);
	*retval = (register_t)mr;

	return 0;
}

int 
mach_port_deallocate(p, msgh, maxlen, dst)
	struct proc *p;
	mach_msg_header_t *msgh;
	size_t maxlen;
	mach_msg_header_t *dst;
{
	mach_port_deallocate_request_t req;
	mach_port_deallocate_reply_t rep;
	struct mach_right *mr;
	int error;

	if ((error = copyin(msgh, &req, sizeof(req))) != 0)
		return error;
	
	mr = (struct mach_right *)req.req_name;
	if (mach_right_check(mr, p, MACH_PORT_TYPE_PORT_RIGHTS) != 0) 
		mach_right_put(mr);
	else 
		return MACH_MSG_ERROR(p, msgh, 
		    &req, &rep, EINVAL, maxlen, dst);

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
mach_port_allocate(p, msgh, maxlen, dst)
	struct proc *p;
	mach_msg_header_t *msgh;
	size_t maxlen;
	mach_msg_header_t *dst;
{
	mach_port_allocate_request_t req;
	mach_port_allocate_reply_t rep;
	struct mach_right *mr;
	int error;

	if ((error = copyin(msgh, &req, sizeof(req))) != 0)
		return error;

	switch (req.req_right) {
	case MACH_PORT_RIGHT_RECEIVE:
		mr = mach_right_get(mach_port_get(p), p, req.req_right);
		break;

	case MACH_PORT_RIGHT_DEAD_NAME:
		mr = mach_right_get(mach_port_get(NULL), p, req.req_right);
		break;

	case MACH_PORT_RIGHT_PORT_SET:
		mr = mach_right_get(mach_port_get(NULL), p, req.req_right);
		break;

	default:
		uprintf("mach_port_allocate: unknown right %d\n", 
		    req.req_right);
		break;
	}

	bzero(&rep, sizeof(rep));

	rep.rep_msgh.msgh_bits =
	    MACH_MSGH_REPLY_LOCAL_BITS(MACH_MSG_TYPE_MOVE_SEND_ONCE);
	rep.rep_msgh.msgh_size = sizeof(rep) - sizeof(rep.rep_trailer);
	rep.rep_msgh.msgh_local_port = req.req_msgh.msgh_local_port;
	rep.rep_msgh.msgh_id = req.req_msgh.msgh_id + 100;
	rep.rep_name = (mach_port_name_t)mr;
	rep.rep_trailer.msgh_trailer_size = 8;

	return MACH_MSG_RETURN(p, &rep, msgh, sizeof(rep), maxlen, dst);
}

int 
mach_port_insert_right(p, msgh, maxlen, dst)
	struct proc *p;
	mach_msg_header_t *msgh;
	size_t maxlen;
	mach_msg_header_t *dst;
{
	mach_port_insert_right_request_t req;
	mach_port_insert_right_reply_t rep;
	struct mach_right *mr;
	struct mach_right *tmr;
	struct mach_right *nmr;
	struct proc *tp;
	int error;

	if ((error = copyin(msgh, &req, sizeof(req))) != 0)
		return error;

	mr = (struct mach_right *)req.req_name;
	tmr = (struct mach_right *)req.req_msgh.msgh_local_port;
	nmr = NULL;

	tp = tmr->mr_p; /* The target process */

	switch (req.req_poly.name) {
	case MACH_MSG_TYPE_MAKE_SEND:
	case MACH_MSG_TYPE_MOVE_SEND:
	case MACH_MSG_TYPE_COPY_SEND:
		/* 
		 * XXX We require that requester has send right. 
		 * Not sure this is right
		 */
		if (mach_right_check(mr, p, MACH_PORT_TYPE_SEND) == 0)
			return MACH_MSG_ERROR(p, msgh, &req, &rep, 
			    EPERM, maxlen, dst);
		nmr = mach_right_get(mr->mr_port, tp, MACH_PORT_RIGHT_SEND);
		break;

	case MACH_MSG_TYPE_MAKE_SEND_ONCE:
	case MACH_MSG_TYPE_MOVE_SEND_ONCE:
		/* 
		 * XXX We require that requester has send right. 
		 * Not sure this is right
		 */
		if (mach_right_check(mr, p, MACH_PORT_TYPE_SEND_ONCE) == 0)
			return MACH_MSG_ERROR(p, msgh, &req, &rep, 
			    EPERM, maxlen, dst);
		nmr = mach_right_get(mr->mr_port, 
		    tp, MACH_PORT_RIGHT_SEND_ONCE);
		break;

	case MACH_MSG_TYPE_MOVE_RECEIVE:
		/* 
		 * XXX We require that requester has any right. 
		 * Not sure this is right
		 */
		if (mach_right_check(mr, p, MACH_PORT_TYPE_PORT_RIGHTS) == 0)
			return MACH_MSG_ERROR(p, msgh, &req, &rep, 
			    EPERM, maxlen, dst);
		nmr = mach_right_get(mr->mr_port, tp, MACH_PORT_RIGHT_RECEIVE);
		break;

	default:
		uprintf("mach_port_insert_right: unknown right %d\n",
		    req.req_poly.name);
		break;
	}

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
mach_port_type(p, msgh, maxlen, dst)
	struct proc *p;
	mach_msg_header_t *msgh;
	size_t maxlen;
	mach_msg_header_t *dst;
{
	mach_port_type_request_t req;
	mach_port_type_reply_t rep;
	struct mach_right *mr;
	int error;

	if ((error = copyin(msgh, &req, sizeof(req))) != 0)
		return error;

	mr = (struct mach_right *)req.req_name;
	if (mach_right_check(mr, p, MACH_PORT_TYPE_PORT_RIGHTS) == 0)
		return MACH_MSG_ERROR(p, msgh, &req, &rep, EPERM, maxlen, dst);

	bzero(&rep, sizeof(rep));
	
	rep.rep_msgh.msgh_bits =
	    MACH_MSGH_REPLY_LOCAL_BITS(MACH_MSG_TYPE_MOVE_SEND_ONCE);
	rep.rep_msgh.msgh_size = sizeof(rep) - sizeof(rep.rep_trailer);
	rep.rep_msgh.msgh_local_port = req.req_msgh.msgh_local_port;
	rep.rep_msgh.msgh_id = req.req_msgh.msgh_id + 100;
	rep.rep_retval = 0;
	rep.rep_ptype = mr->mr_type;
	rep.rep_trailer.msgh_trailer_size = 8;

	return MACH_MSG_RETURN(p, &rep, msgh, sizeof(rep), maxlen, dst);
}

int 
mach_port_set_attributes(p, msgh, maxlen, dst)
	struct proc *p;
	mach_msg_header_t *msgh;
	size_t maxlen;
	mach_msg_header_t *dst;
{
	mach_port_set_attributes_request_t req;
	mach_port_set_attributes_reply_t rep;
	int error;

	if ((error = copyin(msgh, &req, sizeof(req))) != 0)
		return error;

	switch(req.req_flavor) {
	case MACH_PORT_LIMITS_INFO:
	case MACH_PORT_RECEIVE_STATUS:
	case MACH_PORT_DNREQUESTS_SIZE:
		break;
	default:
		uprintf("mach_port_get_attributes: unknown flavor %d\n",
		    req.req_flavor);
		break;
	}

	bzero(&rep, sizeof(rep));

	rep.rep_msgh.msgh_bits =
	    MACH_MSGH_REPLY_LOCAL_BITS(MACH_MSG_TYPE_MOVE_SEND_ONCE);
	rep.rep_msgh.msgh_size = sizeof(rep) - sizeof(rep.rep_trailer);
	rep.rep_msgh.msgh_local_port = req.req_msgh.msgh_local_port;
	rep.rep_msgh.msgh_id = req.req_msgh.msgh_id + 100;
	rep.rep_trailer.msgh_trailer_size = 8;

	return MACH_MSG_RETURN(p, &rep, msgh, sizeof(rep), maxlen, dst);
}

/* XXX need to implement port sets before doing that */
int 
mach_port_insert_member(p, msgh, maxlen, dst)
	struct proc *p;
	mach_msg_header_t *msgh;
	size_t maxlen;
	mach_msg_header_t *dst;
{
	mach_port_insert_member_request_t req;
	mach_port_insert_member_reply_t rep;
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

/* XXX need to implement port sets before doing that */
int 
mach_port_move_member(p, msgh, maxlen, dst)
	struct proc *p;
	mach_msg_header_t *msgh;
	size_t maxlen;
	mach_msg_header_t *dst;
{
	mach_port_insert_member_request_t req;
	mach_port_insert_member_reply_t rep;
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

void 
mach_port_init(void) 
{
	LIST_INIT(&mach_port_list);
	lockinit(&mach_port_list_lock, PZERO|PCATCH, "mach_port", 0, 0);
	pool_init(&mach_port_pool, sizeof (struct mach_port),
	    0, 0, 128, "mach_port_pool", NULL);
	pool_init(&mach_right_pool, sizeof (struct mach_right),
	    0, 0, 128, "mach_right_pool", NULL);
	return;
}

struct mach_port *
mach_port_get(p)
	struct proc *p;
{
	struct mach_port *mp;

	mp = (struct mach_port *)pool_get(&mach_port_pool, M_WAITOK);
	mp->mp_recv = p;
	mp->mp_count = 0;
	TAILQ_INIT(&mp->mp_msglist);
	lockinit(&mp->mp_msglock, PZERO|PCATCH, "mach_port", 0, 0);

	lockmgr(&mach_port_list_lock, LK_EXCLUSIVE, NULL);
	LIST_INSERT_HEAD(&mach_port_list, mp, mp_alllist);
	lockmgr(&mach_port_list_lock, LK_RELEASE, NULL);

	return mp;
}

void
mach_port_put(mp)
	struct mach_port *mp;
{
	struct mach_message *mm;

	if (mp->mp_refcount != 0)
		uprintf("mach_port_put: attempt to free a referenced port\n");

	lockmgr(&mp->mp_msglock, LK_EXCLUSIVE, NULL);
	while ((mm = TAILQ_FIRST(&mp->mp_msglist)) != NULL)
		mach_message_put(mm);
	lockmgr(&mp->mp_msglock, LK_RELEASE, NULL);
	lockmgr(&mp->mp_msglock, LK_DRAIN, NULL);
	
	lockmgr(&mach_port_list_lock, LK_EXCLUSIVE, NULL);
	LIST_REMOVE(mp, mp_alllist);
	lockmgr(&mach_port_list_lock, LK_RELEASE, NULL);

	pool_put(&mach_port_pool, mp);

	return;
}

struct mach_right *
mach_right_get(mp, p, type)
	struct mach_port *mp;
	struct proc *p;
	int type;
{
	struct mach_right *mr;
	struct mach_right *omr;
	struct mach_emuldata *med;

	mr = pool_get(&mach_right_pool, M_WAITOK);

	mr->mr_port = mp;
	mr->mr_p = p;
	mr->mr_type = type;

	mr->mr_port->mp_refcount++;

	/* Insert the right in one of the process right lists */
	med = (struct mach_emuldata *)p->p_emuldata;
	switch (type) {
	case MACH_PORT_RIGHT_SEND:
		lockmgr(&med->med_slock, LK_EXCLUSIVE, NULL);
		LIST_INSERT_HEAD(&med->med_send, mr, mr_list);
		lockmgr(&med->med_slock, LK_RELEASE, NULL);
		break;

	case MACH_PORT_RIGHT_SEND_ONCE:
		lockmgr(&med->med_solock, LK_EXCLUSIVE, NULL);
		LIST_INSERT_HEAD(&med->med_sendonce, mr, mr_list);
		lockmgr(&med->med_solock, LK_RELEASE, NULL);
		break;

	case MACH_PORT_RIGHT_RECEIVE:
		lockmgr(&med->med_rlock, LK_EXCLUSIVE, NULL);
		LIST_INSERT_HEAD(&med->med_recv, mr, mr_list);
		lockmgr(&med->med_rlock, LK_RELEASE, NULL);

		/* 
		 * The process that owned the receive right on that 
		 * port should now drop it. 
		 */
		if (mr->mr_port->mp_recv != NULL) {
			med = (struct mach_emuldata *)
			    mr->mr_port->mp_recv->p_emuldata;

			/* XXX lock it shared... */
			LIST_FOREACH(omr, &med->med_recv, mr_list)
				if ((omr->mr_port == mr->mr_port) && 
				    (omr != mr) &&
				    (omr->mr_type == MACH_PORT_RIGHT_RECEIVE))
					mach_right_put(omr);
		}

		/* 
		 * Requester now is the receiver 
		 */
		mr->mr_port->mp_recv = p;
		break;

	default:
		uprintf("mach_right_get: unknown right %d\n", type);
		break;
	}
	return mr;
}

void
mach_right_put(mr)
	struct mach_right *mr;
{
	struct mach_emuldata *med;

	/* Remove the right from one of the process right lists */
	med = (struct mach_emuldata *)mr->mr_p->p_emuldata;
	switch(mr->mr_type) {
	case MACH_PORT_RIGHT_SEND:
		lockmgr(&med->med_slock, LK_EXCLUSIVE, NULL);
		LIST_REMOVE(mr, mr_list);
		lockmgr(&med->med_slock, LK_RELEASE, NULL);
		break;

	case MACH_PORT_RIGHT_SEND_ONCE:
		lockmgr(&med->med_solock, LK_EXCLUSIVE, NULL);
		LIST_REMOVE(mr, mr_list);
		lockmgr(&med->med_solock, LK_RELEASE, NULL);
		break;

	case MACH_PORT_RIGHT_RECEIVE:
		lockmgr(&med->med_rlock, LK_EXCLUSIVE, NULL);
		LIST_REMOVE(mr, mr_list);
		lockmgr(&med->med_rlock, LK_RELEASE, NULL);
		break;

	default:
		uprintf("mach_right_put: unknown right %d\n", mr->mr_type);
		break;
	}

	mr->mr_port->mp_refcount--;
	if (mr->mr_port->mp_refcount <= 0)
		mach_port_put(mr->mr_port);

	pool_put(&mach_right_pool, mr);
	return;
}

/* 
 * Check that a process do have a given right. Beware that type is 
 * MACH_PORT_TYPE_*, and not MACH_PORT_RIGHT_*
 */
int
mach_right_check(mr, p, type)
	struct mach_right *mr;
	struct proc *p;
	int type;
{
	struct mach_right *cmr;
	struct mach_emuldata *med;
	int found;

	med = (struct mach_emuldata *)p->p_emuldata;
	found = 0;

	if (type & MACH_PORT_TYPE_RECEIVE) {
		lockmgr(&med->med_rlock, LK_SHARED, NULL);
		LIST_FOREACH(cmr, &med->med_recv, mr_list)
			if ((mr == cmr) && 
			    (cmr->mr_type == MACH_PORT_RIGHT_RECEIVE))
				found = 1;
		lockmgr(&med->med_rlock, LK_RELEASE, NULL);
	}

	if (type & MACH_PORT_TYPE_SEND) {
		lockmgr(&med->med_slock, LK_SHARED, NULL);
		LIST_FOREACH(cmr, &med->med_send, mr_list)
			if ((mr == cmr) &&
			    (cmr->mr_type == MACH_PORT_RIGHT_SEND))
				found = 1;
		lockmgr(&med->med_slock, LK_RELEASE, NULL);
	}

	if (type & MACH_PORT_TYPE_SEND_ONCE) {
		lockmgr(&med->med_solock, LK_SHARED, NULL);
		LIST_FOREACH(cmr, &med->med_sendonce, mr_list)
			if ((mr == cmr) &&
			    (cmr->mr_type == MACH_PORT_RIGHT_SEND_ONCE))
				found = 1;
		lockmgr(&med->med_solock, LK_RELEASE, NULL);
	}

	return found;
}

#ifdef DEBUG_MACH
void 
mach_debug_port(p)
	struct proc *p;
{
	struct mach_emuldata *med;
	struct mach_right *mr;
	
	printf("mach_debug_port(%p)\n", p);

	med = (struct mach_emuldata *)p->p_emuldata;

	printf("receive rights:\n");
	LIST_FOREACH(mr, &med->med_recv, mr_list) {
		printf("  port = %p\n", mr->mr_port);
		printf("    recv = %p\n", mr->mr_port->mp_recv);
		printf("    count = %d\n", mr->mr_port->mp_count);
		printf("    refcount = %d\n", mr->mr_port->mp_refcount);
		printf("  proc = %p\n", mr->mr_p);
		printf("  type = %d\n", mr->mr_type);
	}

	printf("send rights:\n");
	LIST_FOREACH(mr, &med->med_send, mr_list) {
		printf("  port = %p\n", mr->mr_port);
		printf("    recv = %p\n", mr->mr_port->mp_recv);
		printf("    count = %d\n", mr->mr_port->mp_count);
		printf("    refcount = %d\n", mr->mr_port->mp_refcount);
		printf("  proc = %p\n", mr->mr_p);
		printf("  type = %d\n", mr->mr_type);
	}

	printf("send once rights:\n");
	LIST_FOREACH(mr, &med->med_sendonce, mr_list) {
		printf("  port = %p\n", mr->mr_port);
		printf("    recv = %p\n", mr->mr_port->mp_recv);
		printf("    count = %d\n", mr->mr_port->mp_count);
		printf("    refcount = %d\n", mr->mr_port->mp_refcount);
		printf("  proc = %p\n", mr->mr_p);
		printf("  type = %d\n", mr->mr_type);
	}

	return;
}

#endif
