#include <sys/cdefs.h>
 __RCSID("$NetBSD: ipv6ns.c,v 1.1.1.2.2.1 2013/06/23 06:26:31 tls Exp $");

/*
 * dhcpcd - DHCP client daemon
 * Copyright (c) 2006-2013 Roy Marples <roy@marples.name>
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
#include <sys/time.h>
#include <net/if.h>
#include <netinet/in.h>
#include <netinet/ip6.h>
#include <netinet/icmp6.h>

#include <errno.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>

#ifdef __linux__
#  define _LINUX_IN6_H
#  include <linux/ipv6.h>
#endif

#define ELOOP_QUEUE 2
#include "common.h"
#include "dhcpcd.h"
#include "dhcp6.h"
#include "eloop.h"
#include "ipv6.h"
#include "ipv6ns.h"
#include "script.h"

#define MIN_RANDOM_FACTOR	500				/* millisecs */
#define MAX_RANDOM_FACTOR	1500				/* millisecs */
#define MIN_RANDOM_FACTOR_U	MIN_RANDOM_FACTOR * 1000	/* usecs */
#define MAX_RANDOM_FACTOR_U	MAX_RANDOM_FACTOR * 1000	/* usecs */

#if BYTE_ORDER == BIG_ENDIAN
#define IPV6_ADDR_INT32_ONE     1
#define IPV6_ADDR_INT16_MLL     0xff02
#elif BYTE_ORDER == LITTLE_ENDIAN
#define IPV6_ADDR_INT32_ONE     0x01000000
#define IPV6_ADDR_INT16_MLL     0x02ff
#endif

/* Debugging Neighbor Solicitations is a lot of spam, so disable it */
//#define DEBUG_NS
//

/* Currently, no known kernel allows us to send from the unspecified address
 * which is required for DAD to work. This isn't that much of a problem as
 * the kernel will do DAD for us correctly, however we don't know the exact
 * randomness the kernel applies to the timeouts. So we just follow the same
 * logic and have a little faith.
 * This define is purely for completeness */
// #define IPV6_SEND_DAD

static int sock = -1;
#ifdef IPV6_SEND_DAD
static int unspec_sock = -1;
#endif
static struct sockaddr_in6 from;
static struct msghdr sndhdr;
static struct iovec sndiov[2];
static unsigned char *sndbuf;
static struct msghdr rcvhdr;
static struct iovec rcviov[2];
static unsigned char *rcvbuf;
static unsigned char ansbuf[1500];
static char ntopbuf[INET6_ADDRSTRLEN];

static void ipv6ns_handledata(__unused void *arg);

#if DEBUG_MEMORY
static void
ipv6ns_cleanup(void)
{

	free(sndbuf);
	free(rcvbuf);
}
#endif

