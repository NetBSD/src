#include <sys/cdefs.h>
 __RCSID("$NetBSD: dhcp.c,v 1.16 2014/09/16 22:27:04 roy Exp $");

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
#include <sys/socket.h>
#include <sys/stat.h>

#ifdef __linux__
#  include <asm/types.h> /* for systems with broken headers */
#  include <linux/rtnetlink.h>
#endif

#include <arpa/inet.h>
#include <net/if.h>
#include <net/route.h>
#include <netinet/if_ether.h>
#include <netinet/in_systm.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#define __FAVOR_BSD /* Nasty glibc hack so we can use BSD semantics for UDP */
#include <netinet/udp.h>
#undef __FAVOR_BSD

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>

#define ELOOP_QUEUE 2
#include "config.h"
#include "arp.h"
#include "common.h"
#include "dhcp.h"
#include "dhcpcd.h"
#include "dhcp-common.h"
#include "duid.h"
#include "eloop.h"
#include "if.h"
#include "ipv4.h"
#include "ipv4ll.h"
#include "script.h"

#define DAD		"Duplicate address detected"
#define DHCP_MIN_LEASE	20

#define IPV4A		ADDRIPV4 | ARRAY
#define IPV4R		ADDRIPV4 | REQUEST

/* We should define a maximum for the NAK exponential backoff */
#define NAKOFF_MAX              60

/* Wait N nanoseconds between sending a RELEASE and dropping the address.
 * This gives the kernel enough time to actually send it. */
#define RELEASE_DELAY_S		0
#define RELEASE_DELAY_NS	10000000

#ifndef IPDEFTTL
#define IPDEFTTL 64 /* RFC1340 */
#endif

struct dhcp_op {
	uint8_t value;
	const char *name;
};

static const struct dhcp_op dhcp_ops[] = {
	{ DHCP_DISCOVER,   "DISCOVER" },
	{ DHCP_OFFER,      "OFFER" },
	{ DHCP_REQUEST,    "REQUEST" },
	{ DHCP_DECLINE,    "DECLINE" },
	{ DHCP_ACK,        "ACK" },
	{ DHCP_NAK,        "NAK" },
	{ DHCP_RELEASE,    "RELEASE" },
	{ DHCP_INFORM,     "INFORM" },
	{ DHCP_FORCERENEW, "DHCP_FORCERENEW"},
	{ 0, NULL }
};

static const char * const dhcp_params[] = {
	"ip_address",
	"subnet_cidr",
	"network_number",
	"filename",
	"server_name",
	NULL
};

struct udp_dhcp_packet
{
	struct ip ip;
	struct udphdr udp;
	struct dhcp_message dhcp;
};

static const size_t udp_dhcp_len = sizeof(struct udp_dhcp_packet);

static int dhcp_open(struct interface *ifp);

void
dhcp_printoptions(const struct dhcpcd_ctx *ctx,
    const struct dhcp_opt *opts, size_t opts_len)
{
	const char * const *p;
	size_t i, j;
	const struct dhcp_opt *opt, *opt2;

	for (p = dhcp_params; *p; p++)
		printf("    %s\n", *p);

	for (i = 0, opt = ctx->dhcp_opts; i < ctx->dhcp_opts_len; i++, opt++) {
		for (j = 0, opt2 = opts; j < opts_len; j++, opt2++)
			if (opt->option == opt2->option)
				break;
		if (j == opts_len)
			printf("%03d %s\n", opt->option, opt->var);
	}
	for (i = 0, opt = opts; i < opts_len; i++, opt++)
		printf("%03d %s\n", opt->option, opt->var);
}

#define get_option_raw(ctx, dhcp, opt) get_option(ctx, dhcp, opt, NULL)
static const uint8_t *
get_option(struct dhcpcd_ctx *ctx,
    const struct dhcp_message *dhcp, unsigned int opt, size_t *len)
{
	const uint8_t *p = dhcp->options;
	const uint8_t *e = p + sizeof(dhcp->options);
	uint8_t l, ol = 0;
	uint8_t o = 0;
	uint8_t overl = 0;
	uint8_t *bp = NULL;
	const uint8_t *op = NULL;
	size_t bl = 0;

	while (p < e) {
		o = *p++;
		if (o == opt) {
			if (op) {
				if (!ctx->opt_buffer) {
					ctx->opt_buffer =
					    malloc(DHCP_OPTION_LEN +
					    BOOTFILE_LEN + SERVERNAME_LEN);
					if (ctx->opt_buffer == NULL)
						return NULL;
				}
				if (!bp)
					bp = ctx->opt_buffer;
				memcpy(bp, op, ol);
				bp += ol;
			}
			ol = *p;
			if (p + ol > e) {
				errno = EINVAL;
				return NULL;
			}
			op = p + 1;
			bl += ol;
		}
		switch (o) {
		case DHO_PAD:
			continue;
		case DHO_END:
			if (overl & 1) {
				/* bit 1 set means parse boot file */
				overl &= ~1;
				p = dhcp->bootfile;
				e = p + sizeof(dhcp->bootfile);
			} else if (overl & 2) {
				/* bit 2 set means parse server name */
				overl &= ~2;
				p = dhcp->servername;
				e = p + sizeof(dhcp->servername);
			} else
				goto exit;
			break;
		case DHO_OPTIONSOVERLOADED:
			/* Ensure we only get this option once by setting
			 * the last bit as well as the value.
			 * This is valid because only the first two bits
			 * actually mean anything in RFC2132 Section 9.3 */
			if (!overl)
				overl = 0x80 | p[1];
			break;
		}
		l = *p++;
		p += l;
	}

exit:
	if (len)
		*len = bl;
	if (bp) {
		memcpy(bp, op, ol);
		return (const uint8_t *)ctx->opt_buffer;
	}
	if (op)
		return op;
	errno = ENOENT;
	return NULL;
}

int
get_option_addr(struct dhcpcd_ctx *ctx,
    struct in_addr *a, const struct dhcp_message *dhcp,
    uint8_t option)
{
	const uint8_t *p;
	size_t len;

	p = get_option(ctx, dhcp, option, &len);
	if (!p || len < (ssize_t)sizeof(a->s_addr))
		return -1;
	memcpy(&a->s_addr, p, sizeof(a->s_addr));
	return 0;
}

static int
get_option_uint32(struct dhcpcd_ctx *ctx,
    uint32_t *i, const struct dhcp_message *dhcp, uint8_t option)
{
	const uint8_t *p;
	size_t len;
	uint32_t d;

	p = get_option(ctx, dhcp, option, &len);
	if (!p || len < (ssize_t)sizeof(d))
		return -1;
	memcpy(&d, p, sizeof(d));
	if (i)
		*i = ntohl(d);
	return 0;
}

static int
get_option_uint8(struct dhcpcd_ctx *ctx,
    uint8_t *i, const struct dhcp_message *dhcp, uint8_t option)
{
	const uint8_t *p;
	size_t len;

	p = get_option(ctx, dhcp, option, &len);
	if (!p || len < (ssize_t)sizeof(*p))
		return -1;
	if (i)
		*i = *(p);
	return 0;
}

ssize_t
decode_rfc3442(char *out, size_t len, const uint8_t *p, size_t pl)
{
	const uint8_t *e;
	size_t bytes = 0, ocets;
	int b;
	uint8_t cidr;
	struct in_addr addr;
	char *o = out;

	/* Minimum is 5 -first is CIDR and a router length of 4 */
	if (pl < 5) {
		errno = EINVAL;
		return -1;
	}

	e = p + pl;
	while (p < e) {
		cidr = *p++;
		if (cidr > 32) {
			errno = EINVAL;
			return -1;
		}
		ocets = (cidr + 7) / NBBY;
		if (p + 4 + ocets > e) {
			errno = ERANGE;
			return -1;
		}
		if (!out) {
			p += 4 + ocets;
			bytes += ((4 * 4) * 2) + 4;
			continue;
		}
		if ((((4 * 4) * 2) + 4) > len) {
			errno = ENOBUFS;
			return -1;
		}
		if (o != out) {
			*o++ = ' ';
			len--;
		}
		/* If we have ocets then we have a destination and netmask */
		if (ocets > 0) {
			addr.s_addr = 0;
			memcpy(&addr.s_addr, p, ocets);
			b = snprintf(o, len, "%s/%d", inet_ntoa(addr), cidr);
			p += ocets;
		} else
			b = snprintf(o, len, "0.0.0.0/0");
		o += b;
		len -= (size_t)b;

		/* Finally, snag the router */
		memcpy(&addr.s_addr, p, 4);
		p += 4;
		b = snprintf(o, len, " %s", inet_ntoa(addr));
		o += b;
		len -= (size_t)b;
	}

	if (out)
		return o - out;
	return (ssize_t)bytes;
}

static struct rt_head *
decode_rfc3442_rt(const uint8_t *data, size_t dl)
{
	const uint8_t *p = data;
	const uint8_t *e;
	uint8_t cidr;
	size_t ocets;
	struct rt_head *routes;
	struct rt *rt = NULL;

	/* Minimum is 5 -first is CIDR and a router length of 4 */
	if (dl < 5)
		return NULL;

	routes = malloc(sizeof(*routes));
	TAILQ_INIT(routes);
	e = p + dl;
	while (p < e) {
		cidr = *p++;
		if (cidr > 32) {
			ipv4_freeroutes(routes);
			errno = EINVAL;
			return NULL;
		}

		ocets = (cidr + 7) / NBBY;
		if (p + 4 + ocets > e) {
			ipv4_freeroutes(routes);
			errno = ERANGE;
			return NULL;
		}

		rt = calloc(1, sizeof(*rt));
		if (rt == NULL) {
			syslog(LOG_ERR, "%s: %m", __func__);
			ipv4_freeroutes(routes);
			return NULL;
		}
		TAILQ_INSERT_TAIL(routes, rt, next);

		/* If we have ocets then we have a destination and netmask */
		if (ocets > 0) {
			memcpy(&rt->dest.s_addr, p, ocets);
			p += ocets;
			rt->net.s_addr = htonl(~0U << (32 - cidr));
		}

		/* Finally, snag the router */
		memcpy(&rt->gate.s_addr, p, 4);
		p += 4;
	}
	return routes;
}

char *
decode_rfc3361(const uint8_t *data, size_t dl)
{
	uint8_t enc;
	size_t l;
	ssize_t r;
	char *sip = NULL;
	struct in_addr addr;
	char *p;

	if (dl < 2) {
		errno = EINVAL;
		return 0;
	}

	enc = *data++;
	dl--;
	switch (enc) {
	case 0:
		if ((r = decode_rfc3397(NULL, 0, data, dl)) > 0) {
			l = (size_t)r;
			sip = malloc(l);
			if (sip == NULL)
				return 0;
			decode_rfc3397(sip, l, data, dl);
		}
		break;
	case 1:
		if (dl == 0 || dl % 4 != 0) {
			errno = EINVAL;
			break;
		}
		addr.s_addr = INADDR_BROADCAST;
		l = ((dl / sizeof(addr.s_addr)) * ((4 * 4) + 1)) + 1;
		sip = p = malloc(l);
		if (sip == NULL)
			return 0;
		while (dl != 0) {
			memcpy(&addr.s_addr, data, sizeof(addr.s_addr));
			data += sizeof(addr.s_addr);
			p += snprintf(p, l - (size_t)(p - sip),
			    "%s ", inet_ntoa(addr));
			dl -= sizeof(addr.s_addr);
		}
		*--p = '\0';
		break;
	default:
		errno = EINVAL;
		return 0;
	}

	return sip;
}

/* Decode an RFC5969 6rd order option into a space
 * separated string. Returns length of string (including
 * terminating zero) or zero on error. */
