/* 
 * dhcpcd - DHCP client daemon
 * Copyright (c) 2006-2012 Roy Marples <roy@marples.name>
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

#ifdef __linux__
#  define _LINUX_IN6_H
#  include <linux/ipv6.h>
#endif

#define ELOOP_QUEUE 1
#include "common.h"
#include "configure.h"
#include "dhcpcd.h"
#include "eloop.h"
#include "ipv6.h"
#include "ipv6ns.h"

#define MIN_RANDOM_FACTOR	(500 * 1000)	/* milliseconds in usecs */
#define MAX_RANDOM_FACTOR	(1500 * 1000)	/* milliseconds in usecs */

/* Debugging Neighbor Solicitations is a lot of spam, so disable it */
//#define DEBUG_NS

static int sock;
static struct sockaddr_in6 from;
static struct msghdr sndhdr;
static struct iovec sndiov[2];
static unsigned char *sndbuf;
static struct msghdr rcvhdr;
static struct iovec rcviov[2];
static unsigned char *rcvbuf;
static unsigned char ansbuf[1500];
static char ntopbuf[INET6_ADDRSTRLEN];

#if DEBUG_MEMORY
static void
ipv6ns_cleanup(void)
{

	free(sndbuf);
	free(rcvbuf);
}
#endif

int
ipv6ns_open(void)
{
	int on;
	int len;
	struct icmp6_filter filt;

	sock = socket(AF_INET6, SOCK_RAW, IPPROTO_ICMPV6);
	if (sock == -1)
		return -1;
	on = 1;
	if (setsockopt(sock, IPPROTO_IPV6, IPV6_RECVPKTINFO,
		&on, sizeof(on)) == -1)
		return -1;

	on = 1;
	if (setsockopt(sock, IPPROTO_IPV6, IPV6_RECVHOPLIMIT,
		&on, sizeof(on)) == -1)
		return -1;

	ICMP6_FILTER_SETBLOCKALL(&filt);
	ICMP6_FILTER_SETPASS(ND_NEIGHBOR_ADVERT, &filt);
	if (setsockopt(sock, IPPROTO_ICMPV6, ICMP6_FILTER,
		&filt, sizeof(filt)) == -1)
		return -1;

	set_cloexec(sock);
#if DEBUG_MEMORY
	atexit(ipv6ns_cleanup);
#endif

	len = CMSG_SPACE(sizeof(struct in6_pktinfo)) + CMSG_SPACE(sizeof(int));
	sndbuf = xzalloc(len);
	if (sndbuf == NULL)
		return -1;
	sndhdr.msg_namelen = sizeof(struct sockaddr_in6);
	sndhdr.msg_iov = sndiov;
	sndhdr.msg_iovlen = 1;
	sndhdr.msg_control = sndbuf;
	sndhdr.msg_controllen = len;
	rcvbuf = xzalloc(len);
	if (rcvbuf == NULL)
		return -1;
	rcvhdr.msg_name = &from;
	rcvhdr.msg_namelen = sizeof(from);
	rcvhdr.msg_iov = rcviov;
	rcvhdr.msg_iovlen = 1;
	rcvhdr.msg_control = rcvbuf;
	rcvhdr.msg_controllen = len;
	rcviov[0].iov_base = ansbuf;
	rcviov[0].iov_len = sizeof(ansbuf);
	return sock;
}

static int
ipv6ns_makeprobe(struct ra *rap)
{
	struct nd_neighbor_solicit *ns;
	struct nd_opt_hdr *nd;

	free(rap->ns);
	rap->nslen = sizeof(*ns) + ROUNDUP8(rap->iface->hwlen + 2);
	rap->ns = xzalloc(rap->nslen);
	if (rap->ns == NULL)
		return -1;
	ns = (struct nd_neighbor_solicit *)(void *)rap->ns;
	ns->nd_ns_type = ND_NEIGHBOR_SOLICIT;
	ns->nd_ns_cksum = 0;
	ns->nd_ns_code = 0;
	ns->nd_ns_reserved = 0;
	ns->nd_ns_target = rap->from;
	nd = (struct nd_opt_hdr *)(rap->ns + sizeof(*ns));
	nd->nd_opt_type = ND_OPT_SOURCE_LINKADDR;
	nd->nd_opt_len = (ROUNDUP8(rap->iface->hwlen + 2)) >> 3;
	memcpy(nd + 1, rap->iface->hwaddr, rap->iface->hwlen);
	return 0;
}

static void
ipv6ns_unreachable(void *arg)
{
	struct ra *rap = arg;

	/* We could add an unreachable flag and persist the information,
	 * but that is more effort than it's probably worth. */
	syslog(LOG_WARNING, "%s: %s is unreachable, expiring it",
	    rap->iface->name, rap->sfrom);
	rap->expired = 1;
	ipv6_build_routes();
	run_script_reason(rap->iface, "ROUTERADVERT"); /* XXX not RA */
}

void
ipv6ns_sendprobe(void *arg)
{
	struct ra *rap = arg;
	struct sockaddr_in6 dst;
	struct cmsghdr *cm;
	struct in6_pktinfo pi;
	int hoplimit = HOPLIMIT;
	struct timeval tv, rtv;

	if (!rap->ns) {
		if (ipv6ns_makeprobe(rap) == -1)
			return;
	}

	memset(&dst, 0, sizeof(dst));
	dst.sin6_family = AF_INET6;
#ifdef SIN6_LEN
	dst.sin6_len = sizeof(dst);
#endif
	memcpy(&dst.sin6_addr, &rap->from, sizeof(dst.sin6_addr));
	//dst.sin6_scope_id = rap->iface->index;

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
	if (sendmsg(sock, &sndhdr, 0) == -1)
		syslog(LOG_ERR, "%s: sendmsg: %m", rap->iface->name);


	ms_to_tv(&tv, rap->retrans ? rap->retrans : RETRANS_TIMER);
	ms_to_tv(&rtv, MIN_RANDOM_FACTOR);
	timeradd(&tv, &rtv, &tv);
	rtv.tv_sec = 0;
	rtv.tv_usec = arc4random() % (MAX_RANDOM_FACTOR - MIN_RANDOM_FACTOR);
	timeradd(&tv, &rtv, &tv);
	add_timeout_tv(&tv, ipv6ns_sendprobe, rap);

	if (rap->nsprobes++ == 0)
		add_timeout_sec(DELAY_FIRST_PROBE_TIME,
		    ipv6ns_unreachable, rap);
}

/* ARGSUSED */
void
ipv6ns_handledata(_unused void *arg)
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
	struct timeval tv;

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

	for (ifp = ifaces; ifp; ifp = ifp->next)
		if (ifp->index == (unsigned int)pkt.ipi6_ifindex)
			break;
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

	TAILQ_FOREACH(rap, &ipv6_routers, next) {
		if (memcmp(rap->from.s6_addr, from.sin6_addr.s6_addr,
		    sizeof(rap->from.s6_addr)) == 0)
			break;
	}
	if (rap == NULL) {
#ifdef DEBUG_NS
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
		ipv6_build_routes();
		run_script_reason(ifp, "ROUTERADVERT");
		return;
	}

	if (is_solicited) {
		rap->nsprobes = 0;
		if (rap->reachable) {
			ms_to_tv(&tv, rap->reachable);
		} else {
			tv.tv_sec = REACHABLE_TIME;
			tv.tv_usec = 0;
		}
		add_timeout_tv(&tv, ipv6ns_sendprobe, rap);
		delete_timeout(ipv6ns_unreachable, rap);
	}
}
