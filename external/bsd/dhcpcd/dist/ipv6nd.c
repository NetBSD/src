#include <sys/cdefs.h>
 __RCSID("$NetBSD: ipv6nd.c,v 1.7.4.2 2014/05/22 15:44:40 yamt Exp $");

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
#include <net/if.h>
#include <netinet/in.h>
#include <netinet/ip6.h>
#include <netinet/icmp6.h>

#ifdef __linux__
#  define _LINUX_IN6_H
#  include <linux/ipv6.h>
#endif

#include <errno.h>
#include <fcntl.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>

#define ELOOP_QUEUE 2
#include "common.h"
#include "dhcpcd.h"
#include "dhcp6.h"
#include "eloop.h"
#include "ipv6.h"
#include "ipv6nd.h"
#include "script.h"

/* Debugging Router Solicitations is a lot of spam, so disable it */
//#define DEBUG_RS

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
} __packed;
#endif

#ifndef ND_OPT_DNSSL
#define ND_OPT_DNSSL			31
struct nd_opt_dnssl {		/* DNSSL option RFC 6106 */
	uint8_t		nd_opt_dnssl_type;
	uint8_t		nd_opt_dnssl_len;
	uint16_t	nd_opt_dnssl_reserved;
	uint32_t	nd_opt_dnssl_lifetime;
	/* followed by list of DNS servers */
} __packed;
#endif

/* Minimal IPv6 MTU */
#ifndef IPV6_MMTU
#define IPV6_MMTU 1280
#endif

#ifndef ND_RA_FLAG_RTPREF_HIGH
#define ND_RA_FLAG_RTPREF_MASK		0x18
#define ND_RA_FLAG_RTPREF_HIGH		0x08
#define ND_RA_FLAG_RTPREF_MEDIUM	0x00
#define ND_RA_FLAG_RTPREF_LOW		0x18
#define ND_RA_FLAG_RTPREF_RSV		0x10
#endif

/* RTPREF_MEDIUM has to be 0! */
#define RTPREF_HIGH	1
#define RTPREF_MEDIUM	0
#define RTPREF_LOW	(-1)
#define RTPREF_RESERVED	(-2)
#define RTPREF_INVALID	(-3)	/* internal */

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

static void ipv6nd_handledata(void *arg);

/*
 * Android ships buggy ICMP6 filter headers.
 * Supply our own until they fix their shit.
 * References:
 *     https://android-review.googlesource.com/#/c/58438/
 *     http://code.google.com/p/android/issues/original?id=32621&seq=24
 */
#ifdef __ANDROID__
#undef ICMP6_FILTER_WILLPASS
#undef ICMP6_FILTER_WILLBLOCK
#undef ICMP6_FILTER_SETPASS
#undef ICMP6_FILTER_SETBLOCK
#undef ICMP6_FILTER_SETPASSALL
#undef ICMP6_FILTER_SETBLOCKALL
#define ICMP6_FILTER_WILLPASS(type, filterp) \
	((((filterp)->icmp6_filt[(type) >> 5]) & (1 << ((type) & 31))) == 0)
#define ICMP6_FILTER_WILLBLOCK(type, filterp) \
	((((filterp)->icmp6_filt[(type) >> 5]) & (1 << ((type) & 31))) != 0)
#define ICMP6_FILTER_SETPASS(type, filterp) \
	((((filterp)->icmp6_filt[(type) >> 5]) &= ~(1 << ((type) & 31))))
#define ICMP6_FILTER_SETBLOCK(type, filterp) \
	((((filterp)->icmp6_filt[(type) >> 5]) |=  (1 << ((type) & 31))))
#define ICMP6_FILTER_SETPASSALL(filterp) \
	memset(filterp, 0, sizeof(struct icmp6_filter));
#define ICMP6_FILTER_SETBLOCKALL(filterp) \
	memset(filterp, 0xff, sizeof(struct icmp6_filter));
#endif

static int
ipv6nd_open(struct dhcpcd_ctx *dctx)
{
	struct ipv6_ctx *ctx;
	int on;
	struct icmp6_filter filt;

	ctx = dctx->ipv6;
	if (ctx->nd_fd != -1)
		return ctx->nd_fd;
#ifdef SOCK_CLOEXEC
	ctx->nd_fd = socket(AF_INET6, SOCK_RAW | SOCK_CLOEXEC | SOCK_NONBLOCK,
	    IPPROTO_ICMPV6);
	if (ctx->nd_fd == -1)
		return -1;
#else
	if ((ctx->nd_fd = socket(AF_INET6, SOCK_RAW, IPPROTO_ICMPV6)) == -1)
		return -1;
	if ((on = fcntl(ctx->nd_fd, F_GETFD, 0)) == -1 ||
	    fcntl(ctx->nd_fd, F_SETFD, on | FD_CLOEXEC) == -1)
	{
		close(ctx->nd_fd);
		ctx->nd_fd = -1;
	        return -1;
	}
	if ((on = fcntl(ctx->nd_fd, F_GETFL, 0)) == -1 ||
	    fcntl(ctx->nd_fd, F_SETFL, on | O_NONBLOCK) == -1)
	{
		close(ctx->nd_fd);
		ctx->nd_fd = -1;
	        return -1;
	}
#endif

	/* RFC4861 4.1 */
	on = 255;
	if (setsockopt(ctx->nd_fd, IPPROTO_IPV6, IPV6_MULTICAST_HOPS,
	    &on, sizeof(on)) == -1)
		goto eexit;

	on = 1;
	if (setsockopt(ctx->nd_fd, IPPROTO_IPV6, IPV6_RECVPKTINFO,
	    &on, sizeof(on)) == -1)
		goto eexit;

	on = 1;
	if (setsockopt(ctx->nd_fd, IPPROTO_IPV6, IPV6_RECVHOPLIMIT,
	    &on, sizeof(on)) == -1)
		goto eexit;

	ICMP6_FILTER_SETBLOCKALL(&filt);
	ICMP6_FILTER_SETPASS(ND_NEIGHBOR_ADVERT, &filt);
	ICMP6_FILTER_SETPASS(ND_ROUTER_ADVERT, &filt);
	if (setsockopt(ctx->nd_fd, IPPROTO_ICMPV6, ICMP6_FILTER,
	    &filt, sizeof(filt)) == -1)
		goto eexit;

	eloop_event_add(dctx->eloop, ctx->nd_fd, ipv6nd_handledata, dctx);
	return ctx->nd_fd;

eexit:
	if (ctx->nd_fd != -1) {
		eloop_event_delete(dctx->eloop, ctx->nd_fd);
		close(ctx->nd_fd);
		ctx->nd_fd = -1;
	}
	return -1;
}

