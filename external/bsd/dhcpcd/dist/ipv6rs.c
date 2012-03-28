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

#include <sys/param.h>
#include <sys/socket.h>
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
#include "bind.h"
#include "common.h"
#include "configure.h"
#include "dhcpcd.h"
#include "eloop.h"
#include "ipv6rs.h"

#define ALLROUTERS "ff02::2"
#define HOPLIMIT 255

#define ROUNDUP8(a) (1 + (((a) - 1) | 7))

#define RTR_SOLICITATION_INTERVAL       4 /* seconds */
#define MAX_RTR_SOLICITATIONS           3 /* times */

#ifndef ND_OPT_RDNSS
#define ND_OPT_RDNSS			25
struct nd_opt_rdnss {           /* RDNSS option RFC 6106 */
	uint8_t		nd_opt_rdnss_type;
	uint8_t		nd_opt_rdnss_len;
	uint16_t	nd_opt_rdnss_reserved;
	uint32_t	nd_opt_rdnss_lifetime;
        /* followed by list of IP prefixes */
} _packed;
#endif

#ifndef ND_OPT_DNSSL
#define ND_OPT_DNSSL			31
struct nd_opt_dnssl {		/* DNSSL option RFC 6106 */
	uint8_t		nd_opt_dnssl_type;
	uint8_t		nd_opt_dnssl_len;
	uint16_t	nd_opt_dnssl_reserved;
	uint32_t	nd_opt_dnssl_lifetime;
	/* followed by list of DNS servers */
} _packed;
#endif

static int sock;
static struct sockaddr_in6 allrouters, from;
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
ipv6rs_cleanup(void)
{

	free(sndbuf);
	free(rcvbuf);
}
#endif

