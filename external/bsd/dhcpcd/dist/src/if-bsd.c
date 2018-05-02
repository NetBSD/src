/*
 * BSD interface driver for dhcpcd
 * Copyright (c) 2006-2018 Roy Marples <roy@marples.name>
 * All rights reserved

 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/ioctl.h>
#include <sys/param.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/sysctl.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <sys/utsname.h>

#include "config.h"

#include <arpa/inet.h>
#include <net/bpf.h>
#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_media.h>
#include <net/route.h>
#include <netinet/if_ether.h>
#include <netinet/in.h>
#include <netinet/in_var.h>
#include <netinet6/in6_var.h>
#include <netinet6/nd6.h>
#ifdef __NetBSD__
#include <net/if_vlanvar.h> /* Needs netinet/if_ether.h */
#else
#include <net/if_vlan_var.h>
#endif
#ifdef __DragonFly__
#  include <netproto/802_11/ieee80211_ioctl.h>
#elif __APPLE__
  /* FIXME: Add apple includes so we can work out SSID */
#else
#  include <net80211/ieee80211.h>
#  include <net80211/ieee80211_ioctl.h>
#endif

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <fnmatch.h>
#include <paths.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#if defined(OpenBSD) && OpenBSD >= 201411
/* OpenBSD dropped the global setting from sysctl but left the #define
 * which causes a EPERM error when trying to use it.
 * I think both the error and keeping the define are wrong, so we #undef it. */
#undef IPV6CTL_ACCEPT_RTADV
#endif

#include "common.h"
#include "dhcp.h"
#include "if.h"
#include "if-options.h"
#include "ipv4.h"
#include "ipv4ll.h"
#include "ipv6.h"
#include "ipv6nd.h"
#include "logerr.h"
#include "route.h"
#include "sa.h"

#ifndef RT_ROUNDUP
#define RT_ROUNDUP(a)							      \
	((a) > 0 ? (1 + (((a) - 1) | (sizeof(long) - 1))) : sizeof(long))
#define RT_ADVANCE(x, n) (x += RT_ROUNDUP((n)->sa_len))
#endif

#ifdef INET6
static void
ifa_scope(struct sockaddr_in6 *, unsigned int);
#endif

struct priv {
	int pf_inet6_fd;
};

int
if_init(__unused struct interface *iface)
{
	/* BSD promotes secondary address by default */
	return 0;
}

int
if_conf(__unused struct interface *iface)
{
	/* No extra checks needed on BSD */
	return 0;
}

int
if_opensockets_os(struct dhcpcd_ctx *ctx)
{
	struct priv *priv;
#if defined(RO_MSGFILTER) || defined(ROUTE_MSGFILTER)
	unsigned char msgfilter[] = {
	    RTM_IFINFO,
#ifdef RTM_IFANNOUNCE
	    RTM_IFANNOUNCE,
#endif
	    RTM_ADD, RTM_CHANGE, RTM_DELETE,
#ifdef RTM_CHGADDR
	    RTM_CHGADDR,
#endif
	    RTM_NEWADDR, RTM_DELADDR
	};
#ifdef ROUTE_MSGFILTER
	unsigned int i, msgfilter_mask;
#endif
#endif

	if ((priv = malloc(sizeof(*priv))) == NULL)
		return -1;
	ctx->priv = priv;

#ifdef INET6
	priv->pf_inet6_fd = xsocket(PF_INET6, SOCK_DGRAM | SOCK_CLOEXEC, 0);
	/* Don't return an error so we at least work on kernels witout INET6
	 * even though we expect INET6 support.
	 * We will fail noisily elsewhere anyway. */
#else
	priv->pf_inet6_fd = -1;
#endif

#define SOCK_FLAGS	(SOCK_CLOEXEC | SOCK_NONBLOCK)
	ctx->link_fd = xsocket(PF_ROUTE, SOCK_RAW | SOCK_FLAGS, AF_UNSPEC);
#undef SOCK_FLAGS
	if (ctx->link_fd == -1)
		return -1;

#if defined(RO_MSGFILTER)
	if (setsockopt(ctx->link_fd, PF_ROUTE, RO_MSGFILTER,
	    &msgfilter, sizeof(msgfilter)) == -1)
		logerr(__func__);
#elif defined(ROUTE_MSGFILTER)
	/* Convert the array into a bitmask. */
	msgfilter_mask = 0;
	for (i = 0; i < __arraycount(msgfilter); i++)
		msgfilter_mask |= ROUTE_FILTER(msgfilter[i]);
	if (setsockopt(ctx->link_fd, PF_ROUTE, ROUTE_MSGFILTER,
	    &msgfilter_mask, sizeof(msgfilter_mask)) == -1)
		logerr(__func__);
#endif

	return 0;
}

void
if_closesockets_os(struct dhcpcd_ctx *ctx)
{
	struct priv *priv;

	priv = (struct priv *)ctx->priv;
	if (priv->pf_inet6_fd != -1)
		close(priv->pf_inet6_fd);
}

#if defined(INET) || defined(INET6)
static void
if_linkaddr(struct sockaddr_dl *sdl, const struct interface *ifp)
{

	memset(sdl, 0, sizeof(*sdl));
	sdl->sdl_family = AF_LINK;
	sdl->sdl_len = sizeof(*sdl);
	sdl->sdl_nlen = sdl->sdl_alen = sdl->sdl_slen = 0;
	sdl->sdl_index = (unsigned short)ifp->index;
}
#endif

#if defined(SIOCG80211NWID) || defined(SIOCGETVLAN)
static int if_direct_ioctl(int s, const char *ifname,
    unsigned long cmd, void *data)
{

	strlcpy(data, ifname, IFNAMSIZ);
	return ioctl(s, cmd, data);
}

static int if_indirect_ioctl(int s, const char *ifname,
    unsigned long cmd, void *data)
{
	struct ifreq ifr;

	memset(&ifr, 0, sizeof(ifr));
	ifr.ifr_data = data;
	return if_direct_ioctl(s, ifname, cmd, &ifr);
}
#endif