static int
ipv6nd_makersprobe(struct interface *ifp)
{
	struct rs_state *state;
	struct nd_router_solicit *rs;
	struct nd_opt_hdr *nd;

	state = RS_STATE(ifp);
	free(state->rs);
	state->rslen = sizeof(*rs) + ROUNDUP8(ifp->hwlen + 2);
	state->rs = calloc(1, state->rslen);
	if (state->rs == NULL)
		return -1;
	rs = (struct nd_router_solicit *)(void *)state->rs;
	rs->nd_rs_type = ND_ROUTER_SOLICIT;
	rs->nd_rs_code = 0;
	rs->nd_rs_cksum = 0;
	rs->nd_rs_reserved = 0;
	nd = (struct nd_opt_hdr *)(state->rs + sizeof(*rs));
	nd->nd_opt_type = ND_OPT_SOURCE_LINKADDR;
	nd->nd_opt_len = (ROUNDUP8(ifp->hwlen + 2)) >> 3;
	memcpy(nd + 1, ifp->hwaddr, ifp->hwlen);
	return 0;
}

static void
ipv6nd_sendrsprobe(void *arg)
{
	struct interface *ifp = arg;
	struct ipv6_ctx *ctx;
	struct rs_state *state;
	struct sockaddr_in6 dst;
	struct cmsghdr *cm;
	struct in6_pktinfo pi;

	if (ipv6_linklocal(ifp) == NULL) {
		syslog(LOG_DEBUG,
		    "%s: delaying Router Solicitation for LL address",
		    ifp->name);
		ipv6_addlinklocalcallback(ifp, ipv6nd_sendrsprobe, ifp);
		return;
	}

	memset(&dst, 0, sizeof(dst));
	dst.sin6_family = AF_INET6;
#ifdef SIN6_LEN
	dst.sin6_len = sizeof(dst);
#endif
	dst.sin6_scope_id = ifp->index;
	if (inet_pton(AF_INET6, ALLROUTERS, &dst.sin6_addr.s6_addr) != 1) {
		syslog(LOG_ERR, "%s: %m", __func__);
		return;
	}

	state = RS_STATE(ifp);
	ctx = ifp->ctx->ipv6;
	ctx->sndhdr.msg_name = (caddr_t)&dst;
	ctx->sndhdr.msg_iov[0].iov_base = state->rs;
	ctx->sndhdr.msg_iov[0].iov_len = state->rslen;

	/* Set the outbound interface */
	cm = CMSG_FIRSTHDR(&ctx->sndhdr);
	if (cm == NULL) /* unlikely */
		return;
	cm->cmsg_level = IPPROTO_IPV6;
	cm->cmsg_type = IPV6_PKTINFO;
	cm->cmsg_len = CMSG_LEN(sizeof(pi));
	memset(&pi, 0, sizeof(pi));
	pi.ipi6_ifindex = ifp->index;
	memcpy(CMSG_DATA(cm), &pi, sizeof(pi));

	syslog(LOG_DEBUG, "%s: sending Router Solicitation", ifp->name);
	if (sendmsg(ctx->nd_fd, &ctx->sndhdr, 0) == -1) {
		syslog(LOG_ERR, "%s: %s: sendmsg: %m", ifp->name, __func__);
		ipv6nd_drop(ifp);
		ifp->options->options &= ~(DHCPCD_IPV6 | DHCPCD_IPV6RS);
		return;
	}

	if (state->rsprobes++ < MAX_RTR_SOLICITATIONS)
		eloop_timeout_add_sec(ifp->ctx->eloop,
		    RTR_SOLICITATION_INTERVAL, ipv6nd_sendrsprobe, ifp);
	else
		syslog(LOG_WARNING, "%s: no IPv6 Routers available", ifp->name);
}

static void
ipv6nd_free_opts(struct ra *rap)
{
	struct ra_opt *rao;

	while ((rao = TAILQ_FIRST(&rap->options))) {
		TAILQ_REMOVE(&rap->options, rao, next);
		free(rao->option);
		free(rao);
	}
}

int
ipv6nd_addrexists(struct dhcpcd_ctx *ctx, const struct ipv6_addr *addr)
{
	struct ra *rap;
	struct ipv6_addr *ap;

	TAILQ_FOREACH(rap, ctx->ipv6->ra_routers, next) {
		TAILQ_FOREACH(ap, &rap->addrs, next) {
			if (addr == NULL) {
				if ((ap->flags &
				    (IPV6_AF_ADDED | IPV6_AF_DADCOMPLETED)) ==
				    (IPV6_AF_ADDED | IPV6_AF_DADCOMPLETED))
					return 1;
			} else if (IN6_ARE_ADDR_EQUAL(&ap->addr, &addr->addr))
				return 1;
		}
	}
	return 0;
}

void ipv6nd_freedrop_ra(struct ra *rap, int drop)
{

	eloop_timeout_delete(rap->iface->ctx->eloop, NULL, rap->iface);
	eloop_timeout_delete(rap->iface->ctx->eloop, NULL, rap);
	if (!drop)
		TAILQ_REMOVE(rap->iface->ctx->ipv6->ra_routers, rap, next);
	ipv6_freedrop_addrs(&rap->addrs, drop, NULL);
	ipv6nd_free_opts(rap);
	free(rap->data);
	free(rap->ns);
	free(rap);
}

