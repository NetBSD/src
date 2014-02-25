#include <sys/cdefs.h>
 __RCSID("$NetBSD: ipv6.c,v 1.1.1.8 2014/02/25 13:14:29 roy Exp $");

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

#include <sys/param.h>
#include <sys/types.h>
#include <sys/socket.h>

#include <net/route.h>
#include <netinet/in.h>

#ifdef __linux__
#  include <asm/types.h> /* for systems with broken headers */
#  include <linux/rtnetlink.h>
   /* Match Linux defines to BSD */
#  define IN6_IFF_TENTATIVE	(IFA_F_TENTATIVE | IFA_F_OPTIMISTIC)
#  define IN6_IFF_DUPLICATED	IFA_F_DADFAILED
#else
#ifdef __FreeBSD__ /* Needed so that including netinet6/in6_var.h works */
#  include <net/if.h>
#  include <net/if_var.h>
#endif
#  include <netinet6/in6_var.h>
#endif

#include <errno.h>
#include <ifaddrs.h>
#include <inttypes.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>

#include "common.h"
#include "dhcpcd.h"
#include "dhcp6.h"
#include "eloop.h"
#include "ipv6.h"
#include "ipv6nd.h"

/* Hackery at it's finest. */
#ifndef s6_addr32
#  define s6_addr32 __u6_addr.__u6_addr32
#endif

#define EUI64_GBIT	0x01
#define EUI64_UBIT	0x02
#define EUI64_TO_IFID(in6)	do {(in6)->s6_addr[8] ^= EUI64_UBIT; } \
				    while (/*CONSTCOND*/ 0)
#define EUI64_GROUP(in6)	((in6)->s6_addr[8] & EUI64_GBIT)


struct ipv6_ctx *
ipv6_init(struct dhcpcd_ctx *dhcpcd_ctx)
{
	struct ipv6_ctx *ctx;

	if (dhcpcd_ctx->ipv6)
		return dhcpcd_ctx->ipv6;

	ctx = calloc(1, sizeof(*ctx));
	if (ctx == NULL)
		return NULL;

	ctx->routes = malloc(sizeof(*ctx->routes));
	if (ctx->routes == NULL) {
		free(ctx);
		return NULL;
	}
	TAILQ_INIT(ctx->routes);

	ctx->ra_routers = malloc(sizeof(*ctx->ra_routers));
	if (ctx->ra_routers == NULL) {
		free(ctx->routes);
		free(ctx);
		return NULL;
	}
	TAILQ_INIT(ctx->ra_routers);

	ctx->sndhdr.msg_namelen = sizeof(struct sockaddr_in6);
	ctx->sndhdr.msg_iov = ctx->sndiov;
	ctx->sndhdr.msg_iovlen = 1;
	ctx->sndhdr.msg_control = ctx->sndbuf;
	ctx->sndhdr.msg_controllen = sizeof(ctx->sndbuf);
	ctx->rcvhdr.msg_name = &ctx->from;
	ctx->rcvhdr.msg_namelen = sizeof(ctx->from);
	ctx->rcvhdr.msg_iov = ctx->rcviov;
	ctx->rcvhdr.msg_iovlen = 1;
	ctx->rcvhdr.msg_control = ctx->rcvbuf;
	ctx->rcvhdr.msg_controllen = sizeof(ctx->rcvbuf);
	ctx->rcviov[0].iov_base = ctx->ansbuf;
	ctx->rcviov[0].iov_len = sizeof(ctx->ansbuf);

	ctx->nd_fd = -1;
	ctx->dhcp_fd = -1;

	dhcpcd_ctx->ipv6 = ctx;
	return ctx;
}

ssize_t
ipv6_printaddr(char *s, ssize_t sl, const uint8_t *d, const char *ifname)
{
	char buf[INET6_ADDRSTRLEN];
	const char *p;
	ssize_t l;

	p = inet_ntop(AF_INET6, d, buf, sizeof(buf));
	if (p == NULL)
		return -1;

	l = strlen(p);
	if (d[0] == 0xfe && (d[1] & 0xc0) == 0x80)
		l += 1 + strlen(ifname);

	if (s == NULL)
		return l;

	if (sl < l) {
		errno = ENOMEM;
		return -1;
	}

	s += strlcpy(s, p, sl);
	if (d[0] == 0xfe && (d[1] & 0xc0) == 0x80) {
		*s++ = '%';
		s += strlcpy(s, ifname, sl);
	}
	*s = '\0';
	return l;
}

