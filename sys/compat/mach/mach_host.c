/*	$NetBSD: mach_host.c,v 1.1 2002/11/10 02:18:03 manu Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: mach_host.c,v 1.1 2002/11/10 02:18:03 manu Exp $");

#include <sys/types.h>
#include <sys/malloc.h>
#include <sys/param.h>
#include <sys/systm.h>

#include <compat/mach/mach_types.h>
#include <compat/mach/mach_host.h>

int 
mach_host_info(msgh)
	mach_msg_header_t *msgh;
{
	mach_host_info_request_t req;
	mach_host_info_reply_t *rep = NULL;
	size_t msglen;
	int error;

	DPRINTF(("mach_host_info\n"));

	if ((error = copyin(msgh, &req, sizeof(req))) != 0)
		return error;

	switch(req.req_flavor) {
	case MACH_HOST_MACH_MSG_TRAP:
		msglen = sizeof(mach_host_info_reply_t);
		rep = (mach_host_info_reply_t *)malloc(msglen,
		    M_TEMP, M_ZERO|M_WAITOK);
		rep->rep_msgh.msgh_bits = 0x1200; /* XXX why? */
		rep->rep_msgh.msgh_local_port = req.req_msgh.msgh_local_port;
		rep->rep_msgh.msgh_id = 300;	/* XXX why? */
		break;

	case MACH_HOST_BASIC_INFO:
	case MACH_HOST_SCHED_INFO:
	case MACH_HOST_RESOURCE_SIZES:
	case MACH_HOST_PRIORITY_INFO:
	case MACH_HOST_SEMAPHORE_TRAPS:
		DPRINTF(("unimplemented host_info flavor %d\n", 
		    req.req_flavor));
	default:
		return EINVAL;
		break;
	}

	if (rep != NULL && (error = copyout(rep, msgh, msglen)) != 0)
		return error;

	return 0;
}