static int
ipv6ns_open(void)
{
	int on;
	int len;
	struct icmp6_filter filt;
#ifdef IPV6_SEND_DAD
	union {
		struct sockaddr sa;
		struct sockaddr_in6 sin;
	} su;
#endif

	if (sock != -1)
		return sock;

	sock = socket(AF_INET6, SOCK_RAW, IPPROTO_ICMPV6);
	if (sock == -1)
		return -1;
	on = 1;
	if (setsockopt(sock, IPPROTO_IPV6, IPV6_RECVPKTINFO,
	    &on, sizeof(on)) == -1)
		goto eexit;

	on = 1;
	if (setsockopt(sock, IPPROTO_IPV6, IPV6_RECVHOPLIMIT,
	    &on, sizeof(on)) == -1)
		goto eexit;

	ICMP6_FILTER_SETBLOCKALL(&filt);

#ifdef IPV6_SEND_DAD
	/* We send DAD requests from the unspecified address. */
	unspec_sock = socket(AF_INET6, SOCK_RAW, IPPROTO_ICMPV6);
	if (unspec_sock == -1)
		goto eexit;
	if (setsockopt(unspec_sock, IPPROTO_ICMPV6, ICMP6_FILTER,
	    &filt, sizeof(filt)) == -1)
		goto eexit;
	memset(&su, 0, sizeof(su));
	su.sin.sin6_family = AF_INET6;
#ifdef SIN6_LEN
	su.sin.sin6_len = sizeof(su.sin);
#endif
	if (bind(unspec_sock, &su.sa, sizeof(su.sin)) == -1)
		goto eexit;
#endif

	ICMP6_FILTER_SETPASS(ND_NEIGHBOR_ADVERT, &filt);
	if (setsockopt(sock, IPPROTO_ICMPV6, ICMP6_FILTER,
	    &filt, sizeof(filt)) == -1)
		goto eexit;

	set_cloexec(sock);
#if DEBUG_MEMORY
	atexit(ipv6ns_cleanup);
#endif

#ifdef LISTEN_DAD
	syslog(LOG_WARNING, "kernel does not report DAD results to userland");
	syslog(LOG_WARNING,
	    "warning listening to duplicated addresses on the wire");
#endif

	len = CMSG_SPACE(sizeof(struct in6_pktinfo)) + CMSG_SPACE(sizeof(int));
	sndbuf = calloc(1, len);
	if (sndbuf == NULL)
		goto eexit;
	sndhdr.msg_namelen = sizeof(struct sockaddr_in6);
	sndhdr.msg_iov = sndiov;
	sndhdr.msg_iovlen = 1;
	sndhdr.msg_control = sndbuf;
	sndhdr.msg_controllen = len;
	rcvbuf = calloc(1, len);
	if (rcvbuf == NULL)
		goto eexit;
	rcvhdr.msg_name = &from;
	rcvhdr.msg_namelen = sizeof(from);
	rcvhdr.msg_iov = rcviov;
	rcvhdr.msg_iovlen = 1;
	rcvhdr.msg_control = rcvbuf;
	rcvhdr.msg_controllen = len;
	rcviov[0].iov_base = ansbuf;
	rcviov[0].iov_len = sizeof(ansbuf);

	eloop_event_add(sock, ipv6ns_handledata, NULL);
	return sock;

eexit:
	syslog(LOG_ERR, "%s: %m", __func__);
	close(sock);
	sock = -1;
	free(sndbuf);
	sndbuf = NULL;
	free(rcvbuf);
	rcvbuf = NULL;
	return -1;
}

static void
ipv6ns_unreachable(void *arg)
{
	struct ra *rap = arg;
	struct timeval tv;

	/* We could add an unreachable flag and persist the information,
	 * but that is more effort than it's probably worth. */
	syslog(LOG_WARNING, "%s: %s is unreachable, expiring it",
	    rap->iface->name, rap->sfrom);
	rap->expired = 1;
	ipv6_buildroutes();
	script_runreason(rap->iface, "ROUTERADVERT"); /* XXX not RA */

	/* We should still test if it's reachable or not so
	 * incase it comes back to life and it's preferable. */
	if (rap->reachable) {
		ms_to_tv(&tv, rap->reachable);
	} else {
		tv.tv_sec = REACHABLE_TIME;
		tv.tv_usec = 0;
	}
	eloop_timeout_add_tv(&tv, ipv6ns_proberouter, rap);
}

#ifdef LISTEN_DAD
void
ipv6ns_cancelprobeaddr(struct ipv6_addr *ap)
{

	eloop_timeout_delete(ipv6ns_probeaddr, ap);
	if (ap->dadcallback)
		eloop_timeout_delete(ap->dadcallback, ap);
}
#endif