ssize_t
ipv6nd_free(struct interface *ifp)
{
	struct rs_state *state;
	struct ra *rap, *ran;
	struct dhcpcd_ctx *ctx;
	ssize_t n;

	state = RS_STATE(ifp);
	if (state == NULL)
		return 0;

	free(state->rs);
	free(state);
	ifp->if_data[IF_DATA_IPV6ND] = NULL;
	n = 0;
	TAILQ_FOREACH_SAFE(rap, ifp->ctx->ipv6->ra_routers, next, ran) {
		if (rap->iface == ifp) {
			ipv6nd_free_ra(rap);
			n++;
		}
	}

	/* If we don't have any more IPv6 enabled interfaces,
	 * close the global socket and release resources */
	ctx = ifp->ctx;
	TAILQ_FOREACH(ifp, ctx->ifaces, next) {
		if (RS_STATE(ifp))
			break;
	}
	if (ifp == NULL) {
		if (ctx->ipv6->nd_fd != -1) {
			eloop_event_delete(ctx->eloop, ctx->ipv6->nd_fd);
			close(ctx->ipv6->nd_fd);
			ctx->ipv6->nd_fd = -1;
		}
	}

	return n;
}

static int
rtpref(struct ra *rap)
{

	switch (rap->flags & ND_RA_FLAG_RTPREF_MASK) {
	case ND_RA_FLAG_RTPREF_HIGH:
		return (RTPREF_HIGH);
	case ND_RA_FLAG_RTPREF_MEDIUM:
	case ND_RA_FLAG_RTPREF_RSV:
		return (RTPREF_MEDIUM);
	case ND_RA_FLAG_RTPREF_LOW:
		return (RTPREF_LOW);
	default:
		syslog(LOG_ERR, "rtpref: impossible RA flag %x", rap->flags);
		return (RTPREF_INVALID);
	}
	/* NOTREACHED */
}

static void
add_router(struct ipv6_ctx *ctx, struct ra *router)
{
	struct ra *rap;

	TAILQ_FOREACH(rap, ctx->ra_routers, next) {
		if (router->iface->metric < rap->iface->metric ||
		    (router->iface->metric == rap->iface->metric &&
		    rtpref(router) > rtpref(rap)))
		{
			TAILQ_INSERT_BEFORE(rap, router, next);
			return;
		}
	}
	TAILQ_INSERT_TAIL(ctx->ra_routers, router, next);
}

static int
ipv6nd_scriptrun(struct ra *rap)
{
	int hasdns;
	struct ipv6_addr *ap;
	const struct ra_opt *rao;

	/* If all addresses have completed DAD run the script */
	TAILQ_FOREACH(ap, &rap->addrs, next) {
		if ((ap->flags & (IPV6_AF_ONLINK | IPV6_AF_AUTOCONF)) ==
		    (IPV6_AF_ONLINK | IPV6_AF_AUTOCONF))
		{
			if (!(ap->flags & IPV6_AF_DADCOMPLETED) &&
			    ipv6_findaddr(ap->iface, &ap->addr))
				ap->flags |= IPV6_AF_DADCOMPLETED;
			if ((ap->flags & IPV6_AF_DADCOMPLETED) == 0) {
				syslog(LOG_DEBUG,
				    "%s: waiting for Router Advertisement"
				    " DAD to complete",
				    rap->iface->name);
				return 0;
			}
		}
	}

	/* If we don't require RDNSS then set hasdns = 1 so we fork */
	if (!(rap->iface->options->options & DHCPCD_IPV6RA_REQRDNSS))
		hasdns = 1;
	else {
		hasdns = 0;
		TAILQ_FOREACH(rao, &rap->options, next) {
			if (rao->type == ND_OPT_RDNSS &&
			    rao->option &&
			    timerisset(&rao->expire))
			{
				hasdns = 1;
				break;
			}
		}
	}

	script_runreason(rap->iface, "ROUTERADVERT");
	if (hasdns)
		hasdns = daemonise(rap->iface->ctx);
#if 0
	else if (options & DHCPCD_DAEMONISE &&
	    !(options & DHCPCD_DAEMONISED) && new_data)
		syslog(LOG_WARNING,
		    "%s: did not fork due to an absent"
		    " RDNSS option in the RA",
		    ifp->name);
}
#endif
	return hasdns;
}

static void
ipv6nd_dadcallback(void *arg)
{
	struct ipv6_addr *ap = arg, *rapap;
	struct interface *ifp;
	struct ra *rap;
	int wascompleted, found;

	wascompleted = (ap->flags & IPV6_AF_DADCOMPLETED);
	ap->flags |= IPV6_AF_DADCOMPLETED;
	if (ap->flags & IPV6_AF_DUPLICATED)
		/* No idea what how to try and make another address :( */
		syslog(LOG_WARNING, "%s: DAD detected %s",
		    ap->iface->name, ap->saddr);

	if (!wascompleted) {
		ifp = ap->iface;

		TAILQ_FOREACH(rap, ifp->ctx->ipv6->ra_routers, next) {
			if (rap->iface != ifp)
				continue;
			wascompleted = 1;
			found = 0;
			TAILQ_FOREACH(rapap, &rap->addrs, next) {
				if (rapap->flags & IPV6_AF_AUTOCONF &&
				    (rapap->flags & IPV6_AF_DADCOMPLETED) == 0)
				{
					wascompleted = 0;
					break;
				}
				if (rapap == ap)
					found = 1;
			}

			if (wascompleted && found) {
				syslog(LOG_DEBUG,
				    "%s: Router Advertisement DAD completed",
				    rap->iface->name);
				if (ipv6nd_scriptrun(rap))
					return;
			}
		}
	}
}

static void
ipv6nd_handlera(struct ipv6_ctx *ctx, struct interface *ifp,
    struct icmp6_hdr *icp, ssize_t len)
{
	ssize_t l, m, n, olen;
	struct nd_router_advert *nd_ra;
	struct nd_opt_prefix_info *pi;
	struct nd_opt_mtu *mtu;
	struct nd_opt_rdnss *rdnss;
	struct nd_opt_dnssl *dnssl;
	uint32_t lifetime, mtuv;
	uint8_t *p, *op;
	struct in6_addr addr;
	char buf[INET6_ADDRSTRLEN];
	const char *cbp;
	struct ra *rap;
	struct nd_opt_hdr *ndo;
	struct ra_opt *rao;
	struct ipv6_addr *ap;
	char *opt, *tmp;
	struct timeval expire;
	uint8_t new_rap, new_data;