static int
if_getssid1(int s, const char *ifname, void *ssid)
{
	int retval = -1;
#if defined(SIOCG80211NWID)
	struct ieee80211_nwid nwid;
#elif defined(IEEE80211_IOC_SSID)
	struct ieee80211req ireq;
	char nwid[IEEE80211_NWID_LEN];
#endif

#if defined(SIOCG80211NWID) /* NetBSD */
	memset(&nwid, 0, sizeof(nwid));
	if (if_indirect_ioctl(s, ifname, SIOCG80211NWID, &nwid) == 0) {
		if (ssid == NULL)
			retval = nwid.i_len;
		else if (nwid.i_len > IF_SSIDLEN)
			errno = ENOBUFS;
		else {
			retval = nwid.i_len;
			memcpy(ssid, nwid.i_nwid, nwid.i_len);
		}
	}
#elif defined(IEEE80211_IOC_SSID) /* FreeBSD */
	memset(&ireq, 0, sizeof(ireq));
	strlcpy(ireq.i_name, ifname, sizeof(ireq.i_name));
	ireq.i_type = IEEE80211_IOC_SSID;
	ireq.i_val = -1;
	memset(nwid, 0, sizeof(nwid));
	ireq.i_data = &nwid;
	if (ioctl(s, SIOCG80211, &ireq) == 0) {
		if (ssid == NULL)
			retval = ireq.i_len;
		else if (ireq.i_len > IF_SSIDLEN)
			errno = ENOBUFS;
		else  {
			retval = ireq.i_len;
			memcpy(ssid, nwid, ireq.i_len);
		}
	}
#else
	errno = ENOSYS;
#endif

	return retval;
}

int
if_getssid(struct interface *ifp)
{
	int r;

	r = if_getssid1(ifp->ctx->pf_inet_fd, ifp->name, ifp->ssid);
	if (r != -1)
		ifp->ssid_len = (unsigned int)r;
	else
		ifp->ssid_len = 0;
	ifp->ssid[ifp->ssid_len] = '\0';
	return r;
}

/*
 * FreeBSD allows for Virtual Access Points
 * We need to check if the interface is a Virtual Interface Master
 * and if so, don't use it.
 * This check is made by virtue of being a IEEE80211 device but
 * returning the SSID gives an error.
 */
int
if_vimaster(const struct dhcpcd_ctx *ctx, const char *ifname)
{
	int r;
	struct ifmediareq ifmr;

	memset(&ifmr, 0, sizeof(ifmr));
	strlcpy(ifmr.ifm_name, ifname, sizeof(ifmr.ifm_name));
	r = ioctl(ctx->pf_inet_fd, SIOCGIFMEDIA, &ifmr);
	if (r == -1)
		return -1;
	if (ifmr.ifm_status & IFM_AVALID &&
	    IFM_TYPE(ifmr.ifm_active) == IFM_IEEE80211)
	{
		if (if_getssid1(ctx->pf_inet_fd, ifname, NULL) == -1)
			return 1;
	}
	return 0;
}

unsigned short
if_vlanid(const struct interface *ifp)
{
#ifdef SIOCGETVLAN
	struct vlanreq vlr;

	memset(&vlr, 0, sizeof(vlr));
	if (if_indirect_ioctl(ifp->ctx->pf_inet_fd,
	    ifp->name, SIOCGETVLAN, &vlr) != 0)
		return 0; /* 0 means no VLANID */
	return vlr.vlr_tag;
#elif defined(SIOCGVNETID)
	struct ifreq ifr;

	memset(&ifr, 0, sizeof(ifr));
	strlcpy(ifr.ifr_name, ifp->name, sizeof(ifr.ifr_name));
	if (ioctl(ifp->ctx->pf_inet_fd, SIOCGVNETID, &ifr) != 0)
		return 0; /* 0 means no VLANID */
	return ifr.ifr_vnetid;
#else
	UNUSED(ifp);
	return 0; /* 0 means no VLANID */
#endif
}

static void
get_addrs(int type, const void *data, const struct sockaddr **sa)
{
	const char *cp;
	int i;

	cp = data;
	for (i = 0; i < RTAX_MAX; i++) {
		if (type & (1 << i)) {
			sa[i] = (const struct sockaddr *)cp;
			RT_ADVANCE(cp, sa[i]);
		} else
			sa[i] = NULL;
	}
}

#if defined(INET) || defined(INET6)
static struct interface *
if_findsdl(struct dhcpcd_ctx *ctx, const struct sockaddr_dl *sdl)
{

	if (sdl->sdl_index)
		return if_findindex(ctx->ifaces, sdl->sdl_index);

	if (sdl->sdl_nlen) {
		char ifname[IF_NAMESIZE];

		memcpy(ifname, sdl->sdl_data, sdl->sdl_nlen);
		ifname[sdl->sdl_nlen] = '\0';
		return if_find(ctx->ifaces, ifname);
	}
	if (sdl->sdl_alen) {
		struct interface *ifp;

		TAILQ_FOREACH(ifp, ctx->ifaces, next) {
			if (ifp->hwlen == sdl->sdl_alen &&
			    memcmp(ifp->hwaddr,
			    sdl->sdl_data, sdl->sdl_alen) == 0)
				return ifp;
		}
	}

	errno = ENOENT;
	return NULL;
}

static struct interface *
if_findsa(struct dhcpcd_ctx *ctx, const struct sockaddr *sa)
{
	if (sa == NULL) {
		errno = EINVAL;
		return NULL;
	}

	switch (sa->sa_family) {
	case AF_LINK:
	{
		const struct sockaddr_dl *sdl;

		sdl = (const void *)sa;
		return if_findsdl(ctx, sdl);
	}
#ifdef INET
	case AF_INET:
	{
		const struct sockaddr_in *sin;
		struct ipv4_addr *ia;

		sin = (const void *)sa;
		if ((ia = ipv4_findmaskaddr(ctx, &sin->sin_addr)))
			return ia->iface;
		break;
	}
#endif
#ifdef INET6
	case AF_INET6:
	{
		const struct sockaddr_in6 *sin;
		struct ipv6_addr *ia;

		sin = (const void *)sa;
		if ((ia = ipv6_findmaskaddr(ctx, &sin->sin6_addr)))
			return ia->iface;
		break;
	}
#endif
	default:
		errno = EAFNOSUPPORT;
		return NULL;
	}

	errno = ENOENT;
	return NULL;
}

static void
if_copysa(struct sockaddr *dst, const struct sockaddr *src)
{

	assert(dst != NULL);
	assert(src != NULL);

	memcpy(dst, src, src->sa_len);
#if defined(INET6) && defined(__KAME__)
	if (dst->sa_family == AF_INET6) {
		struct in6_addr *in6;

		in6 = &satosin6(dst)->sin6_addr;
		if (IN6_IS_ADDR_LINKLOCAL(in6))
			in6->s6_addr[2] = in6->s6_addr[3] = '\0';
	}
#endif
}

