/*	$NetBSD: mach_task.c,v 1.2 2002/11/10 22:05:35 manu Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: mach_task.c,v 1.2 2002/11/10 22:05:35 manu Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>

#include <compat/mach/mach_types.h>
#include <compat/mach/mach_message.h>
#include <compat/mach/mach_task.h>
#include <compat/mach/mach_syscallargs.h>


int 
mach_task_get_special_port(p, msgh)
	struct proc *p;
	mach_msg_header_t *msgh;
{
	mach_task_get_special_port_request_t req;
	mach_task_get_special_port_reply_t rep;
	int error;

	if ((error = copyin(msgh, &req, sizeof(req))) != 0)
		return error;

	bzero(&rep, sizeof(rep));
	rep.rep_msgh.msgh_bits = 0x8001200; /* XXX why? */
	rep.rep_msgh.msgh_size = sizeof(rep);
	rep.rep_msgh.msgh_local_port = req.req_msgh.msgh_local_port;
	rep.rep_msgh.msgh_id = req.req_msgh.msgh_id + 100;
	rep.rep_msgh_body.msgh_descriptor_count = 1;	/* XXX why ? */
	rep.rep_special_port.name = 0x90f; /* XXX why? */
	rep.rep_special_port.disposition = 0x11; /* XXX why? */
	rep.rep_trailer.msgh_trailer_size = 8; /* XXX why? */

	if ((error = copyout(&rep, msgh, sizeof(rep))) != 0)
		return error;
	return 0;
}

int 
mach_ports_lookup(p, msgh)
	struct proc *p;
	mach_msg_header_t *msgh;
{
	mach_ports_lookup_request_t req;
	mach_ports_lookup_reply_t rep;
	int error;

	if ((error = copyin(msgh, &req, sizeof(req))) != 0)
		return error;

	bzero(&rep, sizeof(rep));
	rep.rep_msgh.msgh_bits = 0x8001200; /* XXX why? */
	rep.rep_msgh.msgh_size = sizeof(rep);
	rep.rep_msgh.msgh_local_port = req.req_msgh.msgh_local_port;
	rep.rep_msgh.msgh_id = req.req_msgh.msgh_id + 100;
	rep.rep_msgh_body.msgh_descriptor_count = 1;	/* XXX why ? */
	rep.rep_init_port_set.address = (void *)0x8000; /* XXX why? */
	rep.rep_init_port_set.count = 3; /* XXX why ? */
	rep.rep_init_port_set.copy = 2; /* XXX why ? */
	rep.rep_init_port_set.disposition = 0x11; /* XXX why? */
	rep.rep_init_port_set.type = 2; /* XXX why? */

	if ((error = copyout(&rep, msgh, sizeof(rep))) != 0)
		return error;
	return 0;
}