int
ipv6_makeaddr(struct in6_addr *addr, const struct interface *ifp,
    const struct in6_addr *prefix, int prefix_len)
{
	const struct ipv6_addr_l *ap;
#if 0
	static u_int8_t allzero[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
	static u_int8_t allone[8] =
	    { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };
#endif

	if (prefix_len < 0 || prefix_len > 64) {
		errno = EINVAL;
		return -1;
	}

	memcpy(addr, prefix, sizeof(*prefix));

	/* Try and make the address from the first local-link address */
	ap = ipv6_linklocal(ifp);
	if (ap) {
		addr->s6_addr32[2] = ap->addr.s6_addr32[2];
		addr->s6_addr32[3] = ap->addr.s6_addr32[3];
		return 0;
	}

	/* Because we delay a few functions until we get a local-link address
	 * there is little point in the below code.
	 * It exists in-case we need to create local-link addresses
	 * ourselves, but then we would need to be able to send RFC
	 * conformant DAD requests.
	 * See ipv6ns.c for why we need the kernel to do this. */
	errno = ENOENT;
	return -1;

#if 0
	/* Make an EUI64 based off our hardware address */
	switch (ifp->family) {
	case ARPHRD_ETHER:
		/* Check for a valid hardware address */
		if (ifp->hwlen != 8 && ifp->hwlen != 6) {
			errno = ENOTSUP;
			return -1;
		}
		if (memcmp(ifp->hwaddr, allzero, ifp->hwlen) == 0 ||
		    memcmp(ifp->hwaddr, allone, ifp->hwlen) == 0)
		{
			errno = EINVAL;
			return -1;
		}

		/* make a EUI64 address */
		if (ifp->hwlen == 8)
			memcpy(&addr->s6_addr[8], ifp->hwaddr, 8);
		else if (ifp->hwlen == 6) {
			addr->s6_addr[8] = ifp->hwaddr[0];
			addr->s6_addr[9] = ifp->hwaddr[1];
			addr->s6_addr[10] = ifp->hwaddr[2];
			addr->s6_addr[11] = 0xff;
			addr->s6_addr[12] = 0xfe;
			addr->s6_addr[13] = ifp->hwaddr[3];
			addr->s6_addr[14] = ifp->hwaddr[4];
			addr->s6_addr[15] = ifp->hwaddr[5];
		}
		break;
	default:
		errno = ENOTSUP;
		return -1;
	}

	/* sanity check: g bit must not indicate "group" */
	if (EUI64_GROUP(addr)) {
		errno = EINVAL;
		return -1;
	}

	EUI64_TO_IFID(addr);

	/* sanity check: ifid must not be all zero, avoid conflict with
	 * subnet router anycast */
	if ((addr->s6_addr[8] & ~(EUI64_GBIT | EUI64_UBIT)) == 0x00 &&
		memcmp(&addr->s6_addr[9], allzero, 7) == 0)
	{
		errno = EINVAL;
		return -1;
	}

	return 0;
#endif
}

int
ipv6_makeprefix(struct in6_addr *prefix, const struct in6_addr *addr, int len)
{
	int bytelen, bitlen;

	if (len < 0 || len > 128) {
		errno = EINVAL;
		return -1;
	}

	bytelen = len / NBBY;
	bitlen = len % NBBY;
	memcpy(&prefix->s6_addr, &addr->s6_addr, bytelen);
	if (bitlen != 0)
		prefix->s6_addr[bytelen] >>= NBBY - bitlen;
	memset((char *)prefix->s6_addr + bytelen, 0,
	    sizeof(prefix->s6_addr) - bytelen);
	return 0;
}

int
ipv6_mask(struct in6_addr *mask, int len)
{
	static const unsigned char masks[NBBY] =
	    { 0x80, 0xc0, 0xe0, 0xf0, 0xf8, 0xfc, 0xfe, 0xff };
	int bytes, bits, i;

	if (len < 0 || len > 128) {
		errno = EINVAL;
		return -1;
	}

	memset(mask, 0, sizeof(*mask));
	bytes = len / NBBY;
	bits = len % NBBY;
	for (i = 0; i < bytes; i++)
		mask->s6_addr[i] = 0xff;
	if (bits)
		mask->s6_addr[bytes] = masks[bits - 1];
	return 0;
}

int
ipv6_prefixlen(const struct in6_addr *mask)
{
	int x = 0, y;
	const unsigned char *lim, *p;

	lim = (const unsigned char *)mask + sizeof(*mask);
	for (p = (const unsigned char *)mask; p < lim; x++, p++) {
		if (*p != 0xff)
			break;
	}
	y = 0;
	if (p < lim) {
		for (y = 0; y < NBBY; y++) {
			if ((*p & (0x80 >> y)) == 0)
				break;
		}
	}

	/*
	 * when the limit pointer is given, do a stricter check on the
	 * remaining bits.
	 */
	if (p < lim) {
		if (y != 0 && (*p & (0x00ff >> y)) != 0)
			return -1;
		for (p = p + 1; p < lim; p++)
			if (*p != 0)
				return -1;
	}

	return x * NBBY + y;
}

static void
in6_to_h64(const struct in6_addr *add, uint64_t *vhigh, uint64_t *vlow)
{
	uint64_t l, h;
	const uint8_t *p = (const uint8_t *)&add->s6_addr;

	h = ((uint64_t)p[0] << 56) |
	    ((uint64_t)p[1] << 48) |
	    ((uint64_t)p[2] << 40) |
	    ((uint64_t)p[3] << 32) |
	    ((uint64_t)p[4] << 24) |
	    ((uint64_t)p[5] << 16) |
	    ((uint64_t)p[6] << 8) |
	    (uint64_t)p[7];
	p += 8;
	l = ((uint64_t)p[0] << 56) |
	    ((uint64_t)p[1] << 48) |
	    ((uint64_t)p[2] << 40) |
	    ((uint64_t)p[3] << 32) |
	    ((uint64_t)p[4] << 24) |
	    ((uint64_t)p[5] << 16) |
	    ((uint64_t)p[6] << 8) |
	    (uint64_t)p[7];

	*vhigh = h;
	*vlow = l;
}

static void
h64_to_in6(uint64_t vhigh, uint64_t vlow, struct in6_addr *add)
{
	uint8_t *p = (uint8_t *)&add->s6_addr;

	p[0] = vhigh >> 56;
	p[1] = vhigh >> 48;
	p[2] = vhigh >> 40;
	p[3] = vhigh >> 32;
	p[4] = vhigh >> 24;
	p[5] = vhigh >> 16;
	p[6] = vhigh >> 8;
	p[7] = vhigh;
	p += 8;
	p[0] = vlow >> 56;
	p[1] = vlow >> 48;
	p[2] = vlow >> 40;
	p[3] = vlow >> 32;
	p[4] = vlow >> 24;
	p[5] = vlow >> 16;
	p[6] = vlow >> 8;
	p[7] = vlow;
}

int
ipv6_userprefix(
	const struct in6_addr *prefix,	// prefix from router
	short prefix_len,		// length of prefix received
	uint64_t user_number,		// "random" number from user
	struct in6_addr *result,	// resultant prefix
	short result_len)		// desired prefix length
{
	uint64_t vh, vl, user_low, user_high;

	if (prefix_len < 0 || prefix_len > 64 ||
	    result_len < 0 || result_len > 64)
	{
		errno = EINVAL;
		return -1;
	}

	/* Check that the user_number fits inside result_len less prefix_len */
	if (result_len < prefix_len || user_number > INT_MAX ||
	    ffs((int)user_number) > result_len - prefix_len)
	{
	       errno = ERANGE;
	       return -1;
	}

	/* virtually shift user number by dest_len, then split at 64 */
	if (result_len >= 64) {
		user_high = user_number << (result_len - 64);
		user_low = 0;
	} else {
		user_high = user_number >> (64 - result_len);
		user_low = user_number << result_len;
	}

	/* convert to two 64bit host order values */
	in6_to_h64(prefix, &vh, &vl);

	vh |= user_high;
	vl |= user_low;

	/* copy back result */
	h64_to_in6(vh, vl, result);

	return 0;
}

int
ipv6_addaddr(struct ipv6_addr *ap)
{

	syslog(ap->flags & IPV6_AF_NEW ? LOG_INFO : LOG_DEBUG,
	    "%s: adding address %s", ap->iface->name, ap->saddr);
	if (!(ap->flags & IPV6_AF_DADCOMPLETED) &&
	    ipv6_findaddr(ap->iface, &ap->addr))
		ap->flags |= IPV6_AF_DADCOMPLETED;
	if (add_address6(ap) == -1) {
		syslog(LOG_ERR, "add_address6 %m");
		return -1;
	}
	ap->flags &= ~IPV6_AF_NEW;
	ap->flags |= IPV6_AF_ADDED;
	if (ap->delegating_iface)
		ap->flags |= IPV6_AF_DELEGATED;
	if (ap->iface->options->options & DHCPCD_IPV6RA_OWN &&
	    ipv6_removesubnet(ap->iface, ap) == -1)
		syslog(LOG_ERR,"ipv6_removesubnet %m");
	if (ap->prefix_pltime == ND6_INFINITE_LIFETIME &&
	    ap->prefix_vltime == ND6_INFINITE_LIFETIME)
		syslog(LOG_DEBUG,
		    "%s: vltime infinity, pltime infinity",
		    ap->iface->name);
	else if (ap->prefix_pltime == ND6_INFINITE_LIFETIME)
		syslog(LOG_DEBUG,
		    "%s: vltime %"PRIu32" seconds, pltime infinity",
		    ap->iface->name, ap->prefix_vltime);
	else if (ap->prefix_vltime == ND6_INFINITE_LIFETIME)
		syslog(LOG_DEBUG,
		    "%s: vltime infinity, pltime %"PRIu32"seconds",
		    ap->iface->name, ap->prefix_pltime);
	else
		syslog(LOG_DEBUG,
		    "%s: vltime %"PRIu32" seconds, pltime %"PRIu32" seconds",
		    ap->iface->name, ap->prefix_vltime, ap->prefix_pltime);
	return 0;
}

void
ipv6_freedrop_addrs(struct ipv6_addrhead *addrs, int drop,
    const struct interface *ifd)
{
	struct ipv6_addr *ap, *apn;

	TAILQ_FOREACH_SAFE(ap, addrs, next, apn) {
		if (ifd && ap->delegating_iface != ifd)
			continue;
		TAILQ_REMOVE(addrs, ap, next);
		if (ap->dadcallback)
			eloop_q_timeout_delete(ap->iface->ctx->eloop,
			    0, NULL, ap->dadcallback);
		/* Only drop the address if no other RAs have assigned it.
		 * This is safe because the RA is removed from the list
		 * before we are called. */
		if (drop && ap->flags & IPV6_AF_ADDED &&
		    !ipv6nd_addrexists(ap->iface->ctx, ap) &&
		    !dhcp6_addrexists(ap->iface->ctx, ap) &&
		    (ap->iface->options->options &
		    (DHCPCD_EXITING | DHCPCD_PERSISTENT)) !=
		    (DHCPCD_EXITING | DHCPCD_PERSISTENT))
		{
			syslog(LOG_INFO, "%s: deleting address %s",
			    ap->iface->name, ap->saddr);
			if (del_address6(ap) == -1 &&
			    errno != EADDRNOTAVAIL && errno != ENXIO)
				syslog(LOG_ERR, "del_address6 %m");
		}
		free(ap);
	}
}

static struct ipv6_state *
ipv6_getstate(struct interface *ifp)
{
	struct ipv6_state *state;

	state = IPV6_STATE(ifp);
	if (state == NULL) {
	        ifp->if_data[IF_DATA_IPV6] = malloc(sizeof(*state));
		state = IPV6_STATE(ifp);
		if (state == NULL) {
			syslog(LOG_ERR, "%s: %m", __func__);
			return NULL;
		}
		TAILQ_INIT(&state->addrs);
		TAILQ_INIT(&state->ll_callbacks);
	}
	return state;
}

void
ipv6_handleifa(struct dhcpcd_ctx *ctx,
    int cmd, struct if_head *ifs, const char *ifname,
    const struct in6_addr *addr, int flags)
{
	struct interface *ifp;
	struct ipv6_state *state;
	struct ipv6_addr_l *ap;
	struct ll_callback *cb;

#if 0
	char buf[INET6_ADDRSTRLEN];
	inet_ntop(AF_INET6, &addr->s6_addr,
	    buf, INET6_ADDRSTRLEN);
	syslog(LOG_DEBUG, "%s: cmd %d addr %s flags %d",
	    ifname, cmd, buf, flags);
#endif

	/* Safety, remove tentative addresses */
	if (cmd == RTM_NEWADDR) {
		if (flags & (IN6_IFF_TENTATIVE | IN6_IFF_DUPLICATED))
			cmd = RTM_DELADDR;
#ifdef IN6_IFF_DETACHED
		if (flags & IN6_IFF_DETACHED)
			cmd = RTM_DELADDR;
#endif
	}

	if (ifs == NULL)
		ifs = ctx->ifaces;
	if (ifs == NULL) {
		errno = ESRCH;
		return;
	}
	TAILQ_FOREACH(ifp, ifs, next) {
		if (strcmp(ifp->name, ifname) == 0)
			break;
	}
	if (ifp == NULL) {
		errno = ESRCH;
		return;
	}

	state = ipv6_getstate(ifp);
	if (state == NULL)
		return;

	if (!IN6_IS_ADDR_LINKLOCAL(addr)) {
		ipv6nd_handleifa(ctx, cmd, ifname, addr, flags);
		dhcp6_handleifa(ctx, cmd, ifname, addr, flags);
	}

	/* We don't care about duplicated addresses, so remove them */
	if (flags & IN6_IFF_DUPLICATED)
		cmd = RTM_DELADDR;

	TAILQ_FOREACH(ap, &state->addrs, next) {
		if (IN6_ARE_ADDR_EQUAL(&ap->addr, addr))
			break;
	}

	switch (cmd) {
	case RTM_DELADDR:
		if (ap) {
			TAILQ_REMOVE(&state->addrs, ap, next);
			free(ap);
		}
		break;
	case RTM_NEWADDR:
		if (ap == NULL) {
			ap = calloc(1, sizeof(*ap));
			memcpy(ap->addr.s6_addr, addr->s6_addr,
			    sizeof(ap->addr.s6_addr));
			TAILQ_INSERT_TAIL(&state->addrs,
			    ap, next);

			if (IN6_IS_ADDR_LINKLOCAL(&ap->addr)) {
				/* Now run any callbacks.
				 * Typically IPv6RS or DHCPv6 */
				while ((cb =
				    TAILQ_FIRST(&state->ll_callbacks)))
				{
					TAILQ_REMOVE(&state->ll_callbacks,
					    cb, next);
					cb->callback(cb->arg);
					free(cb);
				}
			}
		}
		break;
	}
}

const struct ipv6_addr_l *
ipv6_linklocal(const struct interface *ifp)
{
	const struct ipv6_state *state;
	const struct ipv6_addr_l *ap;

	state = IPV6_CSTATE(ifp);
	if (state) {
		TAILQ_FOREACH(ap, &state->addrs, next) {
			if (IN6_IS_ADDR_LINKLOCAL(&ap->addr))
				return ap;
		}
	}
	return NULL;
}

const struct ipv6_addr_l *
ipv6_findaddr(const struct interface *ifp, const struct in6_addr *addr)
{
	const struct ipv6_state *state;
	const struct ipv6_addr_l *ap;

	state = IPV6_CSTATE(ifp);
	if (state) {
		TAILQ_FOREACH(ap, &state->addrs, next) {
			if (IN6_ARE_ADDR_EQUAL(&ap->addr, addr))
				return ap;
		}
	}
	return NULL;
}

int ipv6_addlinklocalcallback(struct interface *ifp,
    void (*callback)(void *), void *arg)
{
	struct ipv6_state *state;
	struct ll_callback *cb;

	state = ipv6_getstate(ifp);
	TAILQ_FOREACH(cb, &state->ll_callbacks, next) {
		if (cb->callback == callback && cb->arg == arg)
			break;
	}
	if (cb == NULL) {
		cb = malloc(sizeof(*cb));
		if (cb == NULL) {
			syslog(LOG_ERR, "%s: %m", __func__);
			return -1;
		}
		cb->callback = callback;
		cb->arg = arg;
		TAILQ_INSERT_TAIL(&state->ll_callbacks, cb, next);
	}
	return 0;
}

void
ipv6_free_ll_callbacks(struct interface *ifp)
{
	struct ipv6_state *state;
	struct ll_callback *cb;

	state = IPV6_STATE(ifp);
	if (state) {
		while ((cb = TAILQ_FIRST(&state->ll_callbacks))) {
			TAILQ_REMOVE(&state->ll_callbacks, cb, next);
			free(cb);
		}
	}
}

void
ipv6_free(struct interface *ifp)
{
	struct ipv6_state *state;
	struct ipv6_addr_l *ap;

	if (ifp) {
		ipv6_free_ll_callbacks(ifp);
		state = IPV6_STATE(ifp);
		if (state) {
			while ((ap = TAILQ_FIRST(&state->addrs))) {
				TAILQ_REMOVE(&state->addrs, ap, next);
				free(ap);
			}
			free(state);
			ifp->if_data[IF_DATA_IPV6] = NULL;
		}
	}
}

void
ipv6_ctxfree(struct dhcpcd_ctx *ctx)
{

	if (ctx->ipv6 == NULL)
		return;

	free(ctx->ipv6->routes);
	free(ctx->ipv6->ra_routers);
	free(ctx->ipv6);
}

int
ipv6_handleifa_addrs(int cmd,
    struct ipv6_addrhead *addrs, const struct in6_addr *addr, int flags)
{
	struct ipv6_addr *ap, *apn;
	uint8_t found, alldadcompleted;

	alldadcompleted = 1;
	found = 0;
	TAILQ_FOREACH_SAFE(ap, addrs, next, apn) {
		if (!IN6_ARE_ADDR_EQUAL(addr, &ap->addr)) {
			if ((ap->flags & IPV6_AF_DADCOMPLETED) == 0)
				alldadcompleted = 0;
			continue;
		}
		switch (cmd) {
		case RTM_DELADDR:
			syslog(LOG_INFO, "%s: deleted address %s",
			    ap->iface->name, ap->saddr);
			TAILQ_REMOVE(addrs, ap, next);
			free(ap);
			break;
		case RTM_NEWADDR:
			/* Safety - ignore tentative announcements */
			if (flags & IN6_IFF_TENTATIVE)
				break;
			if ((ap->flags & IPV6_AF_DADCOMPLETED) == 0) {
				found++;
				if (flags & IN6_IFF_DUPLICATED)
					ap->flags |= IPV6_AF_DUPLICATED;
				else
					ap->flags &= ~IPV6_AF_DUPLICATED;
				if (ap->dadcallback)
					ap->dadcallback(ap);
				/* We need to set this here in-case the
				 * dadcallback function checks it */
				ap->flags |= IPV6_AF_DADCOMPLETED;
			}
			break;
		}
	}

	return alldadcompleted ? found : 0;
}

static struct rt6 *
find_route6(struct rt6_head *rts, const struct rt6 *r)
{
	struct rt6 *rt;

	TAILQ_FOREACH(rt, rts, next) {
		if (IN6_ARE_ADDR_EQUAL(&rt->dest, &r->dest) &&
#if HAVE_ROUTE_METRIC
		    rt->iface->metric == r->iface->metric &&
#endif
		    IN6_ARE_ADDR_EQUAL(&rt->net, &r->net))
			return rt;
	}
	return NULL;
}

static void
desc_route(const char *cmd, const struct rt6 *rt)
{
	char destbuf[INET6_ADDRSTRLEN];
	char gatebuf[INET6_ADDRSTRLEN];
	const char *ifname = rt->iface->name, *dest, *gate;

	dest = inet_ntop(AF_INET6, &rt->dest.s6_addr,
	    destbuf, INET6_ADDRSTRLEN);
	gate = inet_ntop(AF_INET6, &rt->gate.s6_addr,
	    gatebuf, INET6_ADDRSTRLEN);
	if (IN6_ARE_ADDR_EQUAL(&rt->gate, &in6addr_any))
		syslog(LOG_INFO, "%s: %s route to %s/%d", ifname, cmd,
		    dest, ipv6_prefixlen(&rt->net));
	else if (IN6_ARE_ADDR_EQUAL(&rt->dest, &in6addr_any) &&
	    IN6_ARE_ADDR_EQUAL(&rt->net, &in6addr_any))
		syslog(LOG_INFO, "%s: %s default route via %s", ifname, cmd,
		    gate);
	else
		syslog(LOG_INFO, "%s: %s route to %s/%d via %s", ifname, cmd,
		    dest, ipv6_prefixlen(&rt->net), gate);
}

#define n_route(a)	 nc_route(1, a, a)
#define c_route(a, b)	 nc_route(0, a, b)
static int
nc_route(int add, struct rt6 *ort, struct rt6 *nrt)
{

	/* Don't set default routes if not asked to */
	if (IN6_IS_ADDR_UNSPECIFIED(&nrt->dest) &&
	    IN6_IS_ADDR_UNSPECIFIED(&nrt->net) &&
	    !(nrt->iface->options->options & DHCPCD_GATEWAY))
		return -1;

	desc_route(add ? "adding" : "changing", nrt);
	/* We delete and add the route so that we can change metric and
	 * prefer the interface. */
	if (del_route6(ort) == -1 && errno != ESRCH)
		syslog(LOG_ERR, "%s: del_route6: %m", ort->iface->name);
	if (add_route6(nrt) == 0)
		return 0;
	syslog(LOG_ERR, "%s: add_route6: %m", nrt->iface->name);
	return -1;
}

static int
d_route(struct rt6 *rt)
{
	int retval;

	desc_route("deleting", rt);
	retval = del_route6(rt);
	if (retval != 0 && errno != ENOENT && errno != ESRCH)
		syslog(LOG_ERR,"%s: del_route6: %m", rt->iface->name);
	return retval;
}

static struct rt6 *
make_route(const struct interface *ifp, const struct ra *rap)
{
	struct rt6 *r;

	r = calloc(1, sizeof(*r));
	if (r == NULL) {
		syslog(LOG_ERR, "%s: %m", __func__);
		return NULL;
	}
	r->iface = ifp;
	r->metric = ifp->metric;
	if (rap)
		r->mtu = rap->mtu;
	else
		r->mtu = 0;
	return r;
}

static struct rt6 *
make_prefix(const struct interface * ifp, const struct ra *rap,
    const struct ipv6_addr *addr)
{
	struct rt6 *r;

	if (addr == NULL || addr->prefix_len > 128) {
		errno = EINVAL;
		return NULL;
	}

	/* There is no point in trying to manage a /128 prefix. */
	if (addr->prefix_len == 128)
		return NULL;

	r = make_route(ifp, rap);
	if (r == NULL)
		return r;
	r->dest = addr->prefix;
	ipv6_mask(&r->net, addr->prefix_len);
	r->gate = in6addr_any;
	return r;
}


static struct rt6 *
make_router(const struct ra *rap)
{
	struct rt6 *r;

	r = make_route(rap->iface, rap);
	if (r == NULL)
		return NULL;
	r->dest = in6addr_any;
	r->net = in6addr_any;
	r->gate = rap->from;
	return r;
}

int
ipv6_removesubnet(struct interface *ifp, struct ipv6_addr *addr)
{
	struct rt6 *rt;
#if HAVE_ROUTE_METRIC
	struct rt6 *ort;
#endif
	int r;

	/* We need to delete the subnet route to have our metric or
	 * prefer the interface. */
	r = 0;
	rt = make_prefix(ifp, NULL, addr);
	if (rt) {
		rt->iface = ifp;
#ifdef __linux__
		rt->metric = 256;
#else
		rt->metric = 0;
#endif
#if HAVE_ROUTE_METRIC
		/* For some reason, Linux likes to re-add the subnet
		   route under the original metric.
		   I would love to find a way of stopping this! */
		if ((ort = find_route6(ifp->ctx->ipv6->routes, rt)) == NULL ||
		    ort->metric != rt->metric)
#else
		if (!find_route6(ifp->ctx->ipv6->routes, rt))
#endif
		{
			r = del_route6(rt);
			if (r == -1 && errno == ESRCH)
				r = 0;
		}
		free(rt);
	}
	return r;
}

#define RT_IS_DEFAULT(rtp) \
	(IN6_ARE_ADDR_EQUAL(&((rtp)->dest), &in6addr_any) &&		      \
	    IN6_ARE_ADDR_EQUAL(&((rtp)->net), &in6addr_any))

static void
ipv6_build_ra_routes(struct ipv6_ctx *ctx, struct rt6_head *dnr, int expired)
{
	struct rt6 *rt;
	const struct ra *rap;
	const struct ipv6_addr *addr;

	TAILQ_FOREACH(rap, ctx->ra_routers, next) {
		if (rap->expired != expired)
			continue;
		if (rap->iface->options->options & DHCPCD_IPV6RA_OWN) {
			TAILQ_FOREACH(addr, &rap->addrs, next) {
				if ((addr->flags & IPV6_AF_ONLINK) == 0)
					continue;
				rt = make_prefix(rap->iface, rap, addr);
				if (rt)
					TAILQ_INSERT_TAIL(dnr, rt, next);
			}
		}
		if (rap->iface->options->options &
		    (DHCPCD_IPV6RA_OWN | DHCPCD_IPV6RA_OWN_DEFAULT))
		{
			rt = make_router(rap);
			if (rt)
				TAILQ_INSERT_TAIL(dnr, rt, next);
		}
	}
}

static void
ipv6_build_dhcp_routes(struct dhcpcd_ctx *ctx,
    struct rt6_head *dnr, enum DH6S dstate)
{
	const struct interface *ifp;
	const struct dhcp6_state *d6_state;
	const struct ipv6_addr *addr;
	struct rt6 *rt;

	TAILQ_FOREACH(ifp, ctx->ifaces, next) {
		if (!(ifp->options->options & DHCPCD_IPV6RA_OWN))
			continue;
		d6_state = D6_CSTATE(ifp);
		if (d6_state && d6_state->state == dstate) {
			TAILQ_FOREACH(addr, &d6_state->addrs, next) {
				if ((addr->flags & IPV6_AF_ONLINK) == 0 ||
				    IN6_IS_ADDR_UNSPECIFIED(&addr->addr))
					continue;
				rt = make_prefix(ifp, NULL, addr);
				if (rt)
					TAILQ_INSERT_TAIL(dnr, rt, next);
			}
		}
	}
}

void
ipv6_buildroutes(struct dhcpcd_ctx *ctx)
{
	struct rt6_head dnr, *nrs;
	struct rt6 *rt, *rtn, *or;
	uint8_t have_default;
	unsigned long long o;

	TAILQ_INIT(&dnr);

	/* First add reachable routers and their prefixes */
	ipv6_build_ra_routes(ctx->ipv6, &dnr, 0);
#if HAVE_ROUTE_METRIC
	have_default = (TAILQ_FIRST(&dnr) != NULL);
#endif

	/* We have no way of knowing if prefixes added by DHCP are reachable
	 * or not, so we have to assume they are.
	 * Add bound before delegated so we can prefer interfaces better */
	ipv6_build_dhcp_routes(ctx, &dnr, DH6S_BOUND);
	ipv6_build_dhcp_routes(ctx, &dnr, DH6S_DELEGATED);

#if HAVE_ROUTE_METRIC
	/* If we have an unreachable router, we really do need to remove the
	 * route to it beause it could be a lower metric than a reachable
	 * router. Of course, we should at least have some routers if all
	 * are unreachable. */
	if (!have_default)
#endif
	/* Add our non-reachable routers and prefixes
	 * Unsure if this is needed, but it's a close match to kernel
	 * behaviour */
	ipv6_build_ra_routes(ctx->ipv6, &dnr, 1);

	nrs = malloc(sizeof(*nrs));
	if (nrs == NULL) {
		syslog(LOG_ERR, "%s: %m", __func__);
		return;
	}
	TAILQ_INIT(nrs);
	have_default = 0;
	TAILQ_FOREACH_SAFE(rt, &dnr, next, rtn) {
		/* Is this route already in our table? */
		if (find_route6(nrs, rt) != NULL)
			continue;
		//rt->src.s_addr = ifp->addr.s_addr;
		/* Do we already manage it? */
		if ((or = find_route6(ctx->ipv6->routes, rt))) {
			if (or->iface != rt->iface ||
		//	    or->src.s_addr != ifp->addr.s_addr ||
			    !IN6_ARE_ADDR_EQUAL(&rt->gate, &or->gate) ||
			    rt->metric != or->metric)
			{
				if (c_route(or, rt) != 0)
					continue;
			}
			TAILQ_REMOVE(ctx->ipv6->routes, or, next);
			free(or);
		} else {
			if (n_route(rt) != 0)
				continue;
		}
		if (RT_IS_DEFAULT(rt))
			have_default = 1;
		TAILQ_REMOVE(&dnr, rt, next);
		TAILQ_INSERT_TAIL(nrs, rt, next);
	}

	/* Free any routes we failed to add/change */
	while ((rt = TAILQ_FIRST(&dnr))) {
		TAILQ_REMOVE(&dnr, rt, next);
		free(rt);
	}

	/* Remove old routes we used to manage
	 * If we own the default route, but not RA management itself
	 * then we need to preserve the last best default route we had */
	while ((rt = TAILQ_LAST(ctx->ipv6->routes, rt6_head))) {
		TAILQ_REMOVE(ctx->ipv6->routes, rt, next);
		if (find_route6(nrs, rt) == NULL) {
			o = rt->iface->options->options;
			if (!have_default &&
			    (o & DHCPCD_IPV6RA_OWN_DEFAULT) &&
			    !(o & DHCPCD_IPV6RA_OWN) &&
			    RT_IS_DEFAULT(rt))
				have_default = 1;
				/* no need to add it back to our routing table
				 * as we delete an exiting route when we add
				 * a new one */
			else if ((rt->iface->options->options &
				(DHCPCD_EXITING | DHCPCD_PERSISTENT)) !=
				(DHCPCD_EXITING | DHCPCD_PERSISTENT))
				d_route(rt);
		}
		free(rt);
	}

	free(ctx->ipv6->routes);
	ctx->ipv6->routes = nrs;
}