ssize_t
decode_rfc5969(char *out, size_t len, const uint8_t *p, size_t pl)
{
	uint8_t ipv4masklen, ipv6prefixlen;
	uint8_t ipv6prefix[16];
	uint8_t br[4];
	int i;
	ssize_t b, bytes = 0;

	if (pl < 22) {
		errno = EINVAL;
		return 0;
	}

	ipv4masklen = *p++;
	pl--;
	ipv6prefixlen = *p++;
	pl--;

	for (i = 0; i < 16; i++) {
		ipv6prefix[i] = *p++;
		pl--;
	}
	if (out) {
		b= snprintf(out, len,
		    "%d %d "
		    "%02x%02x:%02x%02x:"
		    "%02x%02x:%02x%02x:"
		    "%02x%02x:%02x%02x:"
		    "%02x%02x:%02x%02x",
		    ipv4masklen, ipv6prefixlen,
		    ipv6prefix[0], ipv6prefix[1], ipv6prefix[2], ipv6prefix[3],
		    ipv6prefix[4], ipv6prefix[5], ipv6prefix[6], ipv6prefix[7],
		    ipv6prefix[8], ipv6prefix[9], ipv6prefix[10],ipv6prefix[11],
		    ipv6prefix[12],ipv6prefix[13],ipv6prefix[14], ipv6prefix[15]
		);

		len -= (size_t)b;
		out += b;
		bytes += b;
	} else {
		bytes += 16 * 2 + 8 + 2 + 1 + 2;
	}

	while (pl >= 4) {
		br[0] = *p++;
		br[1] = *p++;
		br[2] = *p++;
		br[3] = *p++;
		pl -= 4;

		if (out) {
			b= snprintf(out, len, " %d.%d.%d.%d",
			    br[0], br[1], br[2], br[3]);
			len -= (size_t)b;
			out += b;
			bytes += b;
		} else {
			bytes += (4 * 4);
		}
	}

	return bytes;
}

static char *
get_option_string(struct dhcpcd_ctx *ctx,
    const struct dhcp_message *dhcp, uint8_t option)
{
	size_t len;
	const uint8_t *p;
	char *s;

	p = get_option(ctx, dhcp, option, &len);
	if (!p || len == 0 || *p == '\0')
		return NULL;

	s = malloc(sizeof(char) * (len + 1));
	if (s) {
		memcpy(s, p, len);
		s[len] = '\0';
	}
	return s;
}

/* This calculates the netmask that we should use for static routes.
 * This IS different from the calculation used to calculate the netmask
 * for an interface address. */
static uint32_t
route_netmask(uint32_t ip_in)
{
	/* used to be unsigned long - check if error */
	uint32_t p = ntohl(ip_in);
	uint32_t t;

	if (IN_CLASSA(p))
		t = ~IN_CLASSA_NET;
	else {
		if (IN_CLASSB(p))
			t = ~IN_CLASSB_NET;
		else {
			if (IN_CLASSC(p))
				t = ~IN_CLASSC_NET;
			else
				t = 0;
		}
	}

	while (t & p)
		t >>= 1;

	return (htonl(~t));
}

/* We need to obey routing options.
 * If we have a CSR then we only use that.
 * Otherwise we add static routes and then routers. */
struct rt_head *
get_option_routes(struct interface *ifp, const struct dhcp_message *dhcp)
{
	struct if_options *ifo = ifp->options;
	const uint8_t *p;
	const uint8_t *e;
	struct rt_head *routes = NULL;
	struct rt *route = NULL;
	size_t len;
	const char *csr = "";

	/* If we have CSR's then we MUST use these only */
	if (!has_option_mask(ifo->nomask, DHO_CSR))
		p = get_option(ifp->ctx, dhcp, DHO_CSR, &len);
	else
		p = NULL;
	/* Check for crappy MS option */
	if (!p && !has_option_mask(ifo->nomask, DHO_MSCSR)) {
		p = get_option(ifp->ctx, dhcp, DHO_MSCSR, &len);
		if (p)
			csr = "MS ";
	}
	if (p) {
		routes = decode_rfc3442_rt(p, len);
		if (routes) {
			if (!(ifo->options & DHCPCD_CSR_WARNED)) {
				syslog(LOG_DEBUG,
				    "%s: using %sClassless Static Routes",
				    ifp->name, csr);
				ifo->options |= DHCPCD_CSR_WARNED;
			}
			return routes;
		}
	}

	/* OK, get our static routes first. */
	routes = malloc(sizeof(*routes));
	if (routes == NULL) {
		syslog(LOG_ERR, "%s: %m", __func__);
		return NULL;
	}
	TAILQ_INIT(routes);
	if (!has_option_mask(ifo->nomask, DHO_STATICROUTE))
		p = get_option(ifp->ctx, dhcp, DHO_STATICROUTE, &len);
	else
		p = NULL;
	if (p) {
		e = p + len;
		while (p < e) {
			route = calloc(1, sizeof(*route));
			if (route == NULL) {
				syslog(LOG_ERR, "%s: %m", __func__);
				ipv4_freeroutes(routes);
				return NULL;
			}
			memcpy(&route->dest.s_addr, p, 4);
			p += 4;
			memcpy(&route->gate.s_addr, p, 4);
			p += 4;
			route->net.s_addr = route_netmask(route->dest.s_addr);
			TAILQ_INSERT_TAIL(routes, route, next);
		}
	}

	/* Now grab our routers */
	if (!has_option_mask(ifo->nomask, DHO_ROUTER))
		p = get_option(ifp->ctx, dhcp, DHO_ROUTER, &len);
	else
		p = NULL;
	if (p) {
		e = p + len;
		while (p < e) {
			route = calloc(1, sizeof(*route));
			if (route == NULL) {
				syslog(LOG_ERR, "%s: %m", __func__);
				ipv4_freeroutes(routes);
				return NULL;
			}
			memcpy(&route->gate.s_addr, p, 4);
			p += 4;
			TAILQ_INSERT_TAIL(routes, route, next);
		}
	}

	return routes;
}

#define PUTADDR(_type, _val)						      \
	{								      \
		*p++ = _type;						      \
		*p++ = 4;						      \
		memcpy(p, &_val.s_addr, 4);				      \
		p += 4;							      \
	}

int
dhcp_message_add_addr(struct dhcp_message *dhcp,
    uint8_t type, struct in_addr addr)
{
	uint8_t *p;
	size_t len;

	p = dhcp->options;
	while (*p != DHO_END) {
		p++;
		p += *p + 1;
	}

	len = (size_t)(p - (uint8_t *)dhcp);
	if (len + 6 > sizeof(*dhcp)) {
		errno = ENOMEM;
		return -1;
	}

	PUTADDR(type, addr);
	*p = DHO_END;
	return 0;
}

ssize_t
make_message(struct dhcp_message **message,
    const struct interface *iface,
    uint8_t type)
{
	struct dhcp_message *dhcp;
	uint8_t *m, *lp, *p, *auth;
	uint8_t *n_params = NULL;
	uint32_t ul;
	uint16_t sz;
	size_t len, i, auth_len;
	const struct dhcp_opt *opt;
	struct if_options *ifo = iface->options;
	const struct dhcp_state *state = D_CSTATE(iface);
	const struct dhcp_lease *lease = &state->lease;
	time_t up = uptime() - state->start_uptime;
	char hbuf[HOSTNAME_MAX_LEN + 1];
	const char *hostname;
	const struct vivco *vivco;

	dhcp = calloc(1, sizeof (*dhcp));
	if (dhcp == NULL)
		return -1;
	m = (uint8_t *)dhcp;
	p = dhcp->options;

	if ((type == DHCP_INFORM || type == DHCP_RELEASE ||
		(type == DHCP_REQUEST &&
		    state->net.s_addr == lease->net.s_addr &&
		    (state->new == NULL ||
			state->new->cookie == htonl(MAGIC_COOKIE)))))
	{
		dhcp->ciaddr = state->addr.s_addr;
		/* In-case we haven't actually configured the address yet */
		if (type == DHCP_INFORM && state->addr.s_addr == 0)
			dhcp->ciaddr = lease->addr.s_addr;
	}

	dhcp->op = DHCP_BOOTREQUEST;
	dhcp->hwtype = (uint8_t)iface->family;
	switch (iface->family) {
	case ARPHRD_ETHER:
	case ARPHRD_IEEE802:
		dhcp->hwlen = (uint8_t)iface->hwlen;
		memcpy(&dhcp->chaddr, &iface->hwaddr, iface->hwlen);
		break;
	}

	if (ifo->options & DHCPCD_BROADCAST &&
	    dhcp->ciaddr == 0 &&
	    type != DHCP_DECLINE &&
	    type != DHCP_RELEASE)
		dhcp->flags = htons(BROADCAST_FLAG);

	if (type != DHCP_DECLINE && type != DHCP_RELEASE) {
		if (up < 0 || up > (time_t)UINT16_MAX)
			dhcp->secs = htons((uint16_t)UINT16_MAX);
		else
			dhcp->secs = htons(up);
	}
	dhcp->xid = htonl(state->xid);
	dhcp->cookie = htonl(MAGIC_COOKIE);

	*p++ = DHO_MESSAGETYPE;
	*p++ = 1;
	*p++ = type;

	if (state->clientid) {
		*p++ = DHO_CLIENTID;
		memcpy(p, state->clientid, state->clientid[0] + 1);
		p += state->clientid[0] + 1;
	}

	if (lease->addr.s_addr && lease->cookie == htonl(MAGIC_COOKIE)) {
		if (type == DHCP_DECLINE ||
		    (type == DHCP_REQUEST &&
			lease->addr.s_addr != state->addr.s_addr))
		{
			PUTADDR(DHO_IPADDRESS, lease->addr);
			if (lease->server.s_addr)
				PUTADDR(DHO_SERVERID, lease->server);
		}

		if (type == DHCP_RELEASE) {
			if (lease->server.s_addr)
				PUTADDR(DHO_SERVERID, lease->server);
		}
	}

	if (type == DHCP_DECLINE) {
		*p++ = DHO_MESSAGE;
		len = strlen(DAD);
		*p++ = (uint8_t)len;
		memcpy(p, DAD, len);
		p += len;
	}

	if (type == DHCP_DISCOVER &&
	    !(iface->ctx->options & DHCPCD_TEST) &&
	    has_option_mask(ifo->requestmask, DHO_RAPIDCOMMIT))
	{
		/* RFC 4039 Section 3 */
		*p++ = DHO_RAPIDCOMMIT;
		*p++ = 0;
	}

	if (type == DHCP_DISCOVER && ifo->options & DHCPCD_REQUEST)
		PUTADDR(DHO_IPADDRESS, ifo->req_addr);

	/* RFC 2563 Auto Configure */
	if (type == DHCP_DISCOVER && ifo->options & DHCPCD_IPV4LL) {
		*p++ = DHO_AUTOCONFIGURE;
		*p++ = 1;
		*p++ = 1;
	}

	if (type == DHCP_DISCOVER ||
	    type == DHCP_INFORM ||
	    type == DHCP_REQUEST)
	{
		int mtu;

		*p++ = DHO_MAXMESSAGESIZE;
		*p++ = 2;
		mtu = if_getmtu(iface->name);
		if (mtu < MTU_MIN) {
			if (if_setmtu(iface->name, MTU_MIN) == 0)
				sz = MTU_MIN;
		} else if (mtu > MTU_MAX) {
			/* Even though our MTU could be greater than
			 * MTU_MAX (1500) dhcpcd does not presently
			 * handle DHCP packets any bigger. */
			mtu = MTU_MAX;
		}
		sz = htons(mtu);
		memcpy(p, &sz, 2);
		p += 2;

		if (ifo->userclass[0]) {
			*p++ = DHO_USERCLASS;
			memcpy(p, ifo->userclass, ifo->userclass[0] + 1);
			p += ifo->userclass[0] + 1;
		}

		if (ifo->vendorclassid[0]) {
			*p++ = DHO_VENDORCLASSID;
			memcpy(p, ifo->vendorclassid,
			    ifo->vendorclassid[0] + 1);
			p += ifo->vendorclassid[0] + 1;
		}

		if (type != DHCP_INFORM) {
			if (ifo->leasetime != 0) {
				*p++ = DHO_LEASETIME;
				*p++ = 4;
				ul = htonl(ifo->leasetime);
				memcpy(p, &ul, 4);
				p += 4;
			}
		}

		if (ifo->hostname[0] == '\0')
			hostname = get_hostname(hbuf, sizeof(hbuf),
			    ifo->options & DHCPCD_HOSTNAME_SHORT ? 1 : 0);
		else
			hostname = ifo->hostname;

		/*
		 * RFC4702 3.1 States that if we send the Client FQDN option
		 * then we MUST NOT also send the Host Name option.
		 * Technically we could, but that is not RFC conformant and
		 * also seems to break some DHCP server implemetations such as
		 * Windows. On the other hand, ISC dhcpd is just as non RFC
		 * conformant by not accepting a partially qualified FQDN.
		 */
		if (ifo->fqdn != FQDN_DISABLE) {
			/* IETF DHC-FQDN option (81), RFC4702 */
			*p++ = DHO_FQDN;
			lp = p;
			*p++ = 3;
			/*
			 * Flags: 0000NEOS
			 * S: 1 => Client requests Server to update
			 *         a RR in DNS as well as PTR
			 * O: 1 => Server indicates to client that
			 *         DNS has been updated
			 * E: 1 => Name data is DNS format
			 * N: 1 => Client requests Server to not
			 *         update DNS
			 */
			if (hostname)
				*p++ = (ifo->fqdn & 0x09) | 0x04;
			else
				*p++ = (FQDN_NONE & 0x09) | 0x04;
			*p++ = 0; /* from server for PTR RR */
			*p++ = 0; /* from server for A RR if S=1 */
			if (hostname) {
				i = encode_rfc1035(hostname, p);
				*lp += i;
				p += i;
			}
		} else if (ifo->options & DHCPCD_HOSTNAME && hostname) {
			*p++ = DHO_HOSTNAME;
			len = strlen(hostname);
			*p++ = (uint8_t)len;
			memcpy(p, hostname, len);
			p += len;
		}

		/* vendor is already encoded correctly, so just add it */
		if (ifo->vendor[0]) {
			*p++ = DHO_VENDOR;
			memcpy(p, ifo->vendor, ifo->vendor[0] + 1);
			p += ifo->vendor[0] + 1;
		}

		if ((ifo->auth.options & DHCPCD_AUTH_SENDREQUIRE) !=
		    DHCPCD_AUTH_SENDREQUIRE)
		{
			/* We support HMAC-MD5 */
			*p++ = DHO_FORCERENEW_NONCE;
			*p++ = 1;
			*p++ = AUTH_ALG_HMAC_MD5;
		}

		if (ifo->vivco_len) {
			*p++ = DHO_VIVCO;
			lp = p++;
			*lp = sizeof(ul);
			ul = htonl(ifo->vivco_en);
			memcpy(p, &ul, sizeof(ul));
			p += sizeof(ul);
			for (i = 0, vivco = ifo->vivco;
			    i < ifo->vivco_len;
			    i++, vivco++)
			{
				len = (size_t)(p - m) + vivco->len + 1;
				if (len > sizeof(*dhcp))
					goto toobig;
				if (vivco->len + 2 + *lp > 255) {
					syslog(LOG_ERR,
					    "%s: VIVCO option too big",
					    iface->name);
					free(dhcp);
					return -1;
				}
				*p++ = (uint8_t)vivco->len;
				memcpy(p, vivco->data, vivco->len);
				p += vivco->len;
				*lp += (uint8_t)vivco->len + 1;
			}
		}

		len = (size_t)((p - m) + 3);
		if (len > sizeof(*dhcp))
			goto toobig;
		*p++ = DHO_PARAMETERREQUESTLIST;
		n_params = p;
		*p++ = 0;
		for (i = 0, opt = iface->ctx->dhcp_opts;
		    i < iface->ctx->dhcp_opts_len;
		    i++, opt++)
		{
			if (!(opt->type & REQUEST ||
				has_option_mask(ifo->requestmask, opt->option)))
				continue;
			if (opt->type & NOREQ)
				continue;
			if (type == DHCP_INFORM &&
			    (opt->option == DHO_RENEWALTIME ||
				opt->option == DHO_REBINDTIME))
				continue;
			len = (size_t)((p - m) + 2);
			if (len > sizeof(*dhcp))
				goto toobig;
			*p++ = (uint8_t)opt->option;
		}
		for (i = 0, opt = ifo->dhcp_override;
		    i < ifo->dhcp_override_len;
		    i++, opt++)
		{
			/* Check if added above */
			for (lp = n_params + 1; lp < p; lp++)
				if (*lp == (uint8_t)opt->option)
					break;
			if (lp < p)
				continue;
			if (!(opt->type & REQUEST ||
				has_option_mask(ifo->requestmask, opt->option)))
				continue;
			if (opt->type & NOREQ)
				continue;
			if (type == DHCP_INFORM &&
			    (opt->option == DHO_RENEWALTIME ||
				opt->option == DHO_REBINDTIME))
				continue;
			len = (size_t)((p - m) + 2);
			if (len > sizeof(*dhcp))
				goto toobig;
			*p++ = (uint8_t)opt->option;
		}
		*n_params = (uint8_t)(p - n_params - 1);
	}

	/* silence GCC */
	auth_len = 0;
	auth = NULL;

	if (ifo->auth.options & DHCPCD_AUTH_SEND) {
		auth_len = (size_t)dhcp_auth_encode(&ifo->auth,
		    state->auth.token,
		    NULL, 0, 4, type, NULL, 0);
		if ((ssize_t)auth_len == -1) {
			syslog(LOG_ERR, "%s: dhcp_auth_encode: %m",
			    iface->name);
			auth_len = 0;
		} else if (auth_len != 0) {
			len = (size_t)((p + auth_len) - m);
			if (auth_len > 255 || len > sizeof(*dhcp))
				goto toobig;
			*p++ = DHO_AUTHENTICATION;
			*p++ = (uint8_t)auth_len;
			auth = p;
			p += auth_len;
		}
	}

	*p++ = DHO_END;

#ifdef BOOTP_MESSAGE_LENTH_MIN
	/* Some crappy DHCP servers think they have to obey the BOOTP minimum
	 * message length.
	 * They are wrong, but we should still cater for them. */
	while (p - m < BOOTP_MESSAGE_LENTH_MIN)
		*p++ = DHO_PAD;
#endif

	len = (size_t)(p - m);
	if (ifo->auth.options & DHCPCD_AUTH_SEND && auth_len != 0)
		dhcp_auth_encode(&ifo->auth, state->auth.token,
		    m, len, 4, type, auth, auth_len);

	*message = dhcp;
	return (ssize_t)len;

toobig:
	syslog(LOG_ERR, "%s: DHCP messge too big", iface->name);
	free(dhcp);
	return -1;
}