	if ((size_t)len < sizeof(struct nd_router_advert)) {
		syslog(LOG_ERR, "IPv6 RA packet too short from %s", ctx->sfrom);
		return;
	}

	if (!IN6_IS_ADDR_LINKLOCAL(&ctx->from.sin6_addr)) {
		syslog(LOG_ERR, "RA from non local address %s", ctx->sfrom);
		return;
	}

	if (ifp == NULL) {
#ifdef DEBUG_RS
		syslog(LOG_DEBUG, "RA for unexpected interface from %s",
		    ctx->sfrom);
#endif
		return;
	}
	if (!(ifp->options->options & DHCPCD_IPV6RS)) {
#ifdef DEBUG_RS
		syslog(LOG_DEBUG, "%s: unexpected RA from %s",
		    ifp->name, ctx->sfrom);
#endif
		return;
	}

	/* We could receive a RA before we sent a RS*/
	if (ipv6_linklocal(ifp) == NULL) {
#ifdef DEBUG_RS
		syslog(LOG_DEBUG, "%s: received RA from %s (no link-local)",
		    ifp->name, ctx->sfrom);
#endif
		return;
	}

	TAILQ_FOREACH(rap, ctx->ra_routers, next) {
		if (ifp == rap->iface &&
		    memcmp(rap->from.s6_addr, ctx->from.sin6_addr.s6_addr,
		    sizeof(rap->from.s6_addr)) == 0)
			break;
	}

	nd_ra = (struct nd_router_advert *)icp;

	/* We don't want to spam the log with the fact we got an RA every
	 * 30 seconds or so, so only spam the log if it's different. */
	if (rap == NULL || (rap->data_len != len ||
	     memcmp(rap->data, (unsigned char *)icp, rap->data_len) != 0))
	{
		if (rap) {
			free(rap->data);
			rap->data_len = 0;
			free(rap->ns);
			rap->ns = NULL;
			rap->nslen = 0;
		}
		new_data = 1;
	} else
		new_data = 0;
	if (new_data || ifp->options->options & DHCPCD_DEBUG)
		syslog(LOG_INFO, "%s: Router Advertisement from %s",
		    ifp->name, ctx->sfrom);

	if (rap == NULL) {
		rap = calloc(1, sizeof(*rap));
		if (rap == NULL) {
			syslog(LOG_ERR, "%s: %m", __func__);
			return;
		}
		rap->iface = ifp;
		memcpy(rap->from.s6_addr, ctx->from.sin6_addr.s6_addr,
		    sizeof(rap->from.s6_addr));
		strlcpy(rap->sfrom, ctx->sfrom, sizeof(rap->sfrom));
		TAILQ_INIT(&rap->addrs);
		TAILQ_INIT(&rap->options);
		new_rap = 1;
	} else
		new_rap = 0;
	if (rap->data_len == 0) {
		rap->data = malloc(len);
		if (rap->data == NULL) {
			syslog(LOG_ERR, "%s: %m", __func__);
			if (new_rap)
				free(rap);
			return;
		}
		memcpy(rap->data, icp, len);
		rap->data_len = len;
	}

	get_monotonic(&rap->received);
	rap->flags = nd_ra->nd_ra_flags_reserved;
	if (new_rap == 0 && rap->lifetime == 0)
		syslog(LOG_WARNING, "%s: %s router available",
		   ifp->name, rap->sfrom);
	rap->lifetime = ntohs(nd_ra->nd_ra_router_lifetime);
	if (nd_ra->nd_ra_reachable) {
		rap->reachable = ntohl(nd_ra->nd_ra_reachable);
		if (rap->reachable > MAX_REACHABLE_TIME)
			rap->reachable = 0;
	}
	if (nd_ra->nd_ra_retransmit)
		rap->retrans = ntohl(nd_ra->nd_ra_retransmit);
	if (rap->lifetime)
		rap->expired = 0;

