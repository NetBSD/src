#include <sys/cdefs.h>
 __RCSID("$NetBSD: if-bsd.c,v 1.1.1.23 2014/02/25 13:14:29 roy Exp $");

/*
 * dhcpcd - DHCP client daemon
 * Copyright (c) 2006-2014 Roy Marples <roy@marples.name>
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
#include <sys/types.h>

#include <arpa/inet.h>
#include <net/if.h>
#include <net/if_dl.h>
#ifdef __FreeBSD__ /* Needed so that including netinet6/in6_var.h works */
#  include <net/if_var.h>
#endif
#include <net/if_media.h>
#include <net/route.h>
#include <netinet/in.h>
#include <netinet6/in6_var.h>
#ifdef __DragonFly__
#  include <netproto/802_11/ieee80211_ioctl.h>
#elif __APPLE__
  /* FIXME: Add apple includes so we can work out SSID */
#else
#  include <net80211/ieee80211_ioctl.h>
#endif

#include <errno.h>
#include <fcntl.h>
#include <fnmatch.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>

#include "config.h"
#include "common.h"
#include "dhcp.h"
#include "if-options.h"
#include "ipv4.h"
#include "ipv6.h"

#ifndef RT_ROUNDUP
#define RT_ROUNDUP(a)							      \
	((a) > 0 ? (1 + (((a) - 1) | (sizeof(long) - 1))) : sizeof(long))
#define RT_ADVANCE(x, n) (x += RT_ROUNDUP((n)->sa_len))
#endif

#define COPYOUT(sin, sa)						      \
	sin.s_addr = ((sa) != NULL) ?					      \
	    (((struct sockaddr_in *)(void *)sa)->sin_addr).s_addr : 0

#define COPYOUT6(sin, sa)						      \
	sin.s6_addr = ((sa) != NULL) ?					      \
	    (((struct sockaddr_in6 *)(void *)sa)->sin6_addr).s6_addr : 0

#ifndef CLLADDR
#  define CLLADDR(s) ((const char *)((s)->sdl_data + (s)->sdl_nlen))
#endif

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
open_link_socket(void)
{

#ifdef SOCK_CLOEXEC
	return socket(PF_ROUTE, SOCK_RAW | SOCK_CLOEXEC | SOCK_NONBLOCK, 0);
#else
	int s, flags;

	if ((s = socket(PF_ROUTE, SOCK_RAW, 0)) == -1)
		return -1;
	if ((flags = fcntl(s, F_GETFD, 0)) == -1 ||
	    fcntl(s, F_SETFD, flags | FD_CLOEXEC) == -1)
	{
		close(s);
	        return -1;
	}
	if ((flags = fcntl(s, F_GETFL, 0)) == -1 ||
	    fcntl(s, F_SETFL, flags | O_NONBLOCK) == -1)
	{
		close(s);
	        return -1;
	}
	return s;
#endif
}