int
if_route(unsigned char cmd, const struct rt *rt)
{
	struct dhcpcd_ctx *ctx;
	struct rtm
	{
		struct rt_msghdr hdr;
		char buffer[sizeof(struct sockaddr_storage) * RTAX_MAX];
	} rtmsg;
	struct rt_msghdr *rtm = &rtmsg.hdr;
	char *bp = rtmsg.buffer;
	struct sockaddr_dl sdl;
	bool gateway_unspec;

	assert(rt != NULL);
	ctx = rt->rt_ifp->ctx;

	if ((cmd == RTM_ADD || cmd == RTM_DELETE || cmd == RTM_CHANGE) &&
	    ctx->options & DHCPCD_DAEMONISE &&
	    !(ctx->options & DHCPCD_DAEMONISED))
		ctx->options |= DHCPCD_RTM_PPID;

#define ADDSA(sa) do {							      \
		memcpy(bp, (sa), (sa)->sa_len);				      \
		bp += RT_ROUNDUP((sa)->sa_len);				      \
	}  while (0 /* CONSTCOND */)

	memset(&rtmsg, 0, sizeof(rtmsg));
	rtm->rtm_version = RTM_VERSION;
	rtm->rtm_type = cmd;
#ifdef __OpenBSD__
	rtm->rtm_pid = getpid();
#endif
	rtm->rtm_seq = ++ctx->seq;
	rtm->rtm_flags = (int)rt->rt_flags;
	rtm->rtm_addrs = RTA_DST;
#ifdef RTF_PINNED
	if (cmd != RTM_ADD)
		rtm->rtm_flags |= RTF_PINNED;
#endif

	gateway_unspec = sa_is_unspecified(&rt->rt_gateway);

	if (cmd == RTM_ADD || cmd == RTM_CHANGE) {
		bool netmask_bcast = sa_is_allones(&rt->rt_netmask);

		rtm->rtm_flags |= RTF_UP;
		rtm->rtm_addrs |= RTA_GATEWAY;
		if (!(rtm->rtm_flags & RTF_REJECT) &&
		    !sa_is_loopback(&rt->rt_gateway))
		{
			rtm->rtm_index = (unsigned short)rt->rt_ifp->index;
			if (!gateway_unspec)
				rtm->rtm_addrs |= RTA_IFP;
			if (!sa_is_unspecified(&rt->rt_ifa))
				rtm->rtm_addrs |= RTA_IFA;
		}
		if (netmask_bcast)
			rtm->rtm_flags |= RTF_HOST;
		/* Network routes are cloning or connected if supported.
		 * All other routes are static. */
		if (gateway_unspec) {
#ifdef RTF_CLONING
			rtm->rtm_flags |= RTF_CLONING;
#endif
#ifdef RTF_CONNECTED
			rtm->rtm_flags |= RTF_CONNECTED;
#endif
#ifdef RTP_CONNECTED
			rtm->rtm_priority = RTP_CONNECTED;
#endif
#ifdef RTF_CLONING
			if (netmask_bcast) {
				/*
				 * We add a cloning network route for a single
				 * host. Traffic to the host will generate a
				 * cloned route and the hardware address will
				 * resolve correctly.
				 * It might be more correct to use RTF_HOST
				 * instead of RTF_CLONING, and that does work,
				 * but some OS generate an arp warning
				 * diagnostic which we don't want to do.
				 */
				rtm->rtm_flags &= ~RTF_HOST;
			}
#endif
		} else
			rtm->rtm_flags |= RTF_GATEWAY;

		/* Emulate the kernel by marking address generated
		 * network routes non-static. */
		if (!(rt->rt_dflags & RTDF_IFA_ROUTE))
			rtm->rtm_flags |= RTF_STATIC;

		if (rt->rt_mtu != 0) {
			rtm->rtm_inits |= RTV_MTU;
			rtm->rtm_rmx.rmx_mtu = rt->rt_mtu;
		}
	}

	if (!(rtm->rtm_flags & RTF_HOST))
		rtm->rtm_addrs |= RTA_NETMASK;

	if_linkaddr(&sdl, rt->rt_ifp);

	ADDSA(&rt->rt_dest);

	if (rtm->rtm_addrs & RTA_GATEWAY) {
		if (gateway_unspec)
			ADDSA((struct sockaddr *)&sdl);
		else {
			union sa_ss gateway;

			if_copysa(&gateway.sa, &rt->rt_gateway);
#ifdef INET6
			if (gateway.sa.sa_family == AF_INET6)
				ifa_scope(&gateway.sin6, rt->rt_ifp->index);
#endif
			ADDSA(&gateway.sa);
		}
	}

	if (rtm->rtm_addrs & RTA_NETMASK)
		ADDSA(&rt->rt_netmask);

	if (rtm->rtm_addrs & RTA_IFP)
		ADDSA((struct sockaddr *)&sdl);

	if (rtm->rtm_addrs & RTA_IFA)
		ADDSA(&rt->rt_ifa);

#undef ADDSA

	rtm->rtm_msglen = (unsigned short)(bp - (char *)rtm);
	if (write(ctx->link_fd, rtm, rtm->rtm_msglen) == -1)
		return -1;
	ctx->sseq = ctx->seq;
	return 0;
}

static int
if_copyrt(struct dhcpcd_ctx *ctx, struct rt *rt, const struct rt_msghdr *rtm)
{
	const struct sockaddr *rti_info[RTAX_MAX];

	if (~rtm->rtm_addrs & (RTA_DST | RTA_GATEWAY))
		return -1;
#ifdef RTF_CLONED
	if (rtm->rtm_flags & RTF_CLONED)
		return -1;
#endif
#ifdef RTF_LOCAL
	if (rtm->rtm_flags & RTF_LOCAL)
		return -1;
#endif
#ifdef RTF_BROADCAST
	if (rtm->rtm_flags & RTF_BROADCAST)
		return -1;
#endif

	get_addrs(rtm->rtm_addrs, rtm + 1, rti_info);
	memset(rt, 0, sizeof(*rt));

	rt->rt_flags = (unsigned int)rtm->rtm_flags;
	if_copysa(&rt->rt_dest, rti_info[RTAX_DST]);
	if (rtm->rtm_addrs & RTA_NETMASK) {
		if_copysa(&rt->rt_netmask, rti_info[RTAX_NETMASK]);
		if (rt->rt_netmask.sa_family == 255) /* Why? */
			rt->rt_netmask.sa_family = rt->rt_dest.sa_family;
	}
	/* dhcpcd likes an unspecified gateway to indicate via the link. */
	if (rt->rt_flags & RTF_GATEWAY &&
	    rti_info[RTAX_GATEWAY]->sa_family != AF_LINK)
		if_copysa(&rt->rt_gateway, rti_info[RTAX_GATEWAY]);
	if (rtm->rtm_addrs & RTA_IFA)
		if_copysa(&rt->rt_ifa, rti_info[RTAX_IFA]);
	rt->rt_mtu = (unsigned int)rtm->rtm_rmx.rmx_mtu;

	if (rtm->rtm_index)
		rt->rt_ifp = if_findindex(ctx->ifaces, rtm->rtm_index);
	else if (rtm->rtm_addrs & RTA_IFP)
		rt->rt_ifp = if_findsa(ctx, rti_info[RTAX_IFP]);
	else if (rtm->rtm_addrs & RTA_GATEWAY)
		rt->rt_ifp = if_findsa(ctx, rti_info[RTAX_GATEWAY]);
	else
		rt->rt_ifp = if_findsa(ctx, rti_info[RTAX_DST]);

	if (rt->rt_ifp == NULL) {
		errno = ESRCH;
		return -1;
	}
	return 0;
}

