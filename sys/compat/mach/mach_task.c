/*	$NetBSD: mach_task.c,v 1.3.2.4 2002/12/19 00:44:34 thorpej Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: mach_task.c,v 1.3.2.4 2002/12/19 00:44:34 thorpej Exp $");

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


int 
mach_task_get_special_port(args)
	struct mach_trap_args *args;
{
	mach_task_get_special_port_request_t *req = args->smsg;
	mach_task_get_special_port_reply_t *rep = args->rmsg;
	size_t *msglen = args->rsize;
	struct proc *p = args->p;
	struct mach_emuldata *med;
	struct mach_right *mr;
	int error;

	med = (struct mach_emuldata *)p->p_emuldata;

	switch (req->req_which_port) {
	case MACH_TASK_KERNEL_PORT:
		mr = mach_right_get(med->med_kernel, p, MACH_PORT_RIGHT_SEND);
		break;

	case MACH_TASK_HOST_PORT:
		mr = mach_right_get(med->med_host, p, MACH_PORT_RIGHT_SEND);
		break;

	case MACH_TASK_BOOTSTRAP_PORT:
		mr = mach_right_get(med->med_bootstrap, 
		    p, MACH_PORT_RIGHT_SEND);
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
	rep->rep_special_port.name = (mach_port_t)mr;
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
	struct proc *p = args->p;
	vaddr_t va;
	int error;

	/* 
	 * This is some out of band data sent with the reply. In the 
	 * encountered situation the out of band data has always been null
	 * filled. We have to see more of his in order to fully understand
	 * how this trap works.
	 */
	va = vm_map_min(&p->p_vmspace->vm_map);
	if ((error = uvm_map(&p->p_vmspace->vm_map, &va, PAGE_SIZE, NULL, 
	    UVM_UNKNOWN_OFFSET, 0, UVM_MAPFLAG(UVM_PROT_RW, UVM_PROT_ALL,
	    UVM_INH_COPY, UVM_ADV_NORMAL, UVM_FLAG_COPYONW))) != 0)
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

	rep->rep_msgh.msgh_bits =
	    MACH_MSGH_REPLY_LOCAL_BITS(MACH_MSG_TYPE_MOVE_SEND_ONCE);
	rep->rep_msgh.msgh_size = sizeof(*rep) - sizeof(rep->rep_trailer);
	rep->rep_msgh.msgh_local_port = req->req_msgh.msgh_local_port;
	rep->rep_msgh.msgh_id = req->req_msgh.msgh_id + 100;
	rep->rep_trailer.msgh_trailer_size = 8;

	*msglen = sizeof(*rep);
	return 0;
}