int
ipv6rs_open(void)
{
	int on;
	int len;
	struct icmp6_filter filt;

	memset(&allrouters, 0, sizeof(allrouters));
	allrouters.sin6_family = AF_INET6;
#ifdef SIN6_LEN
	allrouters.sin6_len = sizeof(allrouters);
#endif
	if (inet_pton(AF_INET6, ALLROUTERS, &allrouters.sin6_addr.s6_addr) != 1)
		return -1;
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
	ICMP6_FILTER_SETPASS(ND_ROUTER_ADVERT, &filt);
	if (setsockopt(sock, IPPROTO_ICMPV6, ICMP6_FILTER,
		&filt, sizeof(filt)) == -1)
		return -1;

#if DEBUG_MEMORY
	atexit(ipv6rs_cleanup);
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
ipv6rs_makeprobe(struct interface *ifp)
{
	struct nd_router_solicit *rs;
	struct nd_opt_hdr *nd;

	free(ifp->rs);
	ifp->rslen = sizeof(*rs) + ROUNDUP8(ifp->hwlen + 2);
	ifp->rs = xzalloc(ifp->rslen);
	if (ifp->rs == NULL)
		return -1;
	rs = (struct nd_router_solicit *)ifp->rs;
	rs->nd_rs_type = ND_ROUTER_SOLICIT;
	rs->nd_rs_code = 0;
	rs->nd_rs_cksum = 0;
	rs->nd_rs_reserved = 0;
	nd = (struct nd_opt_hdr *)(ifp->rs + sizeof(*rs));
	nd->nd_opt_type = ND_OPT_SOURCE_LINKADDR;
	nd->nd_opt_len = (ROUNDUP8(ifp->hwlen + 2)) >> 3;
	memcpy(nd + 1, ifp->hwaddr, ifp->hwlen);
	return 0;
}
	
static void
ipv6rs_sendprobe(void *arg)
{
	struct interface *ifp = arg;
	struct sockaddr_in6 dst;
	struct cmsghdr *cm;
	struct in6_pktinfo pi;
	int hoplimit = HOPLIMIT;

	dst = allrouters;
	//dst.sin6_scope_id = ifp->linkid;

	ipv6rs_makeprobe(ifp);
	sndhdr.msg_name = (caddr_t)&dst;
	sndhdr.msg_iov[0].iov_base = ifp->rs;
	sndhdr.msg_iov[0].iov_len = ifp->rslen;

	/* Set the outbound interface */
	cm = CMSG_FIRSTHDR(&sndhdr);
	cm->cmsg_level = IPPROTO_IPV6;
	cm->cmsg_type = IPV6_PKTINFO;
	cm->cmsg_len = CMSG_LEN(sizeof(pi));
	memset(&pi, 0, sizeof(pi));
	pi.ipi6_ifindex = if_nametoindex(ifp->name);
	memcpy(CMSG_DATA(cm), &pi, sizeof(pi));

	/* Hop limit */
	cm = CMSG_NXTHDR(&sndhdr, cm);
	cm->cmsg_level = IPPROTO_IPV6;
	cm->cmsg_type = IPV6_HOPLIMIT;
	cm->cmsg_len = CMSG_LEN(sizeof(hoplimit));
	memcpy(CMSG_DATA(cm), &hoplimit, sizeof(hoplimit));

	syslog(LOG_INFO, "%s: sending IPv6 Router Solicitation", ifp->name);
	if (sendmsg(sock, &sndhdr, 0) == -1)
		syslog(LOG_ERR, "%s: sendmsg: %m", ifp->name);

	if (ifp->rsprobes++ < MAX_RTR_SOLICITATIONS)
		add_timeout_sec(RTR_SOLICITATION_INTERVAL,
		    ipv6rs_sendprobe, ifp);
	else
		syslog(LOG_INFO, "%s: no IPv6 Routers available", ifp->name);
}

static void
ipv6rs_sort(struct interface *ifp)
{
	struct ra *rap, *sorted, *ran, *rat;

	if (ifp->ras == NULL || ifp->ras->next == NULL)
		return;

	/* Sort our RA's - most recent first */
	sorted = ifp->ras;
	ifp->ras = ifp->ras->next;
	sorted->next = NULL;
	for (rap = ifp->ras; rap && (ran = rap->next, 1); rap = ran) {
		/* Are we the new head? */
		if (timercmp(&rap->received, &sorted->received, <)) {
			rap->next = sorted;
			sorted = rap;
			continue;
		}
		/* Do we fit in the middle? */
		for (rat = sorted; rat->next; rat = rat->next) {
			if (timercmp(&rap->received, &rat->next->received, <)) {
				rap->next = rat->next;
				rat->next = rap;
				break;
			}
		}
		/* We must be at the end */
		if (!rat->next) {
			rat->next = rap;
			rap->next = NULL;
		}
	}
}

void
ipv6rs_handledata(_unused void *arg)
{
	ssize_t len, l, n, olen;
	struct cmsghdr *cm;
	int hoplimit;
	struct in6_pktinfo pkt;
	struct icmp6_hdr *icp;
	struct interface *ifp;
	const char *sfrom;
	struct nd_router_advert *nd_ra;
	struct nd_opt_prefix_info *pi;
	struct nd_opt_mtu *mtu;
	struct nd_opt_rdnss *rdnss;
	struct nd_opt_dnssl *dnssl;
	uint32_t lifetime;
	uint8_t *p, *op;
	struct in6_addr addr;
	char buf[INET6_ADDRSTRLEN];
	const char *cbp;
	struct ra *rap;
	struct nd_opt_hdr *ndo;
	struct ra_opt *rao, *raol;
	char *opt;
	struct timeval expire;
	int has_dns;

	len = recvmsg(sock, &rcvhdr, 0);
	if (len == -1) {
		syslog(LOG_ERR, "recvmsg: %m");
		return;
	}
	sfrom = inet_ntop(AF_INET6, &from.sin6_addr,
	    ntopbuf, INET6_ADDRSTRLEN);
	if ((size_t)len < sizeof(struct nd_router_advert)) {
		syslog(LOG_ERR, "IPv6 RA packet too short from %s", sfrom);
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

	if (pkt.ipi6_ifindex == 0 || hoplimit == 0) {
		syslog(LOG_ERR,
		    "IPv6 RA did not contain index or hop limit from %s",
		    sfrom);
		return;
	}

	icp = (struct icmp6_hdr *)rcvhdr.msg_iov[0].iov_base;
	if (icp->icmp6_type != ND_ROUTER_ADVERT ||
	    icp->icmp6_code != 0)
	{
		syslog(LOG_ERR, "invalid IPv6 type or code from %s", sfrom);
		return;
	}

	if (!IN6_IS_ADDR_LINKLOCAL(&from.sin6_addr)) {
		syslog(LOG_ERR, "RA recieved from non local IPv6 address %s",
		    sfrom);
		return;
	}

	for (ifp = ifaces; ifp; ifp = ifp->next)
		if (if_nametoindex(ifp->name) == (unsigned int)pkt.ipi6_ifindex)
			break;
	if (ifp == NULL) {
		syslog(LOG_ERR,"received RA for unexpected interface from %s",
		    sfrom);
		return;
	}
	for (rap = ifp->ras; rap; rap = rap->next) {
		if (memcmp(rap->from.s6_addr, from.sin6_addr.s6_addr,
		    sizeof(rap->from.s6_addr)) == 0)
			break;
	}

	/* We don't want to spam the log with the fact we got an RA every
	 * 30 seconds or so, so only spam the log if it's different. */
	if (options & DHCPCD_DEBUG || rap == NULL ||
	    (rap->expired || rap->data_len != len ||
	     memcmp(rap->data, (unsigned char *)icp, rap->data_len) != 0))
	{
		if (rap) {
			free(rap->data);
			rap->data_len = 0;
		}
		syslog(LOG_INFO, "%s: Router Advertisement from %s",
		    ifp->name, sfrom);
	}

	if (rap == NULL) {
		rap = xmalloc(sizeof(*rap));
		rap->next = ifp->ras;
		rap->options = NULL;
		ifp->ras = rap;
		memcpy(rap->from.s6_addr, from.sin6_addr.s6_addr,
		    sizeof(rap->from.s6_addr));
		strlcpy(rap->sfrom, sfrom, sizeof(rap->sfrom));
		rap->data_len = 0;
	}
	if (rap->data_len == 0) {
		rap->data = xmalloc(len);
		memcpy(rap->data, icp, len);
		rap->data_len = len;
	}

	get_monotonic(&rap->received);
	nd_ra = (struct nd_router_advert *)icp;
	rap->lifetime = ntohs(nd_ra->nd_ra_router_lifetime);
	rap->expired = 0;

	len -= sizeof(struct nd_router_advert);
	p = ((uint8_t *)icp) + sizeof(struct nd_router_advert);
	olen = 0;
	lifetime = ~0U;
	has_dns = 0;
	for (olen = 0; len > 0; p += olen, len -= olen) {
		if ((size_t)len < sizeof(struct nd_opt_hdr)) {
			syslog(LOG_ERR, "%s: Short option", ifp->name);
			break;
		}
		ndo = (struct nd_opt_hdr *)p;
		olen = ndo->nd_opt_len * 8 ;
		if (olen == 0) {
			syslog(LOG_ERR, "%s: zero length option", ifp->name);
			break;
		}
		if (olen > len) {
			syslog(LOG_ERR,
			    "%s: Option length exceeds message", ifp->name);
			break;
		}

		opt = NULL;
		switch (ndo->nd_opt_type) {
		case ND_OPT_PREFIX_INFORMATION:
			pi = (struct nd_opt_prefix_info *)ndo;
			if (pi->nd_opt_pi_len != 4) {
				syslog(LOG_ERR,
				    "%s: invalid option len for prefix",
				    ifp->name);
				break;
			}
			if (pi->nd_opt_pi_prefix_len > 128) {
				syslog(LOG_ERR, "%s: invalid prefix len",
				    ifp->name);
				break;
			}
			if (IN6_IS_ADDR_MULTICAST(&pi->nd_opt_pi_prefix) ||
			    IN6_IS_ADDR_LINKLOCAL(&pi->nd_opt_pi_prefix))
			{
				syslog(LOG_ERR,
				    "%s: invalid prefix in RA", ifp->name);
				break;
			}
			opt = xstrdup(inet_ntop(AF_INET6,
			    pi->nd_opt_pi_prefix.s6_addr,
			    ntopbuf, INET6_ADDRSTRLEN));
			if (opt) {
				rap->prefix_len = pi->nd_opt_pi_prefix_len;
				rap->prefix_vltime =
					ntohl(pi->nd_opt_pi_valid_time);
				rap->prefix_pltime =
					ntohl(pi->nd_opt_pi_preferred_time);
			}
			break;

		case ND_OPT_MTU:
			mtu = (struct nd_opt_mtu *)p;
			snprintf(buf, sizeof(buf), "%d",
			    ntohl(mtu->nd_opt_mtu_mtu));
			opt = xstrdup(buf);
			break;

		case ND_OPT_RDNSS:
			rdnss = (struct nd_opt_rdnss *)p;
			lifetime = ntohl(rdnss->nd_opt_rdnss_lifetime);
			op = (uint8_t *)ndo;
			op += offsetof(struct nd_opt_rdnss,
			    nd_opt_rdnss_lifetime);
			op += sizeof(rdnss->nd_opt_rdnss_lifetime);
			l = 0;
			for (n = ndo->nd_opt_len - 1; n > 1; n -= 2) {
				memcpy(&addr.s6_addr, op, sizeof(addr.s6_addr));
				cbp = inet_ntop(AF_INET6, &addr,
				    ntopbuf, INET6_ADDRSTRLEN);
				if (cbp == NULL) {
					syslog(LOG_ERR,
					    "%s: invalid RDNSS address",
					    ifp->name);
				} else {
					if (opt) {
						l = strlen(opt);
						opt = xrealloc(opt,
							l + strlen(cbp) + 2);
						opt[l] = ' ';
						strcpy(opt + l + 1, cbp);
					} else
						opt = xstrdup(cbp);
					if (lifetime > 0)
						has_dns = 1;
				}
		        	op += sizeof(addr.s6_addr);
			}
			break;
			
		case ND_OPT_DNSSL:
			dnssl = (struct nd_opt_dnssl *)p;
			lifetime = ntohl(dnssl->nd_opt_dnssl_lifetime);
			op = p + offsetof(struct nd_opt_dnssl,
			    nd_opt_dnssl_lifetime);
			op += sizeof(dnssl->nd_opt_dnssl_lifetime);
			n = (dnssl->nd_opt_dnssl_len - 1) * 8;
			l = decode_rfc3397(NULL, 0, n, op);
			if (l < 1) {
				syslog(LOG_ERR, "%s: invalid DNSSL option",
				    ifp->name);
			} else {
				opt = xmalloc(l);
				decode_rfc3397(opt, l, n, op);
			}
			break;
		}

		if (opt == NULL)
			continue;
		for (raol = NULL, rao = rap->options;
		    rao;
		    raol = rao, rao = rao->next)
		{
			if (rao->type == ndo->nd_opt_type &&
			    strcmp(rao->option, opt) == 0)
				break;
		}
		if (lifetime == 0) {
			if (rao) {
				if (raol)
					raol->next = rao->next;
				else
					rap->options = rao->next;
				free(rao->option);
				free(rao);
			}
			continue;
		}

		if (rao == NULL) {
			rao = xmalloc(sizeof(*rao));
			rao->next = rap->options;
			rap->options = rao;
			rao->type = ndo->nd_opt_type;
			rao->option = opt;
		} else
			free(opt);
		if (lifetime == ~0U)
			timerclear(&rao->expire);
		else {
			expire.tv_sec = lifetime;
			expire.tv_usec = 0;
			timeradd(&rap->received, &expire, &rao->expire);
		}
	}

	ipv6rs_sort(ifp);
	run_script_reason(ifp, options & DHCPCD_TEST ? "TEST" : "ROUTERADVERT");
	if (options & DHCPCD_TEST)
		exit(EXIT_SUCCESS);

	/* If we don't require RDNSS then set has_dns = 1 so we fork */
	if (!(ifp->state->options->options & DHCPCD_IPV6RA_REQRDNSS))
		has_dns = 1;

	if (has_dns)
		delete_q_timeout(0, handle_exit_timeout, NULL);
	delete_timeout(NULL, ifp);
	ipv6rs_expire(ifp);
	if (has_dns)
		daemonise();
	else if (options & DHCPCD_DAEMONISE && !(options & DHCPCD_DAEMONISED))
		syslog(LOG_WARNING,
		    "%s: did not fork due to an absent RDNSS option in the RA",
		    ifp->name);
}

ssize_t
ipv6rs_env(char **env, const char *prefix, const struct interface *ifp)
{
	ssize_t l;
	struct timeval now;
	const struct ra *rap;
	const struct ra_opt *rao;
	int i;
	char buffer[32], buffer2[32];
	const char *optn;
	
	l = 0;
	get_monotonic(&now);
	for (rap = ifp->ras, i = 1; rap; rap = rap->next, i++) {
		if (env) {
			snprintf(buffer, sizeof(buffer),
			    "ra%d_from", i);
			setvar(&env, prefix, buffer, rap->sfrom);
		}
		l++;

		for (rao = rap->options; rao; rao = rao->next) {
			if (rao->option == NULL)
				continue;
			if (env == NULL) {
				switch (rao->type) {
				case ND_OPT_PREFIX_INFORMATION:
					l += 4;
					break;
				default:
					l++;
				}
				continue;
			}
			switch (rao->type) {
			case ND_OPT_PREFIX_INFORMATION:
				optn = "prefix";
				break;
			case ND_OPT_MTU:
				optn = "mtu";
				break;
			case ND_OPT_RDNSS:
				optn = "rdnss";
				break;
			case ND_OPT_DNSSL:
				optn = "dnssl";
				break;
			default:
				continue;
			}
			snprintf(buffer, sizeof(buffer), "ra%d_%s", i, optn);
			setvar(&env, prefix, buffer, rao->option);
			l++;
			switch (rao->type) {
			case ND_OPT_PREFIX_INFORMATION:
				snprintf(buffer, sizeof(buffer),
				    "ra%d_prefix_len", i);
				snprintf(buffer2, sizeof(buffer2),
				    "%d", rap->prefix_len);
				setvar(&env, prefix, buffer, buffer2);

				snprintf(buffer, sizeof(buffer),
				    "ra%d_prefix_vltime", i);
				snprintf(buffer2, sizeof(buffer2),
				    "%d", rap->prefix_vltime);
				setvar(&env, prefix, buffer, buffer2);

				snprintf(buffer, sizeof(buffer),
				    "ra%d_prefix_pltime", i);
				snprintf(buffer2, sizeof(buffer2),
				    "%d", rap->prefix_pltime);
				setvar(&env, prefix, buffer, buffer2);
				l += 3;
				break;
			}
		
		}
	}

	if (env)
		setvard(&env, prefix, "ra_count", i - 1);
	l++;
	return l;
}

static void
ipv6rs_free_opts(struct ra *rap)
{
	struct ra_opt *rao, *raon;

	for (rao = rap->options; rao && (raon = rao->next, 1); rao = raon) {
		free(rao->option);
		free(rao);
	}
}

void
ipv6rs_free(struct interface *ifp)
{
	struct ra *rap, *ran;

	free(ifp->rs);
	ifp->rs = NULL;
	for (rap = ifp->ras; rap && (ran = rap->next, 1); rap = ran) {
		ipv6rs_free_opts(rap);
		free(rap->data);
		free(rap);
	}
	ifp->ras = NULL;
}

void
ipv6rs_expire(void *arg)
{
	struct interface *ifp;
	struct ra *rap, *ran, *ral;
	struct ra_opt *rao, *raol, *raon;
	struct timeval now, lt, expire, next;
	int expired;
	uint32_t expire_secs;

	ifp = arg;
	get_monotonic(&now);
	expired = 0;
	expire_secs = ~0U;
	timerclear(&next);

	for (rap = ifp->ras, ral = NULL;
	    rap && (ran = rap->next, 1);
	    ral = rap, rap = ran)
	{
		lt.tv_sec = rap->lifetime;
		lt.tv_usec = 0;
		timeradd(&rap->received, &lt, &expire);
		if (timercmp(&now, &expire, >)) {
			syslog(LOG_INFO, "%s: %s: expired Router Advertisement",
			    ifp->name, rap->sfrom);
			rap->expired = expired = 1;
			if (ral)
				ral->next = ran;
			else
				ifp->ras = ran;
			ipv6rs_free_opts(rap);
			free(rap);
			continue;
		}
		timersub(&expire, &now, &lt);
		if (!timerisset(&next) || timercmp(&next, &lt, >))
			next = lt;
		
		for (rao = rap->options, raol = NULL;
		    rao && (raon = rao->next);
		    raol = rao, rao = raon)
		{
			if (!timerisset(&rao->expire))
				continue;
			if (timercmp(&now, &rao->expire, >)) {
				syslog(LOG_INFO,
				    "%s: %s: expired option %d",
				    ifp->name, rap->sfrom, rao->type);
				rap->expired = expired = 1;
				if (raol)
					raol = raon;
				else
					rap->options = raon;
				continue;
			}
			timersub(&rao->expire, &now, &lt);
			if (!timerisset(&next) || timercmp(&next, &lt, >))
				next = lt;
		}
	}

	if (timerisset(&next))
		add_timeout_tv(&next, ipv6rs_expire, ifp);
	if (expired)
		run_script_reason(ifp, "ROUTERADVERT");
}

int
ipv6rs_start(struct interface *ifp)
{

	delete_timeout(NULL, ifp);

	/* Always make a new probe as the underlying hardware
	 * address could have changed. */
	ipv6rs_makeprobe(ifp);
	if (ifp->rs == NULL)
		return -1;

	ifp->rsprobes = 0;
	ipv6rs_sendprobe(ifp);
	return 0;
}
