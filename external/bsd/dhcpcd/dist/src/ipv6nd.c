/*
 * dhcpcd - IPv6 ND handling
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
#include <net/if.h>
#include <net/route.h>
#include <netinet/in.h>
#include <netinet/ip6.h>
#include <netinet/icmp6.h>

#include <errno.h>
#include <fcntl.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define ELOOP_QUEUE 3
#include "common.h"
#include "dhcpcd.h"
#include "dhcp6.h"
#include "eloop.h"
#include "if.h"
#include "ipv6.h"
#include "ipv6nd.h"
#include "logerr.h"
#include "route.h"
#include "script.h"

/* Debugging Router Solicitations is a lot of spam, so disable it */
//#define DEBUG_RS

#ifndef ND_OPT_RDNSS
#define ND_OPT_RDNSS			25
struct nd_opt_rdnss {           /* RDNSS option RFC 6106 */
	uint8_t		nd_opt_rdnss_type;
	uint8_t		nd_opt_rdnss_len;
	uint16_t	nd_opt_rdnss_reserved;
	uint32_t	nd_opt_rdnss_lifetime;
        /* followed by list of IP prefixes */
};
__CTASSERT(sizeof(struct nd_opt_rdnss) == 8);
#endif

#ifndef ND_OPT_DNSSL
#define ND_OPT_DNSSL			31
struct nd_opt_dnssl {		/* DNSSL option RFC 6106 */
	uint8_t		nd_opt_dnssl_type;
	uint8_t		nd_opt_dnssl_len;
	uint16_t	nd_opt_dnssl_reserved;
	uint32_t	nd_opt_dnssl_lifetime;
	/* followed by list of DNS servers */
};
__CTASSERT(sizeof(struct nd_opt_rdnss) == 8);
#endif

/* Impossible options, so we can easily add extras */
#define _ND_OPT_PREFIX_ADDR	255 + 1

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

static void ipv6nd_handledata(void *);

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

/* Support older systems with different defines */
#if !defined(IPV6_RECVHOPLIMIT) && defined(IPV6_HOPLIMIT)
#define IPV6_RECVHOPLIMIT IPV6_HOPLIMIT
#endif
#if !defined(IPV6_RECVPKTINFO) && defined(IPV6_PKTINFO)
#define IPV6_RECVPKTINFO IPV6_PKTINFO
#endif

/* Handy defines */
#define ipv6nd_free_ra(ra) ipv6nd_freedrop_ra((ra),  0)
#define ipv6nd_drop_ra(ra) ipv6nd_freedrop_ra((ra),  1)

void
ipv6nd_printoptions(const struct dhcpcd_ctx *ctx,
    const struct dhcp_opt *opts, size_t opts_len)
{
	size_t i, j;
	const struct dhcp_opt *opt, *opt2;
	int cols;

	for (i = 0, opt = ctx->nd_opts;
	    i < ctx->nd_opts_len; i++, opt++)
	{
		for (j = 0, opt2 = opts; j < opts_len; j++, opt2++)
			if (opt2->option == opt->option)
				break;
		if (j == opts_len) {
			cols = printf("%03d %s", opt->option, opt->var);
			dhcp_print_option_encoding(opt, cols);
		}
	}
	for (i = 0, opt = opts; i < opts_len; i++, opt++) {
		cols = printf("%03d %s", opt->option, opt->var);
		dhcp_print_option_encoding(opt, cols);
	}
}