	len -= sizeof(struct nd_router_advert);
	p = ((uint8_t *)icp) + sizeof(struct nd_router_advert);
	lifetime = ~0U;
	for (; len > 0; p += olen, len -= olen) {
		if ((size_t)len < sizeof(struct nd_opt_hdr)) {
			syslog(LOG_ERR, "%s: short option", ifp->name);
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
			pi = (struct nd_opt_prefix_info *)(void *)ndo;
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
			if (ntohl(pi->nd_opt_pi_preferred_time) >
			    ntohl(pi->nd_opt_pi_valid_time))
			{
				syslog(LOG_ERR,
				    "%s: pltime > vltime", ifp->name);
				break;
			}
			TAILQ_FOREACH(ap, &rap->addrs, next)
				if (ap->prefix_len ==pi->nd_opt_pi_prefix_len &&
				    memcmp(ap->prefix.s6_addr,
				    pi->nd_opt_pi_prefix.s6_addr,
				    sizeof(ap->prefix.s6_addr)) == 0)
					break;
			if (ap == NULL) {
				if (!(pi->nd_opt_pi_flags_reserved &
				    ND_OPT_PI_FLAG_AUTO) &&
				    !(pi->nd_opt_pi_flags_reserved &
				    ND_OPT_PI_FLAG_ONLINK))
					continue;
				ap = calloc(1, sizeof(*ap));
				if (ap == NULL) {
					syslog(LOG_ERR, "%s: %m", __func__);
					break;
				}
				ap->iface = rap->iface;
				ap->flags = IPV6_AF_NEW;
				ap->prefix_len = pi->nd_opt_pi_prefix_len;
				memcpy(ap->prefix.s6_addr,
				   pi->nd_opt_pi_prefix.s6_addr,
				   sizeof(ap->prefix.s6_addr));
				if (pi->nd_opt_pi_flags_reserved &
				    ND_OPT_PI_FLAG_AUTO)
				{
					ap->flags |= IPV6_AF_AUTOCONF;
					ipv6_makeaddr(&ap->addr, ifp,
					    &ap->prefix,
					    pi->nd_opt_pi_prefix_len);
					cbp = inet_ntop(AF_INET6,
					    ap->addr.s6_addr,
					    buf, sizeof(buf));
					if (cbp)
						snprintf(ap->saddr,
						    sizeof(ap->saddr),
						    "%s/%d",
						    cbp, ap->prefix_len);
					else
						ap->saddr[0] = '\0';
				} else {
					memset(&ap->addr, 0, sizeof(ap->addr));
					ap->saddr[0] = '\0';
				}
				ap->dadcallback = ipv6nd_dadcallback;
				TAILQ_INSERT_TAIL(&rap->addrs, ap, next);
			}
			if (pi->nd_opt_pi_flags_reserved &
			    ND_OPT_PI_FLAG_ONLINK)
				ap->flags |= IPV6_AF_ONLINK;
			ap->prefix_vltime =
			    ntohl(pi->nd_opt_pi_valid_time);
			ap->prefix_pltime =
			    ntohl(pi->nd_opt_pi_preferred_time);
			ap->nsprobes = 0;
			if (opt) {
				l = strlen(opt) + 1;
				m = strlen(ap->saddr) + 1;
				tmp = realloc(opt, l + m);
				if (tmp) {
					opt = tmp;
					opt[l - 1] = ' ';
					strlcpy(opt + l, ap->saddr, m);
				}
			} else
				opt = strdup(ap->saddr);
			lifetime = ap->prefix_vltime;
			break;

		case ND_OPT_MTU:
			mtu = (struct nd_opt_mtu *)(void *)p;
			mtuv = ntohl(mtu->nd_opt_mtu_mtu);
			if (mtuv < IPV6_MMTU) {
				syslog(LOG_ERR, "%s: invalid MTU %d",
				    ifp->name, mtuv);
				break;
			}
			rap->mtu = mtuv;
			snprintf(buf, sizeof(buf), "%d", mtuv);
			opt = strdup(buf);
			break;

		case ND_OPT_RDNSS:
			rdnss = (struct nd_opt_rdnss *)p;
			lifetime = ntohl(rdnss->nd_opt_rdnss_lifetime);
			op = (uint8_t *)ndo;
			op += offsetof(struct nd_opt_rdnss,
			    nd_opt_rdnss_lifetime);
			op += sizeof(rdnss->nd_opt_rdnss_lifetime);
			l = 0;
			for (n = ndo->nd_opt_len - 1; n > 1; n -= 2,
			    op += sizeof(addr.s6_addr))
			{
				m = ipv6_printaddr(NULL, 0, op, ifp->name);
				if (m != -1)
					l += m + 1;
			}
			op = (uint8_t *)ndo;
			op += offsetof(struct nd_opt_rdnss,
			    nd_opt_rdnss_lifetime);
			op += sizeof(rdnss->nd_opt_rdnss_lifetime);
			tmp = opt = malloc(l);
			if (opt) {
				for (n = ndo->nd_opt_len - 1; n > 1; n -= 2,
				    op += sizeof(addr.s6_addr))
				{
					m = ipv6_printaddr(tmp, l, op,
					    ifp->name);
					if (m != -1) {
						l -= (m + 1);
						tmp += m;
						*tmp++ = ' ';
					}
				}
				if (tmp != opt)
					(*--tmp) = '\0';
				else
					*opt = '\0';
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
				tmp = malloc(l);
				if (tmp) {
					decode_rfc3397(tmp, l, n, op);
					n = print_string(NULL, 0,
					    l - 1, (const uint8_t *)tmp);
					opt = malloc(n);
					if (opt)
						print_string(opt, n,
						    l - 1,
						    (const uint8_t *)tmp);
					free(tmp);
				}
			}
			break;

		default:
			continue;
		}

		if (opt == NULL) {
			syslog(LOG_ERR, "%s: %m", __func__);
			continue;
		}
		TAILQ_FOREACH(rao, &rap->options, next) {
			if (rao->type == ndo->nd_opt_type &&
			    strcmp(rao->option, opt) == 0)
				break;
		}
		if (lifetime == 0) {
			if (rao) {
				TAILQ_REMOVE(&rap->options, rao, next);
				free(rao->option);
				free(rao);
			}
			free(opt);
			continue;
		}

		if (rao == NULL) {
			rao = malloc(sizeof(*rao));
			if (rao == NULL) {
				syslog(LOG_ERR, "%s: %m", __func__);
				continue;
			}
			rao->type = ndo->nd_opt_type;
			rao->option = opt;
			TAILQ_INSERT_TAIL(&rap->options, rao, next);
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

	if (new_rap)
		add_router(ifp->ctx->ipv6, rap);
	if (ifp->ctx->options & DHCPCD_TEST) {
		script_runreason(ifp, "TEST");
		goto handle_flag;
	}
	ipv6_addaddrs(&rap->addrs);
	ipv6_buildroutes(ifp->ctx);
	if (ipv6nd_scriptrun(rap))
		return;

	eloop_timeout_delete(ifp->ctx->eloop, NULL, ifp);
	eloop_timeout_delete(ifp->ctx->eloop, NULL, rap); /* reachable timer */

	/* If we're owning the RA then we need to try and ensure the
	 * router is actually reachable */
	if (ifp->options->options & DHCPCD_IPV6RA_OWN ||
	    ifp->options->options & DHCPCD_IPV6RA_OWN_DEFAULT)
	{
		rap->nsprobes = 0;
		if (rap->lifetime)
			ipv6nd_proberouter(rap);
	}

handle_flag:
	if (rap->flags & ND_RA_FLAG_MANAGED) {
		if (new_data && dhcp6_start(ifp, DH6S_INIT) == -1)
			syslog(LOG_ERR, "dhcp6_start: %s: %m", ifp->name);
	} else if (rap->flags & ND_RA_FLAG_OTHER) {
		if (new_data && dhcp6_start(ifp, DH6S_INFORM) == -1)
			syslog(LOG_ERR, "dhcp6_start: %s: %m", ifp->name);
	} else {
		if (new_data)
			syslog(LOG_DEBUG, "%s: No DHCPv6 instruction in RA",
			    ifp->name);
		if (ifp->ctx->options & DHCPCD_TEST) {
			eloop_exit(ifp->ctx->eloop, EXIT_SUCCESS);
			return;
		}
	}

	/* Expire should be called last as the rap object could be destroyed */
	ipv6nd_expirera(ifp);
}

int
ipv6nd_has_ra(const struct interface *ifp)
{
	const struct ra *rap;

	if (ifp->ctx->ipv6) {
		TAILQ_FOREACH(rap, ifp->ctx->ipv6->ra_routers, next)
			if (rap->iface == ifp)
				return 1;
	}
	return 0;
}

ssize_t
ipv6nd_env(char **env, const char *prefix, const struct interface *ifp)
{
	ssize_t l;
	size_t len;
	const struct ra *rap;
	const struct ra_opt *rao;
	int i;
	char buffer[32];
	const char *optn;
	char **pref, **mtu, **rdnss, **dnssl, ***var, *new;

	i = 0;
	l = 0;
	TAILQ_FOREACH(rap, ifp->ctx->ipv6->ra_routers, next) {
		i++;
		if (rap->iface != ifp)
			continue;
		if (env) {
			snprintf(buffer, sizeof(buffer),
			    "ra%d_from", i);
			if (setvar(&env, prefix, buffer, rap->sfrom) == -1)
				return -1;
		}
		l++;

		pref = mtu = rdnss = dnssl = NULL;
		TAILQ_FOREACH(rao, &rap->options, next) {
			if (rao->option == NULL)
				continue;
			var = NULL;
			switch(rao->type) {
			case ND_OPT_PREFIX_INFORMATION:
				optn = "prefix";
				var = &pref;
				break;
			case ND_OPT_MTU:
				optn = "mtu";
				var = &mtu;
				break;
			case ND_OPT_RDNSS:
				optn = "rdnss";
				var = &rdnss;
				break;
			case ND_OPT_DNSSL:
				optn = "dnssl";
				var = &dnssl;
				break;
			default:
				continue;
			}
			if (*var == NULL) {
				*var = env ? env : &new;
				l++;
			} else if (env) {
				/* With single only options, last one takes
				 * precedence */
				if (rao->type == ND_OPT_MTU) {
					new = strchr(**var, '=');
					if (new == NULL) {
						syslog(LOG_ERR, "new is null");
						continue;
					} else
						new++;
					len = (new - **var) +
					    strlen(rao->option) + 1;
					if (len > strlen(**var))
						new = realloc(**var, len);
					else
						new = **var;
					if (new) {
						**var = new;
						new = strchr(**var, '=');
						if (new) {
							len -= (new - **var);
							strlcpy(new + 1,
							    rao->option,
							    len - 1);
						} else
							syslog(LOG_ERR,
							    "new is null");
					}
					continue;
				}
				len = strlen(rao->option) + 1;
				new = realloc(**var, strlen(**var) + 1 + len);
				if (new == NULL)
					return -1;
				**var = new;
				new += strlen(new);
				*new++ = ' ';
				strlcpy(new, rao->option, len);
				continue;
			}
			if (env) {
				snprintf(buffer, sizeof(buffer),
				    "ra%d_%s", i, optn);
				if (setvar(&env, prefix, buffer, rao->option)
				    == -1)
					return -1;
			}
		}
	}

	if (env) {
		if (setvard(&env, prefix, "ra_count", i) == -1)
			return -1;
	}
	l++;
	return l;
}

void
ipv6nd_handleifa(struct dhcpcd_ctx *ctx, int cmd, const char *ifname,
    const struct in6_addr *addr, int flags)
{
	struct ra *rap;

	if (ctx->ipv6 == NULL)
		return;
	TAILQ_FOREACH(rap, ctx->ipv6->ra_routers, next) {
		if (strcmp(rap->iface->name, ifname))
			continue;
		ipv6_handleifa_addrs(cmd, &rap->addrs, addr, flags);
	}
}

void
ipv6nd_expirera(void *arg)
{
	struct interface *ifp;
	struct ra *rap, *ran;
	struct ra_opt *rao, *raon;
	struct timeval now, lt, expire, next;
	int expired, valid;

	ifp = arg;
	get_monotonic(&now);
	expired = 0;
	timerclear(&next);

	TAILQ_FOREACH_SAFE(rap, ifp->ctx->ipv6->ra_routers, next, ran) {
		if (rap->iface != ifp)
			continue;
		valid = 0;
		if (rap->lifetime) {
			lt.tv_sec = rap->lifetime;
			lt.tv_usec = 0;
			timeradd(&rap->received, &lt, &expire);
			if (rap->lifetime == 0 || timercmp(&now, &expire, >)) {
				if (!rap->expired) {
					syslog(LOG_WARNING,
					    "%s: %s: router expired",
					    ifp->name, rap->sfrom);
					rap->expired = expired = 1;
					ipv6nd_cancelproberouter(rap);
				}
			} else {
				valid = 1;
				timersub(&expire, &now, &lt);
				if (!timerisset(&next) ||
				    timercmp(&next, &lt, >))
					next = lt;
			}
		}

		/* Addresses are expired in ipv6_addaddrs
		 * so that DHCPv6 addresses can be removed also. */
		TAILQ_FOREACH_SAFE(rao, &rap->options, next, raon) {
			if (rap->expired) {
				switch(rao->type) {
				case ND_OPT_RDNSS: /* FALLTHROUGH */
				case ND_OPT_DNSSL:
					/* RFC6018 end of section 5.2 states
					 * that if tha RA has a lifetime of 0
					 * then we should expire these
					 * options */
					TAILQ_REMOVE(&rap->options, rao, next);
					expired = 1;
					free(rao->option);
					free(rao);
					continue;
				}
			}
			if (!timerisset(&rao->expire))
				continue;
			if (timercmp(&now, &rao->expire, >)) {
				/* Expired prefixes are logged above */
				if (rao->type != ND_OPT_PREFIX_INFORMATION)
					syslog(LOG_WARNING,
					    "%s: %s: expired option %d",
					    ifp->name, rap->sfrom, rao->type);
				TAILQ_REMOVE(&rap->options, rao, next);
				expired = 1;
				free(rao->option);
				free(rao);
				continue;
			}
			valid = 1;
			timersub(&rao->expire, &now, &lt);
			if (!timerisset(&next) || timercmp(&next, &lt, >))
				next = lt;
		}

		/* No valid lifetimes are left on the RA, so we might
		 * as well punt it. */
		if (!valid && TAILQ_FIRST(&rap->addrs) == NULL)
			ipv6nd_free_ra(rap);
	}

	if (timerisset(&next))
		eloop_timeout_add_tv(ifp->ctx->eloop,
		    &next, ipv6nd_expirera, ifp);
	if (expired) {
		ipv6_buildroutes(ifp->ctx);
		script_runreason(ifp, "ROUTERADVERT");
	}
}

void
ipv6nd_drop(struct interface *ifp)
{
	struct ra *rap;
	int expired = 0;
	TAILQ_HEAD(rahead, ra) rtrs;

	if (ifp->ctx->ipv6 == NULL)
		return;

	eloop_timeout_delete(ifp->ctx->eloop, NULL, ifp);
	TAILQ_INIT(&rtrs);
	TAILQ_FOREACH(rap, ifp->ctx->ipv6->ra_routers, next) {
		if (rap->iface == ifp) {
			rap->expired = expired = 1;
			TAILQ_REMOVE(ifp->ctx->ipv6->ra_routers, rap, next);
			TAILQ_INSERT_TAIL(&rtrs, rap, next);
		}
	}
	if (expired) {
		while ((rap = TAILQ_FIRST(&rtrs))) {
			TAILQ_REMOVE(&rtrs, rap, next);
			ipv6nd_drop_ra(rap);
		}
		ipv6_buildroutes(ifp->ctx);
		if ((ifp->options->options &
		    (DHCPCD_EXITING | DHCPCD_PERSISTENT)) !=
		    (DHCPCD_EXITING | DHCPCD_PERSISTENT))
			script_runreason(ifp, "ROUTERADVERT");
	}
}

static void
ipv6nd_unreachable(void *arg)
{
	struct ra *rap = arg;
	struct timeval tv;

	/* We could add an unreachable flag and persist the information,
	 * but that is more effort than it's probably worth. */
	syslog(LOG_WARNING, "%s: %s is unreachable, expiring it",
	    rap->iface->name, rap->sfrom);
	rap->expired = 1;
	ipv6_buildroutes(rap->iface->ctx);
	script_runreason(rap->iface, "ROUTERADVERT"); /* XXX not RA */

	/* We should still test if it's reachable or not so
	 * incase it comes back to life and it's preferable. */
	if (rap->reachable) {
		ms_to_tv(&tv, rap->reachable);
	} else {
		tv.tv_sec = REACHABLE_TIME;
		tv.tv_usec = 0;
	}
	eloop_timeout_add_tv(rap->iface->ctx->eloop,
	    &tv, ipv6nd_proberouter, rap);
}

void
ipv6nd_proberouter(void *arg)
{
	struct ra *rap = arg;
	struct nd_neighbor_solicit *ns;
	struct nd_opt_hdr *nd;
	struct sockaddr_in6 dst;
	struct cmsghdr *cm;
	struct in6_pktinfo pi;
	struct timeval tv, rtv;
	struct ipv6_ctx *ctx;

	if (ipv6nd_open(rap->iface->ctx) == -1) {
		syslog(LOG_ERR, "%s: ipv6nd_open: %m", __func__);
		return;
	}

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

	ctx = rap->iface->ctx->ipv6;
	ctx->sndhdr.msg_name = (caddr_t)&dst;
	ctx->sndhdr.msg_iov[0].iov_base = rap->ns;
	ctx->sndhdr.msg_iov[0].iov_len = rap->nslen;

	/* Set the outbound interface */
	cm = CMSG_FIRSTHDR(&ctx->sndhdr);
	if (cm == NULL) /* unlikely */
		return;
	cm->cmsg_level = IPPROTO_IPV6;
	cm->cmsg_type = IPV6_PKTINFO;
	cm->cmsg_len = CMSG_LEN(sizeof(pi));
	memset(&pi, 0, sizeof(pi));
	pi.ipi6_ifindex = rap->iface->index;
	memcpy(CMSG_DATA(cm), &pi, sizeof(pi));

#ifdef DEBUG_NS
	syslog(LOG_INFO, "%s: sending IPv6 NS for %s",
	    rap->iface->name, rap->sfrom);
#endif
	if (sendmsg(ctx->nd_fd, &ctx->sndhdr, 0) == -1) {
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
	eloop_timeout_add_tv(rap->iface->ctx->eloop,
	    &tv, ipv6nd_proberouter, rap);

	if (rap->nsprobes++ == 0)
		eloop_timeout_add_sec(rap->iface->ctx->eloop,
		    DELAY_FIRST_PROBE_TIME, ipv6nd_unreachable, rap);
}

void
ipv6nd_cancelproberouter(struct ra *rap)
{

	eloop_timeout_delete(rap->iface->ctx->eloop, ipv6nd_proberouter, rap);
	eloop_timeout_delete(rap->iface->ctx->eloop, ipv6nd_unreachable, rap);
}

static void
ipv6nd_handlena(struct ipv6_ctx *ctx, struct interface *ifp,
    struct icmp6_hdr *icp, ssize_t len)
{
	struct nd_neighbor_advert *nd_na;
	struct ra *rap;
	int is_router, is_solicited;
	struct timeval tv;

	if ((size_t)len < sizeof(struct nd_neighbor_advert)) {
		syslog(LOG_ERR, "IPv6 NA packet too short from %s", ctx->sfrom);
		return;
	}

	if (ifp == NULL) {
#ifdef DEBUG_NS
		syslog(LOG_DEBUG, "NA for unexpected interface from %s",
		    ctx->sfrom);
#endif
		return;
	}

	nd_na = (struct nd_neighbor_advert *)icp;
	is_router = nd_na->nd_na_flags_reserved & ND_NA_FLAG_ROUTER;
	is_solicited = nd_na->nd_na_flags_reserved & ND_NA_FLAG_SOLICITED;

	if (IN6_IS_ADDR_MULTICAST(&nd_na->nd_na_target)) {
		syslog(LOG_ERR, "%s: NA for multicast address from %s",
		    ifp->name, ctx->sfrom);
		return;
	}

	TAILQ_FOREACH(rap, ctx->ra_routers, next) {
		if (rap->iface == ifp &&
		    memcmp(rap->from.s6_addr, nd_na->nd_na_target.s6_addr,
		    sizeof(rap->from.s6_addr)) == 0)
			break;
	}
	if (rap == NULL) {
#ifdef DEBUG_NS
		syslog(LOG_DEBUG, "%s: unexpected NA from s",
		    ifp->name, ctx->sfrom);
#endif
		return;
	}

#ifdef DEBUG_NS
	syslog(LOG_DEBUG, "%s: %sNA from %s",
	    ifp->name, is_solicited ? "solicited " : "", ctx->sfrom);
#endif

	/* Node is no longer a router, so remove it from consideration */
	if (!is_router && !rap->expired) {
		syslog(LOG_INFO, "%s: %s is no longer a router",
		    ifp->name, ctx->sfrom);
		rap->expired = 1;
		ipv6nd_cancelproberouter(rap);
		ipv6_buildroutes(ifp->ctx);
		script_runreason(ifp, "ROUTERADVERT");
		return;
	}

	if (is_solicited && is_router && rap->lifetime) {
		if (rap->expired) {
			rap->expired = 0;
			syslog(LOG_INFO, "%s: %s is reachable again",
			    ifp->name, ctx->sfrom);
			ipv6_buildroutes(ifp->ctx);
			script_runreason(rap->iface, "ROUTERADVERT"); /* XXX */
		}
		rap->nsprobes = 0;
		if (rap->reachable) {
			ms_to_tv(&tv, rap->reachable);
		} else {
			tv.tv_sec = REACHABLE_TIME;
			tv.tv_usec = 0;
		}
		eloop_timeout_add_tv(rap->iface->ctx->eloop,
		    &tv, ipv6nd_proberouter, rap);
		eloop_timeout_delete(rap->iface->ctx->eloop,
		    ipv6nd_unreachable, rap);
	}
}

static void
ipv6nd_handledata(void *arg)
{
	struct dhcpcd_ctx *dhcpcd_ctx;
	struct ipv6_ctx *ctx;
	ssize_t len;
	struct cmsghdr *cm;
	int hoplimit;
	struct in6_pktinfo pkt;
	struct icmp6_hdr *icp;
	struct interface *ifp;

	dhcpcd_ctx = arg;
	ctx = dhcpcd_ctx->ipv6;
	ctx->rcvhdr.msg_controllen = CMSG_SPACE(sizeof(struct in6_pktinfo)) +
	    CMSG_SPACE(sizeof(int));
	len = recvmsg(ctx->nd_fd, &ctx->rcvhdr, 0);
	if (len == -1) {
		syslog(LOG_ERR, "recvmsg: %m");
		return;
	}
	ctx->sfrom = inet_ntop(AF_INET6, &ctx->from.sin6_addr,
	    ctx->ntopbuf, INET6_ADDRSTRLEN);
	if ((size_t)len < sizeof(struct icmp6_hdr)) {
		syslog(LOG_ERR, "IPv6 ICMP packet too short from %s",
		    ctx->sfrom);
		return;
	}

	pkt.ipi6_ifindex = hoplimit = 0;
	for (cm = (struct cmsghdr *)CMSG_FIRSTHDR(&ctx->rcvhdr);
	     cm;
	     cm = (struct cmsghdr *)CMSG_NXTHDR(&ctx->rcvhdr, cm))
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
		    "IPv6 RA/NA did not contain index or hop limit from %s",
		    ctx->sfrom);
		return;
	}

	TAILQ_FOREACH(ifp, dhcpcd_ctx->ifaces, next) {
		if (ifp->index == (unsigned int)pkt.ipi6_ifindex)
			break;
	}

	icp = (struct icmp6_hdr *)ctx->rcvhdr.msg_iov[0].iov_base;
	if (icp->icmp6_code == 0) {
		switch(icp->icmp6_type) {
			case ND_NEIGHBOR_ADVERT:
				ipv6nd_handlena(ctx, ifp, icp, len);
				return;
			case ND_ROUTER_ADVERT:
				ipv6nd_handlera(ctx, ifp, icp, len);
				return;
		}
	}

	syslog(LOG_ERR, "invalid IPv6 type %d or code %d from %s",
	    icp->icmp6_type, icp->icmp6_code, ctx->sfrom);
}

int
ipv6nd_startrs(struct interface *ifp)
{
	struct rs_state *state;

	syslog(LOG_INFO, "%s: soliciting an IPv6 router", ifp->name);
	if (ipv6nd_open(ifp->ctx) == -1) {
		syslog(LOG_ERR, "%s: ipv6nd_open: %m", __func__);
		return -1;
	}

	eloop_timeout_delete(ifp->ctx->eloop, NULL, ifp);

	state = RS_STATE(ifp);
	if (state == NULL) {
		ifp->if_data[IF_DATA_IPV6ND] = calloc(1, sizeof(*state));
		state = RS_STATE(ifp);
		if (state == NULL) {
			syslog(LOG_ERR, "%s: %m", __func__);
			return -1;
		}
	}

	/* Always make a new probe as the underlying hardware
	 * address could have changed. */
	ipv6nd_makersprobe(ifp);
	if (state->rs == NULL) {
		syslog(LOG_ERR, "%s: ipv6ns_makersprobe: %m", __func__);
		return -1;
	}

	state->rsprobes = 0;
	ipv6nd_sendrsprobe(ifp);
	return 0;
}
