/*	$NetBSD: mach_semaphore.c,v 1.5 2003/01/21 04:06:08 matt Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: mach_semaphore.c,v 1.5 2003/01/21 04:06:08 matt Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/signal.h>
#include <sys/pool.h>
#include <sys/lock.h>
#include <sys/malloc.h>
#include <sys/proc.h>

#include <compat/mach/mach_types.h>
#include <compat/mach/mach_message.h>
#include <compat/mach/mach_semaphore.h>
#include <compat/mach/mach_clock.h>
#include <compat/mach/mach_syscallargs.h>

/* Semaphore list, lock, pools */
static LIST_HEAD(mach_semaphore_list, mach_semaphore) mach_semaphore_list; 
static struct lock mach_semaphore_list_lock;
static struct pool mach_semaphore_list_pool;
static struct pool mach_waiting_proc_pool;

/* Function to manipulate them */
static struct mach_semaphore *mach_semaphore_get(int, int);
static void mach_semaphore_put(struct mach_semaphore *);
static struct mach_waiting_proc *mach_waiting_proc_get
    (struct proc *, struct mach_semaphore *);
static void mach_waiting_proc_put
    (struct mach_waiting_proc *, struct mach_semaphore *, int);

int
mach_sys_semaphore_wait_trap(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct mach_sys_semaphore_wait_trap_args /* {
		syscallarg(mach_port_name_t) wait_name;
	} */ *uap = v;
	struct mach_semaphore *ms;
	struct mach_waiting_proc *mwp;
	int blocked = 0;

	ms = (struct mach_semaphore *)SCARG(uap, wait_name);

	lockmgr(&ms->ms_lock, LK_EXCLUSIVE, NULL);	
	ms->ms_value--;
	if (ms->ms_value < 0)
		blocked = 1;
	lockmgr(&ms->ms_lock, LK_RELEASE, NULL);	

	if (blocked != 0) {
		mwp = mach_waiting_proc_get(l->l_proc, ms);	
		while (ms->ms_value < 0)
			tsleep(mwp, PZERO|PCATCH, "sem_wait", 0);
		mach_waiting_proc_put(mwp, ms, 0);
	}
	return 0;
}

int
mach_sys_semaphore_signal_trap(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct mach_sys_semaphore_signal_trap_args /* {
		syscallarg(mach_port_name_t) signal_name;
	} */ *uap = v;
	struct mach_semaphore *ms;
	struct mach_waiting_proc *mwp;
	int unblocked = 0;

	ms = (struct mach_semaphore *)SCARG(uap, signal_name);

	lockmgr(&ms->ms_lock, LK_EXCLUSIVE, NULL);	
	ms->ms_value++;
	if (ms->ms_value >= 0)
		unblocked = 1;
	lockmgr(&ms->ms_lock, LK_RELEASE, NULL);	

	if (unblocked != 0) {
		lockmgr(&ms->ms_lock, LK_SHARED, NULL);	
		mwp = TAILQ_FIRST(&ms->ms_waiting);
		wakeup(mwp);
		lockmgr(&ms->ms_lock, LK_RELEASE, NULL);	
	}
	return 0;
}

int 
mach_semaphore_create(args)
	struct mach_trap_args *args;
{
	mach_semaphore_create_request_t *req = args->smsg;
	mach_semaphore_create_reply_t *rep = args->rmsg;
	size_t *msglen = args->rsize;
	struct mach_semaphore *ms;

	ms = mach_semaphore_get(req->req_value, req->req_policy);
	
	rep->rep_msgh.msgh_bits =
	    MACH_MSGH_REPLY_LOCAL_BITS(MACH_MSG_TYPE_MOVE_SEND_ONCE);
	rep->rep_msgh.msgh_size = sizeof(*rep) - sizeof(rep->rep_trailer);
	rep->rep_msgh.msgh_local_port = req->req_msgh.msgh_local_port;
	rep->rep_msgh.msgh_id = req->req_msgh.msgh_id + 100;
	rep->rep_sem.name = (mach_port_t)ms; /* Waiting for better */
	rep->rep_trailer.msgh_trailer_size = 8;

	*msglen = sizeof(*rep);
	return 0;
}

int 
mach_semaphore_destroy(args)
	struct mach_trap_args *args;
{
	mach_semaphore_destroy_request_t *req = args->smsg;
	mach_semaphore_destroy_reply_t *rep = args->rmsg;
	size_t *msglen = args->rsize;
	struct mach_semaphore *ms;