int
if_initrt(struct dhcpcd_ctx *ctx, int af)
{
	struct rt_msghdr *rtm;
	int mib[6];
	size_t needed;
	char *buf, *p, *end;
	struct rt rt;

	rt_headclear(&ctx->kroutes, af);

	mib[0] = CTL_NET;
	mib[1] = PF_ROUTE;
	mib[2] = 0;
	mib[3] = af;
	mib[4] = NET_RT_DUMP;
	mib[5] = 0;

	if (sysctl(mib, 6, NULL, &needed, NULL, 0) == -1)
		return -1;
	if (needed == 0)
		return 0;
	if ((buf = malloc(needed)) == NULL)
		return -1;
	if (sysctl(mib, 6, buf, &needed, NULL, 0) == -1) {
		free(buf);
		return -1;
	}

	end = buf + needed;
	for (p = buf; p < end; p += rtm->rtm_msglen) {
		rtm = (void *)p;
		if (if_copyrt(ctx, &rt, rtm) == 0) {
			rt.rt_dflags |= RTDF_INIT;
			rt_recvrt(RTM_ADD, &rt);
		}
	}
	free(buf);
	return 0;
}

#endif

#ifdef INET
int
if_address(unsigned char cmd, const struct ipv4_addr *ia)
{
	int r;
	struct in_aliasreq ifra;

	memset(&ifra, 0, sizeof(ifra));
	strlcpy(ifra.ifra_name, ia->iface->name, sizeof(ifra.ifra_name));

#define ADDADDR(var, addr) do {						      \
		(var)->sin_family = AF_INET;				      \
		(var)->sin_len = sizeof(*(var));			      \
		(var)->sin_addr = *(addr);				      \
	} while (/*CONSTCOND*/0)
	ADDADDR(&ifra.ifra_addr, &ia->addr);
	ADDADDR(&ifra.ifra_mask, &ia->mask);
	if (cmd == RTM_NEWADDR && ia->brd.s_addr != INADDR_ANY)
		ADDADDR(&ifra.ifra_broadaddr, &ia->brd);
#undef ADDADDR

	r = ioctl(ia->iface->ctx->pf_inet_fd,
	    cmd == RTM_DELADDR ? SIOCDIFADDR : SIOCAIFADDR, &ifra);
	return r;
}



#if !(defined(HAVE_IFADDRS_ADDRFLAGS) && defined(HAVE_IFAM_ADDRFLAGS))
int
if_addrflags(const struct interface *ifp, const struct in_addr *addr,
    __unused const char *alias)
{
#ifdef SIOCGIFAFLAG_IN
	struct ifreq ifr;
	struct sockaddr_in *sin;

	memset(&ifr, 0, sizeof(ifr));
	strlcpy(ifr.ifr_name, ifp->name, sizeof(ifr.ifr_name));
	sin = (void *)&ifr.ifr_addr;
	sin->sin_family = AF_INET;
	sin->sin_addr = *addr;
	if (ioctl(ifp->ctx->pf_inet_fd, SIOCGIFAFLAG_IN, &ifr) == -1)
		return -1;
	return ifr.ifr_addrflags;
#else
	UNUSED(ifp);
	UNUSED(addr);
	return 0;
#endif
}
#endif
#endif /* INET */

#ifdef INET6
static void
ifa_scope(struct sockaddr_in6 *sin, unsigned int ifindex)
{

#ifdef __KAME__
	/* KAME based systems want to store the scope inside the sin6_addr
	 * for link local addresses */
	if (IN6_IS_ADDR_LINKLOCAL(&sin->sin6_addr)) {
		uint16_t scope = htons((uint16_t)ifindex);
		memcpy(&sin->sin6_addr.s6_addr[2], &scope,
		    sizeof(scope));
	}
	sin->sin6_scope_id = 0;
#else
	if (IN6_IS_ADDR_LINKLOCAL(&sin->sin6_addr))
		sin->sin6_scope_id = ifindex;
	else
		sin->sin6_scope_id = 0;
#endif
}

int
if_address6(unsigned char cmd, const struct ipv6_addr *ia)
{
	struct in6_aliasreq ifa;
	struct in6_addr mask;
	struct priv *priv;

	priv = (struct priv *)ia->iface->ctx->priv;

	memset(&ifa, 0, sizeof(ifa));
	strlcpy(ifa.ifra_name, ia->iface->name, sizeof(ifa.ifra_name));
	/*
	 * We should not set IN6_IFF_TENTATIVE as the kernel should be
	 * able to work out if it's a new address or not.
	 *
	 * We should set IN6_IFF_AUTOCONF, but the kernel won't let us.
	 * This is probably a safety measure, but still it's not entirely right
	 * either.
	 */
#if 0
	if (ia->autoconf)
		ifa.ifra_flags |= IN6_IFF_AUTOCONF;
#endif
#ifdef IPV6_MANGETEMPADDR
	if (ia->flags & IPV6_AF_TEMPORARY)
		ifa.ifra_flags |= IN6_IFF_TEMPORARY;
#endif

#define ADDADDR(v, addr) {						      \
		(v)->sin6_family = AF_INET6;				      \
		(v)->sin6_len = sizeof(*v);				      \
		(v)->sin6_addr = *(addr);				      \
	}

	ADDADDR(&ifa.ifra_addr, &ia->addr);
	ifa_scope(&ifa.ifra_addr, ia->iface->index);
	ipv6_mask(&mask, ia->prefix_len);
	ADDADDR(&ifa.ifra_prefixmask, &mask);