void
ipv6ns_probeaddr(void *arg)
{
	struct ipv6_addr *ap = arg;
#ifdef IPV6_SEND_DAD
	struct nd_neighbor_solicit *ns;
	struct nd_opt_hdr *nd;
	struct sockaddr_in6 dst;
	struct cmsghdr *cm;
	struct in6_pktinfo pi;
	int hoplimit = HOPLIMIT;
#else
#ifdef LISTEN_DAD
	struct timeval tv, rtv;
	struct timeval mtv;
	int i;
#endif
#endif

	if (ap->dadcallback &&
	    ((ap->flags & IPV6_AF_NEW) == 0 ||
	    ap->nsprobes >= ap->iface->options->dadtransmits))
	{
#ifdef IPV6_SEND_DAD
		ap->dadcallback(ap);
#else
		ipv6_addaddr(ap);
#endif
		return;
	}

	if (ipv6ns_open() == -1)
		return;

	ap->flags &= ~IPV6_AF_DADCOMPLETED;

#ifdef IPV6_SEND_DAD
	if (!ap->ns) {
	        ap->nslen = sizeof(*ns) + ROUNDUP8(ap->iface->hwlen + 2);
		ap->ns = calloc(1, ap->nslen);
		if (ap->ns == NULL) {
			syslog(LOG_ERR, "%s: %m", __func__);
			return;
		}
		ns = (struct nd_neighbor_solicit *)(void *)ap->ns;
		ns->nd_ns_type = ND_NEIGHBOR_SOLICIT;
		//ns->nd_ns_cksum = 0;
		//ns->nd_ns_code = 0;
		//ns->nd_ns_reserved = 0;
		ns->nd_ns_target = ap->addr;
		nd = (struct nd_opt_hdr *)(ap->ns + sizeof(*ns));
		nd->nd_opt_type = ND_OPT_SOURCE_LINKADDR;
		nd->nd_opt_len = (ROUNDUP8(ap->iface->hwlen + 2)) >> 3;
		memcpy(nd + 1, ap->iface->hwaddr, ap->iface->hwlen);
	}

	memset(&dst, 0, sizeof(dst));
	dst.sin6_family = AF_INET6;
#ifdef SIN6_LEN
	dst.sin6_len = sizeof(dst);
#endif
	dst.sin6_addr.s6_addr16[0] = IPV6_ADDR_INT16_MLL;
	dst.sin6_addr.s6_addr16[1] = 0;
	dst.sin6_addr.s6_addr32[1] = 0;
	dst.sin6_addr.s6_addr32[2] = IPV6_ADDR_INT32_ONE;
	dst.sin6_addr.s6_addr32[3] = ap->addr.s6_addr32[3];
	dst.sin6_addr.s6_addr[12] = 0xff;

	//memcpy(&dst.sin6_addr, &ap->addr, sizeof(dst.sin6_addr));
	dst.sin6_scope_id = ap->iface->index;

	sndhdr.msg_name = (caddr_t)&dst;
	sndhdr.msg_iov[0].iov_base = ap->ns;
	sndhdr.msg_iov[0].iov_len = ap->nslen;

	/* Set the outbound interface */
	cm = CMSG_FIRSTHDR(&sndhdr);
	cm->cmsg_level = IPPROTO_IPV6;
	cm->cmsg_type = IPV6_PKTINFO;
	cm->cmsg_len = CMSG_LEN(sizeof(pi));
	memset(&pi, 0, sizeof(pi));
	pi.ipi6_ifindex = ap->iface->index;
	memcpy(CMSG_DATA(cm), &pi, sizeof(pi));

	/* Hop limit */
	cm = CMSG_NXTHDR(&sndhdr, cm);
	cm->cmsg_level = IPPROTO_IPV6;
	cm->cmsg_type = IPV6_HOPLIMIT;
	cm->cmsg_len = CMSG_LEN(sizeof(hoplimit));
	memcpy(CMSG_DATA(cm), &hoplimit, sizeof(hoplimit));

#ifdef DEBUG_NS
	syslog(LOG_INFO, "%s: sending IPv6 NS for %s",
	    ap->iface->name, ap->saddr);
	if (ap->dadcallback == NULL)
		syslog(LOG_WARNING, "%s: no callback!", ap->iface->name);
#endif
	if (sendmsg(unspec_sock, &sndhdr, 0) == -1) {
		syslog(LOG_ERR, "%s: %s: sendmsg: %m",
		    ap->iface->name, __func__);
		return;
	}

	if (ap->dadcallback) {
		ms_to_tv(&tv, RETRANS_TIMER);
		ms_to_tv(&rtv, MIN_RANDOM_FACTOR);
		timeradd(&tv, &rtv, &tv);
		rtv.tv_sec = 0;
		rtv.tv_usec = arc4random() %
		    (MAX_RANDOM_FACTOR_U - MIN_RANDOM_FACTOR_U);
		timeradd(&tv, &rtv, &tv);

		eloop_timeout_add_tv(&tv,
		    ++(ap->nsprobes) < ap->iface->options->dadtransmits ?
		    ipv6ns_probeaddr : ap->dadcallback,
		    ap);
	}
#else /* IPV6_SEND_DAD */
	ipv6_addaddr(ap);
#ifdef LISTEN_DAD
	/* Let the kernel handle DAD.
	 * We don't know the timings, so just wait for the max */
	if (ap->dadcallback) {
		mtv.tv_sec = 0;
		mtv.tv_usec = 0;
		for (i = 0; i < ap->iface->options->dadtransmits; i++) {
			ms_to_tv(&tv, RETRANS_TIMER);
			ms_to_tv(&rtv, MAX_RANDOM_FACTOR);
			timeradd(&tv, &rtv, &tv);
			timeradd(&mtv, &tv, &mtv);
		}
		eloop_timeout_add_tv(&mtv, ap->dadcallback, ap);
	}
#endif
#endif /* IPV6_SEND_DAD */
}

