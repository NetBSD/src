/* 
 * dhcpcd - DHCP client daemon
 * Copyright (c) 2006-2009 Roy Marples <roy@marples.name>
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
#include <net/route.h>
#include <netinet/in.h>
#ifdef __DragonFly__
#  include <netproto/802_11/ieee80211_ioctl.h>
#else
#  include <net80211/ieee80211_ioctl.h>
#endif

#include <errno.h>
#include <fnmatch.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>

#include "config.h"
#include "common.h"
#include "configure.h"
#include "dhcp.h"
#include "if-options.h"
#include "net.h"

#define ROUNDUP(a)							      \
	((a) > 0 ? (1 + (((a) - 1) | (sizeof(long) - 1))) : sizeof(long))
#define ADVANCE(x, n) (x += ROUNDUP((n)->sa_len))

/* FIXME: Why do we need to check for sa_family 255 */
#define COPYOUT(sin, sa)						      \
	sin.s_addr = ((sa) != NULL && ((sa)->sa_family == AF_INET ||	      \
		(sa)->sa_family == 255))				      \
	    ?								      \
	    (((struct sockaddr_in *)(void *)sa)->sin_addr).s_addr : 0

static int r_fd = -1;

int
init_sockets(void)
{
	if ((socket_afnet = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
		return -1;
	set_cloexec(socket_afnet);
	if ((r_fd = socket(PF_ROUTE, SOCK_RAW, 0)) == -1)
		return -1;
	set_cloexec(r_fd);
	return 0;
}

int
getifssid(const char *ifname, char *ssid)
{
	int retval = -1;
#if defined(SIOCG80211NWID)
	struct ifreq ifr;
	struct ieee80211_nwid nwid;
#elif defined(IEEE80211_IOC_SSID)
	struct ieee80211req ireq;
	char nwid[IEEE80211_NWID_LEN + 1];
#endif

#if defined(SIOCG80211NWID) /* NetBSD */
	memset(&ifr, 0, sizeof(ifr));
	strlcpy(ifr.ifr_name, ifname, sizeof(ifr.ifr_name));
	memset(&nwid, 0, sizeof(nwid));
	ifr.ifr_data = (void *)&nwid;
	if (ioctl(socket_afnet, SIOCG80211NWID, &ifr) == 0) {
		retval = nwid.i_len;
		memcpy(ssid, nwid.i_nwid, nwid.i_len);
		ssid[nwid.i_len] = '\0';
	}
#elif defined(IEEE80211_IOC_SSID) /* FreeBSD */
	memset(&ireq, 0, sizeof(ireq));
	strlcpy(ireq.i_name, ifname, sizeof(ireq.i_name));
	ireq.i_type = IEEE80211_IOC_SSID;
	ireq.i_val = -1;
	ireq.i_data = &nwid;
	if (ioctl(socket_afnet, SIOCG80211, &ireq) == 0) {
		retval = ireq.i_len;
		memcpy(ssid, nwid, ireq.i_len);
		ssid[ireq.i_len] = '\0';
	}
#endif
	return retval;
}

int
if_address(const struct interface *iface, const struct in_addr *address,
    const struct in_addr *netmask, const struct in_addr *broadcast,
    int action)
{
	int retval;
	struct ifaliasreq ifa;
	union {
		struct sockaddr *sa;
		struct sockaddr_in *sin;
	} _s;

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

	if (action < 0)
		retval = ioctl(socket_afnet, SIOCDIFADDR, &ifa);
	else
		retval = ioctl(socket_afnet, SIOCAIFADDR, &ifa);
	return retval;
}

/* ARGSUSED4 */
int
if_route(const struct interface *iface, const struct in_addr *dest,
    const struct in_addr *net, const struct in_addr *gate,
    _unused int metric, int action)
{
	union sockunion {
		struct sockaddr sa;
		struct sockaddr_in sin;
#ifdef INET6
		struct sockaddr_in6 sin6;
#endif
		struct sockaddr_dl sdl;
		struct sockaddr_storage ss;
	} su;
	struct rtm 
	{
		struct rt_msghdr hdr;
		char buffer[sizeof(su) * 4];
	} rtm;
	char *bp = rtm.buffer, *p;
	size_t l;
	int retval = 0;

#define ADDSU(_su) {							      \
		l = ROUNDUP(_su.sa.sa_len);				      \
		memcpy(bp, &(_su), l);					      \
		bp += l;						      \
	}
#define ADDADDR(_a) {							      \
		memset (&su, 0, sizeof(su));				      \
		su.sin.sin_family = AF_INET;				      \
		su.sin.sin_len = sizeof(su.sin);			      \
		memcpy (&su.sin.sin_addr, _a, sizeof(su.sin.sin_addr));	      \
		ADDSU(su);						      \
	}

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
	if (gate->s_addr != INADDR_ANY ||
	    net->s_addr != iface->net.s_addr ||
	    dest->s_addr != (iface->addr.s_addr & iface->net.s_addr))
		rtm.hdr.rtm_flags |= RTF_STATIC;
	rtm.hdr.rtm_addrs = RTA_DST | RTA_GATEWAY;
	if (dest->s_addr == gate->s_addr && net->s_addr == INADDR_BROADCAST)
		rtm.hdr.rtm_flags |= RTF_HOST;
	else {
		rtm.hdr.rtm_addrs |= RTA_NETMASK;
		if (rtm.hdr.rtm_flags & RTF_STATIC)
			rtm.hdr.rtm_flags |= RTF_GATEWAY;
		if (action >= 0)
			rtm.hdr.rtm_addrs |= RTA_IFA;
	}

	ADDADDR(dest);
	if (rtm.hdr.rtm_flags & RTF_HOST ||
	    !(rtm.hdr.rtm_flags & RTF_STATIC))
	{
		/* Make us a link layer socket for the host gateway */
		memset(&su, 0, sizeof(su));
		su.sdl.sdl_len = sizeof(struct sockaddr_dl);
		link_addr(iface->name, &su.sdl);
		ADDSU(su);
	} else
		ADDADDR(gate);

	if (rtm.hdr.rtm_addrs & RTA_NETMASK) {
		/* Ensure that netmask is set correctly */
		memset(&su, 0, sizeof(su));
		su.sin.sin_family = AF_INET;
		su.sin.sin_len = sizeof(su.sin);
		memcpy(&su.sin.sin_addr, &net->s_addr, sizeof(su.sin.sin_addr));
		p = su.sa.sa_len + (char *)&su;
		for (su.sa.sa_len = 0; p > (char *)&su;)
			if (*--p != 0) {
				su.sa.sa_len = 1 + p - (char *)&su;
				break;
			}
		ADDSU(su);
	}

	if (rtm.hdr.rtm_addrs & RTA_IFA)
		ADDADDR(&iface->addr);

	rtm.hdr.rtm_msglen = l = bp - (char *)&rtm;
	if (write(r_fd, &rtm, l) == -1)
		retval = -1;
	return retval;
}