#undef ADDADDR

	/*
	 * Every BSD kernel wants to add the prefix of the address to it's
	 * list of RA received prefixes.
	 * THIS IS WRONG because there (as the comments in the kernel state)
	 * is no API for managing prefix lifetime and the kernel should not
	 * pretend it's from a RA either.
	 *
	 * The issue is that the very first assigned prefix will inherit the
	 * lifetime of the address, but any subsequent alteration of the
	 * address OR it's lifetime will not affect the prefix lifetime.
	 * As such, we cannot stop the prefix from timing out and then
	 * constantly removing the prefix route dhcpcd is capable of adding
	 * in it's absense.
	 *
	 * What we can do to mitigate the issue is to add the address with
	 * infinite lifetimes, so the prefix route will never time out.
	 * Once done, we can then set lifetimes on the address and all is good.
	 * The downside of this approach is that we need to manually remove
	 * the kernel route because it has no lifetime, but this is OK as
	 * dhcpcd will handle this too.
	 *
	 * This issue is discussed on the NetBSD mailing lists here:
	 * http://mail-index.netbsd.org/tech-net/2016/08/05/msg006044.html
	 *
	 * Fixed in NetBSD-7.99.36
	 * NOT fixed in FreeBSD - bug 195197
	 * Fixed in OpenBSD-5.9
	 */

#if !((defined(__NetBSD_Version__) && __NetBSD_Version__ >= 799003600) || \
      (defined(__OpenBSD__) && OpenBSD >= 201605))
	if (cmd == RTM_NEWADDR && !(ia->flags & IPV6_AF_ADDED)) {
		ifa.ifra_lifetime.ia6t_vltime = ND6_INFINITE_LIFETIME;
		ifa.ifra_lifetime.ia6t_pltime = ND6_INFINITE_LIFETIME;
		(void)ioctl(priv->pf_inet6_fd, SIOCAIFADDR_IN6, &ifa);
	}
#endif

#if defined(__OpenBSD__) && OpenBSD <= 201705
	/* BUT OpenBSD older than 6.2 does not reset the address lifetime
	 * for subsequent calls...
	 * Luckily dhcpcd will remove the lease when it expires so
	 * just set an infinite lifetime, unless a temporary address. */
	if (ifa.ifra_flags & IN6_IFF_PRIVACY) {
		ifa.ifra_lifetime.ia6t_vltime = ia->prefix_vltime;
		ifa.ifra_lifetime.ia6t_pltime = ia->prefix_pltime;
	} else {
		ifa.ifra_lifetime.ia6t_vltime = ND6_INFINITE_LIFETIME;
		ifa.ifra_lifetime.ia6t_pltime = ND6_INFINITE_LIFETIME;
	}
#else
	ifa.ifra_lifetime.ia6t_vltime = ia->prefix_vltime;
	ifa.ifra_lifetime.ia6t_pltime = ia->prefix_pltime;
#endif

	return ioctl(priv->pf_inet6_fd,
	    cmd == RTM_DELADDR ? SIOCDIFADDR_IN6 : SIOCAIFADDR_IN6, &ifa);
}

#if !(defined(HAVE_IFADDRS_ADDRFLAGS) && defined(HAVE_IFAM_ADDRFLAGS))
int
if_addrflags6(const struct interface *ifp, const struct in6_addr *addr,
    __unused const char *alias)
{
	int flags;
	struct in6_ifreq ifr6;
	struct priv *priv;

	memset(&ifr6, 0, sizeof(ifr6));
	strlcpy(ifr6.ifr_name, ifp->name, sizeof(ifr6.ifr_name));
	ifr6.ifr_addr.sin6_family = AF_INET6;
	ifr6.ifr_addr.sin6_addr = *addr;
	ifa_scope(&ifr6.ifr_addr, ifp->index);
	priv = (struct priv *)ifp->ctx->priv;
	if (ioctl(priv->pf_inet6_fd, SIOCGIFAFLAG_IN6, &ifr6) != -1)
		flags = ifr6.ifr_ifru.ifru_flags6;
	else
		flags = -1;
	return flags;
}
#endif

int
if_getlifetime6(struct ipv6_addr *ia)
{
	struct in6_ifreq ifr6;
	time_t t;
	struct in6_addrlifetime *lifetime;
	struct priv *priv;

	memset(&ifr6, 0, sizeof(ifr6));
	strlcpy(ifr6.ifr_name, ia->iface->name, sizeof(ifr6.ifr_name));
	ifr6.ifr_addr.sin6_family = AF_INET6;
	ifr6.ifr_addr.sin6_addr = ia->addr;
	ifa_scope(&ifr6.ifr_addr, ia->iface->index);
	priv = (struct priv *)ia->iface->ctx->priv;
	if (ioctl(priv->pf_inet6_fd, SIOCGIFALIFETIME_IN6, &ifr6) == -1)
		return -1;

	t = time(NULL);
	lifetime = &ifr6.ifr_ifru.ifru_lifetime;

	if (lifetime->ia6t_preferred)
		ia->prefix_pltime = (uint32_t)(lifetime->ia6t_preferred -
		    MIN(t, lifetime->ia6t_preferred));
	else
		ia->prefix_pltime = ND6_INFINITE_LIFETIME;
	if (lifetime->ia6t_expire) {
		ia->prefix_vltime = (uint32_t)(lifetime->ia6t_expire -
		    MIN(t, lifetime->ia6t_expire));
		/* Calculate the created time */
		clock_gettime(CLOCK_MONOTONIC, &ia->created);
		ia->created.tv_sec -= lifetime->ia6t_vltime - ia->prefix_vltime;
	} else
		ia->prefix_vltime = ND6_INFINITE_LIFETIME;
	return 0;
}
#endif

static void
if_announce(struct dhcpcd_ctx *ctx, const struct if_announcemsghdr *ifan)
{

	switch(ifan->ifan_what) {
	case IFAN_ARRIVAL:
		dhcpcd_handleinterface(ctx, 1, ifan->ifan_name);
		break;
	case IFAN_DEPARTURE:
		dhcpcd_handleinterface(ctx, -1, ifan->ifan_name);
		break;
	}
}