ssize_t
write_lease(const struct interface *ifp, const struct dhcp_message *dhcp)
{
	int fd;
	size_t len;
	ssize_t bytes;
	const uint8_t *e, *p;
	uint8_t l;
	uint8_t o = 0;
	const struct dhcp_state *state = D_CSTATE(ifp);

	/* We don't write BOOTP leases */
	if (is_bootp(ifp, dhcp)) {
		unlink(state->leasefile);
		return 0;
	}

	syslog(LOG_DEBUG, "%s: writing lease `%s'",
	    ifp->name, state->leasefile);

	fd = open(state->leasefile, O_WRONLY | O_CREAT | O_TRUNC, 0644);
	if (fd == -1)
		return -1;

	/* Only write as much as we need */
	p = dhcp->options;
	e = p + sizeof(dhcp->options);
	len = sizeof(*dhcp);
	while (p < e) {
		o = *p;
		if (o == DHO_END) {
			len = (size_t)(p - (const uint8_t *)dhcp);
			break;
		}
		p++;
		if (o != DHO_PAD) {
			l = *p++;
			p += l;
		}
	}
	bytes = write(fd, dhcp, len);
	close(fd);
	return bytes;
}

struct dhcp_message *
read_lease(struct interface *ifp)
{
	int fd;
	struct dhcp_message *dhcp;
	struct dhcp_state *state = D_STATE(ifp);
	ssize_t bytes;
	const uint8_t *auth;
	uint8_t type;
	size_t auth_len;

	fd = open(state->leasefile, O_RDONLY);
	if (fd == -1) {
		if (errno != ENOENT)
			syslog(LOG_ERR, "%s: open `%s': %m",
			    ifp->name, state->leasefile);
		return NULL;
	}
	syslog(LOG_DEBUG, "%s: reading lease `%s'",
	    ifp->name, state->leasefile);
	dhcp = calloc(1, sizeof(*dhcp));
	if (dhcp == NULL) {
		close(fd);
		return NULL;
	}
	bytes = read(fd, dhcp, sizeof(*dhcp));
	close(fd);
	if (bytes < 0) {
		free(dhcp);
		return NULL;
	}

	/* We may have found a BOOTP server */
	if (get_option_uint8(ifp->ctx, &type, dhcp, DHO_MESSAGETYPE) == -1)
		type = 0;
	/* Authenticate the message */
	auth = get_option(ifp->ctx, dhcp, DHO_AUTHENTICATION, &auth_len);
	if (auth) {
		if (dhcp_auth_validate(&state->auth, &ifp->options->auth,
		    (uint8_t *)dhcp, sizeof(*dhcp), 4, type,
		    auth, auth_len) == NULL)
		{
			syslog(LOG_DEBUG, "%s: dhcp_auth_validate: %m",
			    ifp->name);
			free(dhcp);
			return NULL;
		}
		if (state->auth.token)
			syslog(LOG_DEBUG, "%s: validated using 0x%08" PRIu32,
			    ifp->name, state->auth.token->secretid);
		else
			syslog(LOG_DEBUG, "%s: accepted reconfigure key",
			    ifp->name);
	}

	return dhcp;
}

static const struct dhcp_opt *
dhcp_getoverride(const struct if_options *ifo, unsigned int o)
{
	size_t i;
	const struct dhcp_opt *opt;

	for (i = 0, opt = ifo->dhcp_override;
	    i < ifo->dhcp_override_len;
	    i++, opt++)
	{
		if (opt->option == o)
			return opt;
	}
	return NULL;
}

static const uint8_t *
dhcp_getoption(struct dhcpcd_ctx *ctx,
    size_t *os, unsigned int *code, size_t *len,
    const uint8_t *od, size_t ol, struct dhcp_opt **oopt)
{
	size_t i;
	struct dhcp_opt *opt;

	if (od) {
		if (ol < 2) {
			errno = EINVAL;
			return NULL;
		}
		*os = 2; /* code + len */
		*code = (unsigned int)*od++;
		*len = (size_t)*od++;
		if (*len > ol) {
			errno = EINVAL;
			return NULL;
		}
	}

	for (i = 0, opt = ctx->dhcp_opts; i < ctx->dhcp_opts_len; i++, opt++) {
		if (opt->option == *code) {
			*oopt = opt;
			break;
		}
	}

	return od;
}

