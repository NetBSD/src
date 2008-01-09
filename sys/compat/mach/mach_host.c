/*	$NetBSD: mach_host.c,v 1.29.46.1 2008/01/09 01:51:26 matt Exp $ */

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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: mach_host.c,v 1.29.46.1 2008/01/09 01:51:26 matt Exp $");

#include <sys/types.h>
#include <sys/malloc.h>
#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/systm.h>
#include <sys/signal.h>
#include <sys/proc.h>

#include <uvm/uvm_extern.h>
#include <uvm/uvm_param.h>

#include <compat/mach/mach_types.h>
#include <compat/mach/mach_host.h>
#include <compat/mach/mach_port.h>
#include <compat/mach/mach_clock.h>
#include <compat/mach/mach_errno.h>
#include <compat/mach/mach_services.h>

int
mach_host_info(struct mach_trap_args *args)
{
	mach_host_info_request_t *req = args->smsg;
	mach_host_info_reply_t *rep = args->rmsg;
	size_t *msglen = args->rsize;
	mach_host_info_reply_simple_t *reps;

	*msglen = sizeof(*rep);
	mach_set_header(rep, req, *msglen);

	switch(req->req_flavor) {
	case MACH_HOST_BASIC_INFO: {
		struct mach_host_basic_info *info
		    = (struct mach_host_basic_info *)&rep->rep_data[0];

		rep->rep_msgh.msgh_size = sizeof(*reps)
		    - sizeof(rep->rep_trailer) + sizeof(*info);
		rep->rep_count = sizeof(*info) / sizeof(mach_integer_t);
		mach_host_basic_info(info);
		break;
	}

	case MACH_HOST_PRIORITY_INFO: {
		struct mach_host_priority_info *info
		    = (struct mach_host_priority_info *)&rep->rep_data[0];

		rep->rep_msgh.msgh_size = sizeof(*reps)
		    - sizeof(rep->rep_trailer) + sizeof(*info);
		rep->rep_count = sizeof(*info) / sizeof(mach_integer_t);
		mach_host_priority_info(info);
		break;
	}

	case MACH_HOST_SEMAPHORE_TRAPS:
	case MACH_HOST_MACH_MSG_TRAP:
		reps = (mach_host_info_reply_simple_t *)rep;
		reps->rep_msgh.msgh_size =
		    sizeof(*reps) - sizeof(reps->rep_trailer);
		*msglen = sizeof(*reps);
		break;

	case MACH_HOST_SCHED_INFO: {
		struct mach_host_sched_info *info
		    = (struct mach_host_sched_info *)&rep->rep_data[0];

		rep->rep_msgh.msgh_size = sizeof(*reps)
		    - sizeof(rep->rep_trailer) + sizeof(*info);
		rep->rep_count = sizeof(*info) / sizeof(mach_integer_t);

		info->min_timeout = 1000 / hz; /* XXX timout in ms */
		info->min_quantum = 1000 / hz; /* quantum in ms */

		break;
	}

	case MACH_HOST_RESOURCE_SIZES:
		uprintf("mach_host_info() Unimplemented host_info flavor %d\n",
		    req->req_flavor);
	default:
		uprintf("Unknown host_info flavor %d\n", req->req_flavor);
		rep->rep_retval = native_to_mach_errno[EINVAL];
		break;
	}

	mach_set_trailer(rep, *msglen);

	return 0;
}


int
mach_host_page_size(struct mach_trap_args *args)
{
	mach_host_page_size_request_t *req = args->smsg;
	mach_host_page_size_reply_t *rep = args->rmsg;
	size_t *msglen = args->rsize;

	*msglen = sizeof(*rep);
	mach_set_header(rep, req, *msglen);

	rep->rep_page_size = PAGE_SIZE;

	mach_set_trailer(rep, *msglen);

	return 0;
}

int
mach_host_get_clock_service(struct mach_trap_args *args)
{
	mach_host_get_clock_service_request_t *req = args->smsg;
	mach_host_get_clock_service_reply_t *rep = args->rmsg;
	size_t *msglen = args->rsize;
	struct lwp *l = args->l;
	struct mach_right *mr;

	mr = mach_right_get(mach_clock_port, l, MACH_PORT_TYPE_SEND, 0);

	*msglen = sizeof(*rep);
	mach_set_header(rep, req, *msglen);
	mach_add_port_desc(rep, mr->mr_name);
	mach_set_trailer(rep, *msglen);

	return 0;
}

void
mach_host_priority_info(struct mach_host_priority_info *info)
{
	/* XXX One day, try to fill this correctly */
	info->kernel_priority = 0x50;
	info->system_priority = 0x50;
	info->server_priority = 0x40;
	info->user_priority = 0x1f;
	info->depress_priority = 0x00;
	info->idle_priority = 0x00;
	info->minimum_priority = 0x00;
	info->maximum_priority = 0x4f;

	return;
}

int
mach_host_get_io_master(struct mach_trap_args *args)
{
	mach_host_get_io_master_request_t *req = args->smsg;
	mach_host_get_io_master_reply_t *rep = args->rmsg;
	size_t *msglen = args->rsize;
	struct lwp *l = args->l;
	struct mach_right *mr;

	mr = mach_right_get(mach_io_master_port, l, MACH_PORT_TYPE_SEND, 0);

	*msglen = sizeof(*rep);
	mach_set_header(rep, req, *msglen);
	mach_add_port_desc(rep, mr->mr_name);
	mach_set_trailer(rep, *msglen);

	return 0;
}

int
mach_processor_set_default(struct mach_trap_args *args)
{
	mach_processor_set_default_request_t *req = args->smsg;
	mach_processor_set_default_reply_t *rep = args->rmsg;
	size_t *msglen = args->rsize;
	struct lwp *l = args->l;
	struct mach_right *mr;
	struct mach_port *mp;

	mp = mach_port_get();
	mr = mach_right_get(mp, l, MACH_PORT_TYPE_SEND, 0);

	*msglen = sizeof(*rep);
	mach_set_header(rep, req, *msglen);
	mach_add_port_desc(rep, mr->mr_name);
	mach_set_trailer(rep, *msglen);

	return 0;
}

int
mach_host_processor_set_priv(struct mach_trap_args *args)
{
	mach_host_processor_set_priv_request_t *req = args->smsg;
	mach_host_processor_set_priv_reply_t *rep = args->rmsg;
	size_t *msglen = args->rsize;
	struct lwp *l = args->l;
	mach_port_t mn;
	struct mach_right *mr;
	struct mach_right *smr;
	struct mach_port *smp;

	mn = req->req_set.name;
	if ((mr = mach_right_check(mn, l, MACH_PORT_TYPE_ALL_RIGHTS)) == NULL)
		return mach_msg_error(args, EINVAL);

	smp = mach_port_get();
	smr = mach_right_get(smp, l, MACH_PORT_TYPE_SEND, 0);

	*msglen = sizeof(*rep);
	mach_set_header(rep, req, *msglen);
	mach_add_port_desc(rep, smr->mr_name);
	mach_set_trailer(rep, *msglen);

	return 0;
}
