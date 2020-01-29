/*	$NetBSD: rtsock_50.c,v 1.16 2020/01/29 05:47:12 thorpej Exp $	*/

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

/*
 * Copyright (c) 1988, 1991, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)rtsock.c	8.7 (Berkeley) 10/12/95
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: rtsock_50.c,v 1.16 2020/01/29 05:47:12 thorpej Exp $");

#define	COMPAT_RTSOCK	/* Use the COMPATNAME/COMPATCALL macros and the
			 * various other compat definitions - see
			 * sys/net/rtsock_shared.c for details
			 */

#include <net/rtsock_shared.c>
#include <compat/net/route_50.h>

static struct sysctllog *clog;

void
compat_50_rt_oifmsg(struct ifnet *ifp)
{
	struct if_msghdr50 oifm;
	struct if_data ifi;
	struct mbuf *m;
	struct rt_addrinfo info;

	if (COMPATNAME(route_info).ri_cb.any_count == 0)
		return;
	(void)memset(&info, 0, sizeof(info));
	(void)memset(&oifm, 0, sizeof(oifm));
	if_export_if_data(ifp, &ifi, false);
	oifm.ifm_index = ifp->if_index;
	oifm.ifm_flags = ifp->if_flags;
	oifm.ifm_data.ifi_type = ifi.ifi_type;
	oifm.ifm_data.ifi_addrlen = ifi.ifi_addrlen;
	oifm.ifm_data.ifi_hdrlen = ifi.ifi_hdrlen;
	oifm.ifm_data.ifi_link_state = ifi.ifi_link_state;
	oifm.ifm_data.ifi_mtu = ifi.ifi_mtu;
	oifm.ifm_data.ifi_metric = ifi.ifi_metric;
	oifm.ifm_data.ifi_baudrate = ifi.ifi_baudrate;
	oifm.ifm_data.ifi_ipackets = ifi.ifi_ipackets;
	oifm.ifm_data.ifi_ierrors = ifi.ifi_ierrors;
	oifm.ifm_data.ifi_opackets = ifi.ifi_opackets;
	oifm.ifm_data.ifi_oerrors = ifi.ifi_oerrors;
	oifm.ifm_data.ifi_collisions = ifi.ifi_collisions;
	oifm.ifm_data.ifi_ibytes = ifi.ifi_ibytes;
	oifm.ifm_data.ifi_obytes = ifi.ifi_obytes;
	oifm.ifm_data.ifi_imcasts = ifi.ifi_imcasts;
	oifm.ifm_data.ifi_omcasts = ifi.ifi_omcasts;
	oifm.ifm_data.ifi_iqdrops = ifi.ifi_iqdrops;
	oifm.ifm_data.ifi_noproto = ifi.ifi_noproto;
	TIMESPEC_TO_TIMEVAL(&oifm.ifm_data.ifi_lastchange,
	    &ifi.ifi_lastchange);
	oifm.ifm_addrs = 0;
	m = COMPATNAME(rt_msg1)(RTM_OIFINFO, &info, (void *)&oifm, sizeof(oifm));
	if (m == NULL)
		return;
	COMPATNAME(route_enqueue)(m, 0);
}

int
compat_50_iflist(struct ifnet *ifp, struct rt_walkarg *w,
    struct rt_addrinfo *info, size_t len)
{
	struct if_msghdr50 *ifm;
	struct if_data ifi;
	int error;

	ifm = (struct if_msghdr50 *)w->w_tmem;
	if_export_if_data(ifp, &ifi, false);
	ifm->ifm_index = ifp->if_index;
	ifm->ifm_flags = ifp->if_flags;
	ifm->ifm_data.ifi_type = ifi.ifi_type;
	ifm->ifm_data.ifi_addrlen = ifi.ifi_addrlen;
	ifm->ifm_data.ifi_hdrlen = ifi.ifi_hdrlen;
	ifm->ifm_data.ifi_link_state = ifi.ifi_link_state;
	ifm->ifm_data.ifi_mtu = ifi.ifi_mtu;
	ifm->ifm_data.ifi_metric = ifi.ifi_metric;
	ifm->ifm_data.ifi_baudrate = ifi.ifi_baudrate;
	ifm->ifm_data.ifi_ipackets = ifi.ifi_ipackets;
	ifm->ifm_data.ifi_ierrors = ifi.ifi_ierrors;
	ifm->ifm_data.ifi_opackets = ifi.ifi_opackets;
	ifm->ifm_data.ifi_oerrors = ifi.ifi_oerrors;
	ifm->ifm_data.ifi_collisions = ifi.ifi_collisions;
	ifm->ifm_data.ifi_ibytes = ifi.ifi_ibytes;
	ifm->ifm_data.ifi_obytes = ifi.ifi_obytes;
	ifm->ifm_data.ifi_imcasts = ifi.ifi_imcasts;
	ifm->ifm_data.ifi_omcasts = ifi.ifi_omcasts;
	ifm->ifm_data.ifi_iqdrops = ifi.ifi_iqdrops;
	ifm->ifm_data.ifi_noproto = ifi.ifi_noproto;
	TIMESPEC_TO_TIMEVAL(&ifm->ifm_data.ifi_lastchange,
	    &ifi.ifi_lastchange);
	ifm->ifm_addrs = info->rti_addrs;
	error = copyout(ifm, w->w_where, len);
	if (error)
		return error;
	w->w_where = (char *)w->w_where + len;
	return 0;
}

void
rtsock_50_init(void)
{
 
	MODULE_HOOK_SET(rtsock_iflist_50_hook, compat_50_iflist);
	MODULE_HOOK_SET(rtsock_oifmsg_50_hook, compat_50_rt_oifmsg);
	MODULE_HOOK_SET(rtsock_rt_missmsg_50_hook, compat_50_rt_missmsg);
	MODULE_HOOK_SET(rtsock_rt_ifmsg_50_hook, compat_50_rt_ifmsg);
	MODULE_HOOK_SET(rtsock_rt_addrmsg_rt_50_hook, compat_50_rt_addrmsg_rt);
	MODULE_HOOK_SET(rtsock_rt_addrmsg_src_50_hook,
	    compat_50_rt_addrmsg_src);
	MODULE_HOOK_SET(rtsock_rt_addrmsg_50_hook, compat_50_rt_addrmsg);
	MODULE_HOOK_SET(rtsock_rt_ifannouncemsg_50_hook,
	    compat_50_rt_ifannouncemsg);
	MODULE_HOOK_SET(rtsock_rt_ieee80211msg_50_hook,
	    compat_50_rt_ieee80211msg);
	sysctl_net_route_setup(&clog, PF_OROUTE, "ortable");
}
 
void
rtsock_50_fini(void)
{  

	sysctl_teardown(&clog);
	MODULE_HOOK_UNSET(rtsock_iflist_50_hook); 
	MODULE_HOOK_UNSET(rtsock_oifmsg_50_hook); 
	MODULE_HOOK_UNSET(rtsock_rt_missmsg_50_hook); 
	MODULE_HOOK_UNSET(rtsock_rt_ifmsg_50_hook); 
	MODULE_HOOK_UNSET(rtsock_rt_addrmsg_rt_50_hook); 
	MODULE_HOOK_UNSET(rtsock_rt_addrmsg_src_50_hook); 
	MODULE_HOOK_UNSET(rtsock_rt_addrmsg_50_hook); 
	MODULE_HOOK_UNSET(rtsock_rt_ifannouncemsg_50_hook); 
	MODULE_HOOK_UNSET(rtsock_rt_ieee80211msg_50_hook); 
}