int
getifssid(const char *ifname, char *ssid)
{
	int s, retval = -1;
#if defined(SIOCG80211NWID)
	struct ifreq ifr;
	struct ieee80211_nwid nwid;
#elif defined(IEEE80211_IOC_SSID)
	struct ieee80211req ireq;
	char nwid[IEEE80211_NWID_LEN + 1];
#endif

	if ((s = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
		return -1;

#if defined(SIOCG80211NWID) /* NetBSD */
	memset(&ifr, 0, sizeof(ifr));
	strlcpy(ifr.ifr_name, ifname, sizeof(ifr.ifr_name));
	memset(&nwid, 0, sizeof(nwid));
	ifr.ifr_data = (void *)&nwid;
	if (ioctl(s, SIOCG80211NWID, &ifr) == 0) {
		retval = nwid.i_len;
		if (ssid) {
			memcpy(ssid, nwid.i_nwid, nwid.i_len);
			ssid[nwid.i_len] = '\0';
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
		retval = ireq.i_len;
		if (ssid) {
			memcpy(ssid, nwid, ireq.i_len);
			ssid[ireq.i_len] = '\0';
		}
	}
#endif

	close(s);
	return retval;
}

/*
 * FreeBSD allows for Virtual Access Points
 * We need to check if the interface is a Virtual Interface Master
 * and if so, don't use it.
 * This check is made by virtue of being a IEEE80211 device but
 * returning the SSID gives an error.
 */
int
if_vimaster(const char *ifname)
{
	int s, r;
	struct ifmediareq ifmr;

	if ((s = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
		return -1;
	memset(&ifmr, 0, sizeof(ifmr));
	strlcpy(ifmr.ifm_name, ifname, sizeof(ifmr.ifm_name));
	r = ioctl(s, SIOCGIFMEDIA, &ifmr);
	close(s);
	if (r == -1)
		return -1;
	if (ifmr.ifm_status & IFM_AVALID &&
	    IFM_TYPE(ifmr.ifm_active) == IFM_IEEE80211)
	{
		if (getifssid(ifname, NULL) == -1)
			return 1;
	}
	return 0;
}

#ifdef INET
int
if_address(const struct interface *iface, const struct in_addr *address,
    const struct in_addr *netmask, const struct in_addr *broadcast,
    int action)
{
	int s, r;
	struct ifaliasreq ifa;
	union {
		struct sockaddr *sa;
		struct sockaddr_in *sin;
	} _s;

	if ((s = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
		return -1;

	memset(&ifa, 0, sizeof(ifa));
	strlcpy(ifa.ifra_name, iface->name, sizeof(ifa.ifra_name));

#define ADDADDR(_var, _addr) {						      \
		_s.sa = &_var;						      \
		_s.sin->sin_family = AF_INET;				      \
		_s.sin->sin_len = sizeof(*_s.sin);			      \
		memcpy(&_s.sin->sin_addr, _addr, sizeof(_s.sin->sin_addr));   \
	}

	ADDADDR(ifa.ifra_addr, address);
	ADDADDR(ifa.ifra_mask, netmask);
	if (action >= 0 && broadcast) {
		ADDADDR(ifa.ifra_broadaddr, broadcast);
	}
#undef ADDADDR

	r = ioctl(s,
	    action < 0 ? SIOCDIFADDR :
	    action == 2 ? SIOCSIFADDR :  SIOCAIFADDR, &ifa);
	close(s);
	return r;
}

int
if_route(const struct rt *rt, int action)
{
	const struct dhcp_state *state;
	union sockunion {
		struct sockaddr sa;
		struct sockaddr_in sin;
		struct sockaddr_dl sdl;
		struct sockaddr_storage ss;
	} su;
	struct rtm
	{
		struct rt_msghdr hdr;
		char buffer[sizeof(su) * 4];
	} rtm;
	char *bp = rtm.buffer;
	size_t l;
	int s, retval;

	if ((s = socket(PF_ROUTE, SOCK_RAW, 0)) == -1)
		return -1;

#define ADDSU {								      \
		l = RT_ROUNDUP(su.sa.sa_len);				      \
		memcpy(bp, &su, l);					      \
		bp += l;						      \
	}
#define ADDADDR(addr) {							      \
		memset(&su, 0, sizeof(su));				      \
		su.sin.sin_family = AF_INET;				      \
		su.sin.sin_len = sizeof(su.sin);			      \
		(&su.sin)->sin_addr = *addr;				      \
		ADDSU;							      \
	}

	state = D_CSTATE(rt->iface);
	memset(&rtm, 0, sizeof(rtm));
	rtm.hdr.rtm_version = RTM_VERSION;
	rtm.hdr.rtm_seq = 1;
	if (action == 0)
		rtm.hdr.rtm_type = RTM_CHANGE;
	else if (action > 0)
		rtm.hdr.rtm_type = RTM_ADD;
	else
		rtm.hdr.rtm_type = RTM_DELETE;
	rtm.hdr.rtm_flags = RTF_UP;
	/* None interface subnet routes are static. */
	if (rt->gate.s_addr != INADDR_ANY ||
	    rt->net.s_addr != state->net.s_addr ||
	    rt->dest.s_addr != (state->addr.s_addr & state->net.s_addr))
		rtm.hdr.rtm_flags |= RTF_STATIC;
	rtm.hdr.rtm_addrs = RTA_DST | RTA_GATEWAY;
	if (rt->dest.s_addr == rt->gate.s_addr &&
	    rt->net.s_addr == INADDR_BROADCAST)
		rtm.hdr.rtm_flags |= RTF_HOST;
	else if (rt->gate.s_addr == htonl(INADDR_LOOPBACK) &&
	    rt->net.s_addr == INADDR_BROADCAST)
		rtm.hdr.rtm_flags |= RTF_HOST | RTF_GATEWAY;
	else {
		rtm.hdr.rtm_addrs |= RTA_NETMASK;
		if (rtm.hdr.rtm_flags & RTF_STATIC)
			rtm.hdr.rtm_flags |= RTF_GATEWAY;
		if (action >= 0)
			rtm.hdr.rtm_addrs |= RTA_IFA;
	}

	ADDADDR(&rt->dest);
	if ((rtm.hdr.rtm_flags & RTF_HOST &&
	    rt->gate.s_addr != htonl(INADDR_LOOPBACK)) ||
	    !(rtm.hdr.rtm_flags & RTF_STATIC))
	{
		/* Make us a link layer socket for the host gateway */
		memset(&su, 0, sizeof(su));
		su.sdl.sdl_len = sizeof(struct sockaddr_dl);
		link_addr(rt->iface->name, &su.sdl);
		ADDSU;
	} else
		ADDADDR(&rt->gate);

	if (rtm.hdr.rtm_addrs & RTA_NETMASK)
		ADDADDR(&rt->net);

	if (rtm.hdr.rtm_addrs & RTA_IFP) {
		/* Make us a link layer socket for the host gateway */
		memset(&su, 0, sizeof(su));
		su.sdl.sdl_len = sizeof(struct sockaddr_dl);
		link_addr(rt->iface->name, &su.sdl);
		ADDSU;
	}

	if (rtm.hdr.rtm_addrs & RTA_IFA)
		ADDADDR(&state->addr);

#undef ADDADDR
#undef ADDSU

	rtm.hdr.rtm_msglen = l = bp - (char *)&rtm;

	retval = write(s, &rtm, l) == -1 ? -1 : 0;
	close(s);
	return retval;
}
#endif

#ifdef INET6
int
if_address6(const struct ipv6_addr *a, int action)
{
	int s, r;
	struct in6_aliasreq ifa;
	struct in6_addr mask;

	if ((s = socket(AF_INET6, SOCK_DGRAM, 0)) == -1)
		return -1;

	memset(&ifa, 0, sizeof(ifa));
	strlcpy(ifa.ifra_name, a->iface->name, sizeof(ifa.ifra_name));
	/*
	 * We should not set IN6_IFF_TENTATIVE as the kernel should be
	 * able to work out if it's a new address or not.
	 *
	 * We should set IN6_IFF_AUTOCONF, but the kernel won't let us.
	 * This is probably a safety measure, but still it's not entirely right
	 * either.
	 */
#if 0
	if (a->autoconf)
		ifa.ifra_flags |= IN6_IFF_AUTOCONF;
#endif

#define ADDADDR(v, addr) {						      \
		(v)->sin6_family = AF_INET6;				      \
		(v)->sin6_len = sizeof(*v);				      \
		(v)->sin6_addr = *addr;					      \
	}

	ADDADDR(&ifa.ifra_addr, &a->addr);
	ipv6_mask(&mask, a->prefix_len);
	ADDADDR(&ifa.ifra_prefixmask, &mask);
	ifa.ifra_lifetime.ia6t_vltime = a->prefix_vltime;
	ifa.ifra_lifetime.ia6t_pltime = a->prefix_pltime;
#undef ADDADDR

	r = ioctl(s, action < 0 ? SIOCDIFADDR_IN6 : SIOCAIFADDR_IN6, &ifa);
	close(s);
	return r;
}

int
if_route6(const struct rt6 *rt, int action)
{
	union sockunion {
		struct sockaddr sa;
		struct sockaddr_in6 sin;
		struct sockaddr_dl sdl;
		struct sockaddr_storage ss;
	} su;
	struct rtm
	{
		struct rt_msghdr hdr;
		char buffer[sizeof(su) * 4];
	} rtm;
	char *bp = rtm.buffer;
	size_t l;
	int s, retval;
	const struct ipv6_addr_l *lla;

	if ((s = socket(PF_ROUTE, SOCK_RAW, 0)) == -1)
		return -1;

/* KAME based systems want to store the scope inside the sin6_addr
 * for link local addreses */
#ifdef __KAME__
#define SCOPE {								      \
		if (IN6_IS_ADDR_LINKLOCAL(&su.sin.sin6_addr)) {		      \
			uint16_t scope = htons(su.sin.sin6_scope_id);	      \
			memcpy(&su.sin.sin6_addr.s6_addr[2], &scope,	      \
			    sizeof(scope));				      \
			su.sin.sin6_scope_id = 0;			      \
		}							      \
	}
#else
#define SCOPE
#endif

#define ADDSU {								      \
		l = RT_ROUNDUP(su.sa.sa_len);				      \
		memcpy(bp, &su, l);					      \
		bp += l;						      \
	}
#define ADDADDRS(addr, scope) {						      \
		memset(&su, 0, sizeof(su));				      \
		su.sin.sin6_family = AF_INET6;				      \
		su.sin.sin6_len = sizeof(su.sin);			      \
		(&su.sin)->sin6_addr = *addr;				      \
		su.sin.sin6_scope_id = scope;				      \
		SCOPE;							      \
		ADDSU;							      \
	}
#define ADDADDR(addr) ADDADDRS(addr, 0)

	memset(&rtm, 0, sizeof(rtm));
	rtm.hdr.rtm_version = RTM_VERSION;
	rtm.hdr.rtm_seq = 1;
	if (action == 0)
		rtm.hdr.rtm_type = RTM_CHANGE;
	else if (action > 0)
		rtm.hdr.rtm_type = RTM_ADD;
	else
		rtm.hdr.rtm_type = RTM_DELETE;

	rtm.hdr.rtm_flags = RTF_UP;
	/* None interface subnet routes are static. */
	if (IN6_IS_ADDR_UNSPECIFIED(&rt->dest) &&
	    IN6_IS_ADDR_UNSPECIFIED(&rt->net))
		rtm.hdr.rtm_flags |= RTF_GATEWAY;
#ifdef RTF_CLONING
	else
		rtm.hdr.rtm_flags |= RTF_CLONING;
#endif

	rtm.hdr.rtm_addrs = RTA_DST | RTA_GATEWAY | RTA_NETMASK;
	if (action >= 0)
		rtm.hdr.rtm_addrs |= RTA_IFP | RTA_IFA;

	ADDADDR(&rt->dest);
	if (!(rtm.hdr.rtm_flags & RTF_GATEWAY)) {
		lla = ipv6_linklocal(rt->iface);
		if (lla == NULL) /* unlikely as we need a LL to get here */
			return -1;
		ADDADDRS(&lla->addr, rt->iface->index);
	} else {
		lla = NULL;
		ADDADDRS(&rt->gate, rt->iface->index);
	}

	if (rtm.hdr.rtm_addrs & RTA_NETMASK) {
		if (rtm.hdr.rtm_flags & RTF_GATEWAY) {
			memset(&su, 0, sizeof(su));
			su.sin.sin6_family = AF_INET6;
			ADDSU;
		} else
			ADDADDR(&rt->net);
	}

	if (rtm.hdr.rtm_addrs & RTA_IFP) {
		/* Make us a link layer socket for the host gateway */
		memset(&su, 0, sizeof(su));
		su.sdl.sdl_len = sizeof(struct sockaddr_dl);
		link_addr(rt->iface->name, &su.sdl);
		ADDSU;
	}

	if (rtm.hdr.rtm_addrs & RTA_IFA) {
		if (lla == NULL) {
			lla = ipv6_linklocal(rt->iface);
			if (lla == NULL) /* unlikely */
				return -1;
		}
		ADDADDRS(&lla->addr, rt->iface->index);
	}

#undef ADDADDR
#undef ADDSU
#undef SCOPE

	if (action >= 0 && rt->mtu) {
		rtm.hdr.rtm_inits |= RTV_MTU;
		rtm.hdr.rtm_rmx.rmx_mtu = rt->mtu;
	}

	rtm.hdr.rtm_msglen = l = bp - (char *)&rtm;

	retval = write(s, &rtm, l) == -1 ? -1 : 0;
	close(s);
	return retval;
}
#endif

