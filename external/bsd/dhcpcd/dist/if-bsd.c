#include <sys/cdefs.h>
 __RCSID("$NetBSD: if-bsd.c,v 1.10 2014/09/27 01:17:34 roy Exp $");

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
#include <sys/time.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <sys/utsname.h>

#include <arpa/inet.h>
#include <net/bpf.h>
#include <net/if.h>
#include <net/if_dl.h>
#ifdef __FreeBSD__ /* Needed so that including netinet6/in6_var.h works */
#  include <net/if_var.h>
#endif
#include <net/if_media.h>
#include <net/route.h>
#include <netinet/if_ether.h>
#include <netinet/in.h>
#include <netinet6/in6_var.h>
#include <netinet6/nd6.h>
#ifdef __DragonFly__
#  include <netproto/802_11/ieee80211_ioctl.h>
#elif __APPLE__
  /* FIXME: Add apple includes so we can work out SSID */
#else
#  include <net80211/ieee80211.h>
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
#include "if.h"
#include "if-options.h"
#include "ipv4.h"
#include "ipv6.h"
#include "ipv6nd.h"

#include "bpf-filter.h"

#ifndef RT_ROUNDUP
#define RT_ROUNDUP(a)							      \
	((a) > 0 ? (1 + (((a) - 1) | (sizeof(long) - 1))) : sizeof(long))
#define RT_ADVANCE(x, n) (x += RT_ROUNDUP((n)->sa_len))
#endif

#define COPYOUT(sin, sa)						      \
	if ((sa) && (sa)->sa_family == AF_INET)				      \
		(sin) = ((struct sockaddr_in*)(void *)(sa))->sin_addr

#define COPYOUT6(sin, sa)						      \
	if ((sa) && (sa)->sa_family == AF_INET6)			      \
		(sin) = ((struct sockaddr_in6*)(void *)(sa))->sin6_addr

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
if_openlinksocket(void)
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

