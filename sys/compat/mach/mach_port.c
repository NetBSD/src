/*	$NetBSD: mach_port.c,v 1.2 2002/11/10 22:05:35 manu Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: mach_port.c,v 1.2 2002/11/10 22:05:35 manu Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/signal.h>
#include <sys/proc.h>

#include <compat/mach/mach_types.h>
#include <compat/mach/mach_message.h>
#include <compat/mach/mach_port.h>
#include <compat/mach/mach_syscallargs.h>

int
mach_sys_reply_port(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval; 
{
	static int current_port = 0x80b;

	*retval = current_port++; /* XXX */
	return 0;
}

int
mach_sys_thread_self_trap(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	*retval = 0; /* XXX */
	return 0;
}


int
mach_sys_task_self_trap(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	*retval = 0xa07; /* XXX */
	return 0;
}


int
mach_sys_host_self_trap(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	*retval = 0x90b; /* XXX */
	return 0;
}

int 
mach_port_deallocate(p, msgh)
	struct proc *p;
	mach_msg_header_t *msgh;
{
	mach_port_deallocate_request_t req;
	mach_port_deallocate_reply_t rep;
	int error;

	if ((error = copyin(msgh, &req, sizeof(req))) != 0)
		return error;

	bzero(&rep, sizeof(rep));
	rep.rep_msgh.msgh_bits = 0x1200; /* XXX why? */
	rep.rep_msgh.msgh_size = sizeof(rep);
	rep.rep_msgh.msgh_local_port = req.req_msgh.msgh_local_port;
	rep.rep_msgh.msgh_id = req.req_msgh.msgh_id + 100;

	if ((error = copyout(&rep, msgh, sizeof(rep))) != 0)
		return error;
	return 0;
}

int 
mach_port_allocate(p, msgh)
	struct proc *p;
	mach_msg_header_t *msgh;
{
	mach_port_allocate_request_t req;
	mach_port_allocate_reply_t rep;
	int error;

	if ((error = copyin(msgh, &req, sizeof(req))) != 0)
		return error;

	bzero(&rep, sizeof(rep));
	rep.rep_msgh.msgh_bits = 0x1200; /* XXX why? */
	rep.rep_msgh.msgh_size = sizeof(rep);
	rep.rep_msgh.msgh_local_port = req.req_msgh.msgh_local_port;
	rep.rep_msgh.msgh_id = req.req_msgh.msgh_id + 100;
	rep.rep_trailer.msgh_trailer_size = 1811; /* XXX why? */

	if ((error = copyout(&rep, msgh, sizeof(rep))) != 0)
		return error;
	return 0;
}