static void
if_ifinfo(struct dhcpcd_ctx *ctx, const struct if_msghdr *ifm)
{
	struct interface *ifp;
	int link_state;

	if ((ifp = if_findindex(ctx->ifaces, ifm->ifm_index)) == NULL)
		return;
	switch (ifm->ifm_data.ifi_link_state) {
	case LINK_STATE_DOWN:
		link_state = LINK_DOWN;
		break;
	case LINK_STATE_UP:
		/* dhcpcd considers the link down if IFF_UP is not set. */
		link_state = ifm->ifm_flags & IFF_UP ? LINK_UP : LINK_DOWN;
		break;
	default:
		/* handle_carrier will re-load the interface flags and check for
		 * IFF_RUNNING as some drivers that don't handle link state also
		 * don't set IFF_RUNNING when this routing message is generated.
		 * As such, it is a race ...*/
		link_state = LINK_UNKNOWN;
		break;
	}
	dhcpcd_handlecarrier(ctx, link_state,
	    (unsigned int)ifm->ifm_flags, ifp->name);
}

static int
if_ownmsgpid(struct dhcpcd_ctx *ctx, pid_t pid, int seq)
{

	/* Ignore messages generated by us */
	if (getpid() == pid) {
		ctx->options &= ~DHCPCD_RTM_PPID;
		return 1;
	}

	/* Ignore messages sent by the parent after forking */
	if ((ctx->options &
	    (DHCPCD_RTM_PPID | DHCPCD_DAEMONISED)) ==
	    (DHCPCD_RTM_PPID | DHCPCD_DAEMONISED) &&
	    ctx->ppid == pid)
	{
		/* If this is the last successful message sent,
		 * clear the check flag as it's possible another
		 * process could re-use the same pid and also
		 * manipulate the routing table. */
		if (ctx->pseq == seq)
			ctx->options &= ~DHCPCD_RTM_PPID;
		return 1;
	}

	/* Not a message we made. */
	return 0;
}

static void
if_rtm(struct dhcpcd_ctx *ctx, const struct rt_msghdr *rtm)
{
	struct rt rt;

	if (if_ownmsgpid(ctx, rtm->rtm_pid, rtm->rtm_seq))
		return;

	/* Ignore errors. */
	if (rtm->rtm_errno != 0)
		return;

	if (if_copyrt(ctx, &rt, rtm) == -1)
		return;

#ifdef INET6
	/*
	 * BSD announces host routes.
	 * As such, we should be notified of reachability by its
	 * existance with a hardware address.
	 */
	if (rt.rt_dest.sa_family == AF_INET6 && rt.rt_flags & RTF_HOST) {
		struct sockaddr_in6 dest;
		struct sockaddr_dl sdl;

		memcpy(&dest, &rt.rt_dest, rt.rt_dest.sa_len);
		if (rt.rt_gateway.sa_family == AF_LINK)
			memcpy(&sdl, &rt.rt_gateway, rt.rt_gateway.sa_len);
		else
			sdl.sdl_alen = 0;
		ipv6nd_neighbour(ctx, &dest.sin6_addr,
		    rtm->rtm_type != RTM_DELETE && sdl.sdl_alen ?
		    IPV6ND_REACHABLE : 0);
	}
#endif

	rt_recvrt(rtm->rtm_type, &rt);
}

static void
if_ifa(struct dhcpcd_ctx *ctx, const struct ifa_msghdr *ifam)
{
	struct interface *ifp;
	const struct sockaddr *rti_info[RTAX_MAX];
	int addrflags;
	pid_t pid;

	if ((ifp = if_findindex(ctx->ifaces, ifam->ifam_index)) == NULL)
		return;
	get_addrs(ifam->ifam_addrs, ifam + 1, rti_info);
	if (rti_info[RTAX_IFA] == NULL)
		return;

#ifdef HAVE_IFAM_PID
	if (if_ownmsgpid(ctx, ifam->ifam_pid, 0)) {
#ifdef HAVE_IFAM_ADDRFLAGS
		/* If the kernel isn't doing DAD for the newly added
		 * address we need to let it through. */
		if (ifam->ifam_type != RTM_NEWADDR)
			return;
		switch (rti_info[RTAX_IFA]->sa_family) {
		case AF_INET:
			if (ifam->ifam_addrflags & IN_IFF_TENTATIVE)
				return;
			break;
		case AF_INET6:
			if (ifam->ifam_addrflags & IN6_IFF_TENTATIVE)
				return;
			break;
		default:
			return;
		}
#endif
	}
	pid = ifam->ifam_pid;
#else
	pid = 0;
#endif

#ifdef HAVE_IFAM_ADDRFLAGS
	addrflags = ifam->ifam_addrflags;
#endif
	switch (rti_info[RTAX_IFA]->sa_family) {
	case AF_LINK:
	{
		struct sockaddr_dl sdl;

#ifdef RTM_CHGADDR
		if (ifam->ifam_type != RTM_CHGADDR)
			break;
#else
		if (ifam->ifam_type != RTM_NEWADDR)
			break;
#endif
		memcpy(&sdl, rti_info[RTAX_IFA], rti_info[RTAX_IFA]->sa_len);
		dhcpcd_handlehwaddr(ctx, ifp->name, CLLADDR(&sdl),sdl.sdl_alen);
		break;
	}
#ifdef INET
	case AF_INET:
	case 255: /* FIXME: Why 255? */
	{
		const struct sockaddr_in *sin;
		struct in_addr addr, mask, bcast;

		sin = (const void *)rti_info[RTAX_IFA];
		addr.s_addr = sin != NULL && sin->sin_family == AF_INET ?
		    sin->sin_addr.s_addr : INADDR_ANY;
		sin = (const void *)rti_info[RTAX_NETMASK];
		mask.s_addr = sin != NULL && sin->sin_family == AF_INET ?
		    sin->sin_addr.s_addr : INADDR_ANY;
		sin = (const void *)rti_info[RTAX_BRD];
		bcast.s_addr = sin != NULL && sin->sin_family == AF_INET ?
		    sin->sin_addr.s_addr : INADDR_ANY;

#if defined(__FreeBSD__) || defined(__DragonFly__)
		/* FreeBSD sends RTM_DELADDR for each assigned address
		 * to an interface just brought down.
		 * This is wrong, because the address still exists.
		 * So we need to ignore it.
		 * Oddly enough this only happens for INET addresses. */
		if (ifam->ifam_type == RTM_DELADDR) {
			struct ifreq ifr;
			struct sockaddr_in *ifr_sin;

			memset(&ifr, 0, sizeof(ifr));
			strlcpy(ifr.ifr_name, ifp->name, sizeof(ifr.ifr_name));
			ifr_sin = (void *)&ifr.ifr_addr;
			ifr_sin->sin_family = AF_INET;
			ifr_sin->sin_addr = addr;
			if (ioctl(ctx->pf_inet_fd, SIOCGIFADDR, &ifr) == 0) {
				logwarnx("%s: ignored false RTM_DELADDR for %s",
				    ifp->name, inet_ntoa(addr));
				break;
			}
		}
#endif

#ifndef HAVE_IFAM_ADDRFLAGS
		if (ifam->ifam_type == RTM_DELADDR)
			addrflags = 0 ;
		else if ((addrflags = if_addrflags(ifp, &addr, NULL)) == -1) {
			logerr("%s: if_addrflags: %s",
			    ifp->name, inet_ntoa(addr));
			break;
		}
#endif

		ipv4_handleifa(ctx, ifam->ifam_type, NULL, ifp->name,
		    &addr, &mask, &bcast, addrflags, pid);
		break;
	}
#endif
#ifdef INET6
	case AF_INET6:
	{
		struct in6_addr addr6, mask6;
		const struct sockaddr_in6 *sin6;

		sin6 = (const void *)rti_info[RTAX_IFA];
		addr6 = sin6->sin6_addr;
		sin6 = (const void *)rti_info[RTAX_NETMASK];
		mask6 = sin6->sin6_addr;

#ifndef HAVE_IFAM_ADDRFLAGS
		if (ifam->ifam_type == RTM_DELADDR)
		    addrflags = 0;
		else if ((addrflags = if_addrflags6(ifp, &addr6, NULL)) == -1) {
			logerr("%s: if_addrflags6", ifp->name);
			break;
		}
#endif

#ifdef __KAME__
		if (IN6_IS_ADDR_LINKLOCAL(&addr6))
			/* Remove the scope from the address */
			addr6.s6_addr[2] = addr6.s6_addr[3] = '\0';
#endif

		ipv6_handleifa(ctx, ifam->ifam_type, NULL,
		    ifp->name, &addr6, ipv6_prefixlen(&mask6), addrflags, pid);
		break;
	}
#endif
	}
}