static int
if_getssid1(const char *ifname, uint8_t *ssid)
{
	int s, retval = -1;
#if defined(SIOCG80211NWID)
	struct ifreq ifr;
	struct ieee80211_nwid nwid;
#elif defined(IEEE80211_IOC_SSID)
	struct ieee80211req ireq;
	char nwid[IEEE80211_NWID_LEN + 1];
#endif

	if ((s = socket(PF_INET, SOCK_DGRAM, 0)) == -1)
		return -1;

#if defined(SIOCG80211NWID) /* NetBSD */
	memset(&ifr, 0, sizeof(ifr));
	strlcpy(ifr.ifr_name, ifname, sizeof(ifr.ifr_name));
	memset(&nwid, 0, sizeof(nwid));
	ifr.ifr_data = (void *)&nwid;
	if (ioctl(s, SIOCG80211NWID, &ifr) == 0) {
		if (ssid == NULL)
			retval = nwid.i_len;
		else if (nwid.i_len > IF_SSIDSIZE) {
			errno = ENOBUFS;
			retval = -1;
		} else {
			retval = nwid.i_len;
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
		if (ssid == NULL)
			retval = ireq.i_len;
		else if (ireq.i_len > IF_SSIDSIZE) {
			errno = ENOBUFS;
			retval = -1;
		} else  {
			retval = ireq.i_len;
			memcpy(ssid, nwid, ireq.i_len);
			ssid[ireq.i_len] = '\0';
		}
	}
#endif

	close(s);
	return retval;
}

int
if_getssid(struct interface *ifp)
{
	int r;

	r = if_getssid1(ifp->name, ifp->ssid);
	if (r != -1)
		ifp->ssid_len = (unsigned int)r;
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
if_vimaster(const char *ifname)
{
	int s, r;
	struct ifmediareq ifmr;

	if ((s = socket(PF_INET, SOCK_DGRAM, 0)) == -1)
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
		if (if_getssid1(ifname, NULL) == -1)
			return 1;
	}
	return 0;
}

#ifdef INET
int
if_openrawsocket(struct interface *ifp, int protocol)
{
	struct dhcp_state *state;
	int fd = -1;
	struct ifreq ifr;
	int ibuf_len = 0;
	size_t buf_len;
	struct bpf_version pv;
	struct bpf_program pf;
#ifdef BIOCIMMEDIATE
	int flags;
#endif
#ifdef _PATH_BPF
	fd = open(_PATH_BPF, O_RDWR | O_CLOEXEC | O_NONBLOCK);
#else
	char device[32];
	int n = 0;

	do {
		snprintf(device, sizeof(device), "/dev/bpf%d", n++);
		fd = open(device, O_RDWR | O_CLOEXEC | O_NONBLOCK);
	} while (fd == -1 && errno == EBUSY);
#endif

	if (fd == -1)
		return -1;

	state = D_STATE(ifp);

	memset(&pv, 0, sizeof(pv));
	if (ioctl(fd, BIOCVERSION, &pv) == -1)
		goto eexit;
	if (pv.bv_major != BPF_MAJOR_VERSION ||
	    pv.bv_minor < BPF_MINOR_VERSION) {
		syslog(LOG_ERR, "BPF version mismatch - recompile");
		goto eexit;
	}

	memset(&ifr, 0, sizeof(ifr));
	strlcpy(ifr.ifr_name, ifp->name, sizeof(ifr.ifr_name));
	if (ioctl(fd, BIOCSETIF, &ifr) == -1)
		goto eexit;

	/* Get the required BPF buffer length from the kernel. */
	if (ioctl(fd, BIOCGBLEN, &ibuf_len) == -1)
		goto eexit;
	buf_len = (size_t)ibuf_len;
	if (state->buffer_size != buf_len) {
		free(state->buffer);
		state->buffer = malloc(buf_len);
		if (state->buffer == NULL)
			goto eexit;
		state->buffer_size = buf_len;
		state->buffer_len = state->buffer_pos = 0;
	}

#ifdef BIOCIMMEDIATE
	flags = 1;
	if (ioctl(fd, BIOCIMMEDIATE, &flags) == -1)
		goto eexit;
#endif

	/* Install the DHCP filter */
	memset(&pf, 0, sizeof(pf));
	if (protocol == ETHERTYPE_ARP) {
		pf.bf_insns = UNCONST(arp_bpf_filter);
		pf.bf_len = arp_bpf_filter_len;
	} else {
		pf.bf_insns = UNCONST(dhcp_bpf_filter);
		pf.bf_len = dhcp_bpf_filter_len;
	}
	if (ioctl(fd, BIOCSETF, &pf) == -1)
		goto eexit;

	return fd;

eexit:
	free(state->buffer);
	state->buffer = NULL;
	close(fd);
	return -1;
}

ssize_t
if_sendrawpacket(const struct interface *ifp, int protocol,
    const void *data, size_t len)
{
	struct iovec iov[2];
	struct ether_header hw;
	int fd;
	const struct dhcp_state *state;

	memset(&hw, 0, ETHER_HDR_LEN);
	memset(&hw.ether_dhost, 0xff, ETHER_ADDR_LEN);
	hw.ether_type = htons(protocol);
	iov[0].iov_base = &hw;
	iov[0].iov_len = ETHER_HDR_LEN;
	iov[1].iov_base = UNCONST(data);
	iov[1].iov_len = len;
	state = D_CSTATE(ifp);
	if (protocol == ETHERTYPE_ARP)
		fd = state->arp_fd;
	else
		fd = state->raw_fd;
	return writev(fd, iov, 2);
}

/* BPF requires that we read the entire buffer.
 * So we pass the buffer in the API so we can loop on >1 packet. */
ssize_t
if_readrawpacket(struct interface *ifp, int protocol,
    void *data, size_t len, int *flags)
{
	int fd;
	struct bpf_hdr packet;
	ssize_t bytes;
	const unsigned char *payload;
	struct dhcp_state *state;

	state = D_STATE(ifp);
	if (protocol == ETHERTYPE_ARP)
		fd = state->arp_fd;
	else
		fd = state->raw_fd;

	*flags = 0;
	for (;;) {
		if (state->buffer_len == 0) {
			bytes = read(fd, state->buffer, state->buffer_size);
			if (bytes == -1 || bytes == 0)
				return bytes;
			state->buffer_len = (size_t)bytes;
			state->buffer_pos = 0;
		}
		bytes = -1;
		memcpy(&packet, state->buffer + state->buffer_pos,
		    sizeof(packet));
		if (packet.bh_caplen != packet.bh_datalen)
			goto next; /* Incomplete packet, drop. */
		if (state->buffer_pos + packet.bh_caplen + packet.bh_hdrlen >
		    state->buffer_len)
			goto next; /* Packet beyond buffer, drop. */
		payload = state->buffer + state->buffer_pos +
		    packet.bh_hdrlen + ETHER_HDR_LEN;
		bytes = (ssize_t)packet.bh_caplen - ETHER_HDR_LEN;
		if ((size_t)bytes > len)
			bytes = (ssize_t)len;
		memcpy(data, payload, (size_t)bytes);
next:
		state->buffer_pos += BPF_WORDALIGN(packet.bh_hdrlen +
		    packet.bh_caplen);
		if (state->buffer_pos >= state->buffer_len) {
			state->buffer_len = state->buffer_pos = 0;
			*flags |= RAW_EOF;
		}
		if (bytes != -1)
			return bytes;
	}
}

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

	if ((s = socket(PF_INET, SOCK_DGRAM, 0)) == -1)
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
		char buffer[sizeof(su) * 5];
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
	rtm.hdr.rtm_addrs = RTA_DST;
	if (action == 0)
		rtm.hdr.rtm_type = RTM_CHANGE;
	else if (action > 0) {
		rtm.hdr.rtm_type = RTM_ADD;
		rtm.hdr.rtm_addrs |= RTA_GATEWAY;
	} else
		rtm.hdr.rtm_type = RTM_DELETE;
	rtm.hdr.rtm_flags = RTF_UP;
#ifdef SIOCGIFPRIORITY
	rtm.hdr.rtm_priority = rt->metric;
#endif

	/* None interface subnet routes are static. */
	if (rt->gate.s_addr != INADDR_ANY ||
	    rt->net.s_addr != state->net.s_addr ||
	    rt->dest.s_addr != (state->addr.s_addr & state->net.s_addr))
		rtm.hdr.rtm_flags |= RTF_STATIC;
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
	if (rtm.hdr.rtm_addrs & RTA_GATEWAY) {
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
	}

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

	rtm.hdr.rtm_msglen = (unsigned short)(bp - (char *)&rtm);

	retval = write(s, &rtm, rtm.hdr.rtm_msglen) == -1 ? -1 : 0;
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

	if ((s = socket(PF_INET6, SOCK_DGRAM, 0)) == -1)
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
		char buffer[sizeof(su) * 5];
	} rtm;
	char *bp = rtm.buffer;
	size_t l;
	int s, retval;
	const struct ipv6_addr *lla;

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
	rtm.hdr.rtm_flags = RTF_UP | (int)rt->flags;
	rtm.hdr.rtm_addrs = RTA_DST | RTA_NETMASK;