ssize_t
dhcp_env(char **env, const char *prefix, const struct dhcp_message *dhcp,
    const struct interface *ifp)
{
	const struct if_options *ifo;
	const uint8_t *p;
	struct in_addr addr;
	struct in_addr net;
	struct in_addr brd;
	struct dhcp_opt *opt, *vo;
	size_t e, i, pl;
	char **ep;
	char cidr[4];
	uint8_t overl = 0;
	uint32_t en;

	e = 0;
	ifo = ifp->options;
	get_option_uint8(ifp->ctx, &overl, dhcp, DHO_OPTIONSOVERLOADED);

	if (env == NULL) {
		if (dhcp->yiaddr || dhcp->ciaddr)
			e += 5;
		if (*dhcp->bootfile && !(overl & 1))
			e++;
		if (*dhcp->servername && !(overl & 2))
			e++;
		for (i = 0, opt = ifp->ctx->dhcp_opts;
		    i < ifp->ctx->dhcp_opts_len;
		    i++, opt++)
		{
			if (has_option_mask(ifo->nomask, opt->option))
				continue;
			if (dhcp_getoverride(ifo, opt->option))
				continue;
			p = get_option(ifp->ctx, dhcp, opt->option, &pl);
			if (!p)
				continue;
			e += dhcp_envoption(ifp->ctx, NULL, NULL, ifp->name,
			    opt, dhcp_getoption, p, pl);
		}
		for (i = 0, opt = ifo->dhcp_override;
		    i < ifo->dhcp_override_len;
		    i++, opt++)
		{
			if (has_option_mask(ifo->nomask, opt->option))
				continue;
			p = get_option(ifp->ctx, dhcp, opt->option, &pl);
			if (!p)
				continue;
			e += dhcp_envoption(ifp->ctx, NULL, NULL, ifp->name,
			    opt, dhcp_getoption, p, pl);
		}
		return (ssize_t)e;
	}

	ep = env;
	if (dhcp->yiaddr || dhcp->ciaddr) {
		/* Set some useful variables that we derive from the DHCP
		 * message but are not necessarily in the options */
		addr.s_addr = dhcp->yiaddr ? dhcp->yiaddr : dhcp->ciaddr;
		setvar(&ep, prefix, "ip_address", inet_ntoa(addr));
		if (get_option_addr(ifp->ctx, &net,
		    dhcp, DHO_SUBNETMASK) == -1) {
			net.s_addr = ipv4_getnetmask(addr.s_addr);
			setvar(&ep, prefix, "subnet_mask", inet_ntoa(net));
		}
		snprintf(cidr, sizeof(cidr), "%d", inet_ntocidr(net));
		setvar(&ep, prefix, "subnet_cidr", cidr);
		if (get_option_addr(ifp->ctx, &brd,
		    dhcp, DHO_BROADCAST) == -1) {
			brd.s_addr = addr.s_addr | ~net.s_addr;
			setvar(&ep, prefix, "broadcast_address",
			    inet_ntoa(brd));
		}
		addr.s_addr = dhcp->yiaddr & net.s_addr;
		setvar(&ep, prefix, "network_number", inet_ntoa(addr));
	}

	if (*dhcp->bootfile && !(overl & 1))
		setvar(&ep, prefix, "filename", (const char *)dhcp->bootfile);
	if (*dhcp->servername && !(overl & 2))
		setvar(&ep, prefix, "server_name",
		    (const char *)dhcp->servername);

	/* Zero our indexes */
	if (env) {
		for (i = 0, opt = ifp->ctx->dhcp_opts;
		    i < ifp->ctx->dhcp_opts_len;
		    i++, opt++)
			dhcp_zero_index(opt);
		for (i = 0, opt = ifp->options->dhcp_override;
		    i < ifp->options->dhcp_override_len;
		    i++, opt++)
			dhcp_zero_index(opt);
		for (i = 0, opt = ifp->ctx->vivso;
		    i < ifp->ctx->vivso_len;
		    i++, opt++)
			dhcp_zero_index(opt);
	}

	for (i = 0, opt = ifp->ctx->dhcp_opts;
	    i < ifp->ctx->dhcp_opts_len;
	    i++, opt++)
	{
		if (has_option_mask(ifo->nomask, opt->option))
			continue;
		if (dhcp_getoverride(ifo, opt->option))
			continue;
		if ((p = get_option(ifp->ctx, dhcp, opt->option, &pl))) {
			ep += dhcp_envoption(ifp->ctx, ep, prefix, ifp->name,
			    opt, dhcp_getoption, p, pl);
			if (opt->option == DHO_VIVSO &&
			    pl > (int)sizeof(uint32_t))
			{
			        memcpy(&en, p, sizeof(en));
				en = ntohl(en);
				vo = vivso_find(en, ifp);
				if (vo) {
					/* Skip over en + total size */
					p += sizeof(en) + 1;
					pl -= sizeof(en) + 1;
					ep += dhcp_envoption(ifp->ctx,
					    ep, prefix, ifp->name,
					    vo, dhcp_getoption, p, pl);
				}
			}
		}
	}

	for (i = 0, opt = ifo->dhcp_override;
	    i < ifo->dhcp_override_len;
	    i++, opt++)
	{
		if (has_option_mask(ifo->nomask, opt->option))
			continue;
		if ((p = get_option(ifp->ctx, dhcp, opt->option, &pl)))
			ep += dhcp_envoption(ifp->ctx, ep, prefix, ifp->name,
			    opt, dhcp_getoption, p, pl);
	}

	return ep - env;
}

static void
get_lease(struct dhcpcd_ctx *ctx,
    struct dhcp_lease *lease, const struct dhcp_message *dhcp)
{

	lease->cookie = dhcp->cookie;
	/* BOOTP does not set yiaddr for replies when ciaddr is set. */
	if (dhcp->yiaddr)
		lease->addr.s_addr = dhcp->yiaddr;
	else
		lease->addr.s_addr = dhcp->ciaddr;
	if (get_option_addr(ctx, &lease->net, dhcp, DHO_SUBNETMASK) == -1)
		lease->net.s_addr = ipv4_getnetmask(lease->addr.s_addr);
	if (get_option_addr(ctx, &lease->brd, dhcp, DHO_BROADCAST) == -1)
		lease->brd.s_addr = lease->addr.s_addr | ~lease->net.s_addr;
	if (get_option_uint32(ctx, &lease->leasetime, dhcp, DHO_LEASETIME) != 0)
		lease->leasetime = ~0U; /* Default to infinite lease */
	if (get_option_uint32(ctx, &lease->renewaltime,
	    dhcp, DHO_RENEWALTIME) != 0)
		lease->renewaltime = 0;
	if (get_option_uint32(ctx, &lease->rebindtime,
	    dhcp, DHO_REBINDTIME) != 0)
		lease->rebindtime = 0;
	if (get_option_addr(ctx, &lease->server, dhcp, DHO_SERVERID) != 0)
		lease->server.s_addr = INADDR_ANY;
}

static const char *
get_dhcp_op(uint8_t type)
{
	const struct dhcp_op *d;

	for (d = dhcp_ops; d->name; d++)
		if (d->value == type)
			return d->name;
	return NULL;
}

static void
dhcp_fallback(void *arg)
{
	struct interface *iface;

	iface = (struct interface *)arg;
	dhcpcd_selectprofile(iface, iface->options->fallback);
	dhcpcd_startinterface(iface);
}

uint32_t
dhcp_xid(const struct interface *ifp)
{
	uint32_t xid;

	if (ifp->options->options & DHCPCD_XID_HWADDR &&
	    ifp->hwlen >= sizeof(xid))
		/* The lower bits are probably more unique on the network */
		memcpy(&xid, (ifp->hwaddr + ifp->hwlen) - sizeof(xid),
		    sizeof(xid));
	else
		xid = arc4random();

	return xid;
}

void
dhcp_close(struct interface *ifp)
{
	struct dhcp_state *state = D_STATE(ifp);

	if (state == NULL)
		return;

	if (state->arp_fd != -1) {
		eloop_event_delete(ifp->ctx->eloop, state->arp_fd, 0);
		close(state->arp_fd);
		state->arp_fd = -1;
	}
	if (state->raw_fd != -1) {
		eloop_event_delete(ifp->ctx->eloop, state->raw_fd, 0);
		close(state->raw_fd);
		state->raw_fd = -1;
	}

	state->interval = 0;
}

