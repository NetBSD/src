/*	$NetBSD: mach_port.c,v 1.33 2003/02/02 19:07:18 manu Exp $ */

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

#include "opt_compat_darwin.h"

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: mach_port.c,v 1.33 2003/02/02 19:07:18 manu Exp $");

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

#ifdef COMPAT_DARWIN
#include <compat/darwin/darwin_exec.h>
#endif

/* Right and port pools, list of all rights and its lock */
static struct pool mach_port_pool;
static struct pool mach_right_pool;

struct mach_port *mach_bootstrap_port;
struct mach_port *mach_clock_port;
struct mach_port *mach_io_master_port;
struct mach_port *mach_saved_bootstrap_port;

int
mach_sys_reply_port(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval; 
{
	struct mach_right *mr;

	mr = mach_right_get(mach_port_get(), l, MACH_PORT_TYPE_RECEIVE, 0);
	*retval = (register_t)mr->mr_name;

	return 0;
}

int
mach_sys_thread_self_trap(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct mach_emuldata *med;
	struct mach_right *mr;

	/* 
	 * XXX for now thread kernel port and task kernel port are the same 
	 * awaiting for struct lwp ...
	 */
	med = (struct mach_emuldata *)l->l_proc->p_emuldata;
	mr = mach_right_get(med->med_kernel, l, MACH_PORT_TYPE_SEND, 0);
	*retval = (register_t)mr->mr_name;

	return 0;
}


int
mach_sys_task_self_trap(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct mach_emuldata *med;
	struct mach_right *mr;

	med = (struct mach_emuldata *)l->l_proc->p_emuldata;
	mr = mach_right_get(med->med_kernel, l, MACH_PORT_TYPE_SEND, 0);
	*retval = (register_t)mr->mr_name;

	return 0;
}


int
mach_sys_host_self_trap(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct mach_emuldata *med;
	struct mach_right *mr;

	med = (struct mach_emuldata *)l->l_proc->p_emuldata;
	mr = mach_right_get(med->med_host, l, MACH_PORT_TYPE_SEND, 0);
	*retval = (register_t)mr->mr_name;

	return 0;
}

int 
mach_port_deallocate(args)
	struct mach_trap_args *args;
{
	mach_port_deallocate_request_t *req = args->smsg;
	mach_port_deallocate_reply_t *rep = args->rmsg;
	size_t *msglen = args->rsize;
	struct lwp *l = args->l;
	mach_port_t mn;
	struct mach_right *mr;

	mn = req->req_name;
	if ((mr = mach_right_check(mn, l, MACH_PORT_TYPE_REF_RIGHTS)) != NULL) 
		mach_right_put(mr, MACH_PORT_TYPE_REF_RIGHTS);

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
mach_port_destroy(args)
	struct mach_trap_args *args;
{
	mach_port_destroy_request_t *req = args->smsg;
	mach_port_destroy_reply_t *rep = args->rmsg;
	size_t *msglen = args->rsize;
	struct lwp *l = args->l;
	mach_port_t mn;
	struct mach_right *mr;

	mn = req->req_name;
	if ((mr = mach_right_check(mn, l, MACH_PORT_TYPE_REF_RIGHTS)) != NULL) 
		mach_right_put(mr, MACH_PORT_TYPE_REF_RIGHTS);

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
mach_port_allocate(args)
	struct mach_trap_args *args;
{
	mach_port_allocate_request_t *req = args->smsg;
	mach_port_allocate_reply_t *rep = args->rmsg;
	size_t *msglen = args->rsize;
	struct lwp *l = args->l;
	struct mach_right *mr;
	struct mach_port *mp;

	switch (req->req_right) {
	case MACH_PORT_RIGHT_RECEIVE:
		mp = mach_port_get();
		mr = mach_right_get(mp, l, MACH_PORT_TYPE_RECEIVE, 0);
		break;

	case MACH_PORT_RIGHT_DEAD_NAME:
		mr = mach_right_get(NULL, l, MACH_PORT_TYPE_DEAD_NAME, 0);
		break;

	case MACH_PORT_RIGHT_PORT_SET:
		mr = mach_right_get(NULL, l, MACH_PORT_TYPE_PORT_SET, 0);
		break;

	default:
		uprintf("mach_port_allocate: unknown right %x\n", 
		    req->req_right);
		break;
	}

	rep->rep_msgh.msgh_bits =
	    MACH_MSGH_REPLY_LOCAL_BITS(MACH_MSG_TYPE_MOVE_SEND_ONCE);
	rep->rep_msgh.msgh_size = sizeof(*rep) - sizeof(rep->rep_trailer);
	rep->rep_msgh.msgh_local_port = req->req_msgh.msgh_local_port;
	rep->rep_msgh.msgh_id = req->req_msgh.msgh_id + 100;
	rep->rep_name = (mach_port_name_t)mr->mr_name;
	rep->rep_trailer.msgh_trailer_size = 8;

	*msglen = sizeof(*rep);
	return 0;
}

int 
mach_port_insert_right(args)
	struct mach_trap_args *args;
{
	mach_port_insert_right_request_t *req = args->smsg;
	mach_port_insert_right_reply_t *rep = args->rmsg;
	size_t *msglen = args->rsize;
	struct lwp *l = args->l;
	mach_port_t name;
	mach_port_t right;
	struct mach_right *mr;
	struct mach_right *nmr;

	name = req->req_name;
	right = req->req_poly.name;
	nmr = NULL;

	mr = mach_right_check(right, l, MACH_PORT_TYPE_ALL_RIGHTS);
	if (mr == NULL)
		return mach_msg_error(args, EPERM);
		
	switch (req->req_poly.disposition) {
	case MACH_MSG_TYPE_MAKE_SEND:
	case MACH_MSG_TYPE_MOVE_SEND:
	case MACH_MSG_TYPE_COPY_SEND:
		nmr = mach_right_get(mr->mr_port, 
		    l, MACH_PORT_TYPE_SEND, name);
		break;

	case MACH_MSG_TYPE_MAKE_SEND_ONCE:
	case MACH_MSG_TYPE_MOVE_SEND_ONCE:
		nmr = mach_right_get(mr->mr_port, 
		    l, MACH_PORT_TYPE_SEND_ONCE, name);
		break;

	case MACH_MSG_TYPE_MOVE_RECEIVE:
		nmr = mach_right_get(mr->mr_port, 
		    l, MACH_PORT_TYPE_RECEIVE, name);
		break;

	default:
		uprintf("mach_port_insert_right: unknown right %x\n",
		    req->req_poly.disposition);
		break;
	}

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
mach_port_type(args)
	struct mach_trap_args *args;
{
	mach_port_type_request_t *req = args->smsg;
	mach_port_type_reply_t *rep = args->rmsg;
	size_t *msglen = args->rsize;
	struct lwp *l = args->l;
	mach_port_t mn;
	struct mach_right *mr;

	mn = req->req_name;
	if ((mr = mach_right_check(mn, l, MACH_PORT_TYPE_ALL_RIGHTS)) == NULL)
		return mach_msg_error(args, EPERM);

	rep->rep_msgh.msgh_bits =
	    MACH_MSGH_REPLY_LOCAL_BITS(MACH_MSG_TYPE_MOVE_SEND_ONCE);
	rep->rep_msgh.msgh_size = sizeof(*rep) - sizeof(rep->rep_trailer);
	rep->rep_msgh.msgh_local_port = req->req_msgh.msgh_local_port;
	rep->rep_msgh.msgh_id = req->req_msgh.msgh_id + 100;
	rep->rep_retval = 0;
	rep->rep_ptype = mr->mr_type;
	rep->rep_trailer.msgh_trailer_size = 8;

	*msglen = sizeof(*rep);
	return 0;
}

int 
mach_port_set_attributes(args)
	struct mach_trap_args *args;
{
	mach_port_set_attributes_request_t *req = args->smsg;
	mach_port_set_attributes_reply_t *rep = args->rmsg;
	size_t *msglen = args->rsize;

	switch(req->req_flavor) {
	case MACH_PORT_LIMITS_INFO:
	case MACH_PORT_RECEIVE_STATUS:
	case MACH_PORT_DNREQUESTS_SIZE:
		break;
	default:
		uprintf("mach_port_get_attributes: unknown flavor %d\n",
		    req->req_flavor);
		break;
	}

	rep->rep_msgh.msgh_bits =
	    MACH_MSGH_REPLY_LOCAL_BITS(MACH_MSG_TYPE_MOVE_SEND_ONCE);
	rep->rep_msgh.msgh_size = sizeof(*rep) - sizeof(rep->rep_trailer);
	rep->rep_msgh.msgh_local_port = req->req_msgh.msgh_local_port;
	rep->rep_msgh.msgh_id = req->req_msgh.msgh_id + 100;
	rep->rep_trailer.msgh_trailer_size = 8;

	*msglen = sizeof(*rep);
	return 0;
}

/* XXX insert a recv right into a port set without removing it from another */
int 
mach_port_insert_member(args)
	struct mach_trap_args *args;
{
	mach_port_insert_member_request_t *req = args->smsg;
	mach_port_insert_member_reply_t *rep = args->rmsg;
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
mach_port_move_member(args)
	struct mach_trap_args *args;
{
	mach_port_move_member_request_t *req = args->smsg;
	mach_port_move_member_reply_t *rep = args->rmsg;
	size_t *msglen = args->rsize;
	struct lwp *l = args->l;
	struct mach_emuldata *med = l->l_proc->p_emuldata;
	mach_port_t member = req->req_member;
	mach_port_t after = req->req_after;
	struct mach_right *mrr;
	struct mach_right *mrs;

	mrr = mach_right_check(member, l, MACH_PORT_TYPE_RECEIVE);
	if (mrr == NULL)
		return mach_msg_error(args, EPERM);

	mrs = mach_right_check(after, l, MACH_PORT_TYPE_PORT_SET);
	if (mrs == NULL)
		return mach_msg_error(args, EPERM);
		
	lockmgr(&med->med_rightlock, LK_EXCLUSIVE, NULL);

	/* Remove it from an existing port set */
	if (mrr->mr_sethead != mrr)
		LIST_REMOVE(mrr, mr_setlist);

	/* Insert it into the new port set */
	LIST_INSERT_HEAD(&mrs->mr_set, mrr, mr_setlist);
	mrr->mr_sethead = mrs;

	lockmgr(&med->med_rightlock, LK_RELEASE, NULL);

	rep->rep_msgh.msgh_bits =
	    MACH_MSGH_REPLY_LOCAL_BITS(MACH_MSG_TYPE_MOVE_SEND_ONCE);
	rep->rep_msgh.msgh_size = sizeof(*rep) - sizeof(rep->rep_trailer);
	rep->rep_msgh.msgh_local_port = req->req_msgh.msgh_local_port;
	rep->rep_msgh.msgh_id = req->req_msgh.msgh_id + 100;
	rep->rep_trailer.msgh_trailer_size = 8;

	*msglen = sizeof(*rep);
	return 0;
}

void 
mach_port_init(void) 
{
	pool_init(&mach_port_pool, sizeof (struct mach_port),
	    0, 0, 128, "mach_port_pool", NULL);
	pool_init(&mach_right_pool, sizeof (struct mach_right),
	    0, 0, 128, "mach_right_pool", NULL);

	mach_bootstrap_port = mach_port_get();
	mach_clock_port = mach_port_get();
	mach_io_master_port = mach_port_get();
	mach_saved_bootstrap_port = mach_bootstrap_port;

	return;
}

struct mach_port *
mach_port_get(void)
{
	struct mach_port *mp;

	mp = (struct mach_port *)pool_get(&mach_port_pool, M_WAITOK);
	bzero(mp, sizeof(*mp));
	mp->mp_recv = NULL;
	mp->mp_count = 0;
	TAILQ_INIT(&mp->mp_msglist);
	lockinit(&mp->mp_msglock, PZERO|PCATCH, "mach_port", 0, 0);

	return mp;
}

void
mach_port_put(mp)
	struct mach_port *mp;
{
	struct mach_message *mm;

	if (mp->mp_refcount > 0) {
		uprintf("mach_port_put: trying to free a referenced port\n");
		return;
	}

	lockmgr(&mp->mp_msglock, LK_EXCLUSIVE, NULL);
	while ((mm = TAILQ_FIRST(&mp->mp_msglist)) != NULL)
		mach_message_put_exclocked(mm);
	lockmgr(&mp->mp_msglock, LK_RELEASE, NULL);
	lockmgr(&mp->mp_msglock, LK_DRAIN, NULL);
	
	pool_put(&mach_port_pool, mp);

	return;
}

struct mach_right *
mach_right_get(mp, l, type, hint)
	struct mach_port *mp;
	struct lwp *l;
	int type;
	mach_port_t hint;
{
	struct mach_right *mr;
	struct mach_emuldata *med;
	int rights;

#ifdef DEBUG_MACH
	if (type == 0)
		uprintf("mach_right_get: right = 0\n");
#endif
	med = (struct mach_emuldata *)l->l_proc->p_emuldata;

	/* Send and receive right must return an existing right */
	rights = (MACH_PORT_TYPE_SEND | MACH_PORT_TYPE_RECEIVE);
	if (type & rights) {
		lockmgr(&med->med_rightlock, LK_SHARED, NULL);
		LIST_FOREACH(mr, &med->med_right, mr_list) {
			if ((mr->mr_port == mp) && (mr->mr_type & rights))
				break;
		}
		lockmgr(&med->med_rightlock, LK_RELEASE, NULL);

		if (mr != NULL) {
			mr->mr_type |= type;
			if (type & MACH_PORT_TYPE_SEND)
				mr->mr_refcount++;
			goto rcvck;
		}
	}
	
	mr = pool_get(&mach_right_pool, M_WAITOK);

	mr->mr_port = mp;
	mr->mr_lwp = l;
	mr->mr_type = type;
	mr->mr_sethead = mr; 
	mr->mr_refcount = 1;

	LIST_INIT(&mr->mr_set);

	if (mp != NULL)
		mp->mp_refcount++;

	/* Insert the right in the right lists */
	if (type & MACH_PORT_TYPE_ALL_RIGHTS) {
		lockmgr(&med->med_rightlock, LK_EXCLUSIVE, NULL);
		mr->mr_name = mach_right_newname(l, hint);
#ifdef DEBUG_MACH_RIGHT
		printf("mach_right_get: insert right %x(%x)\n", 
		    mr->mr_name, mr->mr_type);
#endif
		LIST_INSERT_HEAD(&med->med_right, mr, mr_list);
		lockmgr(&med->med_rightlock, LK_RELEASE, NULL);
	}
		
rcvck:
	if (type & MACH_PORT_TYPE_RECEIVE) {
		/* 
		 * Destroy the former receive right on this port, and
		 * register the new right.
		 */
		if (mr->mr_port->mp_recv != NULL)
			mach_right_put(mr->mr_port->mp_recv, 
			    MACH_PORT_TYPE_RECEIVE);
		mr->mr_port->mp_recv = mr;
	}
	return mr;
}

void 
mach_right_put(mr, right)
	struct mach_right *mr;
	int right;
{
	struct mach_emuldata *med = mr->mr_lwp->l_proc->p_emuldata;

	lockmgr(&med->med_rightlock, LK_EXCLUSIVE, NULL);
	mach_right_put_exclocked(mr, right);
	lockmgr(&med->med_rightlock, LK_RELEASE, NULL);

	return;
}

void
mach_right_put_shlocked(mr, right)
	struct mach_right *mr;
	int right;
{
	struct mach_emuldata *med = mr->mr_lwp->l_proc->p_emuldata;

	lockmgr(&med->med_rightlock, LK_UPGRADE, NULL);
	mach_right_put_exclocked(mr, right);
	lockmgr(&med->med_rightlock, LK_DOWNGRADE, NULL);

	return;
}

void
mach_right_put_exclocked(mr, right)
	struct mach_right *mr;
	int right;
{
	struct mach_right *cmr;
	struct mach_emuldata *med;
	int lright;
	int kill_right;

	med = mr->mr_lwp->l_proc->p_emuldata;

#ifdef DEBUG_MACH_RIGHT
	printf("mach_right_put: mr = %p\n", mr);
	printf("right %x(%x) refcount %d, deallocate %x\n",
	    mr->mr_name, mr->mr_type, mr->mr_refcount, right);
	if ((mr->mr_type & right) == 0) 
		printf("mach_right_put: dropping nonexistant right %x on %x\n", 
		    right, mr->mr_name);
	LIST_FOREACH(cmr, &med->med_right, mr_list) 
		if (cmr == mr)
			break;
	if (cmr == NULL) {
		printf("mach_right_put: dropping already dropped right %x\n",
		    mr->mr_name);
		return;
	}
#endif
	kill_right = 0;

	/* When receive right is deallocated, the port should die */
	lright = (right & MACH_PORT_TYPE_RECEIVE);
	if (mr->mr_type & lright) {
		if (mr->mr_refcount <= 0) {
			mr->mr_type &= ~MACH_PORT_TYPE_RECEIVE;
			kill_right = 1;
		} else {
			mr->mr_type &= ~MACH_PORT_TYPE_RECEIVE;
			mr->mr_type |= MACH_PORT_TYPE_DEAD_NAME;
		}
		if (mr->mr_port != NULL) {
			mr->mr_port->mp_refcount--;
			if (mr->mr_port->mp_refcount <= 0)
				mach_port_put(mr->mr_port);
			mr->mr_port = NULL;
		}
	}

	/* send, send_once and dead_name */
	lright = (right & MACH_PORT_TYPE_REF_RIGHTS);
	if (mr->mr_type & lright) {
		mr->mr_refcount--;
		if (mr->mr_refcount <= 0) {
			mr->mr_type &= ~MACH_PORT_TYPE_REF_RIGHTS;
			if ((mr->mr_type & MACH_PORT_TYPE_RECEIVE) == 0)
				kill_right = 1;
		}
	}

	lright = (right & MACH_PORT_TYPE_PORT_SET);
	if ((mr->mr_type & lright) || (kill_right == 1)) {
		while ((cmr = LIST_FIRST(&mr->mr_set)) != NULL) {
			LIST_REMOVE(cmr, mr_setlist);
			cmr->mr_sethead = cmr;
		}
		mr->mr_type &= ~MACH_PORT_TYPE_PORT_SET;
		if ((mr->mr_type & MACH_PORT_TYPE_RECEIVE) == 0)
			kill_right = 1;
	}

	/* Should we kill it? */
	if (kill_right == 1) {
#ifdef DEBUG_MACH_RIGHT
		printf("mach_right_put: kill name %x\n", mr->mr_name);
#endif
		LIST_REMOVE(mr, mr_list);
		pool_put(&mach_right_pool, mr);
	}
	return;
}

/* 
 * Check that a process do have a given right
 */
struct mach_right *
mach_right_check(mn, l, type)
	mach_port_t mn;
	struct lwp *l;
	int type;
{
	struct mach_right *cmr;
	struct mach_emuldata *med;

	if ((mn == 0) || (mn == -1) || (l == NULL))
		return NULL;

	med = (struct mach_emuldata *)l->l_proc->p_emuldata;

	lockmgr(&med->med_rightlock, LK_SHARED, NULL);

#ifdef DEBUG_MACH_RIGHT
	printf("mach_right_check: type = %x, mn = %x\n", type, mn);
#endif
	LIST_FOREACH(cmr, &med->med_right, mr_list) {
#ifdef DEBUG_MACH_RIGHT
		printf("cmr = %p, cmr->mr_name = %x, cmr->mr_type = %x\n", 
		    cmr, cmr->mr_name, cmr->mr_type);
#endif
		if (cmr->mr_name != mn)
			continue;
		if (type & cmr->mr_type)
			break;
	}

	lockmgr(&med->med_rightlock, LK_RELEASE, NULL);

	return cmr;
}


/* 
 * Find an usnused port name in a given lwp. 
 * Right lists should be locked.
 */
mach_port_t
mach_right_newname(l, hint)
	struct lwp *l;
	mach_port_t hint;
{
	struct mach_emuldata *med;
	struct mach_right *mr;
	mach_port_t newname = -1;

	med = l->l_proc->p_emuldata;

	if (hint == 0)
		hint = med->med_nextright;

	while (newname == -1) {
		LIST_FOREACH(mr, &med->med_right, mr_list)
			if (mr->mr_name == hint)
				break;
		if (mr == NULL)
			newname = hint;
		hint++;
	}

	med->med_nextright = hint;

	return newname;
}

#ifdef DEBUG_MACH
void 
mach_debug_port(void)
{
	struct lwp *l;
	struct mach_emuldata *med;
	struct mach_right *mr;
	struct mach_right *mrs;
	struct proc *p = l->l_proc;

	LIST_FOREACH(l, &alllwp, l_list) {
		if ((p->p_emul != &emul_mach) &&
#ifdef COMPAT_DARWIN
		    (p->p_emul != &emul_darwin) &&
#endif
		    1)
			continue;

		med = p->p_emuldata;
		LIST_FOREACH(mr, &med->med_right, mr_list) {		
			if ((mr->mr_type & MACH_PORT_TYPE_PORT_SET) == 0) {
				printf("pid %d: %p(%x)=>%p", 
				    p->p_pid, mr, mr->mr_type, mr->mr_port);
				if (mr->mr_port != NULL) 
					printf("[%p]\n", 
					    mr->mr_port->mp_recv->mr_sethead);
				else
					printf("[NULL]\n");
				
				continue;
			}

			/* Port set... */
			printf("pid %d: set %p(%x) ", 
			    p->p_pid, mr, mr->mr_type);
			LIST_FOREACH(mrs, &mr->mr_set, mr_setlist) {
				printf("%p(%x)=>%p", 
				    mrs, mrs->mr_type, mrs->mr_port);
				if (mrs->mr_port != NULL) 
					printf("[%p]", 
					    mrs->mr_port->mp_recv->mr_sethead);
				else
					printf("[NULL]");
				
				printf(" ");
			}
			printf("\n");
		}
	}
	return;
}

#endif /* DEBUG_MACH */