static int
ipv6nd_open(struct dhcpcd_ctx *ctx)
{
	int on;
	struct icmp6_filter filt;

	if (ctx->nd_fd != -1)
		return ctx->nd_fd;
#define SOCK_FLAGS	SOCK_CLOEXEC | SOCK_NONBLOCK
	ctx->nd_fd = xsocket(PF_INET6, SOCK_RAW | SOCK_FLAGS, IPPROTO_ICMPV6);
#undef SOCK_FLAGS
	if (ctx->nd_fd == -1)
		return -1;

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

	eloop_event_add(ctx->eloop, ctx->nd_fd,  ipv6nd_handledata, ctx);
	return ctx->nd_fd;

eexit:
	if (ctx->nd_fd != -1) {
		eloop_event_delete(ctx->eloop, ctx->nd_fd);
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

	state = RS_STATE(ifp);
	free(state->rs);
	state->rslen = sizeof(*rs);
	if (ifp->hwlen != 0)
		state->rslen += (size_t)ROUNDUP8(ifp->hwlen + 2);
	state->rs = calloc(1, state->rslen);
	if (state->rs == NULL)
		return -1;
	rs = state->rs;
	rs->nd_rs_type = ND_ROUTER_SOLICIT;
	//rs->nd_rs_code = 0;
	//rs->nd_rs_cksum = 0;
	//rs->nd_rs_reserved = 0;

	if (ifp->hwlen != 0) {
		struct nd_opt_hdr *nd;

		nd = (struct nd_opt_hdr *)(state->rs + 1);
		nd->nd_opt_type = ND_OPT_SOURCE_LINKADDR;
		nd->nd_opt_len = (uint8_t)((ROUNDUP8(ifp->hwlen + 2)) >> 3);
		memcpy(nd + 1, ifp->hwaddr, ifp->hwlen);
	}
	return 0;
}

static void
ipv6nd_sendrsprobe(void *arg)
{
	struct interface *ifp = arg;
	struct dhcpcd_ctx *ctx;
	struct rs_state *state;
	struct sockaddr_in6 dst;
	struct cmsghdr *cm;
	struct in6_pktinfo pi;

	if (ipv6_linklocal(ifp) == NULL) {
		logdebugx("%s: delaying Router Solicitation for LL address",
		    ifp->name);
		ipv6_addlinklocalcallback(ifp, ipv6nd_sendrsprobe, ifp);
		return;
	}

	memset(&dst, 0, sizeof(dst));
	dst.sin6_family = AF_INET6;
#ifdef HAVE_SA_LEN
	dst.sin6_len = sizeof(dst);
#endif
	dst.sin6_scope_id = ifp->index;
	if (inet_pton(AF_INET6, ALLROUTERS, &dst.sin6_addr) != 1) {
		logerr(__func__);
		return;
	}

	state = RS_STATE(ifp);
	ctx = ifp->ctx;
	ctx->sndhdr.msg_name = (void *)&dst;
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

	logdebugx("%s: sending Router Solicitation", ifp->name);
	if (sendmsg(ctx->nd_fd, &ctx->sndhdr, 0) == -1) {
		logerr(__func__);
		/* Allow IPv6ND to continue .... at most a few errors
		 * would be logged.
		 * Generally the error is ENOBUFS when struggling to
		 * associate with an access point. */
	}

	if (state->rsprobes++ < MAX_RTR_SOLICITATIONS)
		eloop_timeout_add_sec(ifp->ctx->eloop,
		    RTR_SOLICITATION_INTERVAL, ipv6nd_sendrsprobe, ifp);
	else {
		logwarnx("%s: no IPv6 Routers available", ifp->name);
		ipv6nd_drop(ifp);
		dhcp6_dropnondelegates(ifp);
	}
}

void
ipv6nd_expire(struct interface *ifp, uint32_t seconds)
{
	struct ra *rap;
	struct timespec now;
	uint32_t vltime = seconds;
	uint32_t pltime = seconds / 2;

	if (ifp->ctx->ra_routers == NULL)
		return;

	clock_gettime(CLOCK_MONOTONIC, &now);

	TAILQ_FOREACH(rap, ifp->ctx->ra_routers, next) {
		if (rap->iface == ifp) {
			rap->acquired = now;
			rap->expired = seconds ? 0 : 1;
			if (seconds) {
				struct ipv6_addr *ia;

				rap->lifetime = seconds;
				TAILQ_FOREACH(ia, &rap->addrs, next) {
					if (ia->prefix_pltime > pltime ||
					    ia->prefix_vltime > vltime)
					{
						ia->acquired = now;
						if (ia->prefix_pltime != 0)
							ia->prefix_pltime =
							    pltime;
						ia->prefix_vltime = vltime;
					}
				}
				ipv6_addaddrs(&rap->addrs);
			}
		}
	}
	if (seconds)
		ipv6nd_expirera(ifp);
	else
		rt_build(ifp->ctx, AF_INET6);
}

static void
ipv6nd_reachable(struct ra *rap, int flags)
{

	if (rap->lifetime == 0)
		return;

	if (flags & IPV6ND_REACHABLE) {
		if (rap->expired == 0)
			return;
		loginfox("%s: %s is reachable again",
		    rap->iface->name, rap->sfrom);
		rap->expired = 0;
	} else {
		if (rap->expired != 0)
			return;
		logwarnx("%s: %s is unreachable, expiring it",
		    rap->iface->name, rap->sfrom);
		rap->expired = 1;
	}

	rt_build(rap->iface->ctx, AF_INET6);
	/* XXX Not really an RA */
	script_runreason(rap->iface, "ROUTERADVERT");
}

void
ipv6nd_neighbour(struct dhcpcd_ctx *ctx, struct in6_addr *addr, int flags)
{
	struct ra *rap;

	if (ctx->ra_routers) {
	        TAILQ_FOREACH(rap, ctx->ra_routers, next) {
			if (IN6_ARE_ADDR_EQUAL(&rap->from, addr)) {
				ipv6nd_reachable(rap, flags);
				break;
			}
		}
	}
}

const struct ipv6_addr *
ipv6nd_iffindaddr(const struct interface *ifp, const struct in6_addr *addr,
    unsigned int flags)
{
	struct ra *rap;
	struct ipv6_addr *ap;

	if (ifp->ctx->ra_routers == NULL)
		return NULL;

	TAILQ_FOREACH(rap, ifp->ctx->ra_routers, next) {
		if (rap->iface != ifp)
			continue;
		TAILQ_FOREACH(ap, &rap->addrs, next) {
			if (ipv6_findaddrmatch(ap, addr, flags))
				return ap;
		}
	}
	return NULL;
}

struct ipv6_addr *
ipv6nd_findaddr(struct dhcpcd_ctx *ctx, const struct in6_addr *addr,
    unsigned int flags)
{
	struct ra *rap;
	struct ipv6_addr *ap;

	if (ctx->ra_routers == NULL)
		return NULL;

	TAILQ_FOREACH(rap, ctx->ra_routers, next) {
		TAILQ_FOREACH(ap, &rap->addrs, next) {
			if (ipv6_findaddrmatch(ap, addr, flags))
				return ap;
		}
	}
	return NULL;
}

static void
ipv6nd_removefreedrop_ra(struct ra *rap, int remove_ra, int drop_ra)
{

	eloop_timeout_delete(rap->iface->ctx->eloop, NULL, rap->iface);
	eloop_timeout_delete(rap->iface->ctx->eloop, NULL, rap);
	if (remove_ra)
		TAILQ_REMOVE(rap->iface->ctx->ra_routers, rap, next);
	ipv6_freedrop_addrs(&rap->addrs, drop_ra, NULL);
	free(rap->data);
	free(rap);
}

static void
ipv6nd_freedrop_ra(struct ra *rap, int drop)
{

	ipv6nd_removefreedrop_ra(rap, 1, drop);
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
	TAILQ_FOREACH_SAFE(rap, ifp->ctx->ra_routers, next, ran) {
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
		if (ctx->nd_fd != -1) {
			eloop_event_delete(ctx->eloop, ctx->nd_fd);
			close(ctx->nd_fd);
			ctx->nd_fd = -1;
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
		logerrx("rtpref: impossible RA flag %x", rap->flags);
		return (RTPREF_INVALID);
	}
	/* NOTREACHED */
}

static void
add_router(struct dhcpcd_ctx *ctx, struct ra *router)
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
	int hasdns, hasaddress, pid;
	struct ipv6_addr *ap;

	hasaddress = 0;
	/* If all addresses have completed DAD run the script */
	TAILQ_FOREACH(ap, &rap->addrs, next) {
		if ((ap->flags & (IPV6_AF_AUTOCONF | IPV6_AF_ADDED)) ==
		    (IPV6_AF_AUTOCONF | IPV6_AF_ADDED))
		{
			hasaddress = 1;
			if (!(ap->flags & IPV6_AF_DADCOMPLETED) &&
			    ipv6_iffindaddr(ap->iface, &ap->addr,
			    IN6_IFF_TENTATIVE))
				ap->flags |= IPV6_AF_DADCOMPLETED;
			if ((ap->flags & IPV6_AF_DADCOMPLETED) == 0) {
				logdebugx("%s: waiting for Router Advertisement"
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
		hasdns = rap->hasdns;
	}

	script_runreason(rap->iface, "ROUTERADVERT");
	pid = 0;
	if (hasdns && (hasaddress ||
	    !(rap->flags & (ND_RA_FLAG_MANAGED | ND_RA_FLAG_OTHER))))
		pid = dhcpcd_daemonise(rap->iface->ctx);
#if 0
	else if (options & DHCPCD_DAEMONISE &&
	    !(options & DHCPCD_DAEMONISED) && new_data)
		logwarnx("%s: did not fork due to an absent"
		    " RDNSS option in the RA",
		    ifp->name);
}
#endif
	return pid;
}

static void
ipv6nd_addaddr(void *arg)
{
	struct ipv6_addr *ap = arg;

	ipv6_addaddr(ap, NULL);
}

int
ipv6nd_dadcompleted(const struct interface *ifp)
{
	const struct ra *rap;
	const struct ipv6_addr *ap;

	TAILQ_FOREACH(rap, ifp->ctx->ra_routers, next) {
		if (rap->iface != ifp)
			continue;
		TAILQ_FOREACH(ap, &rap->addrs, next) {
			if (ap->flags & IPV6_AF_AUTOCONF &&
			    ap->flags & IPV6_AF_ADDED &&
			    !(ap->flags & IPV6_AF_DADCOMPLETED))
				return 0;
		}
	}
	return 1;
}

static void
ipv6nd_dadcallback(void *arg)
{
	struct ipv6_addr *ia = arg, *rapap;
	struct interface *ifp;
	struct ra *rap;
	int wascompleted, found;
	struct timespec tv;
	char buf[INET6_ADDRSTRLEN];
	const char *p;
	int dadcounter;

	ifp = ia->iface;
	wascompleted = (ia->flags & IPV6_AF_DADCOMPLETED);
	ia->flags |= IPV6_AF_DADCOMPLETED;
	if (ia->flags & IPV6_AF_DUPLICATED) {
		ia->dadcounter++;
		logwarnx("%s: DAD detected %s", ifp->name, ia->saddr);

		/* Try and make another stable private address.
		 * Because ap->dadcounter is always increamented,
		 * a different address is generated. */
		/* XXX Cache DAD counter per prefix/id/ssid? */
		if (ifp->options->options & DHCPCD_SLAACPRIVATE) {
			if (ia->dadcounter >= IDGEN_RETRIES) {
				logerrx("%s: unable to obtain a"
				    " stable private address",
				    ifp->name);
				goto try_script;
			}
			loginfox("%s: deleting address %s",
			    ifp->name, ia->saddr);
			if (if_address6(RTM_DELADDR, ia) == -1 &&
			    errno != EADDRNOTAVAIL && errno != ENXIO)
				logerr(__func__);
			dadcounter = ia->dadcounter;
			if (ipv6_makestableprivate(&ia->addr,
			    &ia->prefix, ia->prefix_len,
			    ifp, &dadcounter) == -1)
			{
				logerr("ipv6_makestableprivate");
				return;
			}
			ia->dadcounter = dadcounter;
			ia->flags &= ~(IPV6_AF_ADDED | IPV6_AF_DADCOMPLETED);
			ia->flags |= IPV6_AF_NEW;
			p = inet_ntop(AF_INET6, &ia->addr, buf, sizeof(buf));
			if (p)
				snprintf(ia->saddr,
				    sizeof(ia->saddr),
				    "%s/%d",
				    p, ia->prefix_len);
			else
				ia->saddr[0] = '\0';
			tv.tv_sec = 0;
			tv.tv_nsec = (suseconds_t)
			    arc4random_uniform(IDGEN_DELAY * NSEC_PER_SEC);
			timespecnorm(&tv);
			eloop_timeout_add_tv(ifp->ctx->eloop, &tv,
			    ipv6nd_addaddr, ia);
			return;
		}
	}

try_script:
	if (!wascompleted) {
		TAILQ_FOREACH(rap, ifp->ctx->ra_routers, next) {
			if (rap->iface != ifp)
				continue;
			wascompleted = 1;
			found = 0;
			TAILQ_FOREACH(rapap, &rap->addrs, next) {
				if (rapap->flags & IPV6_AF_AUTOCONF &&
				    rapap->flags & IPV6_AF_ADDED &&
				    (rapap->flags & IPV6_AF_DADCOMPLETED) == 0)
				{
					wascompleted = 0;
					break;
				}
				if (rapap == ia)
					found = 1;
			}

			if (wascompleted && found) {
				logdebugx("%s: Router Advertisement DAD "
				    "completed",
				    rap->iface->name);
				if (ipv6nd_scriptrun(rap))
					return;
			}
		}
	}
}

#ifndef DHCP6
/* If DHCPv6 is compiled out, supply a shim to provide an error message
 * if IPv6RA requests DHCPv6. */
#undef dhcp6_start
static int
dhcp6_start(__unused struct interface *ifp, __unused enum DH6S init_state)
{

	errno = ENOTSUP;
	return -1;
}
#endif

static void
ipv6nd_handlera(struct dhcpcd_ctx *ctx, struct interface *ifp,
    struct icmp6_hdr *icp, size_t len, int hoplimit)
{
	size_t i, olen;
	struct nd_router_advert *nd_ra;
	struct nd_opt_hdr ndo;
	struct nd_opt_prefix_info pi;
	struct nd_opt_mtu mtu;
	struct nd_opt_rdnss rdnss;
	uint8_t *p;
	struct ra *rap;
	struct in6_addr pi_prefix;
	struct ipv6_addr *ap;
	struct dhcp_opt *dho;
	uint8_t new_rap, new_data;
	__printflike(1, 2) void (*logfunc)(const char *, ...);
#ifdef IPV6_MANAGETEMPADDR
	uint8_t new_ap;
#endif

	if (ifp == NULL) {
#ifdef DEBUG_RS
		logdebugx("RA for unexpected interface from %s",
		    ctx->sfrom);
#endif
		return;
	}

	if (len < sizeof(struct nd_router_advert)) {
		logerr("IPv6 RA packet too short from %s", ctx->sfrom);
		return;
	}

	/* RFC 4861 7.1.2 */
	if (hoplimit != 255) {
		logerr("invalid hoplimit(%d) in RA from %s",
		    hoplimit, ctx->sfrom);
		return;
	}

	if (!IN6_IS_ADDR_LINKLOCAL(&ctx->from.sin6_addr)) {
		logerr("RA from non local address %s", ctx->sfrom);
		return;
	}

	if (!(ifp->options->options & DHCPCD_IPV6RS)) {
#ifdef DEBUG_RS
		logerr("%s: unexpected RA from %s",
		    ifp->name, ctx->sfrom);
#endif
		return;
	}

	/* We could receive a RA before we sent a RS*/
	if (ipv6_linklocal(ifp) == NULL) {
#ifdef DEBUG_RS
		logdebugx("%s: received RA from %s (no link-local)",
		    ifp->name, ctx->sfrom);
#endif
		return;
	}

	if (ipv6_iffindaddr(ifp, &ctx->from.sin6_addr, IN6_IFF_TENTATIVE)) {
		logdebugx("%s: ignoring RA from ourself %s",
		    ifp->name, ctx->sfrom);
		return;
	}

	TAILQ_FOREACH(rap, ctx->ra_routers, next) {
		if (ifp == rap->iface &&
		    IN6_ARE_ADDR_EQUAL(&rap->from, &ctx->from.sin6_addr))
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
		}
		new_data = 1;
	} else
		new_data = 0;
	if (rap == NULL) {
		rap = calloc(1, sizeof(*rap));
		if (rap == NULL) {
			logerr(__func__);
			return;
		}
		rap->iface = ifp;
		rap->from = ctx->from.sin6_addr;
		strlcpy(rap->sfrom, ctx->sfrom, sizeof(rap->sfrom));
		TAILQ_INIT(&rap->addrs);
		new_rap = 1;
	} else
		new_rap = 0;
	if (rap->data_len == 0) {
		rap->data = malloc(len);
		if (rap->data == NULL) {
			logerr(__func__);
			if (new_rap)
				free(rap);
			return;
		}
		memcpy(rap->data, icp, len);
		rap->data_len = len;
	}

	/* We could change the debug level based on new_data, but some
	 * routers like to decrease the advertised valid and preferred times
	 * in accordance with the own prefix times which would result in too
	 * much needless log spam. */
	logfunc = new_rap ? loginfox : logdebugx,
	logfunc("%s: Router Advertisement from %s",
	    ifp->name, ctx->sfrom);

	clock_gettime(CLOCK_MONOTONIC, &rap->acquired);
	rap->flags = nd_ra->nd_ra_flags_reserved;
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
	rap->hasdns = 0;

#ifdef IPV6_AF_TEMPORARY
	ipv6_markaddrsstale(ifp, IPV6_AF_TEMPORARY);
#endif
	TAILQ_FOREACH(ap, &rap->addrs, next) {
		ap->flags |= IPV6_AF_STALE;
	}

	len -= sizeof(struct nd_router_advert);
	p = ((uint8_t *)icp) + sizeof(struct nd_router_advert);
	for (; len > 0; p += olen, len -= olen) {
		if (len < sizeof(ndo)) {
			logerrx("%s: short option", ifp->name);
			break;
		}
		memcpy(&ndo, p, sizeof(ndo));
		olen = (size_t)ndo.nd_opt_len * 8;
		if (olen == 0) {
			logerrx("%s: zero length option", ifp->name);
			break;
		}
		if (olen > len) {
			logerrx("%s: option length exceeds message",
			    ifp->name);
			break;
		}

		if (has_option_mask(ifp->options->rejectmasknd,
		    ndo.nd_opt_type))
		{
			for (i = 0, dho = ctx->nd_opts;
			    i < ctx->nd_opts_len;
			    i++, dho++)
			{
				if (dho->option == ndo.nd_opt_type)
					break;
			}
			if (dho != NULL)
				logwarnx("%s: reject RA (option %s) from %s",
				    ifp->name, dho->var, ctx->sfrom);
			else
				logwarnx("%s: reject RA (option %d) from %s",
				    ifp->name, ndo.nd_opt_type, ctx->sfrom);
			if (new_rap)
				ipv6nd_removefreedrop_ra(rap, 0, 0);
			else
				ipv6nd_free_ra(rap);
			return;
		}

		if (has_option_mask(ifp->options->nomasknd, ndo.nd_opt_type))
			continue;

		switch (ndo.nd_opt_type) {
		case ND_OPT_PREFIX_INFORMATION:
			logfunc = new_data ? logerrx : logdebugx;
			if (ndo.nd_opt_len != 4) {
				logfunc("%s: invalid option len for prefix",
				    ifp->name);
				continue;
			}
			memcpy(&pi, p, sizeof(pi));
			if (pi.nd_opt_pi_prefix_len > 128) {
				logfunc("%s: invalid prefix len", ifp->name);
				continue;
			}
			/* nd_opt_pi_prefix is not aligned. */
			memcpy(&pi_prefix, &pi.nd_opt_pi_prefix,
			    sizeof(pi_prefix));
			if (IN6_IS_ADDR_MULTICAST(&pi_prefix) ||
			    IN6_IS_ADDR_LINKLOCAL(&pi_prefix))
			{
				logfunc("%s: invalid prefix in RA", ifp->name);
				continue;
			}
			if (ntohl(pi.nd_opt_pi_preferred_time) >
			    ntohl(pi.nd_opt_pi_valid_time))
			{
				logfunc("%s: pltime > vltime", ifp->name);
				continue;
			}
			TAILQ_FOREACH(ap, &rap->addrs, next)
				if (ap->prefix_len ==pi.nd_opt_pi_prefix_len &&
				    IN6_ARE_ADDR_EQUAL(&ap->prefix, &pi_prefix))
					break;
			if (ap == NULL) {
				unsigned int flags;

				if (!(pi.nd_opt_pi_flags_reserved &
				    ND_OPT_PI_FLAG_AUTO) &&
				    !(pi.nd_opt_pi_flags_reserved &
				    ND_OPT_PI_FLAG_ONLINK))
					continue;

				flags = IPV6_AF_RAPFX;
				if (pi.nd_opt_pi_flags_reserved &
				    ND_OPT_PI_FLAG_AUTO &&
				    rap->iface->options->options &
				    DHCPCD_IPV6RA_AUTOCONF)
					flags |= IPV6_AF_AUTOCONF;

				ap = ipv6_newaddr(rap->iface,
				    &pi_prefix, pi.nd_opt_pi_prefix_len, flags);
				if (ap == NULL)
					break;
				ap->prefix = pi_prefix;
				ap->dadcallback = ipv6nd_dadcallback;
				ap->created = ap->acquired = rap->acquired;
				TAILQ_INSERT_TAIL(&rap->addrs, ap, next);

#ifdef IPV6_MANAGETEMPADDR
				/* New address to dhcpcd RA handling.
				 * If the address already exists and a valid
				 * temporary address also exists then
				 * extend the existing one rather than
				 * create a new one */
				if (ipv6_iffindaddr(ifp, &ap->addr,
				    IN6_IFF_NOTUSEABLE) &&
				    ipv6_settemptime(ap, 0))
					new_ap = 0;
				else
					new_ap = 1;
#endif
			} else {
#ifdef IPV6_MANAGETEMPADDR
				new_ap = 0;
#endif
				ap->flags &= ~IPV6_AF_STALE;
				ap->acquired = rap->acquired;
			}
			if (pi.nd_opt_pi_flags_reserved &
			    ND_OPT_PI_FLAG_ONLINK)
				ap->flags |= IPV6_AF_ONLINK;
			ap->prefix_vltime =
			    ntohl(pi.nd_opt_pi_valid_time);
			ap->prefix_pltime =
			    ntohl(pi.nd_opt_pi_preferred_time);

#ifdef IPV6_MANAGETEMPADDR
			/* RFC4941 Section 3.3.3 */
			if (ap->flags & IPV6_AF_AUTOCONF &&
			    ip6_use_tempaddr(ap->iface->name))
			{
				if (!new_ap) {
					if (ipv6_settemptime(ap, 1) == NULL)
						new_ap = 1;
				}
				if (new_ap && ap->prefix_pltime) {
					if (ipv6_createtempaddr(ap,
					    &ap->acquired) == NULL)
						logerr("ipv6_createtempaddr");
				}
			}
#endif
			break;

		case ND_OPT_MTU:
			memcpy(&mtu, p, sizeof(mtu));
			mtu.nd_opt_mtu_mtu = ntohl(mtu.nd_opt_mtu_mtu);
			if (mtu.nd_opt_mtu_mtu < IPV6_MMTU) {
				logerrx("%s: invalid MTU %d",
				    ifp->name, mtu.nd_opt_mtu_mtu);
				break;
			}
			rap->mtu = mtu.nd_opt_mtu_mtu;
			break;

		case ND_OPT_RDNSS:
			memcpy(&rdnss, p, sizeof(rdnss));
			if (rdnss.nd_opt_rdnss_lifetime &&
			    rdnss.nd_opt_rdnss_len > 1)
				rap->hasdns = 1;
			break;
		default:
			continue;
		}
	}

	for (i = 0, dho = ctx->nd_opts;
	    i < ctx->nd_opts_len;
	    i++, dho++)
	{
		if (has_option_mask(ifp->options->requiremasknd,
		    dho->option))
		{
			logwarnx("%s: reject RA (no option %s) from %s",
			    ifp->name, dho->var, ctx->sfrom);
			if (new_rap)
				ipv6nd_removefreedrop_ra(rap, 0, 0);
			else
				ipv6nd_free_ra(rap);
			return;
		}
	}

	if (new_rap)
		add_router(ifp->ctx, rap);

	if (ifp->ctx->options & DHCPCD_TEST) {
		script_runreason(ifp, "TEST");
		goto handle_flag;
	}
	ipv6_addaddrs(&rap->addrs);
#ifdef IPV6_MANAGETEMPADDR
	ipv6_addtempaddrs(ifp, &rap->acquired);
#endif

	/* Find any freshly added routes, such as the subnet route.
	 * We do this because we cannot rely on recieving the kernel
	 * notification right now via our link socket. */
	if_initrt(ifp->ctx, AF_INET6);

	rt_build(ifp->ctx, AF_INET6);
	if (ipv6nd_scriptrun(rap))
		return;

	eloop_timeout_delete(ifp->ctx->eloop, NULL, ifp);
	eloop_timeout_delete(ifp->ctx->eloop, NULL, rap); /* reachable timer */

handle_flag:
	if (!(ifp->options->options & DHCPCD_DHCP6))
		goto nodhcp6;
/* Only log a DHCPv6 start error if compiled in or debugging is enabled. */
#ifdef DHCP6
#define LOG_DHCP6	logerr
#else
#define LOG_DHCP6	logdebug
#endif
	if (rap->flags & ND_RA_FLAG_MANAGED) {
		if (new_data && dhcp6_start(ifp, DH6S_REQUEST) == -1)
			LOG_DHCP6("dhcp6_start: %s", ifp->name);
	} else if (rap->flags & ND_RA_FLAG_OTHER) {
		if (new_data && dhcp6_start(ifp, DH6S_INFORM) == -1)
			LOG_DHCP6("dhcp6_start: %s", ifp->name);
	} else {
		if (new_data)
			logdebugx("%s: No DHCPv6 instruction in RA", ifp->name);
nodhcp6:
		if (ifp->ctx->options & DHCPCD_TEST) {
			eloop_exit(ifp->ctx->eloop, EXIT_SUCCESS);
			return;
		}
	}

	/* Expire should be called last as the rap object could be destroyed */
	ipv6nd_expirera(ifp);
}

int
ipv6nd_hasra(const struct interface *ifp)
{
	const struct ra *rap;

	if (ifp->ctx->ra_routers) {
		TAILQ_FOREACH(rap, ifp->ctx->ra_routers, next)
			if (rap->iface == ifp && !rap->expired)
				return 1;
	}
	return 0;
}

int
ipv6nd_hasradhcp(const struct interface *ifp)
{
	const struct ra *rap;

	if (ifp->ctx->ra_routers) {
		TAILQ_FOREACH(rap, ifp->ctx->ra_routers, next) {
			if (rap->iface == ifp &&
			    !rap->expired &&
			    (rap->flags & (ND_RA_FLAG_MANAGED | ND_RA_FLAG_OTHER)))
				return 1;
		}
	}
	return 0;
}

static const uint8_t *
ipv6nd_getoption(struct dhcpcd_ctx *ctx,
    size_t *os, unsigned int *code, size_t *len,
    const uint8_t *od, size_t ol, struct dhcp_opt **oopt)
{
	struct nd_opt_hdr ndo;
	size_t i;
	struct dhcp_opt *opt;

	if (od) {
		*os = sizeof(ndo);
		if (ol < *os) {
			errno = EINVAL;
			return NULL;
		}
		memcpy(&ndo, od, sizeof(ndo));
		i = (size_t)(ndo.nd_opt_len * 8);
		if (i > ol) {
			errno = EINVAL;
			return NULL;
		}
		*len = i;
		*code = ndo.nd_opt_type;
	}

	for (i = 0, opt = ctx->nd_opts;
	    i < ctx->nd_opts_len; i++, opt++)
	{
		if (opt->option == *code) {
			*oopt = opt;
			break;
		}
	}

	if (od)
		return od + sizeof(ndo);
	return NULL;
}

ssize_t
ipv6nd_env(char **env, const char *prefix, const struct interface *ifp)
{
	size_t i, j, n, len, olen;
	struct ra *rap;
	char ndprefix[32], abuf[24];
	struct dhcp_opt *opt;
	uint8_t *p;
	struct nd_opt_hdr ndo;
	struct ipv6_addr *ia;
	struct timespec now;

	clock_gettime(CLOCK_MONOTONIC, &now);
	i = n = 0;
	TAILQ_FOREACH(rap, ifp->ctx->ra_routers, next) {
		if (rap->iface != ifp)
			continue;
		i++;
		if (prefix != NULL)
			snprintf(ndprefix, sizeof(ndprefix),
			    "%s_nd%zu", prefix, i);
		else
			snprintf(ndprefix, sizeof(ndprefix),
			    "nd%zu", i);
		if (env)
			setvar(&env[n], ndprefix, "from", rap->sfrom);
		n++;
		if (env)
			setvard(&env[n], ndprefix, "acquired",
			    (size_t)rap->acquired.tv_sec);
		n++;
		if (env)
			setvard(&env[n], ndprefix, "now", (size_t)now.tv_sec);
		n++;

		/* Zero our indexes */
		if (env) {
			for (j = 0, opt = rap->iface->ctx->nd_opts;
			    j < rap->iface->ctx->nd_opts_len;
			    j++, opt++)
				dhcp_zero_index(opt);
			for (j = 0, opt = rap->iface->options->nd_override;
			    j < rap->iface->options->nd_override_len;
			    j++, opt++)
				dhcp_zero_index(opt);
		}

		/* Unlike DHCP, ND6 options *may* occur more than once.
		 * There is also no provision for option concatenation
		 * unlike DHCP. */
		len = rap->data_len - sizeof(struct nd_router_advert);
		for (p = rap->data + sizeof(struct nd_router_advert);
		    len >= sizeof(ndo);
		    p += olen, len -= olen)
		{
			memcpy(&ndo, p, sizeof(ndo));
			olen = (size_t)(ndo.nd_opt_len * 8);
			if (olen > len) {
				errno =	EINVAL;
				break;
			}
			if (has_option_mask(rap->iface->options->nomasknd,
			    ndo.nd_opt_type))
				continue;
			for (j = 0, opt = rap->iface->options->nd_override;
			    j < rap->iface->options->nd_override_len;
			    j++, opt++)
				if (opt->option == ndo.nd_opt_type)
					break;
			if (j == rap->iface->options->nd_override_len) {
				for (j = 0, opt = rap->iface->ctx->nd_opts;
				    j < rap->iface->ctx->nd_opts_len;
				    j++, opt++)
					if (opt->option == ndo.nd_opt_type)
						break;
				if (j == rap->iface->ctx->nd_opts_len)
					opt = NULL;
			}
			if (opt) {
				n += dhcp_envoption(rap->iface->ctx,
				    env == NULL ? NULL : &env[n],
				    ndprefix, rap->iface->name,
				    opt, ipv6nd_getoption,
				    p + sizeof(ndo), olen - sizeof(ndo));
			}
		}

		/* We need to output the addresses we actually made
		 * from the prefix information options as well. */
		j = 0;
		TAILQ_FOREACH(ia, &rap->addrs, next) {
			if (!(ia->flags & IPV6_AF_AUTOCONF)
#ifdef IPV6_AF_TEMPORARY
			    || ia->flags & IPV6_AF_TEMPORARY
#endif
			    )
				continue;
			j++;
			if (env) {
				snprintf(abuf, sizeof(abuf), "addr%zu", j);
				setvar(&env[n], ndprefix, abuf, ia->saddr);
			}
			n++;
		}
	}
	return (ssize_t)n;
}

void
ipv6nd_handleifa(int cmd, struct ipv6_addr *addr, pid_t pid)
{
	struct ra *rap;

	/* IPv6 init may not have happened yet if we are learning
	 * existing addresses when dhcpcd starts. */
	if (addr->iface->ctx->ra_routers == NULL)
		return;

	TAILQ_FOREACH(rap, addr->iface->ctx->ra_routers, next) {
		if (rap->iface != addr->iface)
			continue;
		ipv6_handleifa_addrs(cmd, &rap->addrs, addr, pid);
	}
}

void
ipv6nd_expirera(void *arg)
{
	struct interface *ifp;
	struct ra *rap, *ran;
	struct timespec now, lt, expire, next;
	uint8_t expired, anyvalid, valid, validone;
	struct ipv6_addr *ia;

	ifp = arg;
	clock_gettime(CLOCK_MONOTONIC, &now);
	expired = 0;
	timespecclear(&next);

	anyvalid = 0;
	TAILQ_FOREACH_SAFE(rap, ifp->ctx->ra_routers, next, ran) {
		if (rap->iface != ifp)
			continue;
		valid = validone = 0;
		if (rap->lifetime) {
			lt.tv_sec = (time_t)rap->lifetime;
			lt.tv_nsec = 0;
			timespecadd(&rap->acquired, &lt, &expire);
			if (rap->lifetime == 0 || timespeccmp(&now, &expire, >))
			{
				if (!rap->expired) {
					logwarnx("%s: %s: router expired",
					    ifp->name, rap->sfrom);
					rap->expired = expired = 1;
					rap->lifetime = 0;
				}
			} else {
				valid = 1;
				timespecsub(&expire, &now, &lt);
				if (!timespecisset(&next) ||
				    timespeccmp(&next, &lt, >))
					next = lt;
			}
		}

		/* Not every prefix is tied to an address which
		 * the kernel can expire, so we need to handle it ourself.
		 * Also, some OS don't support address lifetimes (Solaris). */
		TAILQ_FOREACH(ia, &rap->addrs, next) {
			if (ia->prefix_vltime == 0)
				continue;
			if (ia->prefix_vltime == ND6_INFINITE_LIFETIME) {
				validone = 1;
				continue;
			}
			lt.tv_sec = (time_t)ia->prefix_vltime;
			lt.tv_nsec = 0;
			timespecadd(&ia->acquired, &lt, &expire);
			if (timespeccmp(&now, &expire, >)) {
				if (ia->flags & IPV6_AF_ADDED) {
					logwarnx("%s: expired address %s",
					    ia->iface->name, ia->saddr);
					if (if_address6(RTM_DELADDR, ia)== -1 &&
					    errno != EADDRNOTAVAIL &&
					    errno != ENXIO)
						logerr(__func__);
				}
				ia->prefix_vltime = ia->prefix_pltime = 0;
				ia->flags &=
				    ~(IPV6_AF_ADDED | IPV6_AF_DADCOMPLETED);
				expired = 1;
			} else {
				timespecsub(&expire, &now, &lt);
				if (!timespecisset(&next) ||
				    timespeccmp(&next, &lt, >))
					next = lt;
				validone = 1;
			}
		}

		/* XXX FixMe!
		 * We need to extract the lifetime from each option and check
		 * if that has expired or not.
		 * If it has, zero the option out in the returned data. */

		/* No valid lifetimes are left on the RA, so we might
		 * as well punt it. */
		if (!valid && !validone)
			ipv6nd_free_ra(rap);
		else
			anyvalid = 1;
	}

	if (timespecisset(&next))
		eloop_timeout_add_tv(ifp->ctx->eloop,
		    &next, ipv6nd_expirera, ifp);
	if (expired) {
		rt_build(ifp->ctx, AF_INET6);
		script_runreason(ifp, "ROUTERADVERT");
	}

	/* No valid routers? Kill any DHCPv6. */
	if (!anyvalid)
		dhcp6_dropnondelegates(ifp);
}

void
ipv6nd_drop(struct interface *ifp)
{
	struct ra *rap, *ran;
	uint8_t expired = 0;

	if (ifp->ctx->ra_routers == NULL)
		return;

	eloop_timeout_delete(ifp->ctx->eloop, NULL, ifp);
	TAILQ_FOREACH_SAFE(rap, ifp->ctx->ra_routers, next, ran) {
		if (rap->iface == ifp) {
			rap->expired = expired = 1;
			ipv6nd_drop_ra(rap);
		}
	}
	if (expired) {
		rt_build(ifp->ctx, AF_INET6);
		if ((ifp->options->options & DHCPCD_NODROP) != DHCPCD_NODROP)
			script_runreason(ifp, "ROUTERADVERT");
	}
}

static void
ipv6nd_handlena(struct dhcpcd_ctx *ctx, struct interface *ifp,
    struct icmp6_hdr *icp, size_t len, int hoplimit)
{
	struct nd_neighbor_advert *nd_na;
	struct in6_addr nd_na_target;
	struct ra *rap;
	uint32_t is_router, is_solicited;
	char buf[INET6_ADDRSTRLEN];
	const char *taddr;

	if (ifp == NULL) {
#ifdef DEBUG_NS
		logdebugx("NA for unexpected interface from %s",
		    ctx->sfrom);
#endif
		return;
	}

	if ((size_t)len < sizeof(struct nd_neighbor_advert)) {
		logerrx("%s: IPv6 NA too short from %s",
		    ifp->name, ctx->sfrom);
		return;
	}

	/* RFC 4861 7.1.2 */
	if (hoplimit != 255) {
		logerrx("invalid hoplimit(%d) in NA from %s",
		    hoplimit, ctx->sfrom);
		return;
	}

	nd_na = (struct nd_neighbor_advert *)icp;
	is_router = nd_na->nd_na_flags_reserved & ND_NA_FLAG_ROUTER;
	is_solicited = nd_na->nd_na_flags_reserved & ND_NA_FLAG_SOLICITED;
	taddr = inet_ntop(AF_INET6, &nd_na->nd_na_target,
	    buf, INET6_ADDRSTRLEN);

	/* nd_na->nd_na_target is not aligned. */
	memcpy(&nd_na_target, &nd_na->nd_na_target, sizeof(nd_na_target));
	if (IN6_IS_ADDR_MULTICAST(&nd_na_target)) {
		logerrx("%s: NA multicast address %s (%s)",
		    ifp->name, taddr, ctx->sfrom);
		return;
	}

	TAILQ_FOREACH(rap, ctx->ra_routers, next) {
		if (rap->iface == ifp &&
		    IN6_ARE_ADDR_EQUAL(&rap->from, &nd_na_target))
			break;
	}
	if (rap == NULL) {
#ifdef DEBUG_NS
		logdebugx("%s: unexpected NA from %s for %s",
		    ifp->name, ctx->sfrom, taddr);
#endif
		return;
	}

#ifdef DEBUG_NS
	logdebugx("%s: %sNA for %s from %s",
	    ifp->name, is_solicited ? "solicited " : "", taddr, ctx->sfrom);
#endif

	/* Node is no longer a router, so remove it from consideration */
	if (!is_router && !rap->expired) {
		loginfox("%s: %s not a router (%s)",
		    ifp->name, taddr, ctx->sfrom);
		rap->expired = 1;
		rt_build(ifp->ctx,  AF_INET6);
		script_runreason(ifp, "ROUTERADVERT");
		return;
	}

	if (is_solicited && is_router && rap->lifetime)
		ipv6nd_reachable(rap, IPV6ND_REACHABLE);
}

static void
ipv6nd_handledata(void *arg)
{
	struct dhcpcd_ctx *ctx;
	ssize_t len;
	struct cmsghdr *cm;
	int hoplimit;
	struct in6_pktinfo pkt;
	struct icmp6_hdr *icp;
	struct interface *ifp;

	ctx = arg;
	ctx->rcvhdr.msg_controllen = CMSG_SPACE(sizeof(struct in6_pktinfo)) +
	    CMSG_SPACE(sizeof(int));
	len = recvmsg_realloc(ctx->nd_fd, &ctx->rcvhdr, 0);
	if (len == -1) {
		logerr(__func__);
		eloop_event_delete(ctx->eloop, ctx->nd_fd);
		close(ctx->nd_fd);
		ctx->nd_fd = -1;
		return;
	}
	ctx->sfrom = inet_ntop(AF_INET6, &ctx->from.sin6_addr,
	    ctx->ntopbuf, INET6_ADDRSTRLEN);
	if ((size_t)len < sizeof(struct icmp6_hdr)) {
		logerrx("IPv6 ICMP packet too short from %s", ctx->sfrom);
		return;
	}

	pkt.ipi6_ifindex = 0;
	hoplimit = 0;
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

	if (pkt.ipi6_ifindex == 0) {
		logerrx("IPv6 RA/NA did not contain index from %s", ctx->sfrom);
		return;
	}

	/* Find the receiving interface */
	TAILQ_FOREACH(ifp, ctx->ifaces, next) {
		if (ifp->index == (unsigned int)pkt.ipi6_ifindex)
			break;
	}

	/* Don't do anything if the user hasn't configured it. */
	if (ifp != NULL &&
	    (ifp->active != IF_ACTIVE_USER ||
	    !(ifp->options->options & DHCPCD_IPV6)))
		return;

	icp = (struct icmp6_hdr *)ctx->rcvhdr.msg_iov[0].iov_base;
	if (icp->icmp6_code == 0) {
		switch(icp->icmp6_type) {
			case ND_NEIGHBOR_ADVERT:
				ipv6nd_handlena(ctx, ifp, icp, (size_t)len,
				   hoplimit);
				return;
			case ND_ROUTER_ADVERT:
				ipv6nd_handlera(ctx, ifp, icp, (size_t)len,
				   hoplimit);
				return;
		}
	}

	logerrx("invalid IPv6 type %d or code %d from %s",
	    icp->icmp6_type, icp->icmp6_code, ctx->sfrom);
}

static void
ipv6nd_startrs1(void *arg)
{
	struct interface *ifp = arg;
	struct rs_state *state;

	loginfox("%s: soliciting an IPv6 router", ifp->name);
	if (ipv6nd_open(ifp->ctx) == -1) {
		logerr(__func__);
		return;
	}

	state = RS_STATE(ifp);
	if (state == NULL) {
		ifp->if_data[IF_DATA_IPV6ND] = calloc(1, sizeof(*state));
		state = RS_STATE(ifp);
		if (state == NULL) {
			logerr(__func__);
			return;
		}
	}

	/* Always make a new probe as the underlying hardware
	 * address could have changed. */
	ipv6nd_makersprobe(ifp);
	if (state->rs == NULL) {
		logerr(__func__);
		return;
	}

	state->rsprobes = 0;
	ipv6nd_sendrsprobe(ifp);
}

void
ipv6nd_startrs(struct interface *ifp)
{
	struct timespec tv;

	eloop_timeout_delete(ifp->ctx->eloop, NULL, ifp);
	if (!(ifp->options->options & DHCPCD_INITIAL_DELAY)) {
		ipv6nd_startrs1(ifp);
		return;
	}

	tv.tv_sec = 0;
	tv.tv_nsec = (suseconds_t)arc4random_uniform(
	    MAX_RTR_SOLICITATION_DELAY * NSEC_PER_SEC);
	timespecnorm(&tv);
	logdebugx("%s: delaying IPv6 router solicitation for %0.1f seconds",
	    ifp->name, timespec_to_double(&tv));
	eloop_timeout_add_tv(ifp->ctx->eloop, &tv, ipv6nd_startrs1, ifp);
	return;
}