#ifdef SIOCGIFPRIORITY
	rtm.hdr.rtm_priority = rt->metric;
#endif
	/* None interface subnet routes are static. */
	if (IN6_IS_ADDR_UNSPECIFIED(&rt->gate)) {
#ifdef RTF_CLONING
		rtm.hdr.rtm_flags |= RTF_CLONING;
#endif
	 } else
		rtm.hdr.rtm_flags |= RTF_GATEWAY | RTF_STATIC;

	if (action >= 0) {
		rtm.hdr.rtm_addrs |= RTA_GATEWAY;
		if (!(rtm.hdr.rtm_flags & RTF_REJECT))
			rtm.hdr.rtm_addrs |= RTA_IFP | RTA_IFA;
	}

	ADDADDR(&rt->dest);
	lla = NULL;
	if (rtm.hdr.rtm_addrs & RTA_GATEWAY) {
		if (IN6_IS_ADDR_UNSPECIFIED(&rt->gate)) {
			lla = ipv6_linklocal(rt->iface);
			if (lla == NULL) /* unlikely */
				return -1;
			ADDADDRS(&lla->addr, rt->iface->index);
		} else {
			ADDADDRS(&rt->gate,
			    IN6_ARE_ADDR_EQUAL(&rt->gate, &in6addr_loopback)
			    ? 0 : rt->iface->index);
		}
	}

	if (rtm.hdr.rtm_addrs & RTA_NETMASK)
		ADDADDR(&rt->net);

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

	rtm.hdr.rtm_msglen = (unsigned short)(bp - (char *)&rtm);

	retval = write(s, &rtm, rtm.hdr.rtm_msglen) == -1 ? -1 : 0;
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
if_addrflags6(const char *ifname, const struct in6_addr *addr)
{
	int s, flags;
	struct in6_ifreq ifr6;

	s = socket(PF_INET6, SOCK_DGRAM, 0);
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
if_managelink(struct dhcpcd_ctx *ctx)
{
	/* route and ifwatchd like a msg buf size of 2048 */
	char msg[2048], *p, *e, *cp;
	ssize_t bytes;
	struct rt_msghdr *rtm;
	struct if_announcemsghdr *ifan;
	struct if_msghdr *ifm;
	struct ifa_msghdr *ifam;
	struct sockaddr *sa, *rti_info[RTAX_MAX];
	int len;
	struct sockaddr_dl sdl;
	struct interface *ifp;
#ifdef INET
	struct rt rt;
#endif
#ifdef INET6
	struct rt6 rt6;
	struct in6_addr ia6;
	struct sockaddr_in6 *sin6;
	int ifa_flags;
#endif

	bytes = read(ctx->link_fd, msg, sizeof(msg));
	if (bytes == -1)
		return -1;
	if (bytes == 0)
		return 0;
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
				dhcpcd_handleinterface(ctx, 1,
				    ifan->ifan_name);
				break;
			case IFAN_DEPARTURE:
				dhcpcd_handleinterface(ctx, -1,
				    ifan->ifan_name);
				break;
			}
			break;
#endif
		case RTM_IFINFO:
			ifm = (struct if_msghdr *)(void *)p;
			if ((ifp = if_findindex(ctx, ifm->ifm_index)) == NULL)
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
			dhcpcd_handlecarrier(ctx, len,
			    (unsigned int)ifm->ifm_flags, ifp->name);
			break;
		case RTM_DELETE:
			if (~rtm->rtm_addrs &
			    (RTA_DST | RTA_GATEWAY | RTA_NETMASK))
				break;
			cp = (char *)(void *)(rtm + 1);
			sa = (struct sockaddr *)(void *)cp;
			get_addrs(rtm->rtm_addrs, cp, rti_info);
			switch (sa->sa_family) {
#ifdef INET
			case AF_INET:
				memset(&rt, 0, sizeof(rt));
				rt.iface = NULL;
				COPYOUT(rt.dest, rti_info[RTAX_DST]);
				COPYOUT(rt.net, rti_info[RTAX_NETMASK]);
				COPYOUT(rt.gate, rti_info[RTAX_GATEWAY]);
				ipv4_routedeleted(ctx, &rt);
				break;
#endif
#ifdef INET6
			case AF_INET6:
				memset(&rt6, 0, sizeof(rt6));
				rt6.iface = NULL;
				COPYOUT6(rt6.dest, rti_info[RTAX_DST]);
				COPYOUT6(rt6.net, rti_info[RTAX_NETMASK]);
				COPYOUT6(rt6.gate, rti_info[RTAX_GATEWAY]);
				ipv6_routedeleted(ctx, &rt6);
				break;
#endif
			}
#ifdef RTM_CHGADDR
		case RTM_CHGADDR:	/* FALLTHROUGH */
#endif
		case RTM_DELADDR:	/* FALLTHROUGH */
		case RTM_NEWADDR:
			ifam = (struct ifa_msghdr *)(void *)p;
			if ((ifp = if_findindex(ctx, ifam->ifam_index)) == NULL)
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
				dhcpcd_handlehwaddr(ctx, ifp->name,
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
				    NULL, ifp->name,
				    &rt.dest, &rt.net, &rt.gate);
				break;
#endif
#ifdef INET6
			case AF_INET6:
				sin6 = (struct sockaddr_in6*)(void *)
				    rti_info[RTAX_IFA];
				ia6 = sin6->sin6_addr;
				if (rtm->rtm_type == RTM_NEWADDR) {
					ifa_flags = if_addrflags6(ifp->name,
					    &ia6);
					if (ifa_flags == -1)
						break;
				} else
					ifa_flags = 0;
				ipv6_handleifa(ctx, rtm->rtm_type, NULL,
				    ifp->name, &ia6, ifa_flags);
				break;
#endif
			}
			break;
		}
	}

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

