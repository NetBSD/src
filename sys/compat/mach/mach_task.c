/*	$NetBSD: mach_task.c,v 1.21 2003/01/24 21:37:03 manu Exp $ */

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

#include "opt_compat_darwin.h"

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: mach_task.c,v 1.21 2003/01/24 21:37:03 manu Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/exec.h>
#include <sys/systm.h>

#include <uvm/uvm_extern.h>
#include <uvm/uvm_param.h>

#include <compat/mach/mach_types.h>
#include <compat/mach/mach_message.h>
#include <compat/mach/mach_clock.h>
#include <compat/mach/mach_errno.h>
#include <compat/mach/mach_exec.h>
#include <compat/mach/mach_port.h>
#include <compat/mach/mach_task.h>
#include <compat/mach/mach_syscallargs.h>

#ifdef COMPAT_DARWIN
#include <compat/darwin/darwin_exec.h>
#endif

int 
mach_task_get_special_port(args)
	struct mach_trap_args *args;
{
	mach_task_get_special_port_request_t *req = args->smsg;
	mach_task_get_special_port_reply_t *rep = args->rmsg;
	size_t *msglen = args->rsize;
	struct lwp *l = args->l;
	struct mach_emuldata *med;
	struct mach_right *mr;
	int error;

	med = (struct mach_emuldata *)l->l_proc->p_emuldata;

	switch (req->req_which_port) {
	case MACH_TASK_KERNEL_PORT:
		mr = mach_right_get(med->med_kernel, l, MACH_PORT_TYPE_SEND, 0);
		break;

	case MACH_TASK_HOST_PORT:
		mr = mach_right_get(med->med_host, l, MACH_PORT_TYPE_SEND, 0);
		break;

	case MACH_TASK_BOOTSTRAP_PORT:
		mr = mach_right_get(med->med_bootstrap, 
		    l, MACH_PORT_TYPE_SEND, 0);
#ifdef DEBUG_MACH
		printf("*** get bootstrap right %p, port %p, recv %p [%p]\n",
		    mr, mr->mr_port, mr->mr_port->mp_recv,
		    mr->mr_port->mp_recv->mr_sethead);
#endif
		break;

	case MACH_TASK_WIRED_LEDGER_PORT:
	case MACH_TASK_PAGED_LEDGER_PORT:
	default:
		uprintf("mach_task_get_special_port(): unimpl. port %d\n",
		    req->req_which_port);
		return mach_msg_error(args, error);
		break;
	}

	rep->rep_msgh.msgh_bits = 
	    MACH_MSGH_REPLY_LOCAL_BITS(MACH_MSG_TYPE_MOVE_SEND_ONCE) |
	    MACH_MSGH_BITS_COMPLEX;
	rep->rep_msgh.msgh_size = sizeof(*rep) - sizeof(rep->rep_trailer);
	rep->rep_msgh.msgh_local_port = req->req_msgh.msgh_local_port;
	rep->rep_msgh.msgh_id = req->req_msgh.msgh_id + 100;
	rep->rep_msgh_body.msgh_descriptor_count = 1;
	rep->rep_special_port.name = (mach_port_t)mr->mr_name;
	rep->rep_special_port.disposition = 0x11; /* XXX why? */
	rep->rep_trailer.msgh_trailer_size = 8;

	*msglen = sizeof(*rep);
	return 0;
}

int 
mach_ports_lookup(args)
	struct mach_trap_args *args;
{
	mach_ports_lookup_request_t *req = args->smsg;
	mach_ports_lookup_reply_t *rep = args->rmsg;
	size_t *msglen = args->rsize;
	struct lwp *l = args->l;
	struct mach_emuldata *med;
	struct mach_right *msp[7];
	vaddr_t va;
	int error;

	/* 
	 * This is some out of band data sent with the reply. In the 
	 * encountered situation the out of band data has always been null
	 * filled. We have to see more of this in order to fully understand
	 * how this trap works.
	 */
	va = vm_map_min(&l->l_proc->p_vmspace->vm_map);
	if ((error = uvm_map(&l->l_proc->p_vmspace->vm_map, &va, PAGE_SIZE,
	    NULL, UVM_UNKNOWN_OFFSET, 0, UVM_MAPFLAG(UVM_PROT_RW, UVM_PROT_ALL,
	    UVM_INH_COPY, UVM_ADV_NORMAL, UVM_FLAG_COPYONW))) != 0)
		return mach_msg_error(args, error);

