/*	$NetBSD: net_inet6_nd_90.c,v 1.2.2.2 2023/12/10 13:06:16 martin Exp $ */

/*      $NetBSD: net_inet6_nd_90.c,v 1.2.2.2 2023/12/10 13:06:16 martin Exp $        */
/*      $KAME: nd6.c,v 1.279 2002/06/08 11:16:51 itojun Exp $   */

/*
 * Copyright (C) 1995, 1996, 1997, and 1998 WIDE Project.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the project nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE PROJECT AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE PROJECT OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: net_inet6_nd_90.c,v 1.2.2.2 2023/12/10 13:06:16 martin Exp $");

#if defined(_KERNEL_OPT)
#include "opt_compat_netbsd.h"
#endif

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/namei.h>
#include <sys/filedesc.h>
#include <sys/kernel.h>
#include <sys/sysctl.h>

#include <net/if.h>

#include <netinet/in.h>

#include <netinet/icmp6.h>
#include <netinet/ip6.h>

#include <netinet6/in6_var.h>
#include <netinet6/nd6.h>

#include <sys/compat_stub.h>

#include <compat/common/compat_mod.h>

#ifdef INET6

static struct sysctllog *nd6_clog;

/*
 * sysctl helper routine for the net.inet6.icmp6.nd6 nodes.  silly?
 */
static int              
sysctl_net_inet6_icmp6_nd6(SYSCTLFN_ARGS)
{
	(void)&name;
	(void)&l;
	(void)&oname;
                        
	if (namelen != 0)
		return (EINVAL);
   
	return (nd6_sysctl(rnode->sysctl_num, oldp, oldlenp,
	    /*XXXUNCONST*/
	    __UNCONST(newp), newlen));
}
int real_net_inet6_nd_90(int);

/*
 * the hook is only called for valid codes, so the mere presence
 * of the hook means success.
 */
/* ARGSUSED */
int
real_net_inet6_nd_90(int name)
{

	return 0;
}

int
net_inet6_nd_90_init(void)
{
	int error;

	error = sysctl_createv(&nd6_clog, 0, NULL, NULL,
		CTLFLAG_PERMANENT,
		CTLTYPE_STRUCT, "nd6_drlist",
		SYSCTL_DESCR("Default router list"),
		sysctl_net_inet6_icmp6_nd6, 0, NULL, 0,
		CTL_NET, PF_INET6, IPPROTO_ICMPV6,
		OICMPV6CTL_ND6_DRLIST, CTL_EOL);
	if (error != 0)
		return error;

	error = sysctl_createv(&nd6_clog, 0, NULL, NULL,
		CTLFLAG_PERMANENT,
		CTLTYPE_STRUCT, "nd6_prlist",
		SYSCTL_DESCR("Prefix list"),
		sysctl_net_inet6_icmp6_nd6, 0, NULL, 0,
		CTL_NET, PF_INET6, IPPROTO_ICMPV6,
		OICMPV6CTL_ND6_PRLIST, CTL_EOL);

	MODULE_HOOK_SET(net_inet6_nd_90_hook, real_net_inet6_nd_90);
	return error;
}

int
net_inet6_nd_90_fini(void)
{

	sysctl_teardown(&nd6_clog);
	MODULE_HOOK_UNSET(net_inet6_nd_90_hook);
	return 0;
}

#endif /* INET6 */