ssize_t
ipv6ns_probeaddrs(struct ipv6_addrhead *addrs)
{
	struct ipv6_addr *ap, *apn;
	ssize_t i;

	i = 0;
	TAILQ_FOREACH_SAFE(ap, addrs, next, apn) {
		if (ap->prefix_vltime == 0) {
			TAILQ_REMOVE(addrs, ap, next);
			if (ap->flags & IPV6_AF_ADDED) {
				syslog(LOG_INFO, "%s: deleting address %s",
				    ap->iface->name, ap->saddr);
				i++;
			}
			if (!IN6_IS_ADDR_UNSPECIFIED(&ap->addr) &&
			    del_address6(ap) == -1 &&
			    errno != EADDRNOTAVAIL && errno != ENXIO)
				syslog(LOG_ERR, "del_address6 %m");
			if (ap->dadcallback)
				eloop_q_timeout_delete(0, NULL,
				    ap->dadcallback);
			free(ap);
		} else if (!IN6_IS_ADDR_UNSPECIFIED(&ap->addr) &&
		    !(ap->flags & IPV6_AF_DELEGATED))
		{
			ipv6ns_probeaddr(ap);
			if (ap->flags & IPV6_AF_NEW)
				i++;
		}
	}

	return i;
}

void
ipv6ns_proberouter(void *arg)
{
	struct ra *rap = arg;
	struct nd_neighbor_solicit *ns;
	struct nd_opt_hdr *nd;
	struct sockaddr_in6 dst;
	struct cmsghdr *cm;
	struct in6_pktinfo pi;
	int hoplimit = HOPLIMIT;
	struct timeval tv, rtv;

	if (ipv6ns_open() == -1)
		return;

	if (!rap->ns) {
	        rap->nslen = sizeof(*ns) + ROUNDUP8(rap->iface->hwlen + 2);
		rap->ns = calloc(1, rap->nslen);
		if (rap->ns == NULL) {
			syslog(LOG_ERR, "%s: %m", __func__);
			return;
		}
		ns = (struct nd_neighbor_solicit *)(void *)rap->ns;
		ns->nd_ns_type = ND_NEIGHBOR_SOLICIT;
		//ns->nd_ns_cksum = 0;
		//ns->nd_ns_code = 0;
		//ns->nd_ns_reserved = 0;
		ns->nd_ns_target = rap->from;
		nd = (struct nd_opt_hdr *)(rap->ns + sizeof(*ns));
		nd->nd_opt_type = ND_OPT_SOURCE_LINKADDR;
		nd->nd_opt_len = (ROUNDUP8(rap->iface->hwlen + 2)) >> 3;
		memcpy(nd + 1, rap->iface->hwaddr, rap->iface->hwlen);
	}

	memset(&dst, 0, sizeof(dst));
	dst.sin6_family = AF_INET6;
#ifdef SIN6_LEN
	dst.sin6_len = sizeof(dst);
#endif
	memcpy(&dst.sin6_addr, &rap->from, sizeof(dst.sin6_addr));
	dst.sin6_scope_id = rap->iface->index;

	sndhdr.msg_name = (caddr_t)&dst;
	sndhdr.msg_iov[0].iov_base = rap->ns;
	sndhdr.msg_iov[0].iov_len = rap->nslen;

	/* Set the outbound interface */
	cm = CMSG_FIRSTHDR(&sndhdr);
	cm->cmsg_level = IPPROTO_IPV6;
	cm->cmsg_type = IPV6_PKTINFO;
	cm->cmsg_len = CMSG_LEN(sizeof(pi));
	memset(&pi, 0, sizeof(pi));
	pi.ipi6_ifindex = rap->iface->index;
	memcpy(CMSG_DATA(cm), &pi, sizeof(pi));

	/* Hop limit */
	cm = CMSG_NXTHDR(&sndhdr, cm);
	cm->cmsg_level = IPPROTO_IPV6;
	cm->cmsg_type = IPV6_HOPLIMIT;
	cm->cmsg_len = CMSG_LEN(sizeof(hoplimit));
	memcpy(CMSG_DATA(cm), &hoplimit, sizeof(hoplimit));

#ifdef DEBUG_NS
	syslog(LOG_INFO, "%s: sending IPv6 NS for %s",
	    rap->iface->name, rap->sfrom);
#endif
	if (sendmsg(sock, &sndhdr, 0) == -1) {
		syslog(LOG_ERR, "%s: %s: sendmsg: %m",
		    rap->iface->name, __func__);
		return;
	}

	ms_to_tv(&tv, rap->retrans == 0 ? RETRANS_TIMER : rap->retrans);
	ms_to_tv(&rtv, MIN_RANDOM_FACTOR);
	timeradd(&tv, &rtv, &tv);
	rtv.tv_sec = 0;
	rtv.tv_usec = arc4random() % (MAX_RANDOM_FACTOR_U -MIN_RANDOM_FACTOR_U);
	timeradd(&tv, &rtv, &tv);
	eloop_timeout_add_tv(&tv, ipv6ns_proberouter, rap);

	if (rap->nsprobes++ == 0)
		eloop_timeout_add_sec(DELAY_FIRST_PROBE_TIME,
		    ipv6ns_unreachable, rap);
}