static void
get_addrs(int type, char *cp, struct sockaddr **sa)
{
	int i;

	for (i = 0; i < RTAX_MAX; i++) {
		if (type & (1 << i)) {
			sa[i] = (struct sockaddr *)cp;
#ifdef DEBUG
			printf ("got %d %d %s\n", i, sa[i]->sa_family,
			    inet_ntoa(((struct sockaddr_in *)sa[i])->
				sin_addr));
#endif
			RT_ADVANCE(cp, sa[i]);
		} else
			sa[i] = NULL;
	}
}

#ifdef INET6
int
in6_addr_flags(const char *ifname, const struct in6_addr *addr)
{
	int s, flags;
	struct in6_ifreq ifr6;

	s = socket(AF_INET6, SOCK_DGRAM, 0);
	flags = -1;
	if (s != -1) {
		memset(&ifr6, 0, sizeof(ifr6));
		strncpy(ifr6.ifr_name, ifname, sizeof(ifr6.ifr_name));
		ifr6.ifr_addr.sin6_family = AF_INET6;
		ifr6.ifr_addr.sin6_addr = *addr;
		if (ioctl(s, SIOCGIFAFLAG_IN6, &ifr6) != -1)
			flags = ifr6.ifr_ifru.ifru_flags6;
		close(s);
	}
	return flags;
}
#endif

