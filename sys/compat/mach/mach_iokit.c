/*	$NetBSD: mach_iokit.c,v 1.2 2003/02/05 23:58:09 manu Exp $ */

/*-
 * Copyright (c) 2003 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Emmanuel Dreyfus.
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
__KERNEL_RCSID(0, "$NetBSD: mach_iokit.c,v 1.2 2003/02/05 23:58:09 manu Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/malloc.h>
#include <sys/signal.h>
#include <sys/mount.h>
#include <sys/proc.h>

#include <compat/mach/mach_types.h>
#include <compat/mach/mach_message.h>
#include <compat/mach/mach_port.h>

#include <compat/mach/mach_iokit.h>


int
mach_io_service_get_matching_services(args)
	struct mach_trap_args *args;
{
	mach_io_service_get_matching_services_request_t *req = args->smsg;
	mach_io_service_get_matching_services_reply_t *rep = args->rmsg;
	size_t *msglen = args->rsize; 
	struct lwp *l = args->l;
	struct mach_port *mp;
	struct mach_right *mr;

	mp = mach_port_get();
	mp->mp_flags |= MACH_MP_INKERNEL;
	mr = mach_right_get(mp, l, MACH_PORT_TYPE_SEND, 0);
	
	rep->rep_msgh.msgh_bits = 
	    MACH_MSGH_REPLY_LOCAL_BITS(MACH_MSG_TYPE_MOVE_SEND_ONCE) |
	    MACH_MSGH_BITS_COMPLEX;
	rep->rep_msgh.msgh_size = sizeof(*rep) - sizeof(rep->rep_trailer);
	rep->rep_msgh.msgh_local_port = req->req_msgh.msgh_local_port;
	rep->rep_msgh.msgh_id = req->req_msgh.msgh_id + 100;
	rep->rep_body.msgh_descriptor_count = 1;
	rep->rep_match.name = (mach_port_t)mr->mr_name;
	rep->rep_match.disposition = 0x11; /* XXX */
	rep->rep_trailer.msgh_trailer_size = 8;

	*msglen = sizeof(*rep);
	return 0;
}

int
mach_io_iterator_next(args)
	struct mach_trap_args *args;
{
	mach_io_iterator_next_request_t *req = args->smsg;
	mach_io_iterator_next_reply_t *rep = args->rmsg;
	size_t *msglen = args->rsize; 
	struct lwp *l = args->l;
	struct mach_port *mp;
	struct mach_right *mr;

	mp = mach_port_get();
	mp->mp_flags |= MACH_MP_INKERNEL;
	mr = mach_right_get(mp, l, MACH_PORT_TYPE_SEND, 0);
	
	rep->rep_msgh.msgh_bits = 
	    MACH_MSGH_REPLY_LOCAL_BITS(MACH_MSG_TYPE_MOVE_SEND_ONCE) |
	    MACH_MSGH_BITS_COMPLEX;
	rep->rep_msgh.msgh_size = sizeof(*rep) - sizeof(rep->rep_trailer);
	rep->rep_msgh.msgh_local_port = req->req_msgh.msgh_local_port;
	rep->rep_msgh.msgh_id = req->req_msgh.msgh_id + 100;
	rep->rep_body.msgh_descriptor_count = 1;
	rep->rep_object.name = (mach_port_t)mr->mr_name;
	rep->rep_object.disposition = 0x11; /* XXX */
	rep->rep_trailer.msgh_trailer_size = 8;

	*msglen = sizeof(*rep);
	return 0;
}
