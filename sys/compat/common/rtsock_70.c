/*	$NetBSD: rtsock_70.c,v 1.1.18.2 2017/12/03 11:36:53 jdolecek Exp $	*/

/*
 * Copyright (c) 2016 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Roy Marples.
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
__KERNEL_RCSID(0, "$NetBSD: rtsock_70.c,v 1.1.18.2 2017/12/03 11:36:53 jdolecek Exp $");

#ifdef _KERNEL_OPT
#include "opt_compat_netbsd.h"
#endif

#include <sys/mbuf.h>
#include <net/if.h>
#include <net/route.h>

#include <compat/net/if.h>
#include <compat/net/route.h>

#if defined(COMPAT_70)
void
compat_70_rt_newaddrmsg1(int cmd, struct ifaddr *ifa)
{
	struct rt_addrinfo info;
	const struct sockaddr *sa;
	struct mbuf *m;
	struct ifnet *ifp;
	struct ifa_msghdr70 ifam;
	int ncmd;

	KASSERT(ifa != NULL);
	ifp = ifa->ifa_ifp;

	switch (cmd) {
	case RTM_NEWADDR:
		ncmd = RTM_ONEWADDR;
		break;
	case RTM_DELADDR:
		ncmd = RTM_ODELADDR;
		break;
	case RTM_CHGADDR:
		ncmd = RTM_OCHGADDR;
		break;
	default:
		panic("%s: called with wrong command", __func__);
	}

	memset(&info, 0, sizeof(info));
	info.rti_info[RTAX_IFA] = sa = ifa->ifa_addr;
	KASSERT(ifp->if_dl != NULL);
	info.rti_info[RTAX_IFP] = ifp->if_dl->ifa_addr;
	info.rti_info[RTAX_NETMASK] = ifa->ifa_netmask;
	info.rti_info[RTAX_BRD] = ifa->ifa_dstaddr;

	memset(&ifam, 0, sizeof(ifam));
	ifam.ifam_index = ifp->if_index;
	ifam.ifam_metric = ifa->ifa_metric;
	ifam.ifam_flags = ifa->ifa_flags;

	m = rt_msg1(ncmd, &info, &ifam, sizeof(ifam));
	if (m == NULL)
		return;

	mtod(m, struct ifa_msghdr70 *)->ifam_addrs = info.rti_addrs;
	route_enqueue(m, sa ? sa->sa_family : 0);
}

int
compat_70_iflist_addr(struct rt_walkarg *w, struct ifaddr *ifa,
     struct rt_addrinfo *info)
{
	int len, error;

	if ((error = rt_msg3(RTM_ONEWADDR, info, 0, w, &len)))
		return error;
	if (w->w_where && w->w_tmem && w->w_needed <= 0) {
		struct ifa_msghdr70 *ifam;

		ifam = (struct ifa_msghdr70 *)w->w_tmem;
		ifam->ifam_index = ifa->ifa_ifp->if_index;
		ifam->ifam_flags = ifa->ifa_flags;
		ifam->ifam_metric = ifa->ifa_metric;
		ifam->ifam_addrs = info->rti_addrs;
		if ((error = copyout(w->w_tmem, w->w_where, len)) == 0)
			w->w_where = (char *)w->w_where + len;
	}
	return error;
}
#endif /* COMPAT_70 */
