/*	$NetBSD: net_stub.c,v 1.7.2.2 2009/01/17 13:29:37 mjf Exp $	*/

/*
 * Copyright (c) 2008 Antti Kantee.  All Rights Reserved.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: net_stub.c,v 1.7.2.2 2009/01/17 13:29:37 mjf Exp $");

#include <sys/param.h>
#include <sys/protosw.h>
#include <sys/socketvar.h>

#include <net/if.h>
#include <net/route.h>

#include <compat/sys/socket.h>
#include <compat/sys/sockio.h>

int __rumpnet_stub(void);
int
__rumpnet_stub(void)
{

	panic("rumpnet stubs only, linking against librumpnet_net required");
}
__weak_alias(rtioctl,__rumpnet_stub);
__weak_alias(rt_walktree,__rumpnet_stub);
__weak_alias(rtrequest,__rumpnet_stub);
__weak_alias(rtrequest,__rumpnet_stub);
__weak_alias(ifioctl,__rumpnet_stub);
__weak_alias(ifunit,__rumpnet_stub);
__weak_alias(ifreq_setaddr,__rumpnet_stub);

struct ifnet_head ifnet;

u_long
compat_cvtcmd(u_long cmd)
{

	return cmd;
}

int
compat_ifconf(u_long cmd, void *data)
{

	return EOPNOTSUPP;
}

int
compat_ifioctl(struct socket *so, u_long ocmd, u_long cmd, void *data,
	struct lwp *l)
{
	struct ifreq *ifr = data;
	struct ifnet *ifp = ifunit(ifr->ifr_name);

	if (!ifp)
		return ENXIO;

	return (*so->so_proto->pr_usrreq)(so, PRU_CONTROL,
	    (struct mbuf *)cmd, (struct mbuf *)data, (struct mbuf *)ifp, l);
}