static void
if_dispatch(struct dhcpcd_ctx *ctx, const struct rt_msghdr *rtm)
{

	if (rtm->rtm_version != RTM_VERSION)
		return;

	switch(rtm->rtm_type) {
#ifdef RTM_IFANNOUNCE
	case RTM_IFANNOUNCE:
		if_announce(ctx, (const void *)rtm);
		break;
#endif
	case RTM_IFINFO:
		if_ifinfo(ctx, (const void *)rtm);
		break;
	case RTM_ADD:		/* FALLTHROUGH */
	case RTM_CHANGE:	/* FALLTHROUGH */
	case RTM_DELETE:
		if_rtm(ctx, (const void *)rtm);
		break;
#ifdef RTM_CHGADDR
	case RTM_CHGADDR:	/* FALLTHROUGH */
#endif
	case RTM_DELADDR:	/* FALLTHROUGH */
	case RTM_NEWADDR:
		if_ifa(ctx, (const void *)rtm);
		break;
#ifdef RTM_DESYNC
	case RTM_DESYNC:
		dhcpcd_linkoverflow(ctx);
		break;
#endif
	}
}

int
if_handlelink(struct dhcpcd_ctx *ctx)
{
	struct msghdr msg;
	ssize_t len;

	memset(&msg, 0, sizeof(msg));
	msg.msg_iov = ctx->iov;
	msg.msg_iovlen = 1;

	len = recvmsg_realloc(ctx->link_fd, &msg, 0);
	if (len == -1)
		return -1;
	if (len != 0)
		if_dispatch(ctx, ctx->iov[0].iov_base);
	return 0;
}

#ifndef SYS_NMLN	/* OSX */
#  define SYS_NMLN 256
#endif
#ifndef HW_MACHINE_ARCH
#  ifdef HW_MODEL	/* OpenBSD */
#    define HW_MACHINE_ARCH HW_MODEL
#  endif
#endif
int
if_machinearch(char *str, size_t len)
{
	int mib[2] = { CTL_HW, HW_MACHINE_ARCH };
	char march[SYS_NMLN];
	size_t marchlen = sizeof(march);

	if (sysctl(mib, sizeof(mib) / sizeof(mib[0]),
	    march, &marchlen, NULL, 0) != 0)
		return -1;
	return snprintf(str, len, ":%s", march);
}

#ifdef INET6
#if (defined(IPV6CTL_ACCEPT_RTADV) && !defined(ND6_IFF_ACCEPT_RTADV)) || \
    defined(IPV6CTL_USETEMPADDR) || defined(IPV6CTL_TEMPVLTIME)
#define get_inet6_sysctl(code) inet6_sysctl(code, 0, 0)
#define set_inet6_sysctl(code, val) inet6_sysctl(code, val, 1)
static int
inet6_sysctl(int code, int val, int action)
{
	int mib[] = { CTL_NET, PF_INET6, IPPROTO_IPV6, 0 };
	size_t size;

	mib[3] = code;
	size = sizeof(val);
	if (action) {
		if (sysctl(mib, sizeof(mib)/sizeof(mib[0]),
		    NULL, 0, &val, size) == -1)
			return -1;
		return 0;
	}
	if (sysctl(mib, sizeof(mib)/sizeof(mib[0]), &val, &size, NULL, 0) == -1)
		return -1;
	return val;
}
#endif

#ifdef IPV6_MANAGETEMPADDR
#ifndef IPV6CTL_TEMPVLTIME
#define get_inet6_sysctlbyname(code) inet6_sysctlbyname(code, 0, 0)
#define set_inet6_sysctlbyname(code, val) inet6_sysctlbyname(code, val, 1)
static int
inet6_sysctlbyname(const char *name, int val, int action)
{
	size_t size;

	size = sizeof(val);
	if (action) {
		if (sysctlbyname(name, NULL, 0, &val, size) == -1)
			return -1;
		return 0;
	}
	if (sysctlbyname(name, &val, &size, NULL, 0) == -1)
		return -1;
	return val;
}
#endif

int
ip6_use_tempaddr(__unused const char *ifname)
{
	int val;

#ifdef IPV6CTL_USETEMPADDR
	val = get_inet6_sysctl(IPV6CTL_USETEMPADDR);
#else
	val = get_inet6_sysctlbyname("net.inet6.ip6.use_tempaddr");
#endif
	return val == -1 ? 0 : val;
}

int
ip6_temp_preferred_lifetime(__unused const char *ifname)
{
	int val;

#ifdef IPV6CTL_TEMPPLTIME
	val = get_inet6_sysctl(IPV6CTL_TEMPPLTIME);
#else
	val = get_inet6_sysctlbyname("net.inet6.ip6.temppltime");
#endif
	return val < 0 ? TEMP_PREFERRED_LIFETIME : val;
}

