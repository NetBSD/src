/*	$NetBSD: mach_bootstrap.c,v 1.11.4.1 2008/01/21 09:41:38 yamt Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: mach_bootstrap.c,v 1.11.4.1 2008/01/21 09:41:38 yamt Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/signal.h>
#include <sys/proc.h>

#include <compat/mach/mach_types.h>
#include <compat/mach/mach_message.h>
#include <compat/mach/mach_port.h>
#include <compat/mach/mach_bootstrap.h>
#include <compat/mach/mach_errno.h>
#include <compat/mach/mach_services.h>

int
mach_bootstrap_look_up(struct mach_trap_args *args)
{
	mach_bootstrap_look_up_request_t *req = args->smsg;
	mach_bootstrap_look_up_reply_t *rep = args->rmsg;
	struct lwp *l = args->l;
	size_t *msglen = args->rsize;
	const char service_name[] = "lookup\021"; /* XXX Why */
	int service_name_len;
	struct mach_right *mr;
	size_t len;

	/* The trailer is word aligned  */
	service_name_len = (sizeof(service_name) + 1) & ~0x7UL;
	len = sizeof(*rep) - sizeof(rep->rep_service_name) + service_name_len;

	if (len > *msglen)
		return mach_msg_error(args, EINVAL);
	*msglen = len;

	mr = mach_right_get(NULL, l, MACH_PORT_TYPE_DEAD_NAME, 0);

	mach_set_header(rep, req, *msglen);

	rep->rep_count = 1;
	rep->rep_bootstrap_port = mr->mr_name;
	strncpy((char *)rep->rep_service_name, service_name,
	    service_name_len);

	mach_set_trailer(rep, *msglen);

	return 0;
}