static int
dhcp_openudp(struct interface *ifp)
{
	int s;
	struct sockaddr_in sin;
	int n;
	struct dhcp_state *state;
#ifdef SO_BINDTODEVICE
	struct ifreq ifr;
	char *p;
#endif

#ifdef SOCK_CLOEXEC
	if ((s = socket(PF_INET, SOCK_DGRAM | SOCK_CLOEXEC, IPPROTO_UDP)) == -1)
		return -1;
#else
	if ((s = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
		return -1;
	if ((n = fcntl(s, F_GETFD, 0)) == -1 ||
	    fcntl(s, F_SETFD, n | FD_CLOEXEC) == -1)
	{
		close(s);
	        return -1;
	}
#endif

	n = 1;
	if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &n, sizeof(n)) == -1)
		goto eexit;
#ifdef SO_BINDTODEVICE
	if (ifp) {
		memset(&ifr, 0, sizeof(ifr));
		strlcpy(ifr.ifr_name, ifp->name, sizeof(ifr.ifr_name));
		/* We can only bind to the real device */
		p = strchr(ifr.ifr_name, ':');
		if (p)
			*p = '\0';
		if (setsockopt(s, SOL_SOCKET, SO_BINDTODEVICE, &ifr,
		    sizeof(ifr)) == -1)
		        goto eexit;
	}
#endif
	memset(&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_port = htons(DHCP_CLIENT_PORT);
	if (ifp) {
		state = D_STATE(ifp);
		sin.sin_addr.s_addr = state->addr.s_addr;
	} else
		state = NULL; /* appease gcc */
	if (bind(s, (struct sockaddr *)&sin, sizeof(sin)) == -1)
		goto eexit;

	return s;

eexit:
	close(s);
	return -1;
}

static uint16_t
checksum(const void *data, uint16_t len)
{
	const uint8_t *addr = data;
	uint32_t sum = 0;
	uint16_t res;

	while (len > 1) {
		sum += addr[0] * 256 + addr[1];
		addr += 2;
		len -= 2;
	}

	if (len == 1)
		sum += *addr * 256;

	sum = (sum >> 16) + (sum & 0xffff);
	sum += (sum >> 16);

	res = htons(sum);

	return ~res;
}

static struct udp_dhcp_packet *
dhcp_makeudppacket(size_t *sz, const uint8_t *data, size_t length,
	struct in_addr source, struct in_addr dest)
{
	struct udp_dhcp_packet *udpp;
	struct ip *ip;
	struct udphdr *udp;

	udpp = calloc(1, sizeof(*udpp));
	if (udpp == NULL)
		return NULL;
	ip = &udpp->ip;
	udp = &udpp->udp;

	/* OK, this is important :)
	 * We copy the data to our packet and then create a small part of the
	 * ip structure and an invalid ip_len (basically udp length).
	 * We then fill the udp structure and put the checksum
	 * of the whole packet into the udp checksum.
	 * Finally we complete the ip structure and ip checksum.
	 * If we don't do the ordering like so then the udp checksum will be
	 * broken, so find another way of doing it! */

	memcpy(&udpp->dhcp, data, length);

	ip->ip_p = IPPROTO_UDP;
	ip->ip_src.s_addr = source.s_addr;
	if (dest.s_addr == 0)
		ip->ip_dst.s_addr = INADDR_BROADCAST;
	else
		ip->ip_dst.s_addr = dest.s_addr;

	udp->uh_sport = htons(DHCP_CLIENT_PORT);
	udp->uh_dport = htons(DHCP_SERVER_PORT);
	udp->uh_ulen = htons(sizeof(*udp) + length);
	ip->ip_len = udp->uh_ulen;
	udp->uh_sum = checksum(udpp, sizeof(*udpp));

	ip->ip_v = IPVERSION;
	ip->ip_hl = sizeof(*ip) >> 2;
	ip->ip_id = (uint16_t)arc4random_uniform(UINT16_MAX);
	ip->ip_ttl = IPDEFTTL;
	ip->ip_len = htons(sizeof(*ip) + sizeof(*udp) + length);
	ip->ip_sum = checksum(ip, sizeof(*ip));

	*sz = sizeof(*ip) + sizeof(*udp) + length;
	return udpp;
}

static void
send_message(struct interface *iface, uint8_t type,
    void (*callback)(void *))
{
	struct dhcp_state *state = D_STATE(iface);
	struct if_options *ifo = iface->options;
	struct dhcp_message *dhcp;
	struct udp_dhcp_packet *udp;
	size_t len;
	ssize_t r;
	struct in_addr from, to;
	in_addr_t a = INADDR_ANY;
	struct timeval tv;
	int s;

	if (!callback)
		syslog(LOG_DEBUG, "%s: sending %s with xid 0x%x",
		    iface->name, get_dhcp_op(type), state->xid);
	else {
		if (state->interval == 0)
			state->interval = 4;
		else {
			state->interval *= 2;
			if (state->interval > 64)
				state->interval = 64;
		}
		tv.tv_sec = state->interval + DHCP_RAND_MIN;
		tv.tv_usec = (suseconds_t)arc4random_uniform(
		    (DHCP_RAND_MAX - DHCP_RAND_MIN) * 1000000);
		timernorm(&tv);
		syslog(LOG_DEBUG,
		    "%s: sending %s (xid 0x%x), next in %0.1f seconds",
		    iface->name, get_dhcp_op(type), state->xid,
		    timeval_to_double(&tv));
	}

	if (dhcp_open(iface) == -1)
		return;

	if (state->addr.s_addr != INADDR_ANY &&
	    state->new != NULL &&
	    (state->new->cookie == htonl(MAGIC_COOKIE) ||
	    iface->options->options & DHCPCD_INFORM))
	{
		s = dhcp_openudp(iface);
		if (s == -1 && errno != EADDRINUSE)
			syslog(LOG_ERR, "%s: dhcp_openudp: %m", iface->name);
	} else
		s = -1;

	/* If we couldn't open a UDP port for our IP address
	 * then we cannot renew.
	 * This could happen if our IP was pulled out from underneath us.
	 * Also, we should not unicast from a BOOTP lease. */
	if (s == -1 ||
	    (!(ifo->options & DHCPCD_INFORM) &&
	    is_bootp(iface, state->new)))
	{
		a = state->addr.s_addr;
		state->addr.s_addr = INADDR_ANY;
	}
	r = make_message(&dhcp, iface, type);
	if (r == -1)
		goto fail;
	len = (size_t)r;
	if (a)
		state->addr.s_addr = a;
	from.s_addr = dhcp->ciaddr;
	if (from.s_addr)
		to.s_addr = state->lease.server.s_addr;
	else
		to.s_addr = INADDR_ANY;
	if (to.s_addr && to.s_addr != INADDR_BROADCAST) {
		struct sockaddr_in sin;

		memset(&sin, 0, sizeof(sin));
		sin.sin_family = AF_INET;
		sin.sin_addr.s_addr = to.s_addr;
		sin.sin_port = htons(DHCP_SERVER_PORT);
		r = sendto(s, (uint8_t *)dhcp, len, 0,
		    (struct sockaddr *)&sin, sizeof(sin));
		if (r == -1)
			syslog(LOG_ERR, "%s: dhcp_sendpacket: %m", iface->name);
	} else {
		size_t ulen;

		r = 0;
		udp = dhcp_makeudppacket(&ulen, (uint8_t *)dhcp, len, from, to);
		if (udp == NULL) {
			syslog(LOG_ERR, "dhcp_makeudppacket: %m");
		} else {
			r = if_sendrawpacket(iface, ETHERTYPE_IP,
			    (uint8_t *)udp, ulen);
			free(udp);
		}
		/* If we failed to send a raw packet this normally means
		 * we don't have the ability to work beneath the IP layer
		 * for this interface.
		 * As such we remove it from consideration without actually
		 * stopping the interface. */
		if (r == -1) {
			syslog(LOG_ERR, "%s: if_sendrawpacket: %m",
			    iface->name);
			switch(errno) {
			case ENETDOWN:
			case ENETRESET:
			case ENETUNREACH:
				break;
			default:
				if (!(iface->ctx->options & DHCPCD_TEST))
					dhcp_drop(iface, "FAIL");
				dhcp_close(iface);
				eloop_timeout_delete(iface->ctx->eloop,
				    NULL, iface);
				callback = NULL;
			}
		}
	}
	free(dhcp);

fail:
	if (s != -1)
		close(s);

	/* Even if we fail to send a packet we should continue as we are
	 * as our failure timeouts will change out codepath when needed. */
	if (callback)
		eloop_timeout_add_tv(iface->ctx->eloop, &tv, callback, iface);
}

static void
send_inform(void *arg)
{

	send_message((struct interface *)arg, DHCP_INFORM, send_inform);
}

static void
send_discover(void *arg)
{

	send_message((struct interface *)arg, DHCP_DISCOVER, send_discover);
}

static void
send_request(void *arg)
{

	send_message((struct interface *)arg, DHCP_REQUEST, send_request);
}

static void
send_renew(void *arg)
{

	send_message((struct interface *)arg, DHCP_REQUEST, send_renew);
}

static void
send_rebind(void *arg)
{

	send_message((struct interface *)arg, DHCP_REQUEST, send_rebind);
}

void
dhcp_discover(void *arg)
{
	struct interface *ifp = arg;
	struct dhcp_state *state = D_STATE(ifp);
	struct if_options *ifo = ifp->options;
	time_t timeout = ifo->timeout;

	/* If we're rebooting and we're not daemonised then we need
	 * to shorten the normal timeout to ensure we try correctly
	 * for a fallback or IPv4LL address. */
	if (state->state == DHS_REBOOT &&
	    !(ifp->ctx->options & DHCPCD_DAEMONISED))
	{
		if (ifo->reboot >= timeout)
			timeout = 2;
		else
			timeout -= ifo->reboot;
	}

	state->state = DHS_DISCOVER;
	state->xid = dhcp_xid(ifp);
	eloop_timeout_delete(ifp->ctx->eloop, NULL, ifp);
	if (ifo->fallback)
		eloop_timeout_add_sec(ifp->ctx->eloop,
		    timeout, dhcp_fallback, ifp);
	else if (ifo->options & DHCPCD_IPV4LL &&
	    !IN_LINKLOCAL(htonl(state->addr.s_addr)))
	{
		if (IN_LINKLOCAL(htonl(state->fail.s_addr)))
			timeout = RATE_LIMIT_INTERVAL;
		eloop_timeout_add_sec(ifp->ctx->eloop,
		    timeout, ipv4ll_start, ifp);
	}
	if (ifo->options & DHCPCD_REQUEST)
		syslog(LOG_INFO, "%s: soliciting a DHCP lease (requesting %s)",
		    ifp->name, inet_ntoa(ifo->req_addr));
	else
		syslog(LOG_INFO, "%s: soliciting a DHCP lease", ifp->name);
	send_discover(ifp);
}

static void
dhcp_request(void *arg)
{
	struct interface *ifp = arg;
	struct dhcp_state *state = D_STATE(ifp);

	state->state = DHS_REQUEST;
	send_request(ifp);
}

static void
dhcp_expire(void *arg)
{
	struct interface *ifp = arg;
	struct dhcp_state *state = D_STATE(ifp);

	syslog(LOG_ERR, "%s: DHCP lease expired", ifp->name);
	eloop_timeout_delete(ifp->ctx->eloop, NULL, ifp);
	dhcp_drop(ifp, "EXPIRE");
	unlink(state->leasefile);

	state->interval = 0;
	dhcp_discover(ifp);
}

void
dhcp_decline(struct interface *ifp)
{

	send_message(ifp, DHCP_DECLINE, NULL);
}

static void
dhcp_renew(void *arg)
{
	struct interface *ifp = arg;
	struct dhcp_state *state = D_STATE(ifp);
	struct dhcp_lease *lease = &state->lease;

	syslog(LOG_DEBUG, "%s: renewing lease of %s",
	    ifp->name, inet_ntoa(lease->addr));
	syslog(LOG_DEBUG, "%s: rebind in %"PRIu32" seconds,"
	    " expire in %"PRIu32" seconds",
	    ifp->name, lease->rebindtime - lease->renewaltime,
	    lease->leasetime - lease->renewaltime);
	state->state = DHS_RENEW;
	state->xid = dhcp_xid(ifp);
	send_renew(ifp);
}

static void
dhcp_rebind(void *arg)
{
	struct interface *ifp = arg;
	struct dhcp_state *state = D_STATE(ifp);
	struct dhcp_lease *lease = &state->lease;

	syslog(LOG_WARNING, "%s: failed to renew DHCP, rebinding",
	    ifp->name);
	syslog(LOG_DEBUG, "%s: expire in %"PRIu32" seconds",
	    ifp->name, lease->leasetime - lease->rebindtime);
	state->state = DHS_REBIND;
	eloop_timeout_delete(ifp->ctx->eloop, send_renew, ifp);
	state->lease.server.s_addr = 0;
	ifp->options->options &= ~ DHCPCD_CSR_WARNED;
	send_rebind(ifp);
}

void
dhcp_bind(void *arg)
{
	struct interface *iface = arg;
	struct dhcp_state *state = D_STATE(iface);
	struct if_options *ifo = iface->options;
	struct dhcp_lease *lease = &state->lease;
	struct timeval tv;
	uint8_t ipv4ll = 0;

	state->reason = NULL;
	state->xid = 0;
	free(state->old);
	state->old = state->new;
	state->new = state->offer;
	state->offer = NULL;
	get_lease(iface->ctx, lease, state->new);
	if (ifo->options & DHCPCD_STATIC) {
		syslog(LOG_INFO, "%s: using static address %s/%d",
		    iface->name, inet_ntoa(lease->addr),
		    inet_ntocidr(lease->net));
		lease->leasetime = ~0U;
		state->reason = "STATIC";
	} else if (state->new->cookie != htonl(MAGIC_COOKIE)) {
		syslog(LOG_INFO, "%s: using IPv4LL address %s",
		    iface->name, inet_ntoa(lease->addr));
		lease->leasetime = ~0U;
		state->reason = "IPV4LL";
		ipv4ll = 1;
	} else if (ifo->options & DHCPCD_INFORM) {
		if (ifo->req_addr.s_addr != 0)
			lease->addr.s_addr = ifo->req_addr.s_addr;
		else
			lease->addr.s_addr = state->addr.s_addr;
		syslog(LOG_INFO, "%s: received approval for %s", iface->name,
		    inet_ntoa(lease->addr));
		lease->leasetime = ~0U;
		state->reason = "INFORM";
	} else {
		if (gettimeofday(&tv, NULL) == 0)
			lease->leasedfrom = tv.tv_sec;
		else if (lease->frominfo)
			state->reason = "TIMEOUT";
		if (lease->leasetime == ~0U) {
			lease->renewaltime =
			    lease->rebindtime =
			    lease->leasetime;
			syslog(LOG_INFO, "%s: leased %s for infinity",
			    iface->name, inet_ntoa(lease->addr));
		} else {
			if (lease->leasetime < DHCP_MIN_LEASE) {
				syslog(LOG_WARNING,
				    "%s: minimum lease is %d seconds",
				    iface->name, DHCP_MIN_LEASE);
				lease->leasetime = DHCP_MIN_LEASE;
			}
			if (lease->rebindtime == 0)
				lease->rebindtime =
				    (uint32_t)(lease->leasetime * T2);
			else if (lease->rebindtime >= lease->leasetime) {
				lease->rebindtime =
				    (uint32_t)(lease->leasetime * T2);
				syslog(LOG_WARNING,
				    "%s: rebind time greater than lease "
				    "time, forcing to %"PRIu32" seconds",
				    iface->name, lease->rebindtime);
			}
			if (lease->renewaltime == 0)
				lease->renewaltime =
				    (uint32_t)(lease->leasetime * T1);
			else if (lease->renewaltime > lease->rebindtime) {
				lease->renewaltime =
				    (uint32_t)(lease->leasetime * T1);
				syslog(LOG_WARNING,
				    "%s: renewal time greater than rebind "
				    "time, forcing to %"PRIu32" seconds",
				    iface->name, lease->renewaltime);
			}
			syslog(lease->addr.s_addr == state->addr.s_addr ?
			    LOG_DEBUG : LOG_INFO,
			    "%s: leased %s for %"PRIu32" seconds", iface->name,
			    inet_ntoa(lease->addr), lease->leasetime);
		}
	}
	if (iface->ctx->options & DHCPCD_TEST) {
		state->reason = "TEST";
		script_runreason(iface, state->reason);
		eloop_exit(iface->ctx->eloop, EXIT_SUCCESS);
		return;
	}
	if (state->reason == NULL) {
		if (state->old) {
			if (state->old->yiaddr == state->new->yiaddr &&
			    lease->server.s_addr)
				state->reason = "RENEW";
			else
				state->reason = "REBIND";
		} else if (state->state == DHS_REBOOT)
			state->reason = "REBOOT";
		else
			state->reason = "BOUND";
	}
	if (lease->leasetime == ~0U)
		lease->renewaltime = lease->rebindtime = lease->leasetime;
	else {
		eloop_timeout_add_sec(iface->ctx->eloop,
		    (time_t)lease->renewaltime, dhcp_renew, iface);
		eloop_timeout_add_sec(iface->ctx->eloop,
		    (time_t)lease->rebindtime, dhcp_rebind, iface);
		eloop_timeout_add_sec(iface->ctx->eloop,
		    (time_t)lease->leasetime, dhcp_expire, iface);
		syslog(LOG_DEBUG,
		    "%s: renew in %"PRIu32" seconds, rebind in %"PRIu32
		    " seconds",
		    iface->name, lease->renewaltime, lease->rebindtime);
	}
	ipv4_applyaddr(iface);
	if (dhcpcd_daemonise(iface->ctx) == 0) {
		if (!ipv4ll)
			arp_close(iface);
		state->state = DHS_BOUND;
		if (ifo->options & DHCPCD_ARP) {
		        state->claims = 0;
			arp_announce(iface);
		}
	}
}

static void
dhcp_timeout(void *arg)
{
	struct interface *ifp = arg;
	struct dhcp_state *state = D_STATE(ifp);

	dhcp_bind(ifp);
	state->interval = 0;
	dhcp_discover(ifp);
}

struct dhcp_message *
dhcp_message_new(const struct in_addr *addr, const struct in_addr *mask)
{
	struct dhcp_message *dhcp;
	uint8_t *p;

	dhcp = calloc(1, sizeof(*dhcp));
	if (dhcp == NULL)
		return NULL;
	dhcp->yiaddr = addr->s_addr;
	p = dhcp->options;
	if (mask && mask->s_addr != INADDR_ANY) {
		*p++ = DHO_SUBNETMASK;
		*p++ = sizeof(mask->s_addr);
		memcpy(p, &mask->s_addr, sizeof(mask->s_addr));
		p+= sizeof(mask->s_addr);
	}
	*p++ = DHO_END;
	return dhcp;
}

static void
dhcp_static(struct interface *ifp)
{
	struct if_options *ifo;
	struct dhcp_state *state;

	state = D_STATE(ifp);
	ifo = ifp->options;
	if (ifo->req_addr.s_addr == INADDR_ANY) {
		syslog(LOG_INFO,
		    "%s: waiting for 3rd party to "
		    "configure IP address",
		    ifp->name);
		state->reason = "3RDPARTY";
		script_runreason(ifp, state->reason);
		return;
	}
	state->offer = dhcp_message_new(&ifo->req_addr, &ifo->req_mask);
	if (state->offer) {
		eloop_timeout_delete(ifp->ctx->eloop, NULL, ifp);
		dhcp_bind(ifp);
	}
}

void
dhcp_inform(struct interface *ifp)
{
	struct dhcp_state *state;
	struct if_options *ifo;
	struct ipv4_addr *ap;

	state = D_STATE(ifp);
	ifo = ifp->options;
	if (ifp->ctx->options & DHCPCD_TEST) {
		state->addr.s_addr = ifo->req_addr.s_addr;
		state->net.s_addr = ifo->req_mask.s_addr;
	} else {
		if (ifo->req_addr.s_addr == INADDR_ANY) {
			state = D_STATE(ifp);
			ap = ipv4_findaddr(ifp, NULL, NULL);
			if (ap == NULL) {
				syslog(LOG_INFO,
					"%s: waiting for 3rd party to "
					"configure IP address",
					ifp->name);
				state->reason = "3RDPARTY";
				script_runreason(ifp, state->reason);
				return;
			}
			state->offer =
			    dhcp_message_new(&ap->addr, &ap->net);
		} else
			state->offer =
			    dhcp_message_new(&ifo->req_addr, &ifo->req_mask);
		if (state->offer) {
			ifo->options |= DHCPCD_STATIC;
			dhcp_bind(ifp);
			ifo->options &= ~DHCPCD_STATIC;
		}
	}

	state->state = DHS_INFORM;
	state->xid = dhcp_xid(ifp);
	send_inform(ifp);
}

void
dhcp_reboot_newopts(struct interface *ifp, unsigned long long oldopts)
{
	struct if_options *ifo;
	struct dhcp_state *state = D_STATE(ifp);

	if (state == NULL)
		return;
	ifo = ifp->options;
	if ((ifo->options & (DHCPCD_INFORM | DHCPCD_STATIC) &&
		state->addr.s_addr != ifo->req_addr.s_addr) ||
	    (oldopts & (DHCPCD_INFORM | DHCPCD_STATIC) &&
		!(ifo->options & (DHCPCD_INFORM | DHCPCD_STATIC))))
	{
		dhcp_drop(ifp, "EXPIRE");
	}
}

static void
dhcp_reboot(struct interface *ifp)
{
	struct if_options *ifo;
	struct dhcp_state *state = D_STATE(ifp);

	if (state == NULL)
		return;
	ifo = ifp->options;
	state->interval = 0;

	if (ifo->options & DHCPCD_LINK && ifp->carrier == LINK_DOWN) {
		syslog(LOG_INFO, "%s: waiting for carrier", ifp->name);
		return;
	}
	if (ifo->options & DHCPCD_STATIC) {
		dhcp_static(ifp);
		return;
	}
	if (ifo->reboot == 0 || state->offer == NULL) {
		dhcp_discover(ifp);
		return;
	}
	if (ifo->options & DHCPCD_INFORM) {
		syslog(LOG_INFO, "%s: informing address of %s",
		    ifp->name, inet_ntoa(state->lease.addr));
	} else if (state->offer->cookie == 0) {
		if (ifo->options & DHCPCD_IPV4LL) {
			state->claims = 0;
			arp_announce(ifp);
		} else
			dhcp_discover(ifp);
		return;
	} else {
		syslog(LOG_INFO, "%s: rebinding lease of %s",
		    ifp->name, inet_ntoa(state->lease.addr));
	}
	state->state = DHS_REBOOT;
	state->xid = dhcp_xid(ifp);
	state->lease.server.s_addr = 0;
	eloop_timeout_delete(ifp->ctx->eloop, NULL, ifp);
	if (ifo->fallback)
		eloop_timeout_add_sec(ifp->ctx->eloop,
		    ifo->reboot, dhcp_fallback, ifp);
	else if (ifo->options & DHCPCD_LASTLEASE && state->lease.frominfo)
		eloop_timeout_add_sec(ifp->ctx->eloop,
		    ifo->reboot, dhcp_timeout, ifp);
	else if (!(ifo->options & DHCPCD_INFORM &&
	    ifp->ctx->options & (DHCPCD_MASTER | DHCPCD_DAEMONISED)))
		eloop_timeout_add_sec(ifp->ctx->eloop,
		    ifo->reboot, dhcp_expire, ifp);
	/* Don't bother ARP checking as the server could NAK us first. */
	if (ifo->options & DHCPCD_INFORM)
		dhcp_inform(ifp);
	else {
		/* Don't call dhcp_request as that would change the state */
		send_request(ifp);
	}
}

void
dhcp_drop(struct interface *ifp, const char *reason)
{
	struct dhcp_state *state;
#ifdef RELEASE_SLOW
	struct timespec ts;
#endif

	state = D_STATE(ifp);
	if (state == NULL)
		return;
	dhcp_auth_reset(&state->auth);
	dhcp_close(ifp);
	arp_close(ifp);
	eloop_timeout_delete(ifp->ctx->eloop, NULL, ifp);
	if (ifp->options->options & DHCPCD_RELEASE) {
		unlink(state->leasefile);
		if (ifp->carrier != LINK_DOWN &&
		    state->new != NULL &&
		    state->new->cookie == htonl(MAGIC_COOKIE))
		{
			syslog(LOG_INFO, "%s: releasing lease of %s",
			    ifp->name, inet_ntoa(state->lease.addr));
			state->xid = dhcp_xid(ifp);
			send_message(ifp, DHCP_RELEASE, NULL);
#ifdef RELEASE_SLOW
			/* Give the packet a chance to go */
			ts.tv_sec = RELEASE_DELAY_S;
			ts.tv_nsec = RELEASE_DELAY_NS;
			nanosleep(&ts, NULL);
#endif
		}
	}
	free(state->old);
	state->old = state->new;
	state->new = NULL;
	state->reason = reason;
	ipv4_applyaddr(ifp);
	free(state->old);
	state->old = NULL;
	state->lease.addr.s_addr = 0;
	ifp->options->options &= ~ DHCPCD_CSR_WARNED;
}

static void
log_dhcp1(int lvl, const char *msg,
    const struct interface *iface, const struct dhcp_message *dhcp,
    const struct in_addr *from, int ad)
{
	const char *tfrom;
	char *a;
	struct in_addr addr;
	int r;

	if (strcmp(msg, "NAK:") == 0)
		a = get_option_string(iface->ctx, dhcp, DHO_MESSAGE);
	else if (ad && dhcp->yiaddr != 0) {
		addr.s_addr = dhcp->yiaddr;
		a = strdup(inet_ntoa(addr));
		if (a == NULL) {
			syslog(LOG_ERR, "%s: %m", __func__);
			return;
		}
	} else
		a = NULL;

	tfrom = "from";
	r = get_option_addr(iface->ctx, &addr, dhcp, DHO_SERVERID);
	if (dhcp->servername[0] && r == 0) {
		if (a == NULL)
			syslog(lvl, "%s: %s %s %s `%s'", iface->name, msg,
			    tfrom, inet_ntoa(addr), dhcp->servername);
		else
			syslog(lvl, "%s: %s %s %s %s `%s'", iface->name, msg, a,
			    tfrom, inet_ntoa(addr), dhcp->servername);
	} else {
		if (r != 0) {
			tfrom = "via";
			addr = *from;
		}
		if (a == NULL)
			syslog(lvl, "%s: %s %s %s",
			    iface->name, msg, tfrom, inet_ntoa(addr));
		else
			syslog(lvl, "%s: %s %s %s %s",
			    iface->name, msg, a, tfrom, inet_ntoa(addr));
	}
	free(a);
}

static void
log_dhcp(int lvl, const char *msg,
    const struct interface *iface, const struct dhcp_message *dhcp,
    const struct in_addr *from)
{

	log_dhcp1(lvl, msg, iface, dhcp, from, 1);
}

static int
blacklisted_ip(const struct if_options *ifo, in_addr_t addr)
{
	size_t i;

	for (i = 0; i < ifo->blacklist_len; i += 2)
		if (ifo->blacklist[i] == (addr & ifo->blacklist[i + 1]))
			return 1;
	return 0;
}

static int
whitelisted_ip(const struct if_options *ifo, in_addr_t addr)
{
	size_t i;

	if (ifo->whitelist_len == 0)
		return -1;
	for (i = 0; i < ifo->whitelist_len; i += 2)
		if (ifo->whitelist[i] == (addr & ifo->whitelist[i + 1]))
			return 1;
	return 0;
}

static void
dhcp_handledhcp(struct interface *iface, struct dhcp_message **dhcpp,
    const struct in_addr *from)
{
	struct dhcp_state *state = D_STATE(iface);
	struct if_options *ifo = iface->options;
	struct dhcp_message *dhcp = *dhcpp;
	struct dhcp_lease *lease = &state->lease;
	uint8_t type, tmp;
	const uint8_t *auth;
	struct in_addr addr;
	unsigned int i;
	size_t auth_len;
	char *msg;

	/* We may have found a BOOTP server */
	if (get_option_uint8(iface->ctx, &type, dhcp, DHO_MESSAGETYPE) == -1)
		type = 0;

	/* Authenticate the message */
	auth = get_option(iface->ctx, dhcp, DHO_AUTHENTICATION, &auth_len);
	if (auth) {
		if (dhcp_auth_validate(&state->auth, &ifo->auth,
		    (uint8_t *)*dhcpp, sizeof(**dhcpp), 4, type,
		    auth, auth_len) == NULL)
		{
			syslog(LOG_DEBUG, "%s: dhcp_auth_validate: %m",
			    iface->name);
			log_dhcp1(LOG_ERR, "authentication failed",
			    iface, dhcp, from, 0);
			return;
		}
		if (state->auth.token)
			syslog(LOG_DEBUG, "%s: validated using 0x%08" PRIu32,
			    iface->name, state->auth.token->secretid);
		else
			syslog(LOG_DEBUG, "%s: accepted reconfigure key",
			    iface->name);
	} else if (ifo->auth.options & DHCPCD_AUTH_REQUIRE) {
		log_dhcp1(LOG_ERR, "no authentication", iface, dhcp, from, 0);
		return;
	} else if (ifo->auth.options & DHCPCD_AUTH_SEND)
		log_dhcp1(LOG_WARNING, "no authentication",
		    iface, dhcp, from, 0);

	/* RFC 3203 */
	if (type == DHCP_FORCERENEW) {
		if (from->s_addr == INADDR_ANY ||
		    from->s_addr == INADDR_BROADCAST)
		{
			log_dhcp(LOG_ERR, "discarding Force Renew",
			    iface, dhcp, from);
			return;
		}
		if (auth == NULL) {
			log_dhcp(LOG_ERR, "unauthenticated Force Renew",
			    iface, dhcp, from);
			return;
		}
		if (state->state != DHS_BOUND && state->state != DHS_INFORM) {
			log_dhcp(LOG_DEBUG, "not bound, ignoring Force Renew",
			    iface, dhcp, from);
			return;
		}
		log_dhcp(LOG_ERR, "Force Renew from", iface, dhcp, from);
		/* The rebind and expire timings are still the same, we just
		 * enter the renew state early */
		if (state->state == DHS_BOUND) {
			eloop_timeout_delete(iface->ctx->eloop,
			    dhcp_renew, iface);
			dhcp_renew(iface);
		} else {
			eloop_timeout_delete(iface->ctx->eloop,
			    send_inform, iface);
			dhcp_inform(iface);
		}
		return;
	}

	if (state->state == DHS_BOUND) {
		/* Before we supported FORCERENEW we closed off the raw
		 * port so we effectively ignored all messages.
		 * As such we'll not log by default here. */
		//log_dhcp(LOG_DEBUG, "bound, ignoring", iface, dhcp, from);
		return;
	}

	/* Ensure it's the right transaction */
	if (state->xid != ntohl(dhcp->xid)) {
		syslog(LOG_DEBUG,
		    "%s: wrong xid 0x%x (expecting 0x%x) from %s",
		    iface->name, ntohl(dhcp->xid), state->xid,
		    inet_ntoa(*from));
		return;
	}
	/* reset the message counter */
	state->interval = 0;

	if (type == DHCP_NAK) {
		/* For NAK, only check if we require the ServerID */
		if (has_option_mask(ifo->requiremask, DHO_SERVERID) &&
		    get_option_addr(iface->ctx, &addr, dhcp, DHO_SERVERID) == -1)
		{
			log_dhcp(LOG_WARNING, "reject NAK", iface, dhcp, from);
			return;
		}

		/* We should restart on a NAK */
		log_dhcp(LOG_WARNING, "NAK:", iface, dhcp, from);
		if ((msg = get_option_string(iface->ctx, dhcp, DHO_MESSAGE))) {
			syslog(LOG_WARNING, "%s: message: %s",
			    iface->name, msg);
			free(msg);
		}
		if (!(iface->ctx->options & DHCPCD_TEST)) {
			dhcp_drop(iface, "NAK");
			unlink(state->leasefile);
		}

		/* If we constantly get NAKS then we should slowly back off */
		eloop_timeout_add_sec(iface->ctx->eloop,
		    state->nakoff, dhcp_discover, iface);
		if (state->nakoff == 0)
			state->nakoff = 1;
		else {
			state->nakoff *= 2;
			if (state->nakoff > NAKOFF_MAX)
				state->nakoff = NAKOFF_MAX;
		}
		return;
	}

	/* DHCP Auto-Configure, RFC 2563 */
	if (type == DHCP_OFFER && dhcp->yiaddr == 0) {
		log_dhcp(LOG_WARNING, "no address given", iface, dhcp, from);
		if ((msg = get_option_string(iface->ctx, dhcp, DHO_MESSAGE))) {
			syslog(LOG_WARNING, "%s: message: %s",
			    iface->name, msg);
			free(msg);
		}
		if (state->state == DHS_DISCOVER &&
		    get_option_uint8(iface->ctx, &tmp, dhcp,
		    DHO_AUTOCONFIGURE) == 0)
		{
			switch (tmp) {
			case 0:
				log_dhcp(LOG_WARNING, "IPv4LL disabled from",
				    iface, dhcp, from);
				dhcp_close(iface);
				eloop_timeout_delete(iface->ctx->eloop,
				    NULL, iface);
				eloop_timeout_add_sec(iface->ctx->eloop,
				    DHCP_MAX, dhcp_discover,
				    iface);
				break;
			case 1:
				log_dhcp(LOG_WARNING, "IPv4LL enabled from",
				    iface, dhcp, from);
				eloop_timeout_delete(iface->ctx->eloop,
				    NULL, iface);
				if (IN_LINKLOCAL(htonl(state->addr.s_addr)))
					eloop_timeout_add_sec(iface->ctx->eloop,
					    DHCP_MAX, dhcp_discover, iface);
				else
					ipv4ll_start(iface);
				break;
			default:
				syslog(LOG_ERR,
				    "%s: unknown auto configuration option %d",
				    iface->name, tmp);
				break;
			}
		}
		return;
	}

	/* Ensure that all required options are present */
	for (i = 1; i < 255; i++) {
		if (has_option_mask(ifo->requiremask, i) &&
		    get_option_uint8(iface->ctx, &tmp, dhcp, (uint8_t)i) != 0)
		{
			/* If we are bootp, then ignore the need for serverid.
			 * To ignore bootp, require dhcp_message_type. */
			if (type == 0 && i == DHO_SERVERID)
				continue;
			log_dhcp(LOG_WARNING, "reject DHCP", iface, dhcp, from);
			return;
		}
	}

	/* Ensure that the address offered is valid */
	if ((type == 0 || type == DHCP_OFFER || type == DHCP_ACK) &&
	    (dhcp->ciaddr == INADDR_ANY || dhcp->ciaddr == INADDR_BROADCAST) &&
	    (dhcp->yiaddr == INADDR_ANY || dhcp->yiaddr == INADDR_BROADCAST))
	{
		log_dhcp(LOG_WARNING, "reject invalid address",
		    iface, dhcp, from);
		return;
	}

	if ((type == 0 || type == DHCP_OFFER) &&
	    state->state == DHS_DISCOVER)
	{
		lease->frominfo = 0;
		lease->addr.s_addr = dhcp->yiaddr;
		lease->cookie = dhcp->cookie;
		if (type == 0 ||
		    get_option_addr(iface->ctx,
		    &lease->server, dhcp, DHO_SERVERID) != 0)
			lease->server.s_addr = INADDR_ANY;
		log_dhcp(LOG_INFO, "offered", iface, dhcp, from);
		free(state->offer);
		state->offer = dhcp;
		*dhcpp = NULL;
		if (iface->ctx->options & DHCPCD_TEST) {
			free(state->old);
			state->old = state->new;
			state->new = state->offer;
			state->offer = NULL;
			state->reason = "TEST";
			script_runreason(iface, state->reason);
			eloop_exit(iface->ctx->eloop, EXIT_SUCCESS);
			return;
		}
		eloop_timeout_delete(iface->ctx->eloop, send_discover, iface);
		/* We don't request BOOTP addresses */
		if (type) {
			/* We used to ARP check here, but that seems to be in
			 * violation of RFC2131 where it only describes
			 * DECLINE after REQUEST.
			 * It also seems that some MS DHCP servers actually
			 * ignore DECLINE if no REQUEST, ie we decline a
			 * DISCOVER. */
			dhcp_request(iface);
			return;
		}
	}

	if (type) {
		if (type == DHCP_OFFER) {
			log_dhcp(LOG_WARNING, "ignoring offer of",
			    iface, dhcp, from);
			return;
		}

		/* We should only be dealing with acks */
		if (type != DHCP_ACK) {
			log_dhcp(LOG_ERR, "not ACK or OFFER",
			    iface, dhcp, from);
			return;
		}

		if (!(ifo->options & DHCPCD_INFORM))
			log_dhcp(LOG_DEBUG, "acknowledged", iface, dhcp, from);
		else
		    ifo->options &= ~DHCPCD_STATIC;
	}


	/* No NAK, so reset the backoff
	 * We don't reset on an OFFER message because the server could
	 * potentially NAK the REQUEST. */
	state->nakoff = 0;

	/* BOOTP could have already assigned this above, so check we still
	 * have a pointer. */
	if (*dhcpp) {
		free(state->offer);
		state->offer = dhcp;
		*dhcpp = NULL;
	}

	lease->frominfo = 0;
	eloop_timeout_delete(iface->ctx->eloop, NULL, iface);

	if (ifo->options & DHCPCD_ARP &&
	    state->addr.s_addr != state->offer->yiaddr)
	{
		/* If the interface already has the address configured
		 * then we can't ARP for duplicate detection. */
		addr.s_addr = state->offer->yiaddr;
		if (!ipv4_findaddr(iface, &addr, NULL)) {
			state->claims = 0;
			state->probes = 0;
			state->conflicts = 0;
			arp_probe(iface);
			return;
		}
	}

	dhcp_bind(iface);
}

static size_t
get_udp_data(const uint8_t **data, const uint8_t *udp)
{
	struct udp_dhcp_packet p;

	memcpy(&p, udp, sizeof(p));
	*data = udp + offsetof(struct udp_dhcp_packet, dhcp);
	return ntohs(p.ip.ip_len) - sizeof(p.ip) - sizeof(p.udp);
}

static int
valid_udp_packet(const uint8_t *data, size_t data_len, struct in_addr *from,
    int noudpcsum)
{
	struct udp_dhcp_packet p;
	uint16_t bytes, udpsum;

	if (data_len < sizeof(p.ip)) {
		if (from)
			from->s_addr = INADDR_ANY;
		errno = EINVAL;
		return -1;
	}
	memcpy(&p, data, MIN(data_len, sizeof(p)));
	if (from)
		from->s_addr = p.ip.ip_src.s_addr;
	if (data_len > sizeof(p)) {
		errno = EINVAL;
		return -1;
	}
	if (checksum(&p.ip, sizeof(p.ip)) != 0) {
		errno = EINVAL;
		return -1;
	}

	bytes = ntohs(p.ip.ip_len);
	if (data_len < bytes) {
		errno = EINVAL;
		return -1;
	}

	if (noudpcsum == 0) {
		udpsum = p.udp.uh_sum;
		p.udp.uh_sum = 0;
		p.ip.ip_hl = 0;
		p.ip.ip_v = 0;
		p.ip.ip_tos = 0;
		p.ip.ip_len = p.udp.uh_ulen;
		p.ip.ip_id = 0;
		p.ip.ip_off = 0;
		p.ip.ip_ttl = 0;
		p.ip.ip_sum = 0;
		if (udpsum && checksum(&p, bytes) != udpsum) {
			errno = EINVAL;
			return -1;
		}
	}

	return 0;
}

static void
dhcp_handlepacket(void *arg)
{
	struct interface *ifp = arg;
	struct dhcp_message *dhcp = NULL;
	const uint8_t *pp;
	size_t bytes;
	struct in_addr from;
	int i, flags;
	const struct dhcp_state *state = D_CSTATE(ifp);

	/* Need this API due to BPF */
	flags = 0;
	while (!(flags & RAW_EOF)) {
		bytes = (size_t)if_readrawpacket(ifp, ETHERTYPE_IP,
		    ifp->ctx->packet, udp_dhcp_len, &flags);
		if (bytes == 0 || (ssize_t)bytes == -1) {
			syslog(LOG_ERR, "%s: dhcp if_readrawpacket: %m",
			    ifp->name);
			dhcp_close(ifp);
			break;
		}
		if (valid_udp_packet(ifp->ctx->packet, bytes,
			&from, flags & RAW_PARTIALCSUM) == -1)
		{
			syslog(LOG_ERR, "%s: invalid UDP packet from %s",
			    ifp->name, inet_ntoa(from));
			continue;
		}
		i = whitelisted_ip(ifp->options, from.s_addr);
		if (i == 0) {
			syslog(LOG_WARNING,
			    "%s: non whitelisted DHCP packet from %s",
			    ifp->name, inet_ntoa(from));
			continue;
		} else if (i != 1 &&
		    blacklisted_ip(ifp->options, from.s_addr) == 1)
		{
			syslog(LOG_WARNING,
			    "%s: blacklisted DHCP packet from %s",
			    ifp->name, inet_ntoa(from));
			continue;
		}
		if (ifp->flags & IFF_POINTOPOINT &&
		    state->dst.s_addr != from.s_addr)
		{
			syslog(LOG_WARNING,
			    "%s: server %s is not destination",
			    ifp->name, inet_ntoa(from));
		}
		bytes = get_udp_data(&pp, ifp->ctx->packet);
		if (bytes > sizeof(*dhcp)) {
			syslog(LOG_ERR,
			    "%s: packet greater than DHCP size from %s",
			    ifp->name, inet_ntoa(from));
			continue;
		}
		if (dhcp == NULL) {
		        dhcp = calloc(1, sizeof(*dhcp));
			if (dhcp == NULL) {
				syslog(LOG_ERR, "%s: calloc: %m", __func__);
				break;
			}
		}
		memcpy(dhcp, pp, bytes);
		if (dhcp->cookie != htonl(MAGIC_COOKIE)) {
			syslog(LOG_DEBUG, "%s: bogus cookie from %s",
			    ifp->name, inet_ntoa(from));
			continue;
		}
		/* Ensure packet is for us */
		if (ifp->hwlen <= sizeof(dhcp->chaddr) &&
		    memcmp(dhcp->chaddr, ifp->hwaddr, ifp->hwlen))
		{
			char buf[sizeof(dhcp->chaddr) * 3];

			syslog(LOG_DEBUG, "%s: xid 0x%x is not for hwaddr %s",
			    ifp->name, ntohl(dhcp->xid),
			    hwaddr_ntoa(dhcp->chaddr, sizeof(dhcp->chaddr),
				buf, sizeof(buf)));
			continue;
		}
		dhcp_handledhcp(ifp, &dhcp, &from);
		if (state->raw_fd == -1)
			break;
	}
	free(dhcp);
}

static void
dhcp_handleudp(void *arg)
{
	struct dhcpcd_ctx *ctx;
	uint8_t buffer[sizeof(struct dhcp_message)];

	ctx = arg;

	/* Just read what's in the UDP fd and discard it as we always read
	 * from the raw fd */
	if (read(ctx->udp_fd, buffer, sizeof(buffer)) == -1) {
		syslog(LOG_ERR, "%s: %m", __func__);
		eloop_event_delete(ctx->eloop, ctx->udp_fd, 0);
		close(ctx->udp_fd);
		ctx->udp_fd = -1;
	}
}

static int
dhcp_open(struct interface *ifp)
{
	struct dhcp_state *state;

	if (ifp->ctx->packet == NULL) {
		ifp->ctx->packet = malloc(udp_dhcp_len);
		if (ifp->ctx->packet == NULL) {
			syslog(LOG_ERR, "%s: %m", __func__);
			return -1;
		}
	}

	state = D_STATE(ifp);
	if (state->raw_fd == -1) {
		state->raw_fd = if_openrawsocket(ifp, ETHERTYPE_IP);
		if (state->raw_fd == -1) {
			syslog(LOG_ERR, "%s: %s: %m", __func__, ifp->name);
			return -1;
		}
		eloop_event_add(ifp->ctx->eloop,
		    state->raw_fd, dhcp_handlepacket, ifp, NULL, NULL);
	}
	return 0;
}

int
dhcp_dump(struct interface *ifp)
{
	struct dhcp_state *state;

	ifp->if_data[IF_DATA_DHCP] = state = calloc(1, sizeof(*state));
	if (state == NULL)
		goto eexit;
	snprintf(state->leasefile, sizeof(state->leasefile),
	    LEASEFILE, ifp->name);
	state->new = read_lease(ifp);
	if (state->new == NULL && errno == ENOENT) {
		strlcpy(state->leasefile, ifp->name, sizeof(state->leasefile));
		state->new = read_lease(ifp);
	}
	if (state->new == NULL) {
		if (errno == ENOENT)
			syslog(LOG_ERR, "%s: no lease to dump", ifp->name);
		return -1;
	}
	state->reason = "DUMP";
	return script_runreason(ifp, state->reason);

eexit:
	syslog(LOG_ERR, "%s: %m", __func__);
	return -1;
}

void
dhcp_free(struct interface *ifp)
{
	struct dhcp_state *state = D_STATE(ifp);
	struct dhcpcd_ctx *ctx;

	if (state) {
		free(state->old);
		free(state->new);
		free(state->offer);
		free(state->buffer);
		free(state->clientid);
		free(state);
		ifp->if_data[IF_DATA_DHCP] = NULL;
	}

	ctx = ifp->ctx;
	/* If we don't have any more DHCP enabled interfaces,
	 * close the global socket and release resources */
	if (ctx->ifaces) {
		TAILQ_FOREACH(ifp, ctx->ifaces, next) {
			if (D_STATE(ifp))
				break;
		}
	}
	if (ifp == NULL) {
		if (ctx->udp_fd != -1) {
			eloop_event_delete(ctx->eloop, ctx->udp_fd, 0);
			close(ctx->udp_fd);
			ctx->udp_fd = -1;
		}

		free(ctx->packet);
		free(ctx->opt_buffer);
		ctx->packet = NULL;
		ctx->opt_buffer = NULL;
	}
}

static int
dhcp_init(struct interface *ifp)
{
	struct dhcp_state *state;
	const struct if_options *ifo;
	uint8_t len;
	char buf[(sizeof(ifo->clientid) - 1) * 3];

	state = D_STATE(ifp);
	if (state == NULL) {
		ifp->if_data[IF_DATA_DHCP] = calloc(1, sizeof(*state));
		state = D_STATE(ifp);
		if (state == NULL)
			return -1;
		/* 0 is a valid fd, so init to -1 */
		state->raw_fd = state->arp_fd = -1;
	}

	state->state = DHS_INIT;
	state->reason = "PREINIT";
	state->nakoff = 0;
	snprintf(state->leasefile, sizeof(state->leasefile),
	    LEASEFILE, ifp->name);

	ifo = ifp->options;
	/* We need to drop the leasefile so that dhcp_start
	 * doesn't load it. */
	if (ifo->options & DHCPCD_REQUEST)
		unlink(state->leasefile);

	free(state->clientid);
	state->clientid = NULL;

	if (*ifo->clientid) {
		state->clientid = malloc((size_t)(ifo->clientid[0] + 1));
		if (state->clientid == NULL)
			goto eexit;
		memcpy(state->clientid, ifo->clientid,
		    (size_t)(ifo->clientid[0]) + 1);
	} else if (ifo->options & DHCPCD_CLIENTID) {
		if (ifo->options & DHCPCD_DUID) {
			state->clientid = malloc(ifp->ctx->duid_len + 6);
			if (state->clientid == NULL)
				goto eexit;
			state->clientid[0] =(uint8_t)(ifp->ctx->duid_len + 5);
			state->clientid[1] = 255; /* RFC 4361 */
			memcpy(state->clientid + 2, ifo->iaid, 4);
			memcpy(state->clientid + 6, ifp->ctx->duid,
			    ifp->ctx->duid_len);
		} else {
			len = (uint8_t)(ifp->hwlen + 1);
			state->clientid = malloc((size_t)len + 1);
			if (state->clientid == NULL)
				goto eexit;
			state->clientid[0] = len;
			state->clientid[1] = (uint8_t)ifp->family;
			memcpy(state->clientid + 2, ifp->hwaddr,
			    ifp->hwlen);
		}
	}

	if (ifo->options & DHCPCD_DUID)
		/* Don't bother logging as DUID and IAID are reported
		 * at device start. */
		return 0;

	if (ifo->options & DHCPCD_CLIENTID)
		syslog(LOG_DEBUG, "%s: using ClientID %s", ifp->name,
		    hwaddr_ntoa(state->clientid + 1, state->clientid[0],
			buf, sizeof(buf)));
	else if (ifp->hwlen)
		syslog(LOG_DEBUG, "%s: using hwaddr %s", ifp->name,
		    hwaddr_ntoa(ifp->hwaddr, ifp->hwlen, buf, sizeof(buf)));
	return 0;

eexit:
	syslog(LOG_ERR, "%s: Error making ClientID: %m", __func__);
	return -1;
}

static void
dhcp_start1(void *arg)
{
	struct interface *ifp = arg;
	struct if_options *ifo = ifp->options;
	struct dhcp_state *state;
	struct stat st;
	struct timeval now;
	uint32_t l;
	int nolease;

	if (!(ifo->options & DHCPCD_IPV4))
		return;

	/* Listen on *.*.*.*:bootpc so that the kernel never sends an
	 * ICMP port unreachable message back to the DHCP server */
	if (ifp->ctx->udp_fd == -1) {
		ifp->ctx->udp_fd = dhcp_openudp(NULL);
		if (ifp->ctx->udp_fd == -1) {
			syslog(LOG_ERR, "dhcp_openudp: %m");
			return;
		}
		eloop_event_add(ifp->ctx->eloop,
		    ifp->ctx->udp_fd, dhcp_handleudp, ifp->ctx, NULL, NULL);
	}

	if (dhcp_init(ifp) == -1) {
		syslog(LOG_ERR, "%s: dhcp_init: %m", ifp->name);
		return;
	}

	/* Close any pre-existing sockets as we're starting over */
	dhcp_close(ifp);

	state = D_STATE(ifp);
	state->start_uptime = uptime();
	free(state->offer);
	state->offer = NULL;

	if (state->arping_index < ifo->arping_len) {
		arp_start(ifp);
		return;
	}

	if (ifo->options & DHCPCD_STATIC) {
		dhcp_static(ifp);
		return;
	}

	if (ifo->options & DHCPCD_DHCP && dhcp_open(ifp) == -1)
		return;

	if (ifo->options & DHCPCD_INFORM) {
		dhcp_inform(ifp);
		return;
	}
	if (ifp->hwlen == 0 && ifo->clientid[0] == '\0') {
		syslog(LOG_WARNING, "%s: needs a clientid to configure",
		    ifp->name);
		dhcp_drop(ifp, "FAIL");
		eloop_timeout_delete(ifp->ctx->eloop, NULL, ifp);
		return;
	}
	/* We don't want to read the old lease if we NAK an old test */
	nolease = state->offer && ifp->ctx->options & DHCPCD_TEST;
	if (!nolease)
		state->offer = read_lease(ifp);
	if (state->offer) {
		get_lease(ifp->ctx, &state->lease, state->offer);
		state->lease.frominfo = 1;
		if (state->offer->cookie == 0) {
			if (state->offer->yiaddr == state->addr.s_addr) {
				free(state->offer);
				state->offer = NULL;
			}
		} else if (state->lease.leasetime != ~0U &&
		    stat(state->leasefile, &st) == 0)
		{
			/* Offset lease times and check expiry */
			gettimeofday(&now, NULL);
			if ((time_t)state->lease.leasetime <
			    now.tv_sec - st.st_mtime)
			{
				syslog(LOG_DEBUG,
				    "%s: discarding expired lease",
				    ifp->name);
				free(state->offer);
				state->offer = NULL;
				state->lease.addr.s_addr = 0;
			} else {
				l = (uint32_t)(now.tv_sec - st.st_mtime);
				state->lease.leasetime -= l;
				state->lease.renewaltime -= l;
				state->lease.rebindtime -= l;
			}
		}
	}

	if (!(ifo->options & DHCPCD_DHCP)) {
		if (ifo->options & DHCPCD_IPV4LL) {
			if (state->offer && state->offer->cookie != 0) {
				free(state->offer);
				state->offer = NULL;
			}
			ipv4ll_start(ifp);
		}
		return;
	}

	if (state->offer == NULL)
		dhcp_discover(ifp);
	else if (state->offer->cookie == 0 && ifo->options & DHCPCD_IPV4LL)
		ipv4ll_start(ifp);
	else
		dhcp_reboot(ifp);
}

void
dhcp_start(struct interface *ifp)
{
	struct timeval tv;

	if (!(ifp->options->options & DHCPCD_IPV4))
		return;

	/* No point in delaying a static configuration */
	tv.tv_sec = DHCP_MIN_DELAY;
	tv.tv_usec = (suseconds_t)arc4random_uniform(
	    (DHCP_MAX_DELAY - DHCP_MIN_DELAY) * 1000000);
	timernorm(&tv);
	syslog(LOG_DEBUG,
	    "%s: delaying IPv4 for %0.1f seconds",
	    ifp->name, timeval_to_double(&tv));

	eloop_timeout_add_tv(ifp->ctx->eloop, &tv, dhcp_start1, ifp);
}

void
dhcp_handleifa(int type, struct interface *ifp,
	const struct in_addr *addr,
	const struct in_addr *net,
	const struct in_addr *dst)
{
	struct dhcp_state *state;
	struct if_options *ifo;
	uint8_t i;

	state = D_STATE(ifp);
	if (state == NULL)
		return;

	if (type == RTM_DELADDR) {
		if (state->new &&
		    (state->new->yiaddr == addr->s_addr ||
		    (state->new->yiaddr == INADDR_ANY &&
		     state->new->ciaddr == addr->s_addr)))
		{
			syslog(LOG_INFO, "%s: removing IP address %s/%d",
			    ifp->name, inet_ntoa(state->lease.addr),
			    inet_ntocidr(state->lease.net));
			dhcp_drop(ifp, "EXPIRE");
		}
		return;
	}

	if (type != RTM_NEWADDR)
		return;

	ifo = ifp->options;
	if (ifo->options & DHCPCD_INFORM) {
		if (state->state != DHS_INFORM)
			dhcp_inform(ifp);
		return;
	}

	if (!(ifo->options & DHCPCD_STATIC))
		return;
	if (ifo->req_addr.s_addr != INADDR_ANY)
		return;

	free(state->old);
	state->old = state->new;
	state->new = dhcp_message_new(addr, net);
	if (state->new == NULL)
		return;
	state->dst.s_addr = dst ? dst->s_addr : INADDR_ANY;
	if (dst) {
		for (i = 1; i < 255; i++)
			if (i != DHO_ROUTER && has_option_mask(ifo->dstmask,i))
				dhcp_message_add_addr(state->new, i, *dst);
	}
	state->reason = "STATIC";
	ipv4_buildroutes(ifp->ctx);
	script_runreason(ifp, state->reason);
	if (ifo->options & DHCPCD_INFORM) {
		state->state = DHS_INFORM;
		state->xid = dhcp_xid(ifp);
		state->lease.server.s_addr = dst ? dst->s_addr : INADDR_ANY;
		state->addr = *addr;
		state->net = *net;
		dhcp_inform(ifp);
	}
}