int
open_link_socket(void)
{
	int fd;

	fd = socket(PF_ROUTE, SOCK_RAW, 0);
	if (fd != -1) {
		set_cloexec(fd);
		set_nonblock(fd);
	}
	return fd;
}

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
			ADVANCE(cp, sa[i]);
		} else
			sa[i] = NULL;
	}
}

#define BUFFER_LEN	2048
int
manage_link(int fd)
{
	char buffer[2048], *p, *e, *cp;
	char ifname[IF_NAMESIZE];
	ssize_t bytes;
	struct rt_msghdr *rtm;
	struct if_announcemsghdr *ifan;
	struct if_msghdr *ifm;
	struct ifa_msghdr *ifam;
	struct rt rt;
	struct sockaddr *sa, *rti_info[RTAX_MAX];

	for (;;) {
		bytes = read(fd, buffer, BUFFER_LEN);
		if (bytes == -1) {
			if (errno == EAGAIN)
				return 0;
			if (errno == EINTR)
				continue;
			return -1;
		}
		e = buffer + bytes;
		for (p = buffer; p < e; p += rtm->rtm_msglen) {
			rtm = (struct rt_msghdr *)(void *)p;
			switch(rtm->rtm_type) {
			case RTM_IFANNOUNCE:
				ifan = (struct if_announcemsghdr *)(void *)p;
				switch(ifan->ifan_what) {
				case IFAN_ARRIVAL:
					handle_interface(1, ifan->ifan_name);
					break;
				case IFAN_DEPARTURE:
					handle_interface(-1, ifan->ifan_name);
					break;
				}
				break;
			case RTM_IFINFO:
				ifm = (struct if_msghdr *)(void *)p;
				memset(ifname, 0, sizeof(ifname));
				if (if_indextoname(ifm->ifm_index, ifname))
					handle_interface(0, ifname);
				break;
			case RTM_DELETE:
				if (!(rtm->rtm_addrs & RTA_DST) ||
				    !(rtm->rtm_addrs & RTA_GATEWAY) ||
				    !(rtm->rtm_addrs & RTA_NETMASK))
					break;
				if (rtm->rtm_pid == getpid())
					break;
				cp = (char *)(void *)(rtm + 1);
				sa = (struct sockaddr *)(void *)cp;
				if (sa->sa_family != AF_INET)
					break;
				get_addrs(rtm->rtm_addrs, cp, rti_info);
				rt.iface = NULL;
				rt.next = NULL;
				COPYOUT(rt.dest, rti_info[RTAX_DST]);
				COPYOUT(rt.net, rti_info[RTAX_NETMASK]);
				COPYOUT(rt.gate, rti_info[RTAX_GATEWAY]);
				route_deleted(&rt);
				break;
			case RTM_DELADDR:
			case RTM_NEWADDR:
				ifam = (struct ifa_msghdr *)(void *)p;
				cp = (char *)(void *)(ifam + 1);
				get_addrs(ifam->ifam_addrs, cp, rti_info);
				COPYOUT(rt.dest, rti_info[RTAX_IFA]);
				COPYOUT(rt.net, rti_info[RTAX_NETMASK]);
				COPYOUT(rt.gate, rti_info[RTAX_BRD]);
				if (if_indextoname(ifam->ifam_index, ifname))
					handle_ifa(rtm->rtm_type, ifname,
					    &rt.dest, &rt.net, &rt.gate);
				break;
			}
		}
	}
}