void
ipv6ns_cancelproberouter(struct ra *rap)
{

	eloop_timeout_delete(ipv6ns_proberouter, rap);
	eloop_timeout_delete(ipv6ns_unreachable, rap);
}

/* ARGSUSED */
static void
ipv6ns_handledata(__unused void *arg)
{
	ssize_t len;
	struct cmsghdr *cm;
	int hoplimit;
	struct in6_pktinfo pkt;
	struct icmp6_hdr *icp;
	struct interface *ifp;
	const char *sfrom;
	struct nd_neighbor_advert *nd_na;
	struct ra *rap;
	int is_router, is_solicited;
#ifdef DEBUG_NS
	int found;
#endif
	struct timeval tv;

#ifdef LISTEN_DAD
	struct dhcp6_state *d6state;
	struct ipv6_addr *ap;
#endif

	len = recvmsg(sock, &rcvhdr, 0);
	if (len == -1) {
		syslog(LOG_ERR, "recvmsg: %m");
		return;
	}
	sfrom = inet_ntop(AF_INET6, &from.sin6_addr,
	    ntopbuf, INET6_ADDRSTRLEN);
	if ((size_t)len < sizeof(struct nd_neighbor_advert)) {
		syslog(LOG_ERR, "IPv6 NA packet too short from %s", sfrom);
		return;
	}

	pkt.ipi6_ifindex = hoplimit = 0;
	for (cm = (struct cmsghdr *)CMSG_FIRSTHDR(&rcvhdr);
	     cm;
	     cm = (struct cmsghdr *)CMSG_NXTHDR(&rcvhdr, cm))
	{
		if (cm->cmsg_level != IPPROTO_IPV6)
			continue;
		switch(cm->cmsg_type) {
		case IPV6_PKTINFO:
			if (cm->cmsg_len == CMSG_LEN(sizeof(pkt)))
				memcpy(&pkt, CMSG_DATA(cm), sizeof(pkt));
			break;
		case IPV6_HOPLIMIT:
			if (cm->cmsg_len == CMSG_LEN(sizeof(int)))
				memcpy(&hoplimit, CMSG_DATA(cm), sizeof(int));
			break;
		}
	}

	if (pkt.ipi6_ifindex == 0 || hoplimit != 255) {
		syslog(LOG_ERR,
		    "IPv6 NA did not contain index or hop limit from %s",
		    sfrom);
		return;
	}

	icp = (struct icmp6_hdr *)rcvhdr.msg_iov[0].iov_base;
	if (icp->icmp6_type != ND_NEIGHBOR_ADVERT ||
	    icp->icmp6_code != 0)
	{
		syslog(LOG_ERR, "invalid IPv6 type or code from %s", sfrom);
		return;
	}

	TAILQ_FOREACH(ifp, ifaces, next) {
		if (ifp->index == (unsigned int)pkt.ipi6_ifindex)
			break;
	}
	if (ifp == NULL) {
#ifdef DEBUG_NS
		syslog(LOG_DEBUG, "NA for unexpected interface from %s", sfrom);
#endif
		return;
	}

	nd_na = (struct nd_neighbor_advert *)icp;
	is_router = nd_na->nd_na_flags_reserved & ND_NA_FLAG_ROUTER;
	is_solicited = nd_na->nd_na_flags_reserved & ND_NA_FLAG_SOLICITED;

	if (IN6_IS_ADDR_MULTICAST(&nd_na->nd_na_target)) {
		syslog(LOG_ERR, "%s: NA for multicast address from %s",
		    ifp->name, sfrom);
		return;
	}

#ifdef DEBUG_NS
	found = 0;
#endif
	TAILQ_FOREACH(rap, &ipv6_routers, next) {
		if (rap->iface != ifp)
			continue;
		if (memcmp(rap->from.s6_addr, nd_na->nd_na_target.s6_addr,
		    sizeof(rap->from.s6_addr)) == 0)
			break;
#ifdef LISTEN_DAD
		TAILQ_FOREACH(ap, &rap->addrs, next) {
			if (memcmp(ap->addr.s6_addr,
			    nd_na->nd_na_target.s6_addr,
			    sizeof(ap->addr.s6_addr)) == 0)
			{
				ap->flags |= IPV6_AF_DUPLICATED;
				if (ap->dadcallback)
					ap->dadcallback(ap);
#ifdef DEBUG_NS
				found++;
#endif
			}
		}
#endif
	}
	if (rap == NULL) {
#ifdef LISTEN_DAD
		d6state = D6_STATE(ifp);
		if (d6state) {
			TAILQ_FOREACH(ap, &d6state->addrs, next) {
				if (memcmp(ap->addr.s6_addr,
				    nd_na->nd_na_target.s6_addr,
				    sizeof(ap->addr.s6_addr)) == 0)
				{
					ap->flags |= IPV6_AF_DUPLICATED;
					if (ap->dadcallback)
						ap->dadcallback(ap);
#ifdef DEBUG_NS
					found++;
#endif
				}
			}
		}
#endif

#ifdef DEBUG_NS
		if (found == 0)
			syslog(LOG_DEBUG, "%s: unexpected NA from %s",
			    ifp->name, sfrom);
#endif
		return;
	}

#ifdef DEBUG_NS
	syslog(LOG_DEBUG, "%s: %sNA from %s",
	    ifp->name, is_solicited ? "solicited " : "",  sfrom);
#endif

	/* Node is no longer a router, so remove it from consideration */
	if (!is_router && !rap->expired) {
		syslog(LOG_INFO, "%s: %s is no longer a router",
		    ifp->name, sfrom);
		rap->expired = 1;
		ipv6ns_cancelproberouter(rap);
		ipv6_buildroutes();
		script_runreason(ifp, "ROUTERADVERT");
		return;
	}

	if (is_solicited && is_router && rap->lifetime) {
		if (rap->expired) {
			rap->expired = 0;
			syslog(LOG_INFO, "%s: %s is reachable again",
				ifp->name, sfrom);
			ipv6_buildroutes();
			script_runreason(rap->iface, "ROUTERADVERT"); /* XXX */
		}
		rap->nsprobes = 0;
		if (rap->reachable) {
			ms_to_tv(&tv, rap->reachable);
		} else {
			tv.tv_sec = REACHABLE_TIME;
			tv.tv_usec = 0;
		}
		eloop_timeout_add_tv(&tv, ipv6ns_proberouter, rap);
		eloop_timeout_delete(ipv6ns_unreachable, rap);
	}
}