#define del_if_nd6_flag(ifname, flag) if_nd6_flag(ifname, flag, -1)
#define get_if_nd6_flag(ifname, flag) if_nd6_flag(ifname, flag,  0)
#define set_if_nd6_flag(ifname, flag) if_nd6_flag(ifname, flag,  1)
static int
if_nd6_flag(const char *ifname, unsigned int flag, int set)
{
	int s, error;
	struct in6_ndireq nd;
	unsigned int oflags;

	if ((s = socket(PF_INET6, SOCK_DGRAM, 0)) == -1)
		return -1;
	memset(&nd, 0, sizeof(nd));
	strlcpy(nd.ifname, ifname, sizeof(nd.ifname));
	if ((error = ioctl(s, SIOCGIFINFO_IN6, &nd)) == -1)
		goto eexit;
	if (set == 0) {
		close(s);
		return nd.ndi.flags & flag ? 1 : 0;
	}

	oflags = nd.ndi.flags;
	if (set == -1)
		nd.ndi.flags &= ~flag;
	else
		nd.ndi.flags |= flag;
	if (oflags == nd.ndi.flags)
		error = 0;
	else
		error = ioctl(s, SIOCSIFINFO_FLAGS, &nd);

eexit:
	close(s);
	return error;
}

int
if_nd6reachable(const char *ifname, struct in6_addr *addr)
{
	int s, flags;
	struct in6_nbrinfo nbi;

	if ((s = socket(PF_INET6, SOCK_DGRAM, 0)) < 0)
		return -1;

	memset(&nbi, 0, sizeof(nbi));
	strlcpy(nbi.ifname, ifname, sizeof(nbi.ifname));
	nbi.addr = *addr;
	if (ioctl(s, SIOCGNBRINFO_IN6, &nbi) == -1)
		flags = -1;
	else {
		flags = 0;
		switch(nbi.state) {
		case ND6_LLINFO_REACHABLE:
		case ND6_LLINFO_STALE:
		case ND6_LLINFO_DELAY:
		case ND6_LLINFO_PROBE:
			flags |= IPV6ND_REACHABLE;
			break;
		}
		if (nbi.isrouter)
			flags |= IPV6ND_ROUTER;
	}
	close(s);
	return flags;
}