	ms = (struct mach_semaphore *)req->req_sem.name;
	mach_semaphore_put(ms);
	
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
mach_semaphore_init(void)
{
	LIST_INIT(&mach_semaphore_list);
	lockinit(&mach_semaphore_list_lock, PZERO|PCATCH, "mach_sem", 0, 0);
	pool_init(&mach_semaphore_list_pool, sizeof (struct mach_semaphore),
	    0, 0, 128, "mach_sem_pool", NULL);
	pool_init(&mach_waiting_proc_pool, sizeof (struct mach_waiting_proc),
	    0, 0, 128, "mach_waitp_pool", NULL);

	return;
}

static struct mach_semaphore *
mach_semaphore_get(value, policy)
	int value;
	int policy;
{
	struct mach_semaphore *ms;

	ms = (struct mach_semaphore *)pool_get(&mach_semaphore_list_pool,
	    M_WAITOK);
	ms->ms_value = value;
	ms->ms_policy = policy;
	TAILQ_INIT(&ms->ms_waiting);
	lockinit(&ms->ms_lock, PZERO|PCATCH, "mach_waitp", 0, 0);

	lockmgr(&mach_semaphore_list_lock, LK_EXCLUSIVE, NULL);
	LIST_INSERT_HEAD(&mach_semaphore_list, ms, ms_list);
	lockmgr(&mach_semaphore_list_lock, LK_RELEASE, NULL);

	return ms;
}

static void
mach_semaphore_put(ms)
	struct mach_semaphore *ms;
{
	struct mach_waiting_proc *mwp;

	lockmgr(&ms->ms_lock, LK_EXCLUSIVE, NULL);	
	while ((mwp = TAILQ_FIRST(&ms->ms_waiting)) != NULL) 
		mach_waiting_proc_put(mwp, ms, 0);
	lockmgr(&ms->ms_lock, LK_RELEASE, NULL);	
	lockmgr(&ms->ms_lock, LK_DRAIN, NULL);	

	lockmgr(&mach_semaphore_list_lock, LK_EXCLUSIVE, NULL);
	LIST_REMOVE(ms, ms_list);
	lockmgr(&mach_semaphore_list_lock, LK_RELEASE, NULL);

	pool_put(&mach_semaphore_list_pool, ms);

	return;
}

static struct mach_waiting_proc *
mach_waiting_proc_get(p, ms)
	struct proc *p;
	struct mach_semaphore *ms;
{
	struct mach_waiting_proc *mwp;

	mwp = (struct mach_waiting_proc *)pool_get(&mach_waiting_proc_pool,
	    M_WAITOK);
	mwp->mwp_p = p;

	lockmgr(&ms->ms_lock, LK_EXCLUSIVE, NULL);	
	TAILQ_INSERT_TAIL(&ms->ms_waiting, mwp, mwp_list);
	lockmgr(&ms->ms_lock, LK_RELEASE, NULL);	

	return mwp;
}

static void
mach_waiting_proc_put(mwp, ms, locked)
	struct mach_waiting_proc *mwp;
	struct mach_semaphore *ms;
	int locked;
{
	if (!locked)
		lockmgr(&ms->ms_lock, LK_EXCLUSIVE, NULL);	
	TAILQ_REMOVE(&ms->ms_waiting, mwp, mwp_list);
	if (!locked)
		lockmgr(&ms->ms_lock, LK_RELEASE, NULL);	
	pool_put(&mach_waiting_proc_pool, mwp);

	return;
}

/* 
 * Cleanup after process exit. Need improvements, there 
 * can be some memory leaks here.
 */
void
mach_semaphore_cleanup(p)
	struct proc *p;
{
	struct mach_semaphore *ms;
	struct mach_waiting_proc *mwp;

	lockmgr(&mach_semaphore_list_lock, LK_SHARED, NULL);
	LIST_FOREACH(ms, &mach_semaphore_list, ms_list) {
		lockmgr(&ms->ms_lock, LK_SHARED, NULL);
		TAILQ_FOREACH(mwp, &ms->ms_waiting, mwp_list) 
			if (mwp->mwp_p == p) {
				lockmgr(&ms->ms_lock, LK_UPGRADE, NULL);
				mach_waiting_proc_put(mwp, ms, 0);
				ms->ms_value++;
				if (ms->ms_value >= 0)
					wakeup(TAILQ_FIRST(&ms->ms_waiting));
			}
		lockmgr(&ms->ms_lock, LK_RELEASE, NULL);
	}
	lockmgr(&mach_semaphore_list_lock, LK_RELEASE, NULL);
	
	return;
}