int
ip6_temp_valid_lifetime(__unused const char *ifname)
{
	int val;

#ifdef IPV6CTL_TEMPVLTIME
	val = get_inet6_sysctl(IPV6CTL_TEMPVLTIME);
#else
	val = get_inet6_sysctlbyname("net.inet6.ip6.tempvltime");
#endif
	return val < 0 ? TEMP_VALID_LIFETIME : val;
}
#endif

#ifdef SIOCIFAFATTACH
static int
af_attach(int s, const struct interface *ifp, int af)
{
	struct if_afreq ifar;

	strlcpy(ifar.ifar_name, ifp->name, sizeof(ifar.ifar_name));
	ifar.ifar_af = af;
	return ioctl(s, SIOCIFAFATTACH, (void *)&ifar);
}
#endif

#ifdef SIOCGIFXFLAGS
static int
set_ifxflags(int s, const struct interface *ifp)
{
	struct ifreq ifr;
	int flags;

	strlcpy(ifr.ifr_name, ifp->name, sizeof(ifr.ifr_name));
	if (ioctl(s, SIOCGIFXFLAGS, (void *)&ifr) == -1)
		return -1;
	flags = ifr.ifr_flags;
#ifdef IFXF_NOINET6
	flags &= ~IFXF_NOINET6;
#endif
	/*
	 * If not doing autoconf, don't disable the kernel from doing it.
	 * If we need to, we should have another option actively disable it.
	 */
	if (ifp->options->options & DHCPCD_IPV6RS)
		flags &= ~IFXF_AUTOCONF6;
	if (ifr.ifr_flags == flags)
		return 0;
	ifr.ifr_flags = flags;
	return ioctl(s, SIOCSIFXFLAGS, (void *)&ifr);
}
#endif

/* OpenBSD removed ND6 flags entirely, so we need to check for their
 * existance. */
#if defined(ND6_IFF_AUTO_LINKLOCAL) || \
    defined(ND6_IFF_PERFORMNUD) || \
    defined(ND6_IFF_ACCEPT_RTADV) || \
    defined(ND6_IFF_OVERRIDE_RTADV) || \
    defined(ND6_IFF_IFDISABLED)
#define	ND6_NDI_FLAGS
#endif

void
if_setup_inet6(const struct interface *ifp)
{
	struct priv *priv;
	int s;
#ifdef ND6_NDI_FLAGS
	struct in6_ndireq nd;
	int flags;
#endif

	priv = (struct priv *)ifp->ctx->priv;
	s = priv->pf_inet6_fd;

#ifdef ND6_NDI_FLAGS
	memset(&nd, 0, sizeof(nd));
	strlcpy(nd.ifname, ifp->name, sizeof(nd.ifname));
	if (ioctl(s, SIOCGIFINFO_IN6, &nd) == -1)
		logerr("%s: SIOCGIFINFO_FLAGS", ifp->name);
	flags = (int)nd.ndi.flags;
#endif

#ifdef ND6_IFF_AUTO_LINKLOCAL
	/* Unlike the kernel,
	 * dhcpcd make make a stable private address. */
	flags &= ~ND6_IFF_AUTO_LINKLOCAL;
#endif

#ifdef ND6_IFF_PERFORMNUD
	/* NUD is kind of essential. */
	flags |= ND6_IFF_PERFORMNUD;
#endif

#ifdef ND6_IFF_IFDISABLED
	/* Ensure the interface is not disabled. */
	flags &= ~ND6_IFF_IFDISABLED;
#endif

	/*
	 * If not doing autoconf, don't disable the kernel from doing it.
	 * If we need to, we should have another option actively disable it.
	 */
#ifdef ND6_IFF_ACCEPT_RTADV
	if (ifp->options->options & DHCPCD_IPV6RS)
		flags &= ~ND6_IFF_ACCEPT_RTADV;
#ifdef ND6_IFF_OVERRIDE_RTADV
	if (ifp->options->options & DHCPCD_IPV6RS)
		flags |= ND6_IFF_OVERRIDE_RTADV;
#endif
#endif

#ifdef ND6_NDI_FLAGS
	if (nd.ndi.flags != (uint32_t)flags) {
		nd.ndi.flags = (uint32_t)flags;
		if (ioctl(s, SIOCSIFINFO_FLAGS, &nd) == -1)
			logerr("%s: SIOCSIFINFO_FLAGS", ifp->name);
	}
#endif

	/* Enabling IPv6 by whatever means must be the
	 * last action undertaken to ensure kernel RS and
	 * LLADDR auto configuration are disabled where applicable. */
#ifdef SIOCIFAFATTACH
	if (af_attach(s, ifp, AF_INET6) == -1)
		logerr("%s: af_attach", ifp->name);
#endif

#ifdef SIOCGIFXFLAGS
	if (set_ifxflags(s, ifp) == -1)
		logerr("%s: set_ifxflags", ifp->name);
#endif

#if defined(IPV6CTL_ACCEPT_RTADV) && !defined(ND6_IFF_ACCEPT_RTADV)
	/* If we cannot control ra per interface, disable it globally. */
	if (ifp->options->options & DHCPCD_IPV6RS) {
		int ra = get_inet6_sysctl(IPV6CTL_ACCEPT_RTADV);

		if (ra == -1) {
			if (errno != ENOENT)
				logerr("IPV6CTL_ACCEPT_RTADV");
		else if (ra != 0)
			if (set_inet6_sysctl(IPV6CTL_ACCEPT_RTADV, 0) == -1)
				logerr("IPV6CTL_ACCEPT_RTADV");
		}
	}
#endif

#if defined(IPV6CTL_ACCEPT_RTADV) || defined(ND6_IFF_ACCEPT_RTADV)
	/* Flush the kernel knowledge of advertised routers
	 * and prefixes so the kernel does not expire prefixes
	 * and default routes we are trying to own. */
	if (ifp->options->options & DHCPCD_IPV6RS) {
		char ifname[IFNAMSIZ + 8];

		strlcpy(ifname, ifp->name, sizeof(ifname));
		if (ioctl(s, SIOCSRTRFLUSH_IN6, (void *)&ifname) == -1 &&
		    errno != ENOTSUP)
			logwarn("SIOCSRTRFLUSH_IN6");
		if (ioctl(s, SIOCSPFXFLUSH_IN6, (void *)&ifname) == -1 &&
		    errno != ENOTSUP)
			logwarn("SIOCSPFXFLUSH_IN6");
	}
#endif
}
#endif