static int
if_raflush(void)
{
	int s;
	char dummy[IFNAMSIZ + 8];

	s = socket(PF_INET6, SOCK_DGRAM, 0);
	if (s == -1)
		return -1;
	strlcpy(dummy, "lo0", sizeof(dummy));
	if (ioctl(s, SIOCSRTRFLUSH_IN6, (void *)&dummy) == -1)
		syslog(LOG_ERR, "SIOSRTRFLUSH_IN6: %m");
	if (ioctl(s, SIOCSPFXFLUSH_IN6, (void *)&dummy) == -1)
		syslog(LOG_ERR, "SIOSPFXFLUSH_IN6: %m");
	close(s);
	return 0;
}

int
if_checkipv6(struct dhcpcd_ctx *ctx, const struct interface *ifp, int own)
{
	int ra;

	if (ifp) {
#ifdef ND6_IFF_OVERRIDE_RTADV
		int override;
#endif

#ifdef ND6_IFF_IFDISABLED
		if (del_if_nd6_flag(ifp->name, ND6_IFF_IFDISABLED) == -1) {
			syslog(LOG_ERR,
			    "%s: del_if_nd6_flag: ND6_IFF_IFDISABLED: %m",
			    ifp->name);
			return -1;
		}
#endif

#ifdef ND6_IFF_PERFORMNUD
		if (set_if_nd6_flag(ifp->name, ND6_IFF_PERFORMNUD) == -1) {
			syslog(LOG_ERR,
			    "%s: set_if_nd6_flag: ND6_IFF_PERFORMNUD: %m",
			    ifp->name);
			return -1;
		}
#endif

#ifdef ND6_IFF_AUTO_LINKLOCAL
		if (own) {
			int all;

			all = get_if_nd6_flag(ifp->name,ND6_IFF_AUTO_LINKLOCAL);
			if (all == -1)
				syslog(LOG_ERR,
				    "%s: get_if_nd6_flag: "
				    "ND6_IFF_AUTO_LINKLOCAL: %m",
				    ifp->name);
			else if (all != 0) {
				syslog(LOG_DEBUG,
				    "%s: disabling Kernel IPv6 "
				    "auto link-local support",
				    ifp->name);
				if (del_if_nd6_flag(ifp->name,
				    ND6_IFF_AUTO_LINKLOCAL) == -1)
				{
					syslog(LOG_ERR,
					    "%s: del_if_nd6_flag: "
					    "ND6_IFF_AUTO_LINKLOCAL: %m",
					    ifp->name);
					return -1;
				}
			}
		}
#endif

#ifdef ND6_IFF_OVERRIDE_RTADV
		override = get_if_nd6_flag(ifp->name, ND6_IFF_OVERRIDE_RTADV);
		if (override == -1)
			syslog(LOG_ERR,
			    "%s: get_if_nd6_flag: ND6_IFF_OVERRIDE_RTADV: %m",
			    ifp->name);
		else if (override == 0 && !own)
			return 0;
#endif

#ifdef ND6_IFF_ACCEPT_RTADV
		ra = get_if_nd6_flag(ifp->name, ND6_IFF_ACCEPT_RTADV);
		if (ra == -1)
			syslog(LOG_ERR,
			    "%s: get_if_nd6_flag: ND6_IFF_ACCEPT_RTADV: %m",
			    ifp->name);
		else if (ra != 0 && own) {
			syslog(LOG_DEBUG,
			    "%s: disabling Kernel IPv6 RA support",
			    ifp->name);
			if (del_if_nd6_flag(ifp->name, ND6_IFF_ACCEPT_RTADV)
			    == -1)
			{
				syslog(LOG_ERR,
				    "%s: del_if_nd6_flag: "
				    "ND6_IFF_ACCEPT_RTADV: %m",
				    ifp->name);
				return ra;
			}
#ifdef ND6_IFF_OVERRIDE_RTADV
			if (override == 0 &&
			    set_if_nd6_flag(ifp->name, ND6_IFF_OVERRIDE_RTADV)
			    == -1)
			{
				syslog(LOG_ERR,
				    "%s: set_if_nd6_flag: "
				    "ND6_IFF_OVERRIDE_RTADV: %m",
				    ifp->name);
				return ra;
			}
#endif
			return 0;
		}
		return ra;
#else
		return ctx->ra_global;
#endif
	}

	ra = get_inet6_sysctl(IPV6CTL_ACCEPT_RTADV);
	if (ra == -1)
		/* The sysctl probably doesn't exist, but this isn't an
		 * error as such so just log it and continue */
		syslog(errno == ENOENT ? LOG_DEBUG : LOG_WARNING,
		    "IPV6CTL_ACCEPT_RTADV: %m");
	else if (ra != 0 && own) {
		syslog(LOG_DEBUG, "disabling Kernel IPv6 RA support");
		if (set_inet6_sysctl(IPV6CTL_ACCEPT_RTADV, 0) == -1) {
			syslog(LOG_ERR, "IPV6CTL_ACCEPT_RTADV: %m");
			return ra;
		}
		ra = 0;

		/* Flush the kernel knowledge of advertised routers
		 * and prefixes so the kernel does not expire prefixes
		 * and default routes we are trying to own. */
		if_raflush();
	}

	ctx->ra_global = ra;
	return ra;
}
#endif