int
manage_link(struct dhcpcd_ctx *ctx)
{
	/* route and ifwatchd like a msg buf size of 2048 */
	char msg[2048], *p, *e, *cp, ifname[IF_NAMESIZE];
	ssize_t bytes;
	struct rt_msghdr *rtm;
	struct if_announcemsghdr *ifan;
	struct if_msghdr *ifm;
	struct ifa_msghdr *ifam;
	struct sockaddr *sa, *rti_info[RTAX_MAX];
	int len;
	struct sockaddr_dl sdl;
#ifdef INET
	struct rt rt;
#endif
#if defined(INET6) && !defined(LISTEN_DAD)
	struct in6_addr ia6;
	struct sockaddr_in6 *sin6;
	int ifa_flags;
#endif

	for (;;) {
		bytes = read(ctx->link_fd, msg, sizeof(msg));
		if (bytes == -1) {
			if (errno == EAGAIN)
				return 0;
			if (errno == EINTR)
				continue;
			return -1;
		}
		e = msg + bytes;
		for (p = msg; p < e; p += rtm->rtm_msglen) {
			rtm = (struct rt_msghdr *)(void *)p;
			// Ignore messages generated by us
			if (rtm->rtm_pid == getpid())
				break;
			switch(rtm->rtm_type) {
#ifdef RTM_IFANNOUNCE
			case RTM_IFANNOUNCE:
				ifan = (struct if_announcemsghdr *)(void *)p;
				switch(ifan->ifan_what) {
				case IFAN_ARRIVAL:
					handle_interface(ctx, 1,
					    ifan->ifan_name);
					break;
				case IFAN_DEPARTURE:
					handle_interface(ctx, -1,
					    ifan->ifan_name);
					break;
				}
				break;
#endif
			case RTM_IFINFO:
				ifm = (struct if_msghdr *)(void *)p;
				memset(ifname, 0, sizeof(ifname));
				if (!(if_indextoname(ifm->ifm_index, ifname)))
					break;
				switch (ifm->ifm_data.ifi_link_state) {
				case LINK_STATE_DOWN:
					len = LINK_DOWN;
					break;
				case LINK_STATE_UP:
					len = LINK_UP;
					break;
				default:
					/* handle_carrier will re-load
					 * the interface flags and check for
					 * IFF_RUNNING as some drivers that
					 * don't handle link state also don't
					 * set IFF_RUNNING when this routing
					 * message is generated.
					 * As such, it is a race ...*/
					len = LINK_UNKNOWN;
					break;
				}
				handle_carrier(ctx, len,
				    ifm->ifm_flags, ifname);
				break;
			case RTM_DELETE:
				if (~rtm->rtm_addrs &
				    (RTA_DST | RTA_GATEWAY | RTA_NETMASK))
					break;
				cp = (char *)(void *)(rtm + 1);
				sa = (struct sockaddr *)(void *)cp;
				if (sa->sa_family != AF_INET)
					break;
#ifdef INET
				get_addrs(rtm->rtm_addrs, cp, rti_info);
				memset(&rt, 0, sizeof(rt));
				rt.iface = NULL;
				COPYOUT(rt.dest, rti_info[RTAX_DST]);
				COPYOUT(rt.net, rti_info[RTAX_NETMASK]);
				COPYOUT(rt.gate, rti_info[RTAX_GATEWAY]);
				ipv4_routedeleted(ctx, &rt);
#endif
				break;
#ifdef RTM_CHGADDR
			case RTM_CHGADDR:	/* FALLTHROUGH */
#endif
			case RTM_DELADDR:	/* FALLTHROUGH */
			case RTM_NEWADDR:
				ifam = (struct ifa_msghdr *)(void *)p;
				if (!if_indextoname(ifam->ifam_index, ifname))
					break;
				cp = (char *)(void *)(ifam + 1);
				get_addrs(ifam->ifam_addrs, cp, rti_info);
				if (rti_info[RTAX_IFA] == NULL)
					break;
				switch (rti_info[RTAX_IFA]->sa_family) {
				case AF_LINK:
#ifdef RTM_CHGADDR
					if (rtm->rtm_type != RTM_CHGADDR)
						break;
#else
					if (rtm->rtm_type != RTM_NEWADDR)
						break;
#endif
					memcpy(&sdl, rti_info[RTAX_IFA],
					    rti_info[RTAX_IFA]->sa_len);
					handle_hwaddr(ctx, ifname,
					    (const unsigned char*)CLLADDR(&sdl),
					    sdl.sdl_alen);
					break;
#ifdef INET
				case AF_INET:
				case 255: /* FIXME: Why 255? */
					COPYOUT(rt.dest, rti_info[RTAX_IFA]);
					COPYOUT(rt.net, rti_info[RTAX_NETMASK]);
					COPYOUT(rt.gate, rti_info[RTAX_BRD]);
					ipv4_handleifa(ctx, rtm->rtm_type,
					    NULL, ifname,
					    &rt.dest, &rt.net, &rt.gate);
					break;
#endif
#if defined(INET6) && !defined(LISTEN_DAD)
				case AF_INET6:
					sin6 = (struct sockaddr_in6*)(void *)
					    rti_info[RTAX_IFA];
					memcpy(ia6.s6_addr,
					    sin6->sin6_addr.s6_addr,
					    sizeof(ia6.s6_addr));
					if (rtm->rtm_type == RTM_NEWADDR) {
						ifa_flags = in6_addr_flags(
								ifname,
								&ia6);
						if (ifa_flags == -1)
							break;
					} else
						ifa_flags = 0;
					ipv6_handleifa(ctx, rtm->rtm_type, NULL,
					    ifname, &ia6, ifa_flags);
					break;
#endif
				}
				break;
			}
		}
	}
}
