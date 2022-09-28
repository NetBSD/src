/*	$NetBSD: uipc_syscalls_50.c,v 1.12 2022/09/28 15:32:09 msaitoh Exp $	*/

/*-
 * Copyright (c) 2008 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Christos Zoulas.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
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

#if defined(_KERNEL_OPT)
#include "opt_compat_netbsd.h"
#endif

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/msg.h>
#include <sys/sysctl.h>
#include <sys/syscallargs.h>
#include <sys/errno.h>
#include <sys/kauth.h>
#include <sys/proc.h>
#include <sys/time.h>
#include <sys/compat_stub.h>

#include <net/if.h>

#include <compat/net/if.h>
#include <compat/sys/time.h>
#include <compat/sys/socket.h>
#include <compat/sys/sockio.h>

#include <compat/common/compat_mod.h>

/*ARGSUSED*/
static int
compat_ifdatareq(struct lwp *l, u_long cmd, void *data)
{
	struct if_data ifi;
	struct ifdatareq50 *ifdr = data;
	struct ifnet *ifp;
	int error;

	/* Validate arguments. */
	switch (cmd) {
	case OSIOCGIFDATA:
	case OSIOCZIFDATA:
		break;
	default:
		return ENOSYS;
	}

	ifp = ifunit(ifdr->ifdr_name);
	if (ifp == NULL)
		return ENXIO;

	/* Do work. */
	switch (cmd) {
	case OSIOCGIFDATA:
		if_export_if_data(ifp, &ifi, false);
		ifdatan2o(&ifdr->ifdr_data, &ifi);
		return 0;

	case OSIOCZIFDATA:
		if (l != NULL) {
			error = kauth_authorize_network(l->l_cred,
			    KAUTH_NETWORK_INTERFACE,
			    KAUTH_REQ_NETWORK_INTERFACE_SETPRIV, ifp,
			    (void *)cmd, NULL);
			if (error != 0)
				return error;
		}
		if_export_if_data(ifp, &ifi, true);
		ifdatan2o(&ifdr->ifdr_data, &ifi);
		/* XXX if_lastchange? */
		return 0;

	default:
		/* Impossible due to above validation, but makes gcc happy. */
		return ENOSYS;
	}
}

void
uipc_syscalls_50_init(void)
{

	MODULE_HOOK_SET(uipc_syscalls_50_hook, compat_ifdatareq);
}

void
uipc_syscalls_50_fini(void)
{
 
	MODULE_HOOK_UNSET(uipc_syscalls_50_hook);
}