	med = (struct mach_emuldata *)l->l_proc->p_emuldata;
	msp[0] = MACH_PORT_DEAD;
	msp[3] = MACH_PORT_DEAD;
	msp[5] = MACH_PORT_DEAD;
	msp[6] = MACH_PORT_DEAD;

	msp[MACH_TASK_KERNEL_PORT] = 
	    mach_right_get(med->med_kernel, l, MACH_PORT_TYPE_SEND, 0);
	msp[MACH_TASK_HOST_PORT] = 
	    mach_right_get(med->med_host, l, MACH_PORT_TYPE_SEND, 0);
	msp[MACH_TASK_BOOTSTRAP_PORT] = 
	    mach_right_get(med->med_bootstrap, l, MACH_PORT_TYPE_SEND, 0);

	/*
	 * On Darwin, the data seems always null...
	 */
	if ((error = copyout(&msp[0], (void *)va, sizeof(msp))) != 0)
		return mach_msg_error(args, error);

	rep->rep_msgh.msgh_bits =
	    MACH_MSGH_REPLY_LOCAL_BITS(MACH_MSG_TYPE_MOVE_SEND_ONCE) |
	    MACH_MSGH_BITS_COMPLEX;
	rep->rep_msgh.msgh_size = sizeof(*rep) - sizeof(rep->rep_trailer);
	rep->rep_msgh.msgh_local_port = req->req_msgh.msgh_local_port;
	rep->rep_msgh.msgh_id = req->req_msgh.msgh_id + 100;
	rep->rep_msgh_body.msgh_descriptor_count = 1;	/* XXX why ? */
	rep->rep_init_port_set.address = (void *)va;
	rep->rep_init_port_set.count = 3; /* XXX why ? */
	rep->rep_init_port_set.copy = 2; /* XXX why ? */
	rep->rep_init_port_set.disposition = 0x11; /* XXX why? */
	rep->rep_init_port_set.type = 2; /* XXX why? */
	rep->rep_init_port_set_count = 3; /* XXX why? */
	rep->rep_trailer.msgh_trailer_size = 8;

	*msglen = sizeof(*rep);
	return 0;
}

int 
mach_task_set_special_port(args)
	struct mach_trap_args *args;
{
	mach_task_set_special_port_request_t *req = args->smsg;
	mach_task_set_special_port_reply_t *rep = args->rmsg;
	size_t *msglen = args->rsize;
	struct lwp *l = args->l;
	mach_port_t mn;
	struct mach_right *mr;
	struct mach_port *mp;
	struct mach_emuldata *med;

	mn = req->req_special_port.name;

	/* Null port ? */
	if (mn == NULL)
		return mach_msg_error(args, 0);

	/* Does the inserted port exists? */
	if ((mr = mach_right_check(mn, l, MACH_PORT_TYPE_ALL_RIGHTS)) == 0)
		return mach_msg_error(args, EPERM);

	if (mr->mr_type == MACH_PORT_TYPE_DEAD_NAME)
		return mach_msg_error(args, EINVAL);

	med = (struct mach_emuldata *)l->l_proc->p_emuldata;

	switch (req->req_which_port) {
	case MACH_TASK_KERNEL_PORT:
		mp = med->med_kernel;
		med->med_kernel = mr->mr_port;
		mp->mp_refcount--;
		if (mp->mp_refcount == 0)
			mach_port_put(mp);
		break;

	case MACH_TASK_HOST_PORT:
		mp = med->med_host;
		med->med_host = mr->mr_port;
		mp->mp_refcount--;
		if (mp->mp_refcount == 0)
			mach_port_put(mp);
		break;

	case MACH_TASK_BOOTSTRAP_PORT:
		mp = med->med_bootstrap;
		med->med_bootstrap = mr->mr_port;
		mp->mp_refcount--;
		if (mp->mp_refcount == 0)
			mach_port_put(mp);
#ifdef COMPAT_DARWIN
		/*
		 * mach_init sets the bootstrap port for any new process
		 */
		{
			struct darwin_emuldata *ded;

			ded = l->l_proc->p_emuldata;
			if (ded->ded_fakepid == 1) {
				mach_bootstrap_port = med->med_bootstrap;
#ifdef DEBUG_DARWIN
				printf("*** New bootstrap port %p, "
				    "recv %p [%p]\n",
				    mach_bootstrap_port, 
				    mach_bootstrap_port->mp_recv,
				    mach_bootstrap_port->mp_recv->mr_sethead);
#endif /* DEBUG_DARWIN */
			}
		}
#endif /* COMPAT_DARWIN */
		break;

	default:
		uprintf("mach_task_set_special_port: unimplemented port %d\n",
		    req->req_which_port);
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
