/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * dhcpcd - DHCP client daemon
 * Copyright (c) 2006-2019 Roy Marples <roy@marples.name>
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

#ifdef AF_LINK
#  include <net/if_dl.h>
#endif

#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <syslog.h>

#define ELOOP_QUEUE	ELOOP_DHCP
#include "config.h"
#include "arp.h"
#include "bpf.h"
#include "common.h"
#include "dhcp.h"
#include "dhcpcd.h"
#include "dhcp-common.h"
#include "duid.h"
#include "eloop.h"
#include "if.h"
#include "ipv4.h"
#include "ipv4ll.h"
#include "logerr.h"
#include "sa.h"
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

/* Support older systems with different defines */
#if !defined(IP_RECVPKTINFO) && defined(IP_PKTINFO)
#define IP_RECVPKTINFO IP_PKTINFO
#endif

/* Assert the correct structure size for on wire */
__CTASSERT(sizeof(struct ip)		== 20);
__CTASSERT(sizeof(struct udphdr)	== 8);
__CTASSERT(sizeof(struct bootp)		== 300);

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
	{ DHCP_FORCERENEW, "FORCERENEW"},
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

static int dhcp_openbpf(struct interface *);
static void dhcp_start1(void *);
#if defined(ARP) && (!defined(KERNEL_RFC5227) || defined(ARPING))
static void dhcp_arp_found(struct arp_state *, const struct arp_msg *);
#endif
static void dhcp_handledhcp(struct interface *, struct bootp *, size_t,
    const struct in_addr *);
static void dhcp_handleifudp(void *);
static int dhcp_initstate(struct interface *);

void
dhcp_printoptions(const struct dhcpcd_ctx *ctx,
    const struct dhcp_opt *opts, size_t opts_len)
{
	const char * const *p;
	size_t i, j;
	const struct dhcp_opt *opt, *opt2;
	int cols;

	for (p = dhcp_params; *p; p++)
		printf("    %s\n", *p);

	for (i = 0, opt = ctx->dhcp_opts; i < ctx->dhcp_opts_len; i++, opt++) {
		for (j = 0, opt2 = opts; j < opts_len; j++, opt2++)
			if (opt->option == opt2->option)
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

static const uint8_t *
get_option(struct dhcpcd_ctx *ctx,
    const struct bootp *bootp, size_t bootp_len,
    unsigned int opt, size_t *opt_len)
{
	const uint8_t *p, *e;
	uint8_t l, o, ol, overl, *bp;
	const uint8_t *op;
	size_t bl;

	if (bootp == NULL || bootp_len < DHCP_MIN_LEN) {
		errno = EINVAL;
		return NULL;
	}

	/* Check we have the magic cookie */
	if (!IS_DHCP(bootp)) {
		errno = ENOTSUP;
		return NULL;
	}

	p = bootp->vend + 4; /* options after the 4 byte cookie */
	e = (const uint8_t *)bootp + bootp_len;
	ol = o = overl = 0;
	bp = NULL;
	op = NULL;
	bl = 0;
	while (p < e) {
		o = *p++;
		switch (o) {
		case DHO_PAD:
			/* No length to read */
			continue;
		case DHO_END:
			if (overl & 1) {
				/* bit 1 set means parse boot file */
				overl = (uint8_t)(overl & ~1);
				p = bootp->file;
				e = p + sizeof(bootp->file);
			} else if (overl & 2) {
				/* bit 2 set means parse server name */
				overl = (uint8_t)(overl & ~2);
				p = bootp->sname;
				e = p + sizeof(bootp->sname);
			} else
				goto exit;
			/* No length to read */
			continue;
		}

		/* Check we can read the length */
		if (p == e) {
			errno = EINVAL;
			return NULL;
		}
		l = *p++;

		/* Check we can read the option data, if present */
		if (p + l > e) {
			errno = EINVAL;
			return NULL;
		}

		if (o == DHO_OPTSOVERLOADED) {
			/* Ensure we only get this option once by setting
			 * the last bit as well as the value.
			 * This is valid because only the first two bits
			 * actually mean anything in RFC2132 Section 9.3 */
			if (l == 1 && !overl)
				overl = 0x80 | p[0];
		}

		if (o == opt) {
			if (op) {
				/* We must concatonate the options. */
				if (bl + l > ctx->opt_buffer_len) {
					size_t pos;
					uint8_t *nb;

					if (bp)
						pos = (size_t)
						    (bp - ctx->opt_buffer);
					else
						pos = 0;
					nb = realloc(ctx->opt_buffer, bl + l);
					if (nb == NULL)
						return NULL;
					ctx->opt_buffer = nb;
					ctx->opt_buffer_len = bl + l;
					bp = ctx->opt_buffer + pos;
				}
				if (bp == NULL)
					bp = ctx->opt_buffer;
				memcpy(bp, op, ol);
				bp += ol;
			}
			ol = l;
			op = p;
			bl += ol;
		}
		p += l;
	}

exit:
	if (opt_len)
		*opt_len = bl;
	if (bp) {
		memcpy(bp, op, ol);
		return (const uint8_t *)ctx->opt_buffer;
	}
	if (op)
		return op;
	errno = ENOENT;
	return NULL;
}

static int
get_option_addr(struct dhcpcd_ctx *ctx,
    struct in_addr *a, const struct bootp *bootp, size_t bootp_len,
    uint8_t option)
{
	const uint8_t *p;
	size_t len;

	p = get_option(ctx, bootp, bootp_len, option, &len);
	if (!p || len < (ssize_t)sizeof(a->s_addr))
		return -1;
	memcpy(&a->s_addr, p, sizeof(a->s_addr));
	return 0;
}

static int
get_option_uint32(struct dhcpcd_ctx *ctx,
    uint32_t *i, const struct bootp *bootp, size_t bootp_len, uint8_t option)
{
	const uint8_t *p;
	size_t len;
	uint32_t d;

	p = get_option(ctx, bootp, bootp_len, option, &len);
	if (!p || len < (ssize_t)sizeof(d))
		return -1;
	memcpy(&d, p, sizeof(d));
	if (i)
		*i = ntohl(d);
	return 0;
}

static int
get_option_uint16(struct dhcpcd_ctx *ctx,
    uint16_t *i, const struct bootp *bootp, size_t bootp_len, uint8_t option)
{
	const uint8_t *p;
	size_t len;
	uint16_t d;

	p = get_option(ctx, bootp, bootp_len, option, &len);
	if (!p || len < (ssize_t)sizeof(d))
		return -1;
	memcpy(&d, p, sizeof(d));
	if (i)
		*i = ntohs(d);
	return 0;
}

static int
get_option_uint8(struct dhcpcd_ctx *ctx,
    uint8_t *i, const struct bootp *bootp, size_t bootp_len, uint8_t option)
{
	const uint8_t *p;
	size_t len;

	p = get_option(ctx, bootp, bootp_len, option, &len);
	if (!p || len < (ssize_t)sizeof(*p))
		return -1;
	if (i)
		*i = *(p);
	return 0;
}

ssize_t
print_rfc3442(FILE *fp, const uint8_t *data, size_t data_len)
{
	const uint8_t *p = data, *e;
	size_t ocets;
	uint8_t cidr;
	struct in_addr addr;

	/* Minimum is 5 -first is CIDR and a router length of 4 */
	if (data_len < 5) {
		errno = EINVAL;
		return -1;
	}

	e = p + data_len;
	while (p < e) {
		if (p != data) {
			if (fputc(' ', fp) == EOF)
				return -1;
		}
		cidr = *p++;
		if (cidr > 32) {
			errno = EINVAL;
			return -1;
		}
		ocets = (size_t)(cidr + 7) / NBBY;
		if (p + 4 + ocets > e) {
			errno = ERANGE;
			return -1;
		}
		/* If we have ocets then we have a destination and netmask */
		addr.s_addr = 0;
		if (ocets > 0) {
			memcpy(&addr.s_addr, p, ocets);
			p += ocets;
		}
		if (fprintf(fp, "%s/%d", inet_ntoa(addr), cidr) == -1)
			return -1;

		/* Finally, snag the router */
		memcpy(&addr.s_addr, p, 4);
		p += 4;
		if (fprintf(fp, " %s", inet_ntoa(addr)) == -1)
			return -1;
	}

	if (fputc('\0', fp) == EOF)
		return -1;
	return 1;
}

static int
decode_rfc3442_rt(rb_tree_t *routes, struct interface *ifp,
    const uint8_t *data, size_t dl, const struct bootp *bootp)
{
	const uint8_t *p = data;
	const uint8_t *e;
	uint8_t cidr;
	size_t ocets;
	struct rt *rt = NULL;
	struct in_addr dest, netmask, gateway;
	int n;

	/* Minimum is 5 -first is CIDR and a router length of 4 */
	if (dl < 5) {
		errno = EINVAL;
		return -1;
	}

	n = 0;
	e = p + dl;
	while (p < e) {
		cidr = *p++;
		if (cidr > 32) {
			errno = EINVAL;
			return -1;
		}

		ocets = (size_t)(cidr + 7) / NBBY;
		if (p + 4 + ocets > e) {
			errno = ERANGE;
			return -1;
		}

		if ((rt = rt_new(ifp)) == NULL)
			return -1;

		/* If we have ocets then we have a destination and netmask */
		dest.s_addr = 0;
		if (ocets > 0) {
			memcpy(&dest.s_addr, p, ocets);
			p += ocets;
			netmask.s_addr = htonl(~0U << (32 - cidr));
		} else
			netmask.s_addr = 0;

		/* Finally, snag the router */
		memcpy(&gateway.s_addr, p, 4);
		p += 4;

		/* An on-link host route is normally set by having the
		 * gateway match the destination or assigned address */
		if (gateway.s_addr == dest.s_addr ||
		    (gateway.s_addr == bootp->yiaddr ||
		    gateway.s_addr == bootp->ciaddr))
		{
			gateway.s_addr = INADDR_ANY;
			netmask.s_addr = INADDR_BROADCAST;
		}
		if (netmask.s_addr == INADDR_BROADCAST)
			rt->rt_flags = RTF_HOST;

		sa_in_init(&rt->rt_dest, &dest);
		sa_in_init(&rt->rt_netmask, &netmask);
		sa_in_init(&rt->rt_gateway, &gateway);
		if (rt_proto_add(routes, rt))
			n = 1;
	}
	return n;
}

ssize_t
print_rfc3361(FILE *fp, const uint8_t *data, size_t dl)
{
	uint8_t enc;
	char sip[NS_MAXDNAME];
	struct in_addr addr;

	if (dl < 2) {
		errno = EINVAL;
		return 0;
	}

	enc = *data++;
	dl--;
	switch (enc) {
	case 0:
		if (decode_rfc1035(sip, sizeof(sip), data, dl) == -1)
			return -1;
		if (efprintf(fp, "%s", sip) == -1)
			return -1;
		break;
	case 1:
		if (dl % 4 != 0) {
			errno = EINVAL;
			break;
		}
		addr.s_addr = INADDR_BROADCAST;
		for (;
		    dl != 0;
		    data += sizeof(addr.s_addr), dl -= sizeof(addr.s_addr))
		{
			memcpy(&addr.s_addr, data, sizeof(addr.s_addr));
			if (fprintf(fp, "%s", inet_ntoa(addr)) == -1)
				return -1;
			if (dl != 0) {
				if (fputc(' ', fp) == EOF)
					return -1;
			}
		}
		if (fputc('\0', fp) == EOF)
			return -1;
		break;
	default:
		errno = EINVAL;
		return 0;
	}

	return 1;
}

static char *
get_option_string(struct dhcpcd_ctx *ctx,
    const struct bootp *bootp, size_t bootp_len, uint8_t option)
{
	size_t len;
	const uint8_t *p;
	char *s;

	p = get_option(ctx, bootp, bootp_len, option, &len);
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
static int
get_option_routes(rb_tree_t *routes, struct interface *ifp,
    const struct bootp *bootp, size_t bootp_len)
{
	struct if_options *ifo = ifp->options;
	const uint8_t *p;
	const uint8_t *e;
	struct rt *rt = NULL;
	struct in_addr dest, netmask, gateway;
	size_t len;
	const char *csr = "";
	int n;

	/* If we have CSR's then we MUST use these only */
	if (!has_option_mask(ifo->nomask, DHO_CSR))
		p = get_option(ifp->ctx, bootp, bootp_len, DHO_CSR, &len);
	else
		p = NULL;
	/* Check for crappy MS option */
	if (!p && !has_option_mask(ifo->nomask, DHO_MSCSR)) {
		p = get_option(ifp->ctx, bootp, bootp_len, DHO_MSCSR, &len);
		if (p)
			csr = "MS ";
	}
	if (p && (n = decode_rfc3442_rt(routes, ifp, p, len, bootp)) != -1) {
		const struct dhcp_state *state;

		state = D_CSTATE(ifp);
		if (!(ifo->options & DHCPCD_CSR_WARNED) &&
		    !(state->added & STATE_FAKE))
		{
			logdebugx("%s: using %sClassless Static Routes",
			    ifp->name, csr);
			ifo->options |= DHCPCD_CSR_WARNED;
		}
		return n;
	}

	n = 0;
	/* OK, get our static routes first. */
	if (!has_option_mask(ifo->nomask, DHO_STATICROUTE))
		p = get_option(ifp->ctx, bootp, bootp_len,
		    DHO_STATICROUTE, &len);
	else
		p = NULL;
	/* RFC 2131 Section 5.8 states length MUST be in multiples of 8 */
	if (p && len % 8 == 0) {
		e = p + len;
		while (p < e) {
			memcpy(&dest.s_addr, p, sizeof(dest.s_addr));
			p += 4;
			memcpy(&gateway.s_addr, p, sizeof(gateway.s_addr));
			p += 4;
			/* RFC 2131 Section 5.8 states default route is
			 * illegal */
			if (gateway.s_addr == INADDR_ANY)
				continue;
			if ((rt = rt_new(ifp)) == NULL)
				return -1;

			/* A on-link host route is normally set by having the
			 * gateway match the destination or assigned address */
			if (gateway.s_addr == dest.s_addr ||
			     (gateway.s_addr == bootp->yiaddr ||
			      gateway.s_addr == bootp->ciaddr))
			{
				gateway.s_addr = INADDR_ANY;
				netmask.s_addr = INADDR_BROADCAST;
			} else
				netmask.s_addr = route_netmask(dest.s_addr);
			if (netmask.s_addr == INADDR_BROADCAST)
				rt->rt_flags = RTF_HOST;

			sa_in_init(&rt->rt_dest, &dest);
			sa_in_init(&rt->rt_netmask, &netmask);
			sa_in_init(&rt->rt_gateway, &gateway);
			if (rt_proto_add(routes, rt))
				n++;
		}
	}

	/* Now grab our routers */
	if (!has_option_mask(ifo->nomask, DHO_ROUTER))
		p = get_option(ifp->ctx, bootp, bootp_len, DHO_ROUTER, &len);
	else
		p = NULL;
	if (p && len % 4 == 0) {
		e = p + len;
		dest.s_addr = INADDR_ANY;
		netmask.s_addr = INADDR_ANY;
		while (p < e) {
			if ((rt = rt_new(ifp)) == NULL)
				return -1;
			memcpy(&gateway.s_addr, p, sizeof(gateway.s_addr));
			p += 4;
			sa_in_init(&rt->rt_dest, &dest);
			sa_in_init(&rt->rt_netmask, &netmask);
			sa_in_init(&rt->rt_gateway, &gateway);
			if (rt_proto_add(routes, rt))
				n++;
		}
	}

	return n;
}

uint16_t
dhcp_get_mtu(const struct interface *ifp)
{
	const struct dhcp_state *state;
	uint16_t mtu;

	if (ifp->options->mtu)
		return (uint16_t)ifp->options->mtu;
	mtu = 0; /* bogus gcc warning */
	if ((state = D_CSTATE(ifp)) == NULL ||
	    has_option_mask(ifp->options->nomask, DHO_MTU) ||
	    get_option_uint16(ifp->ctx, &mtu,
			      state->new, state->new_len, DHO_MTU) == -1)
		return 0;
	return mtu;
}

/* Grab our routers from the DHCP message and apply any MTU value
 * the message contains */
int
dhcp_get_routes(rb_tree_t *routes, struct interface *ifp)
{
	const struct dhcp_state *state;

	if ((state = D_CSTATE(ifp)) == NULL || !(state->added & STATE_ADDED))
		return 0;
	return get_option_routes(routes, ifp, state->new, state->new_len);
}

/* Assumes DHCP options */
static int
dhcp_message_add_addr(struct bootp *bootp,
    uint8_t type, struct in_addr addr)
{
	uint8_t *p;
	size_t len;

	p = bootp->vend;
	while (*p != DHO_END) {
		p++;
		p += *p + 1;
	}

	len = (size_t)(p - bootp->vend);
	if (len + 6 > sizeof(bootp->vend)) {
		errno = ENOMEM;
		return -1;
	}

	*p++ = type;
	*p++ = 4;
	memcpy(p, &addr.s_addr, 4);
	p += 4;
	*p = DHO_END;
	return 0;
}

static ssize_t
make_message(struct bootp **bootpm, const struct interface *ifp, uint8_t type)
{
	struct bootp *bootp;
	uint8_t *lp, *p, *e;
	uint8_t *n_params = NULL;
	uint32_t ul;
	uint16_t sz;
	size_t len, i;
	const struct dhcp_opt *opt;
	struct if_options *ifo = ifp->options;
	const struct dhcp_state *state = D_CSTATE(ifp);
	const struct dhcp_lease *lease = &state->lease;
	char hbuf[HOSTNAME_MAX_LEN + 1];
	const char *hostname;
	const struct vivco *vivco;
	int mtu;
#ifdef AUTH
	uint8_t *auth, auth_len;
#endif

	if ((mtu = if_getmtu(ifp)) == -1)
		logerr("%s: if_getmtu", ifp->name);
	else if (mtu < MTU_MIN) {
		if (if_setmtu(ifp, MTU_MIN) == -1)
			logerr("%s: if_setmtu", ifp->name);
		mtu = MTU_MIN;
	}

	if (ifo->options & DHCPCD_BOOTP)
		bootp = calloc(1, sizeof (*bootp));
	else
		/* Make the maximal message we could send */
		bootp = calloc(1, (size_t)(mtu - IP_UDP_SIZE));

	if (bootp == NULL)
		return -1;
	*bootpm = bootp;

	if (state->addr != NULL &&
	    (type == DHCP_INFORM || type == DHCP_RELEASE ||
	    (type == DHCP_REQUEST &&
	    state->addr->mask.s_addr == lease->mask.s_addr &&
	    (state->new == NULL || IS_DHCP(state->new)) &&
	    !(state->added & STATE_FAKE))))
		bootp->ciaddr = state->addr->addr.s_addr;

	bootp->op = BOOTREQUEST;
	bootp->htype = (uint8_t)ifp->family;
	switch (ifp->family) {
	case ARPHRD_ETHER:
	case ARPHRD_IEEE802:
		bootp->hlen = (uint8_t)ifp->hwlen;
		memcpy(&bootp->chaddr, &ifp->hwaddr, ifp->hwlen);
		break;
	}

	if (ifo->options & DHCPCD_BROADCAST &&
	    bootp->ciaddr == 0 &&
	    type != DHCP_DECLINE &&
	    type != DHCP_RELEASE)
		bootp->flags = htons(BROADCAST_FLAG);

	if (type != DHCP_DECLINE && type != DHCP_RELEASE) {
		struct timespec tv;

		clock_gettime(CLOCK_MONOTONIC, &tv);
		timespecsub(&tv, &state->started, &tv);
		if (tv.tv_sec < 0 || tv.tv_sec > (time_t)UINT16_MAX)
			bootp->secs = htons((uint16_t)UINT16_MAX);
		else
			bootp->secs = htons((uint16_t)tv.tv_sec);
	}

	bootp->xid = htonl(state->xid);

	if (ifo->options & DHCPCD_BOOTP)
		return sizeof(*bootp);

	p = bootp->vend;
	e = (uint8_t *)bootp + (mtu - IP_UDP_SIZE) - 1; /* -1 for DHO_END */

	ul = htonl(MAGIC_COOKIE);
	memcpy(p, &ul, sizeof(ul));
	p += sizeof(ul);

	*p++ = DHO_MESSAGETYPE;
	*p++ = 1;
	*p++ = type;

#define AREA_LEFT	(size_t)(e - p)
#define AREA_FIT(s)	if ((s) > AREA_LEFT) goto toobig
#define AREA_CHECK(s)	if ((s) + 2UL > AREA_LEFT) goto toobig
#define PUT_ADDR(o, a)	do {		\
	AREA_CHECK(4);			\
	*p++ = (o);			\
	*p++ = 4;			\
	memcpy(p, &(a)->s_addr, 4);	\
	p += 4;				\
} while (0 /* CONSTCOND */)

	if (state->clientid) {
		AREA_CHECK(state->clientid[0]);
		*p++ = DHO_CLIENTID;
		memcpy(p, state->clientid, (size_t)state->clientid[0] + 1);
		p += state->clientid[0] + 1;
	}

	if (lease->addr.s_addr && lease->cookie == htonl(MAGIC_COOKIE)) {
		if (type == DHCP_DECLINE ||
		    (type == DHCP_REQUEST &&
		    (state->addr == NULL ||
		    state->added & STATE_FAKE ||
		    lease->addr.s_addr != state->addr->addr.s_addr)))
		{
			PUT_ADDR(DHO_IPADDRESS, &lease->addr);
			if (lease->server.s_addr)
				PUT_ADDR(DHO_SERVERID, &lease->server);
		}

		if (type == DHCP_RELEASE) {
			if (lease->server.s_addr)
				PUT_ADDR(DHO_SERVERID, &lease->server);
		}
	}

	if (type == DHCP_DECLINE) {
		len = strlen(DAD);
		if (len > AREA_LEFT) {
			*p++ = DHO_MESSAGE;
			*p++ = (uint8_t)len;
			memcpy(p, DAD, len);
			p += len;
		}
	}

	if (type == DHCP_DISCOVER &&
	    !(ifp->ctx->options & DHCPCD_TEST) &&
	    has_option_mask(ifo->requestmask, DHO_RAPIDCOMMIT))
	{
		/* RFC 4039 Section 3 */
		AREA_CHECK(0);
		*p++ = DHO_RAPIDCOMMIT;
		*p++ = 0;
	}

	if (type == DHCP_DISCOVER && ifo->options & DHCPCD_REQUEST)
		PUT_ADDR(DHO_IPADDRESS, &ifo->req_addr);

	/* RFC 2563 Auto Configure */
	if (type == DHCP_DISCOVER && ifo->options & DHCPCD_IPV4LL) {
		AREA_CHECK(1);
		*p++ = DHO_AUTOCONFIGURE;
		*p++ = 1;
		*p++ = 1;
	}

	if (type == DHCP_DISCOVER ||
	    type == DHCP_INFORM ||
	    type == DHCP_REQUEST)
	{
		if (mtu != -1) {
			AREA_CHECK(2);
			*p++ = DHO_MAXMESSAGESIZE;
			*p++ = 2;
			sz = htons((uint16_t)(mtu - IP_UDP_SIZE));
			memcpy(p, &sz, 2);
			p += 2;
		}

		if (ifo->userclass[0]) {
			AREA_CHECK(ifo->userclass[0]);
			*p++ = DHO_USERCLASS;
			memcpy(p, ifo->userclass,
			    (size_t)ifo->userclass[0] + 1);
			p += ifo->userclass[0] + 1;
		}

		if (ifo->vendorclassid[0]) {
			AREA_CHECK(ifo->vendorclassid[0]);
			*p++ = DHO_VENDORCLASSID;
			memcpy(p, ifo->vendorclassid,
			    (size_t)ifo->vendorclassid[0] + 1);
			p += ifo->vendorclassid[0] + 1;
		}

		if (ifo->mudurl[0]) {
		       AREA_CHECK(ifo->mudurl[0]);
		       *p++ = DHO_MUDURL;
		       memcpy(p, ifo->mudurl, (size_t)ifo->mudurl[0] + 1);
		       p += ifo->mudurl[0] + 1;
		}

		if (type != DHCP_INFORM) {
			if (ifo->leasetime != 0) {
				AREA_CHECK(4);
				*p++ = DHO_LEASETIME;
				*p++ = 4;
				ul = htonl(ifo->leasetime);
				memcpy(p, &ul, 4);
				p += 4;
			}
		}

		hostname = dhcp_get_hostname(hbuf, sizeof(hbuf), ifo);

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
			i = 3;
			if (hostname)
				i += encode_rfc1035(hostname, NULL);
			AREA_CHECK(i);
			*p++ = DHO_FQDN;
			*p++ = (uint8_t)i;
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
				*p++ = (uint8_t)((ifo->fqdn & 0x09) | 0x04);
			else
				*p++ = (FQDN_NONE & 0x09) | 0x04;
			*p++ = 0; /* from server for PTR RR */
			*p++ = 0; /* from server for A RR if S=1 */
			if (hostname) {
				i = encode_rfc1035(hostname, p);
				p += i;
			}
		} else if (ifo->options & DHCPCD_HOSTNAME && hostname) {
			len = strlen(hostname);
			AREA_CHECK(len);
			*p++ = DHO_HOSTNAME;
			*p++ = (uint8_t)len;
			memcpy(p, hostname, len);
			p += len;
		}

		/* vendor is already encoded correctly, so just add it */
		if (ifo->vendor[0]) {
			AREA_CHECK(ifo->vendor[0]);
			*p++ = DHO_VENDOR;
			memcpy(p, ifo->vendor, (size_t)ifo->vendor[0] + 1);
			p += ifo->vendor[0] + 1;
		}

#ifdef AUTH
		if ((ifo->auth.options & DHCPCD_AUTH_SENDREQUIRE) !=
		    DHCPCD_AUTH_SENDREQUIRE &&
		    !has_option_mask(ifo->nomask, DHO_FORCERENEW_NONCE))
		{
			/* We support HMAC-MD5 */
			AREA_CHECK(1);
			*p++ = DHO_FORCERENEW_NONCE;
			*p++ = 1;
			*p++ = AUTH_ALG_HMAC_MD5;
		}
#endif

		if (ifo->vivco_len) {
			AREA_CHECK(sizeof(ul));
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
				AREA_FIT(vivco->len);
				if (vivco->len + 2 + *lp > 255) {
					logerrx("%s: VIVCO option too big",
					    ifp->name);
					free(bootp);
					return -1;
				}
				*p++ = (uint8_t)vivco->len;
				memcpy(p, vivco->data, vivco->len);
				p += vivco->len;
				*lp = (uint8_t)(*lp + vivco->len + 1);
			}
		}

		AREA_CHECK(0);
		*p++ = DHO_PARAMETERREQUESTLIST;
		n_params = p;
		*p++ = 0;
		for (i = 0, opt = ifp->ctx->dhcp_opts;
		    i < ifp->ctx->dhcp_opts_len;
		    i++, opt++)
		{
			if (!DHC_REQOPT(opt, ifo->requestmask, ifo->nomask))
				continue;
			if (type == DHCP_INFORM &&
			    (opt->option == DHO_RENEWALTIME ||
				opt->option == DHO_REBINDTIME))
				continue;
			AREA_FIT(1);
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
			if (!DHC_REQOPT(opt, ifo->requestmask, ifo->nomask))
				continue;
			if (type == DHCP_INFORM &&
			    (opt->option == DHO_RENEWALTIME ||
				opt->option == DHO_REBINDTIME))
				continue;
			AREA_FIT(1);
			*p++ = (uint8_t)opt->option;
		}
		*n_params = (uint8_t)(p - n_params - 1);
	}

#ifdef AUTH
	auth = NULL;	/* appease GCC */
	auth_len = 0;
	if (ifo->auth.options & DHCPCD_AUTH_SEND) {
		ssize_t alen = dhcp_auth_encode(&ifo->auth,
		    state->auth.token,
		    NULL, 0, 4, type, NULL, 0);
		if (alen != -1 && alen > UINT8_MAX) {
			errno = ERANGE;
			alen = -1;
		}
		if (alen == -1)
			logerr("%s: dhcp_auth_encode", ifp->name);
		else if (alen != 0) {
			auth_len = (uint8_t)alen;
			AREA_CHECK(auth_len);
			*p++ = DHO_AUTHENTICATION;
			*p++ = auth_len;
			auth = p;
			p += auth_len;
		}
	}
#endif

	*p++ = DHO_END;
	len = (size_t)(p - (uint8_t *)bootp);

	/* Pad out to the BOOTP message length.
	 * Even if we send a DHCP packet with a variable length vendor area,
	 * some servers / relay agents don't like packets smaller than
	 * a BOOTP message which is fine because that's stipulated
	 * in RFC1542 section 2.1. */
	while (len < sizeof(*bootp)) {
		*p++ = DHO_PAD;
		len++;
	}

#ifdef AUTH
	if (ifo->auth.options & DHCPCD_AUTH_SEND && auth_len != 0)
		dhcp_auth_encode(&ifo->auth, state->auth.token,
		    (uint8_t *)bootp, len, 4, type, auth, auth_len);
#endif

	return (ssize_t)len;

toobig:
	logerrx("%s: DHCP message too big", ifp->name);
	free(bootp);
	return -1;
}

static ssize_t
write_lease(const struct interface *ifp, const struct bootp *bootp, size_t len)
{
	int fd;
	ssize_t bytes;
	const struct dhcp_state *state = D_CSTATE(ifp);

	logdebugx("%s: writing lease `%s'", ifp->name, state->leasefile);

	fd = open(state->leasefile, O_WRONLY | O_CREAT | O_TRUNC, 0644);
	if (fd == -1)
		return -1;
	bytes = write(fd, bootp, len);
	close(fd);
	return bytes;
}

static size_t
read_lease(struct interface *ifp, struct bootp **bootp)
{
	int fd;
	bool fd_opened;
	struct dhcp_state *state = D_STATE(ifp);
	struct bootp *lease;
	size_t bytes;
	uint8_t type;
#ifdef AUTH
	const uint8_t *auth;
	size_t auth_len;
#endif

	/* Safety */
	*bootp = NULL;

	if (state->leasefile[0] == '\0') {
		fd = fileno(stdin);
		fd_opened = false;
	} else {
		fd = open(state->leasefile, O_RDONLY);
		fd_opened = true;
	}
	if (fd == -1) {
		if (errno != ENOENT)
			logerr("%s: open `%s'",
			    ifp->name, state->leasefile);
		return 0;
	}
	if (state->leasefile[0] == '\0')
		logdebugx("reading standard input");
	else
		logdebugx("%s: reading lease `%s'",
		    ifp->name, state->leasefile);

	bytes = dhcp_read_lease_fd(fd, (void **)&lease);
	if (fd_opened)
		close(fd);
	if (bytes == 0)
		return 0;

	/* Ensure the packet is at lease BOOTP sized
	 * with a vendor area of 4 octets
	 * (it should be more, and our read packet enforces this so this
	 * code should not be needed, but of course people could
	 * scribble whatever in the stored lease file. */
	if (bytes < DHCP_MIN_LEN) {
		free(lease);
		logerrx("%s: %s: truncated lease", ifp->name, __func__);
		return 0;
	}

	if (ifp->ctx->options & DHCPCD_DUMPLEASE)
		goto out;

	/* We may have found a BOOTP server */
	if (get_option_uint8(ifp->ctx, &type, (struct bootp *)lease, bytes,
	    DHO_MESSAGETYPE) == -1)
		type = 0;

#ifdef AUTH
	/* Authenticate the message */
	auth = get_option(ifp->ctx, (struct bootp *)lease, bytes,
	    DHO_AUTHENTICATION, &auth_len);
	if (auth) {
		if (dhcp_auth_validate(&state->auth, &ifp->options->auth,
		    lease, bytes, 4, type, auth, auth_len) == NULL)
		{
			logerr("%s: authentication failed", ifp->name);
			free(lease);
			return 0;
		}
		if (state->auth.token)
			logdebugx("%s: validated using 0x%08" PRIu32,
			    ifp->name, state->auth.token->secretid);
		else
			logdebugx("%s: accepted reconfigure key", ifp->name);
	} else if ((ifp->options->auth.options & DHCPCD_AUTH_SENDREQUIRE) ==
	    DHCPCD_AUTH_SENDREQUIRE)
	{
		logerrx("%s: authentication now required", ifp->name);
		free(lease);
		return 0;
	}
#endif

out:
	*bootp = (struct bootp *)lease;
	return bytes;
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
		if (*len > ol - *os) {
			errno = ERANGE;
			return NULL;
		}
	}

	*oopt = NULL;
	for (i = 0, opt = ctx->dhcp_opts; i < ctx->dhcp_opts_len; i++, opt++) {
		if (opt->option == *code) {
			*oopt = opt;
			break;
		}
	}

	return od;
}

ssize_t
dhcp_env(FILE *fenv, const char *prefix, const struct interface *ifp,
    const struct bootp *bootp, size_t bootp_len)
{
	const struct if_options *ifo;
	const uint8_t *p;
	struct in_addr addr;
	struct in_addr net;
	struct in_addr brd;
	struct dhcp_opt *opt, *vo;
	size_t i, pl;
	char safe[(BOOTP_FILE_LEN * 4) + 1];
	uint8_t overl = 0;
	uint32_t en;

	ifo = ifp->options;
	if (get_option_uint8(ifp->ctx, &overl, bootp, bootp_len,
	    DHO_OPTSOVERLOADED) == -1)
		overl = 0;

	if (bootp->yiaddr || bootp->ciaddr) {
		/* Set some useful variables that we derive from the DHCP
		 * message but are not necessarily in the options */
		addr.s_addr = bootp->yiaddr ? bootp->yiaddr : bootp->ciaddr;
		if (efprintf(fenv, "%s_ip_address=%s",
		    prefix, inet_ntoa(addr)) == -1)
			return -1;
		if (get_option_addr(ifp->ctx, &net,
		    bootp, bootp_len, DHO_SUBNETMASK) == -1) {
			net.s_addr = ipv4_getnetmask(addr.s_addr);
			if (efprintf(fenv, "%s_subnet_mask=%s",
			    prefix, inet_ntoa(net)) == -1)
				return -1;
		}
		if (efprintf(fenv, "%s_subnet_cidr=%d",
		    prefix, inet_ntocidr(net))== -1)
			return -1;
		if (get_option_addr(ifp->ctx, &brd,
		    bootp, bootp_len, DHO_BROADCAST) == -1)
		{
			brd.s_addr = addr.s_addr | ~net.s_addr;
			if (efprintf(fenv, "%s_broadcast_address=%s",
			    prefix, inet_ntoa(brd)) == -1)
				return -1;
		}
		addr.s_addr = bootp->yiaddr & net.s_addr;
		if (efprintf(fenv, "%s_network_number=%s",
		    prefix, inet_ntoa(addr)) == -1)
			return -1;
	}

	if (*bootp->file && !(overl & 1)) {
		print_string(safe, sizeof(safe), OT_STRING,
		    bootp->file, sizeof(bootp->file));
		if (efprintf(fenv, "%s_filename=%s", prefix, safe) == -1)
			return -1;
	}
	if (*bootp->sname && !(overl & 2)) {
		print_string(safe, sizeof(safe), OT_STRING | OT_DOMAIN,
		    bootp->sname, sizeof(bootp->sname));
		if (efprintf(fenv, "%s_server_name=%s", prefix, safe) == -1)
			return -1;
	}

	/* Zero our indexes */
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

	for (i = 0, opt = ifp->ctx->dhcp_opts;
	    i < ifp->ctx->dhcp_opts_len;
	    i++, opt++)
	{
		if (has_option_mask(ifo->nomask, opt->option))
			continue;
		if (dhcp_getoverride(ifo, opt->option))
			continue;
		p = get_option(ifp->ctx, bootp, bootp_len, opt->option, &pl);
		if (p == NULL)
			continue;
		dhcp_envoption(ifp->ctx, fenv, prefix, ifp->name,
		    opt, dhcp_getoption, p, pl);

		if (opt->option != DHO_VIVSO || pl <= (int)sizeof(uint32_t))
			continue;
		memcpy(&en, p, sizeof(en));
		en = ntohl(en);
		vo = vivso_find(en, ifp);
		if (vo == NULL)
			continue;
		/* Skip over en + total size */
		p += sizeof(en) + 1;
		pl -= sizeof(en) + 1;
		dhcp_envoption(ifp->ctx, fenv, prefix, ifp->name,
		    vo, dhcp_getoption, p, pl);
	}

	for (i = 0, opt = ifo->dhcp_override;
	    i < ifo->dhcp_override_len;
	    i++, opt++)
	{
		if (has_option_mask(ifo->nomask, opt->option))
			continue;
		p = get_option(ifp->ctx, bootp, bootp_len, opt->option, &pl);
		if (p == NULL)
			continue;
		dhcp_envoption(ifp->ctx, fenv, prefix, ifp->name,
		    opt, dhcp_getoption, p, pl);
	}

	return 1;
}

static void
get_lease(struct interface *ifp,
    struct dhcp_lease *lease, const struct bootp *bootp, size_t len)
{
	struct dhcpcd_ctx *ctx;

	assert(bootp != NULL);

	memcpy(&lease->cookie, bootp->vend, sizeof(lease->cookie));
	/* BOOTP does not set yiaddr for replies when ciaddr is set. */
	lease->addr.s_addr = bootp->yiaddr ? bootp->yiaddr : bootp->ciaddr;
	ctx = ifp->ctx;
	if (ifp->options->options & (DHCPCD_STATIC | DHCPCD_INFORM)) {
		if (ifp->options->req_addr.s_addr != INADDR_ANY) {
			lease->mask = ifp->options->req_mask;
			if (ifp->options->req_brd.s_addr != INADDR_ANY)
				lease->brd = ifp->options->req_brd;
			else
				lease->brd.s_addr =
				    lease->addr.s_addr | ~lease->mask.s_addr;
		} else {
			const struct ipv4_addr *ia;

			ia = ipv4_iffindaddr(ifp, &lease->addr, NULL);
			assert(ia != NULL);
			lease->mask = ia->mask;
			lease->brd = ia->brd;
		}
	} else {
		if (get_option_addr(ctx, &lease->mask, bootp, len,
		    DHO_SUBNETMASK) == -1)
			lease->mask.s_addr =
			    ipv4_getnetmask(lease->addr.s_addr);
		if (get_option_addr(ctx, &lease->brd, bootp, len,
		    DHO_BROADCAST) == -1)
			lease->brd.s_addr =
			    lease->addr.s_addr | ~lease->mask.s_addr;
	}
	if (get_option_uint32(ctx, &lease->leasetime,
	    bootp, len, DHO_LEASETIME) != 0)
		lease->leasetime = DHCP_INFINITE_LIFETIME;
	if (get_option_uint32(ctx, &lease->renewaltime,
	    bootp, len, DHO_RENEWALTIME) != 0)
		lease->renewaltime = 0;
	if (get_option_uint32(ctx, &lease->rebindtime,
	    bootp, len, DHO_REBINDTIME) != 0)
		lease->rebindtime = 0;
	if (get_option_addr(ctx, &lease->server, bootp, len, DHO_SERVERID) != 0)
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

static void
dhcp_new_xid(struct interface *ifp)
{
	struct dhcp_state *state;
	const struct interface *ifp1;
	const struct dhcp_state *state1;

	state = D_STATE(ifp);
	if (ifp->options->options & DHCPCD_XID_HWADDR &&
	    ifp->hwlen >= sizeof(state->xid))
		/* The lower bits are probably more unique on the network */
		memcpy(&state->xid,
		    (ifp->hwaddr + ifp->hwlen) - sizeof(state->xid),
		    sizeof(state->xid));
	else {
again:
		state->xid = arc4random();
	}

	/* Ensure it's unique */
	TAILQ_FOREACH(ifp1, ifp->ctx->ifaces, next) {
		if (ifp == ifp1)
			continue;
		if ((state1 = D_CSTATE(ifp1)) == NULL)
			continue;
		if (state1->xid == state->xid)
			break;
	}
	if (ifp1 != NULL) {
		if (ifp->options->options & DHCPCD_XID_HWADDR &&
		    ifp->hwlen >= sizeof(state->xid))
		{
			logerrx("%s: duplicate xid on %s",
			    ifp->name, ifp1->name);
			    return;
		}
		goto again;
	}

	/* We can't do this when sharing leases across interfaes */
#if 0
	/* As the XID changes, re-apply the filter. */
	if (state->bpf_fd != -1) {
		if (bpf_bootp(ifp, state->bpf_fd) == -1)
			logerr(__func__); /* try to continue */
	}
#endif
}

void
dhcp_close(struct interface *ifp)
{
	struct dhcp_state *state = D_STATE(ifp);

	if (state == NULL)
		return;

	if (state->bpf_fd != -1) {
		eloop_event_delete(ifp->ctx->eloop, state->bpf_fd);
		bpf_close(ifp, state->bpf_fd);
		state->bpf_fd = -1;
		state->bpf_flags |= BPF_EOF;
	}
	if (state->udp_fd != -1) {
		eloop_event_delete(ifp->ctx->eloop, state->udp_fd);
		close(state->udp_fd);
		state->udp_fd = -1;
	}

	state->interval = 0;
}

static int
dhcp_openudp(struct in_addr *ia)
{
	int s;
	struct sockaddr_in sin;
	int n;

	if ((s = xsocket(PF_INET, SOCK_DGRAM|SOCK_CLOEXEC, IPPROTO_UDP)) == -1)
		return -1;

	n = 1;
	if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &n, sizeof(n)) == -1)
		goto errexit;
#ifdef IP_RECVIF
	if (setsockopt(s, IPPROTO_IP, IP_RECVIF, &n, sizeof(n)) == -1)
		goto errexit;
#else
	if (setsockopt(s, IPPROTO_IP, IP_RECVPKTINFO, &n, sizeof(n)) == -1)
		goto errexit;
#endif
	memset(&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_port = htons(BOOTPC);
	if (ia != NULL)
		sin.sin_addr = *ia;
	if (bind(s, (struct sockaddr *)&sin, sizeof(sin)) == -1)
		goto errexit;

	return s;

errexit:
	close(s);
	return -1;
}

static uint16_t
in_cksum(const void *data, size_t len, uint32_t *isum)
{
	const uint16_t *word = data;
	uint32_t sum = isum != NULL ? *isum : 0;

	for (; len > 1; len -= sizeof(*word))
		sum += *word++;

	if (len == 1)
		sum += htons((uint16_t)(*(const uint8_t *)word << 8));

	if (isum != NULL)
		*isum = sum;

	sum = (sum >> 16) + (sum & 0xffff);
	sum += (sum >> 16);

	return (uint16_t)~sum;
}

static struct bootp_pkt *
dhcp_makeudppacket(size_t *sz, const uint8_t *data, size_t length,
	struct in_addr source, struct in_addr dest)
{
	struct bootp_pkt *udpp;
	struct ip *ip;
	struct udphdr *udp;

	if ((udpp = calloc(1, sizeof(*ip) + sizeof(*udp) + length)) == NULL)
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

	memcpy(&udpp->bootp, data, length);

	ip->ip_p = IPPROTO_UDP;
	ip->ip_src.s_addr = source.s_addr;
	if (dest.s_addr == 0)
		ip->ip_dst.s_addr = INADDR_BROADCAST;
	else
		ip->ip_dst.s_addr = dest.s_addr;

	udp->uh_sport = htons(BOOTPC);
	udp->uh_dport = htons(BOOTPS);
	udp->uh_ulen = htons((uint16_t)(sizeof(*udp) + length));
	ip->ip_len = udp->uh_ulen;
	udp->uh_sum = in_cksum(udpp, sizeof(*ip) + sizeof(*udp) + length, NULL);

	ip->ip_v = IPVERSION;
	ip->ip_hl = sizeof(*ip) >> 2;
	ip->ip_id = (uint16_t)arc4random_uniform(UINT16_MAX);
	ip->ip_ttl = IPDEFTTL;
	ip->ip_len = htons((uint16_t)(sizeof(*ip) + sizeof(*udp) + length));
	ip->ip_sum = in_cksum(ip, sizeof(*ip), NULL);
	if (ip->ip_sum == 0)
		ip->ip_sum = 0xffff; /* RFC 768 */

	*sz = sizeof(*ip) + sizeof(*udp) + length;
	return udpp;
}

static ssize_t
dhcp_sendudp(struct interface *ifp, struct in_addr *to, void *data, size_t len)
{
	struct sockaddr_in sin = {
		.sin_family = AF_INET,
		.sin_addr = *to,
		.sin_port = htons(BOOTPS),
#ifdef HAVE_SA_LEN
		.sin_len = sizeof(sin),
#endif
	};
	struct iovec iov[] = {
		{ .iov_base = data, .iov_len = len }
	};
	struct msghdr msg = {
		.msg_name = (void *)&sin,
		.msg_namelen = sizeof(sin),
		.msg_iov = iov,
		.msg_iovlen = 1,
	};
	struct dhcp_state *state = D_STATE(ifp);
	ssize_t r;
	int fd;

	fd = state->udp_fd;
	if (fd == -1) {
		fd = dhcp_openudp(state->addr != NULL ?&state->addr->addr:NULL);
		if (fd == -1)
			return -1;
	}
	r = sendmsg(fd, &msg, 0);
	if (state->udp_fd == -1)
		close(fd);
	return r;
}

static void
send_message(struct interface *ifp, uint8_t type,
    void (*callback)(void *))
{
	struct dhcp_state *state = D_STATE(ifp);
	struct if_options *ifo = ifp->options;
	struct bootp *bootp;
	struct bootp_pkt *udp;
	size_t len, ulen;
	ssize_t r;
	struct in_addr from, to;
	struct timespec tv;

	if (!callback) {
		/* No carrier? Don't bother sending the packet. */
		if (ifp->carrier <= LINK_DOWN)
			return;
		logdebugx("%s: sending %s with xid 0x%x",
		    ifp->name,
		    ifo->options & DHCPCD_BOOTP ? "BOOTP" : get_dhcp_op(type),
		    state->xid);
	} else {
		if (state->interval == 0)
			state->interval = 4;
		else {
			state->interval *= 2;
			if (state->interval > 64)
				state->interval = 64;
		}
		tv.tv_sec = state->interval + DHCP_RAND_MIN;
		tv.tv_nsec = (suseconds_t)arc4random_uniform(
		    (DHCP_RAND_MAX - DHCP_RAND_MIN) * NSEC_PER_SEC);
		timespecnorm(&tv);
		/* No carrier? Don't bother sending the packet.
		 * However, we do need to advance the timeout. */
		if (ifp->carrier <= LINK_DOWN)
			goto fail;
		logdebugx("%s: sending %s (xid 0x%x), next in %0.1f seconds",
		    ifp->name,
		    ifo->options & DHCPCD_BOOTP ? "BOOTP" : get_dhcp_op(type),
		    state->xid,
		    timespec_to_double(&tv));
	}

	r = make_message(&bootp, ifp, type);
	if (r == -1)
		goto fail;
	len = (size_t)r;

	if (!(state->added & STATE_FAKE) &&
	    state->addr != NULL &&
	    ipv4_iffindaddr(ifp, &state->lease.addr, NULL) != NULL)
		from.s_addr = state->lease.addr.s_addr;
	else
		from.s_addr = INADDR_ANY;
	if (from.s_addr != INADDR_ANY &&
	    state->lease.server.s_addr != INADDR_ANY)
		to.s_addr = state->lease.server.s_addr;
	else
		to.s_addr = INADDR_BROADCAST;

	/*
	 * If not listening on the unspecified address we can
	 * only receive broadcast messages via BPF.
	 * Sockets bound to an address cannot receive broadcast messages
	 * even if they are setup to send them.
	 * Broadcasting from UDP is only an optimisation for rebinding
	 * and on BSD, at least, is reliant on the subnet route being
	 * correctly configured to receive the unicast reply.
	 * As such, we always broadcast and receive the reply to it via BPF.
	 * This also guarantees we have a DHCP server attached to the
	 * interface we want to configure because we can't dictate the
	 * interface via IP_PKTINFO unlike for IPv6.
	 */
	if (to.s_addr != INADDR_BROADCAST)
	{
		if (dhcp_sendudp(ifp, &to, bootp, len) != -1)
			goto out;
		logerr("%s: dhcp_sendudp", ifp->name);
	}

	if (dhcp_openbpf(ifp) == -1)
		goto out;

	udp = dhcp_makeudppacket(&ulen, (uint8_t *)bootp, len, from, to);
	if (udp == NULL) {
		logerr("%s: dhcp_makeudppacket", ifp->name);
		r = 0;
	} else {
		r = bpf_send(ifp, state->bpf_fd,
		    ETHERTYPE_IP, (uint8_t *)udp, ulen);
		free(udp);
	}
	/* If we failed to send a raw packet this normally means
	 * we don't have the ability to work beneath the IP layer
	 * for this interface.
	 * As such we remove it from consideration without actually
	 * stopping the interface. */
	if (r == -1) {
		logerr("%s: bpf_send", ifp->name);
		switch(errno) {
		case ENETDOWN:
		case ENETRESET:
		case ENETUNREACH:
		case ENOBUFS:
			break;
		default:
			if (!(ifp->ctx->options & DHCPCD_TEST))
				dhcp_drop(ifp, "FAIL");
			eloop_timeout_delete(ifp->ctx->eloop,
			    NULL, ifp);
			callback = NULL;
		}
	}

out:
	free(bootp);

fail:
	/* Even if we fail to send a packet we should continue as we are
	 * as our failure timeouts will change out codepath when needed. */
	if (callback)
		eloop_timeout_add_tv(ifp->ctx->eloop, &tv, callback, ifp);
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

	state->state = DHS_DISCOVER;
	dhcp_new_xid(ifp);
	eloop_timeout_delete(ifp->ctx->eloop, NULL, ifp);
	if (ifo->fallback)
		eloop_timeout_add_sec(ifp->ctx->eloop,
		    ifo->reboot, dhcp_fallback, ifp);
#ifdef IPV4LL
	else if (ifo->options & DHCPCD_IPV4LL)
		eloop_timeout_add_sec(ifp->ctx->eloop,
		    ifo->reboot, ipv4ll_start, ifp);
#endif
	if (ifo->options & DHCPCD_REQUEST)
		loginfox("%s: soliciting a DHCP lease (requesting %s)",
		    ifp->name, inet_ntoa(ifo->req_addr));
	else
		loginfox("%s: soliciting a %s lease",
		    ifp->name, ifo->options & DHCPCD_BOOTP ? "BOOTP" : "DHCP");
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
dhcp_expire1(struct interface *ifp)
{
	struct dhcp_state *state = D_STATE(ifp);

	eloop_timeout_delete(ifp->ctx->eloop, NULL, ifp);
	dhcp_drop(ifp, "EXPIRE");
	unlink(state->leasefile);
	state->interval = 0;
	if (!(ifp->options->options & DHCPCD_LINK) || ifp->carrier > LINK_DOWN)
		dhcp_discover(ifp);
}

static void
dhcp_expire(void *arg)
{
	struct interface *ifp = arg;

	if (ifp->options->options & DHCPCD_LASTLEASE_EXTEND) {
		logwarnx("%s: DHCP lease expired, extending lease", ifp->name);
		return;
	}

	logerrx("%s: DHCP lease expired", ifp->name);
	dhcp_expire1(ifp);
}

#if defined(ARP) || defined(IN_IFF_DUPLICATED)
static void
dhcp_decline(struct interface *ifp)
{

	send_message(ifp, DHCP_DECLINE, NULL);
}
#endif

static void
dhcp_startrenew(void *arg)
{
	struct interface *ifp = arg;
	struct dhcp_state *state;
	struct dhcp_lease *lease;

	if ((state = D_STATE(ifp)) == NULL)
		return;

	/* Only renew in the bound or renew states */
	if (state->state != DHS_BOUND &&
	    state->state != DHS_RENEW)
		return;

	/* Remove the timeout as the renew may have been forced. */
	eloop_timeout_delete(ifp->ctx->eloop, dhcp_startrenew, ifp);

	lease = &state->lease;
	logdebugx("%s: renewing lease of %s", ifp->name,
	    inet_ntoa(lease->addr));
	state->state = DHS_RENEW;
	dhcp_new_xid(ifp);
	state->interval = 0;
	send_renew(ifp);
}

void
dhcp_renew(struct interface *ifp)
{

	dhcp_startrenew(ifp);
}

static void
dhcp_rebind(void *arg)
{
	struct interface *ifp = arg;
	struct dhcp_state *state = D_STATE(ifp);
	struct dhcp_lease *lease = &state->lease;

	logwarnx("%s: failed to renew DHCP, rebinding", ifp->name);
	logdebugx("%s: expire in %"PRIu32" seconds",
	    ifp->name, lease->leasetime - lease->rebindtime);
	state->state = DHS_REBIND;
	eloop_timeout_delete(ifp->ctx->eloop, send_renew, ifp);
	state->lease.server.s_addr = INADDR_ANY;
	state->interval = 0;
	ifp->options->options &= ~(DHCPCD_CSR_WARNED |
	    DHCPCD_ROUTER_HOST_ROUTE_WARNED);
	send_rebind(ifp);
}

#if defined(ARP) || defined(IN_IFF_DUPLICATED)
static void
dhcp_finish_dad(struct interface *ifp, struct in_addr *ia)
{
	struct dhcp_state *state = D_STATE(ifp);

	if (state->state != DHS_PROBE)
		return;
	if (state->offer == NULL || state->offer->yiaddr != ia->s_addr)
		return;

	logdebugx("%s: DAD completed for %s", ifp->name, inet_ntoa(*ia));
	if (!(ifp->options->options & DHCPCD_INFORM))
		dhcp_bind(ifp);
#ifndef IN_IFF_DUPLICATED
	else {
		struct bootp *bootp;
		size_t len;

		bootp = state->new;
		len = state->new_len;
		state->new = state->offer;
		state->new_len = state->offer_len;
		get_lease(ifp, &state->lease, state->new, state->new_len);
		ipv4_applyaddr(ifp);
		state->new = bootp;
		state->new_len = len;
	}
#endif

	/* If we forked, stop here. */
	if (ifp->ctx->options & DHCPCD_FORKED)
		return;

#ifdef IPV4LL
	/* Stop IPv4LL now we have a working DHCP address */
	ipv4ll_drop(ifp);
#endif

	if (ifp->options->options & DHCPCD_INFORM)
		dhcp_inform(ifp);
}


static bool
dhcp_addr_duplicated(struct interface *ifp, struct in_addr *ia)
{
	struct dhcp_state *state = D_STATE(ifp);
	unsigned long long opts = ifp->options->options;
	struct dhcpcd_ctx *ctx = ifp->ctx;
	bool deleted = false;
#ifdef IN_IFF_DUPLICATED
	struct ipv4_addr *iap;
#endif

	if ((state->offer == NULL || state->offer->yiaddr != ia->s_addr) &&
	    !IN_ARE_ADDR_EQUAL(ia, &state->lease.addr))
		return deleted;

	/* RFC 2131 3.1.5, Client-server interaction */
	logerrx("%s: DAD detected %s", ifp->name, inet_ntoa(*ia));
	unlink(state->leasefile);
	if (!(opts & DHCPCD_STATIC) && !state->lease.frominfo)
		dhcp_decline(ifp);
#ifdef IN_IFF_DUPLICATED
	if ((iap = ipv4_iffindaddr(ifp, ia, NULL)) != NULL) {
		ipv4_deladdr(iap, 0);
		deleted = true;
	}
#endif
	eloop_timeout_delete(ctx->eloop, NULL, ifp);
	if (opts & (DHCPCD_STATIC | DHCPCD_INFORM)) {
		state->reason = "EXPIRE";
		script_runreason(ifp, state->reason);
#define NOT_ONLY_SELF (DHCPCD_MASTER | DHCPCD_IPV6RS | DHCPCD_DHCP6)
		if (!(ctx->options & NOT_ONLY_SELF))
			eloop_exit(ifp->ctx->eloop, EXIT_FAILURE);
		return deleted;
	}
	eloop_timeout_add_sec(ifp->ctx->eloop,
	    DHCP_RAND_MAX, dhcp_discover, ifp);
	return deleted;
}
#endif

#if defined(ARP) && (!defined(KERNEL_RFC5227) || defined(ARPING))
static void
dhcp_arp_not_found(struct arp_state *astate)
{
	struct interface *ifp;
#ifdef ARPING
	struct dhcp_state *state;
	struct if_options *ifo;
#endif

	ifp = astate->iface;
#ifdef ARPING
	state = D_STATE(ifp);
	ifo = ifp->options;
	if (ifo->arping_len && state->arping_index < ifo->arping_len) {
		/* We didn't find a profile for this
		 * address or hwaddr, so move to the next
		 * arping profile */
		if (++state->arping_index < ifo->arping_len) {
			astate->addr.s_addr =
			    ifo->arping[state->arping_index];
			arp_probe(astate);
			return;
		}
		arp_free(astate);
		dhcpcd_startinterface(ifp);
		return;
	}
#endif

	dhcp_finish_dad(ifp, &astate->addr);
}

static void
dhcp_arp_found(struct arp_state *astate, const struct arp_msg *amsg)
{
	struct in_addr addr;
	struct interface *ifp = astate->iface;
#ifdef ARPING
	struct dhcp_state *state;
	struct if_options *ifo;

	state = D_STATE(ifp);

	ifo = ifp->options;
	if (state->arping_index != -1 &&
	    state->arping_index < ifo->arping_len &&
	    amsg &&
	    amsg->sip.s_addr == ifo->arping[state->arping_index])
	{
		char buf[HWADDR_LEN * 3];

		hwaddr_ntoa(amsg->sha, ifp->hwlen, buf, sizeof(buf));
		if (dhcpcd_selectprofile(ifp, buf) == -1 &&
		    dhcpcd_selectprofile(ifp, inet_ntoa(amsg->sip)) == -1)
		{
			/* We didn't find a profile for this
			 * address or hwaddr, so move to the next
			 * arping profile */
			dhcp_arp_not_found(astate);
			return;
		}
		arp_free(astate);
		eloop_timeout_delete(ifp->ctx->eloop, NULL, ifp);
		dhcpcd_startinterface(ifp);
		return;
	}
#else
	UNUSED(amsg);
#endif

	addr = astate->addr;
	arp_free(astate);
	dhcp_addr_duplicated(ifp, &addr);
}

#ifdef KERNEL_RFC5227
static void
dhcp_arp_announced(struct arp_state *state)
{

	arp_free(state);
}
#endif /* KERNEL_RFC5227 */
#endif /* ARP */

void
dhcp_bind(struct interface *ifp)
{
	struct dhcpcd_ctx *ctx = ifp->ctx;
	struct dhcp_state *state = D_STATE(ifp);
	struct if_options *ifo = ifp->options;
	struct dhcp_lease *lease = &state->lease;

	state->reason = NULL;
	/* If we don't have an offer, we are re-binding a lease on preference,
	 * normally when two interfaces have a lease matching IP addresses. */
	if (state->offer) {
		free(state->old);
		state->old = state->new;
		state->old_len = state->new_len;
		state->new = state->offer;
		state->new_len = state->offer_len;
		state->offer = NULL;
		state->offer_len = 0;
	}
	get_lease(ifp, lease, state->new, state->new_len);
	if (ifo->options & DHCPCD_STATIC) {
		loginfox("%s: using static address %s/%d",
		    ifp->name, inet_ntoa(lease->addr),
		    inet_ntocidr(lease->mask));
		lease->leasetime = DHCP_INFINITE_LIFETIME;
		state->reason = "STATIC";
	} else if (ifo->options & DHCPCD_INFORM) {
		loginfox("%s: received approval for %s",
		    ifp->name, inet_ntoa(lease->addr));
		lease->leasetime = DHCP_INFINITE_LIFETIME;
		state->reason = "INFORM";
	} else {
		if (lease->frominfo)
			state->reason = "TIMEOUT";
		if (lease->leasetime == DHCP_INFINITE_LIFETIME) {
			lease->renewaltime =
			    lease->rebindtime =
			    lease->leasetime;
			loginfox("%s: leased %s for infinity",
			   ifp->name, inet_ntoa(lease->addr));
		} else {
			if (lease->leasetime < DHCP_MIN_LEASE) {
				logwarnx("%s: minimum lease is %d seconds",
				    ifp->name, DHCP_MIN_LEASE);
				lease->leasetime = DHCP_MIN_LEASE;
			}
			if (lease->rebindtime == 0)
				lease->rebindtime =
				    (uint32_t)(lease->leasetime * T2);
			else if (lease->rebindtime >= lease->leasetime) {
				lease->rebindtime =
				    (uint32_t)(lease->leasetime * T2);
				logwarnx("%s: rebind time greater than lease "
				    "time, forcing to %"PRIu32" seconds",
				    ifp->name, lease->rebindtime);
			}
			if (lease->renewaltime == 0)
				lease->renewaltime =
				    (uint32_t)(lease->leasetime * T1);
			else if (lease->renewaltime > lease->rebindtime) {
				lease->renewaltime =
				    (uint32_t)(lease->leasetime * T1);
				logwarnx("%s: renewal time greater than "
				    "rebind time, forcing to %"PRIu32" seconds",
				    ifp->name, lease->renewaltime);
			}
			if (state->addr &&
			    lease->addr.s_addr == state->addr->addr.s_addr &&
			    !(state->added & STATE_FAKE))
				logdebugx("%s: leased %s for %"PRIu32" seconds",
				    ifp->name, inet_ntoa(lease->addr),
				    lease->leasetime);
			else
				loginfox("%s: leased %s for %"PRIu32" seconds",
				    ifp->name, inet_ntoa(lease->addr),
				    lease->leasetime);
		}
	}
	if (ctx->options & DHCPCD_TEST) {
		state->reason = "TEST";
		script_runreason(ifp, state->reason);
		eloop_exit(ctx->eloop, EXIT_SUCCESS);
		return;
	}
	if (state->reason == NULL) {
		if (state->old && !(state->added & STATE_FAKE)) {
			if (state->old->yiaddr == state->new->yiaddr &&
			    lease->server.s_addr &&
			    state->state != DHS_REBIND)
				state->reason = "RENEW";
			else
				state->reason = "REBIND";
		} else if (state->state == DHS_REBOOT)
			state->reason = "REBOOT";
		else
			state->reason = "BOUND";
	}
	if (lease->leasetime == DHCP_INFINITE_LIFETIME)
		lease->renewaltime = lease->rebindtime = lease->leasetime;
	else {
		eloop_timeout_add_sec(ctx->eloop,
		    (time_t)lease->renewaltime, dhcp_startrenew, ifp);
		eloop_timeout_add_sec(ctx->eloop,
		    (time_t)lease->rebindtime, dhcp_rebind, ifp);
		eloop_timeout_add_sec(ctx->eloop,
		    (time_t)lease->leasetime, dhcp_expire, ifp);
		logdebugx("%s: renew in %"PRIu32" seconds, rebind in %"PRIu32
		    " seconds",
		    ifp->name, lease->renewaltime, lease->rebindtime);
	}
	state->state = DHS_BOUND;
	if (!state->lease.frominfo &&
	    !(ifo->options & (DHCPCD_INFORM | DHCPCD_STATIC)))
		if (write_lease(ifp, state->new, state->new_len) == -1)
			logerr(__func__);

	ipv4_applyaddr(ifp);

	/* Close the BPF filter as we can now receive DHCP messages
	 * on a UDP socket. */
	if (!(state->udp_fd == -1 ||
	    (state->old != NULL && state->old->yiaddr != state->new->yiaddr)))
		return;
	dhcp_close(ifp);


	/* If not in master mode, open an address specific socket. */
	if (ctx->udp_fd != -1)
		return;
	state->udp_fd = dhcp_openudp(&state->addr->addr);
	if (state->udp_fd == -1) {
		logerr(__func__);
		/* Address sharing without master mode is not supported.
		 * It's also possible another DHCP client could be running,
		 * which is even worse.
		 * We still need to work, so re-open BPF. */
		dhcp_openbpf(ifp);
		return;
	}
	eloop_event_add(ctx->eloop, state->udp_fd, dhcp_handleifudp, ifp);
}

static void
dhcp_lastlease(void *arg)
{
	struct interface *ifp = arg;
	struct dhcp_state *state = D_STATE(ifp);

	loginfox("%s: timed out contacting a DHCP server, using last lease",
	    ifp->name);
	dhcp_bind(ifp);
	/* If we forked, stop here. */
	if (ifp->ctx->options & DHCPCD_FORKED)
		return;
	state->interval = 0;
	dhcp_discover(ifp);
}

static size_t
dhcp_message_new(struct bootp **bootp,
    const struct in_addr *addr, const struct in_addr *mask)
{
	uint8_t *p;
	uint32_t cookie;

	if ((*bootp = calloc(1, sizeof(**bootp))) == NULL)
		return 0;

	(*bootp)->yiaddr = addr->s_addr;
	p = (*bootp)->vend;

	cookie = htonl(MAGIC_COOKIE);
	memcpy(p, &cookie, sizeof(cookie));
	p += sizeof(cookie);

	if (mask->s_addr != INADDR_ANY) {
		*p++ = DHO_SUBNETMASK;
		*p++ = sizeof(mask->s_addr);
		memcpy(p, &mask->s_addr, sizeof(mask->s_addr));
		p+= sizeof(mask->s_addr);
	}

	*p = DHO_END;
	return sizeof(**bootp);
}

#ifdef ARP
#ifndef KERNEL_RFC5227
static void
dhcp_arp_defend_failed(struct arp_state *astate)
{
	struct interface *ifp = astate->iface;

	dhcp_drop(ifp, "EXPIRED");
	dhcp_start1(ifp);
}
#endif

#if !defined(KERNEL_RFC5227) || defined(ARPING)
static struct arp_state *
dhcp_arp_new(struct interface *ifp, struct in_addr *addr)
{
	struct arp_state *astate;

	astate = arp_new(ifp, addr);
	if (astate == NULL)
		return NULL;

	astate->found_cb = dhcp_arp_found;
	astate->not_found_cb = dhcp_arp_not_found;
#ifdef KERNEL_RFC5227
	astate->announced_cb = dhcp_arp_announced;
#else
	astate->announced_cb = NULL;
	astate->defend_failed_cb = dhcp_arp_defend_failed;
#endif
	return astate;
}
#endif
#endif /* ARP */

#if defined(ARP) || defined(KERNEL_RFC5227)
static int
dhcp_arp_address(struct interface *ifp)
{
	struct dhcp_state *state;
	struct in_addr addr;
	struct ipv4_addr *ia;

	eloop_timeout_delete(ifp->ctx->eloop, NULL, ifp);

	state = D_STATE(ifp);
	addr.s_addr = state->offer->yiaddr == INADDR_ANY ?
	    state->offer->ciaddr : state->offer->yiaddr;
	/* If the interface already has the address configured
	 * then we can't ARP for duplicate detection. */
	ia = ipv4_iffindaddr(ifp, &addr, NULL);
#ifdef IN_IFF_NOTUSEABLE
	if (ia == NULL || ia->addr_flags & IN_IFF_NOTUSEABLE) {
		state->state = DHS_PROBE;
		if (ia == NULL) {
			struct dhcp_lease l;

			get_lease(ifp, &l, state->offer, state->offer_len);
			/* Add the address now, let the kernel handle DAD. */
			ipv4_addaddr(ifp, &l.addr, &l.mask, &l.brd,
			    l.leasetime, l.rebindtime);
		} else if (ia->addr_flags & IN_IFF_DUPLICATED)
			dhcp_addr_duplicated(ifp, &ia->addr);
		else
			loginfox("%s: waiting for DAD on %s",
			    ifp->name, inet_ntoa(addr));
		return 0;
	}
#else
	if (!(ifp->flags & IFF_NOARP) &&
	    ifp->options->options & DHCPCD_ARP &&
	    ia == NULL)
	{
		struct arp_state *astate;
		struct dhcp_lease l;

		astate = dhcp_arp_new(ifp, &addr);
		if (astate == NULL)
			return -1;

		state->state = DHS_PROBE;
		get_lease(ifp, &l, state->offer, state->offer_len);
		loginfox("%s: probing address %s/%d",
		    ifp->name, inet_ntoa(l.addr), inet_ntocidr(l.mask));
		/* We need to handle DAD. */
		arp_probe(astate);
		return 0;
	}
#endif

	return 1;
}

static void
dhcp_arp_bind(struct interface *ifp)
{

	if (ifp->ctx->options & DHCPCD_TEST ||
	    dhcp_arp_address(ifp) == 1)
		dhcp_bind(ifp);
}
#endif

static void
dhcp_static(struct interface *ifp)
{
	struct if_options *ifo;
	struct dhcp_state *state;
	struct ipv4_addr *ia;

	state = D_STATE(ifp);
	ifo = ifp->options;

	ia = NULL;
	if (ifo->req_addr.s_addr == INADDR_ANY &&
	    (ia = ipv4_iffindaddr(ifp, NULL, NULL)) == NULL)
	{
		loginfox("%s: waiting for 3rd party to "
		    "configure IP address", ifp->name);
		state->reason = "3RDPARTY";
		script_runreason(ifp, state->reason);
		return;
	}

	state->offer_len = dhcp_message_new(&state->offer,
	    ia ? &ia->addr : &ifo->req_addr,
	    ia ? &ia->mask : &ifo->req_mask);
	if (state->offer_len)
#if defined(ARP) || defined(KERNEL_RFC5227)
		dhcp_arp_bind(ifp);
#else
		dhcp_bind(ifp);
#endif
}

void
dhcp_inform(struct interface *ifp)
{
	struct dhcp_state *state;
	struct if_options *ifo;
	struct ipv4_addr *ia;

	state = D_STATE(ifp);
	ifo = ifp->options;

	state->state = DHS_INFORM;
	free(state->offer);
	state->offer = NULL;
	state->offer_len = 0;

	if (ifo->req_addr.s_addr == INADDR_ANY) {
		ia = ipv4_iffindaddr(ifp, NULL, NULL);
		if (ia == NULL) {
			loginfox("%s: waiting for 3rd party to "
			    "configure IP address",
			    ifp->name);
			if (!(ifp->ctx->options & DHCPCD_TEST)) {
				state->reason = "3RDPARTY";
				script_runreason(ifp, state->reason);
			}
			return;
		}
	} else {
		ia = ipv4_iffindaddr(ifp, &ifo->req_addr, &ifo->req_mask);
		if (ia == NULL) {
			if (ifp->ctx->options & DHCPCD_TEST) {
				logerrx("%s: cannot add IP address in test mode",
				    ifp->name);
				return;
			}
			ia = ipv4_iffindaddr(ifp, &ifo->req_addr, NULL);
			if (ia != NULL)
				/* Netmask must be different, delete it. */
				ipv4_deladdr(ia, 1);
			state->offer_len = dhcp_message_new(&state->offer,
			    &ifo->req_addr, &ifo->req_mask);
#ifdef ARP
			if (dhcp_arp_address(ifp) != 1)
				return;
#endif
			ia = ipv4_iffindaddr(ifp,
			    &ifo->req_addr, &ifo->req_mask);
			assert(ia != NULL);
		}
	}

	state->addr = ia;
	state->offer_len = dhcp_message_new(&state->offer,
	    &ia->addr, &ia->mask);
	if (state->offer_len) {
		dhcp_new_xid(ifp);
		get_lease(ifp, &state->lease, state->offer, state->offer_len);
		send_inform(ifp);
	}
}

void
dhcp_reboot_newopts(struct interface *ifp, unsigned long long oldopts)
{
	struct if_options *ifo;
	struct dhcp_state *state = D_STATE(ifp);

	if (state == NULL || state->state == DHS_NONE)
		return;
	ifo = ifp->options;
	if ((ifo->options & (DHCPCD_INFORM | DHCPCD_STATIC) &&
		(state->addr == NULL ||
		state->addr->addr.s_addr != ifo->req_addr.s_addr)) ||
	    (oldopts & (DHCPCD_INFORM | DHCPCD_STATIC) &&
		!(ifo->options & (DHCPCD_INFORM | DHCPCD_STATIC))))
	{
		dhcp_drop(ifp, "EXPIRE");
	}
}

#ifdef ARP
static int
dhcp_activeaddr(const struct interface *ifp, const struct in_addr *addr)
{
	const struct interface *ifp1;
	const struct dhcp_state *state;

	TAILQ_FOREACH(ifp1, ifp->ctx->ifaces, next) {
		if (ifp1 == ifp)
			continue;
		if ((state = D_CSTATE(ifp1)) == NULL)
			continue;
		switch(state->state) {
		case DHS_REBOOT:
		case DHS_RENEW:
		case DHS_REBIND:
		case DHS_BOUND:
		case DHS_INFORM:
			break;
		default:
			continue;
		}
		if (state->lease.addr.s_addr == addr->s_addr)
			return 1;
	}
	return 0;
}
#endif

static void
dhcp_reboot(struct interface *ifp)
{
	struct if_options *ifo;
	struct dhcp_state *state = D_STATE(ifp);
#ifdef ARP
	struct ipv4_addr *ia;
#endif

	if (state == NULL || state->state == DHS_NONE)
		return;
	ifo = ifp->options;
	state->state = DHS_REBOOT;
	state->interval = 0;

	if (ifo->options & DHCPCD_LINK && ifp->carrier <= LINK_DOWN) {
		loginfox("%s: waiting for carrier", ifp->name);
		return;
	}
	if (ifo->options & DHCPCD_STATIC) {
		dhcp_static(ifp);
		return;
	}
	if (ifo->options & DHCPCD_INFORM) {
		loginfox("%s: informing address of %s",
		    ifp->name, inet_ntoa(state->lease.addr));
		dhcp_inform(ifp);
		return;
	}
	if (ifo->reboot == 0 || state->offer == NULL) {
		dhcp_discover(ifp);
		return;
	}
	if (!IS_DHCP(state->offer))
		return;

	loginfox("%s: rebinding lease of %s",
	    ifp->name, inet_ntoa(state->lease.addr));

#ifdef ARP
#ifndef KERNEL_RFC5227
	/* Create the DHCP ARP state so we can defend it. */
	(void)dhcp_arp_new(ifp, &state->lease.addr);
#endif

	/* If the address exists on the interface and no other interface
	 * is currently using it then announce it to ensure this
	 * interface gets the reply. */
	ia = ipv4_iffindaddr(ifp, &state->lease.addr, NULL);
	if (ia != NULL &&
	    !(ifp->ctx->options & DHCPCD_TEST) &&
#ifdef IN_IFF_NOTUSEABLE
	    !(ia->addr_flags & IN_IFF_NOTUSEABLE) &&
#endif
	    dhcp_activeaddr(ifp, &state->lease.addr) == 0)
		arp_ifannounceaddr(ifp, &state->lease.addr);
#endif

	dhcp_new_xid(ifp);
	state->lease.server.s_addr = INADDR_ANY;
	eloop_timeout_delete(ifp->ctx->eloop, NULL, ifp);

#ifdef IPV4LL
	/* Need to add this before dhcp_expire and friends. */
	if (!ifo->fallback && ifo->options & DHCPCD_IPV4LL)
		eloop_timeout_add_sec(ifp->ctx->eloop,
		    ifo->reboot, ipv4ll_start, ifp);
#endif

	if (ifo->options & DHCPCD_LASTLEASE && state->lease.frominfo)
		eloop_timeout_add_sec(ifp->ctx->eloop,
		    ifo->reboot, dhcp_lastlease, ifp);
	else if (!(ifo->options & DHCPCD_INFORM))
		eloop_timeout_add_sec(ifp->ctx->eloop,
		    ifo->reboot, dhcp_expire, ifp);

	/* Don't bother ARP checking as the server could NAK us first.
	 * Don't call dhcp_request as that would change the state */
	send_request(ifp);
}

void
dhcp_drop(struct interface *ifp, const char *reason)
{
	struct dhcp_state *state;
#ifdef RELEASE_SLOW
	struct timespec ts;
#endif

	state = D_STATE(ifp);
	/* dhcp_start may just have been called and we don't yet have a state
	 * but we do have a timeout, so punt it. */
	if (state == NULL || state->state == DHS_NONE) {
		eloop_timeout_delete(ifp->ctx->eloop, NULL, ifp);
		return;
	}

#ifdef ARP
	if (state->addr != NULL)
		arp_freeaddr(ifp, &state->addr->addr);
#endif
#ifdef ARPING
	state->arping_index = -1;
#endif

	if (ifp->options->options & DHCPCD_RELEASE &&
	    !(ifp->options->options & DHCPCD_INFORM))
	{
		/* Failure to send the release may cause this function to
		 * re-enter so guard by setting the state. */
		if (state->state == DHS_RELEASE)
			return;
		state->state = DHS_RELEASE;

		unlink(state->leasefile);
		if (ifp->carrier > LINK_DOWN &&
		    state->new != NULL &&
		    state->lease.server.s_addr != INADDR_ANY)
		{
			loginfox("%s: releasing lease of %s",
			    ifp->name, inet_ntoa(state->lease.addr));
			dhcp_new_xid(ifp);
			send_message(ifp, DHCP_RELEASE, NULL);
#ifdef RELEASE_SLOW
			/* Give the packet a chance to go */
			ts.tv_sec = RELEASE_DELAY_S;
			ts.tv_nsec = RELEASE_DELAY_NS;
			nanosleep(&ts, NULL);
#endif
		}
	}

	eloop_timeout_delete(ifp->ctx->eloop, NULL, ifp);
#ifdef AUTH
	dhcp_auth_reset(&state->auth);
#endif

	state->state = DHS_NONE;
	free(state->offer);
	state->offer = NULL;
	state->offer_len = 0;
	free(state->old);
	state->old = state->new;
	state->old_len = state->new_len;
	state->new = NULL;
	state->new_len = 0;
	state->reason = reason;
	ipv4_applyaddr(ifp);
	free(state->old);
	state->old = NULL;
	state->old_len = 0;
	state->lease.addr.s_addr = 0;
	ifp->options->options &= ~(DHCPCD_CSR_WARNED |
	    DHCPCD_ROUTER_HOST_ROUTE_WARNED);
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

#define	WHTLST_NONE	0
#define	WHTLST_MATCH	1
#define WHTLST_NOMATCH	2
static unsigned int
whitelisted_ip(const struct if_options *ifo, in_addr_t addr)
{
	size_t i;

	if (ifo->whitelist_len == 0)
		return WHTLST_NONE;
	for (i = 0; i < ifo->whitelist_len; i += 2)
		if (ifo->whitelist[i] == (addr & ifo->whitelist[i + 1]))
			return WHTLST_MATCH;
	return WHTLST_NOMATCH;
}

static void
log_dhcp(int loglevel, const char *msg,
    const struct interface *ifp, const struct bootp *bootp, size_t bootp_len,
    const struct in_addr *from, int ad)
{
	const char *tfrom;
	char *a, sname[sizeof(bootp->sname) * 4];
	struct in_addr addr;
	int r;
	uint8_t overl;

	if (strcmp(msg, "NAK:") == 0) {
		a = get_option_string(ifp->ctx, bootp, bootp_len, DHO_MESSAGE);
		if (a) {
			char *tmp;
			size_t al, tmpl;

			al = strlen(a);
			tmpl = (al * 4) + 1;
			tmp = malloc(tmpl);
			if (tmp == NULL) {
				logerr(__func__);
				free(a);
				return;
			}
			print_string(tmp, tmpl, OT_STRING, (uint8_t *)a, al);
			free(a);
			a = tmp;
		}
	} else if (ad && bootp->yiaddr != 0) {
		addr.s_addr = bootp->yiaddr;
		a = strdup(inet_ntoa(addr));
		if (a == NULL) {
			logerr(__func__);
			return;
		}
	} else
		a = NULL;

	tfrom = "from";
	r = get_option_addr(ifp->ctx, &addr, bootp, bootp_len, DHO_SERVERID);
	if (get_option_uint8(ifp->ctx, &overl, bootp, bootp_len,
	    DHO_OPTSOVERLOADED) == -1)
		overl = 0;
	if (bootp->sname[0] && r == 0 && !(overl & 2)) {
		print_string(sname, sizeof(sname), OT_STRING | OT_DOMAIN,
		    bootp->sname, sizeof(bootp->sname));
		if (a == NULL)
			logmessage(loglevel, "%s: %s %s %s `%s'",
			    ifp->name, msg, tfrom, inet_ntoa(addr), sname);
		else
			logmessage(loglevel, "%s: %s %s %s %s `%s'",
			    ifp->name, msg, a, tfrom, inet_ntoa(addr), sname);
	} else {
		if (r != 0) {
			tfrom = "via";
			addr = *from;
		}
		if (a == NULL)
			logmessage(loglevel, "%s: %s %s %s",
			    ifp->name, msg, tfrom, inet_ntoa(addr));
		else
			logmessage(loglevel, "%s: %s %s %s %s",
			    ifp->name, msg, a, tfrom, inet_ntoa(addr));
	}
	free(a);
}

/* If we're sharing the same IP address with another interface on the
 * same network, we may receive the DHCP reply on the wrong interface.
 * Try and re-direct it here. */
static void
dhcp_redirect_dhcp(struct interface *ifp, struct bootp *bootp, size_t bootp_len,
    const struct in_addr *from)
{
	struct interface *ifn;
	const struct dhcp_state *state;
	uint32_t xid;

	xid = ntohl(bootp->xid);
	TAILQ_FOREACH(ifn, ifp->ctx->ifaces, next) {
		state = D_CSTATE(ifn);
		if (state == NULL || state->state == DHS_NONE)
			continue;
		if (state->xid != xid)
			continue;
		if (ifn->hwlen <= sizeof(bootp->chaddr) &&
		    memcmp(bootp->chaddr, ifn->hwaddr, ifn->hwlen))
			continue;
		logdebugx("%s: redirecting DHCP message to %s",
		    ifp->name, ifn->name);
		dhcp_handledhcp(ifn, bootp, bootp_len, from);
	}
}

static void
dhcp_handledhcp(struct interface *ifp, struct bootp *bootp, size_t bootp_len,
    const struct in_addr *from)
{
	struct dhcp_state *state = D_STATE(ifp);
	struct if_options *ifo = ifp->options;
	struct dhcp_lease *lease = &state->lease;
	uint8_t type, tmp;
	struct in_addr addr;
	unsigned int i;
	char *msg;
	bool bootp_copied;
#ifdef AUTH
	const uint8_t *auth;
	size_t auth_len;
#endif
#ifdef IN_IFF_DUPLICATED
	struct ipv4_addr *ia;
#endif

#define LOGDHCP0(l, m) \
	log_dhcp((l), (m), ifp, bootp, bootp_len, from, 0)
#define LOGDHCP(l, m) \
	log_dhcp((l), (m), ifp, bootp, bootp_len, from, 1)

#define IS_STATE_ACTIVE(s) ((s)-state != DHS_NONE && \
	(s)->state != DHS_INIT && (s)->state != DHS_BOUND)

	if (bootp->op != BOOTREPLY) {
		if (IS_STATE_ACTIVE(state))
			logdebugx("%s: op (%d) is not BOOTREPLY",
			    ifp->name, bootp->op);
		return;
	}

	if (state->xid != ntohl(bootp->xid)) {
		if (IS_STATE_ACTIVE(state))
			logdebugx("%s: wrong xid 0x%x (expecting 0x%x) from %s",
			    ifp->name, ntohl(bootp->xid), state->xid,
			    inet_ntoa(*from));
		dhcp_redirect_dhcp(ifp, bootp, bootp_len, from);
		return;
	}

	if (ifp->hwlen <= sizeof(bootp->chaddr) &&
	    memcmp(bootp->chaddr, ifp->hwaddr, ifp->hwlen))
	{
		if (IS_STATE_ACTIVE(state)) {
			char buf[sizeof(bootp->chaddr) * 3];

			logdebugx("%s: xid 0x%x is for hwaddr %s",
			    ifp->name, ntohl(bootp->xid),
			    hwaddr_ntoa(bootp->chaddr, sizeof(bootp->chaddr),
				    buf, sizeof(buf)));
		}
		dhcp_redirect_dhcp(ifp, bootp, bootp_len, from);
		return;
	}

	if (!ifp->active)
		return;

	i = whitelisted_ip(ifp->options, from->s_addr);
	switch (i) {
	case WHTLST_NOMATCH:
		logwarnx("%s: non whitelisted DHCP packet from %s",
		    ifp->name, inet_ntoa(*from));
		return;
	case WHTLST_MATCH:
		break;
	case WHTLST_NONE:
		if (blacklisted_ip(ifp->options, from->s_addr) == 1) {
			logwarnx("%s: blacklisted DHCP packet from %s",
			    ifp->name, inet_ntoa(*from));
			return;
		}
	}

	/* We may have found a BOOTP server */
	if (get_option_uint8(ifp->ctx, &type,
	    bootp, bootp_len, DHO_MESSAGETYPE) == -1)
		type = 0;
	else if (ifo->options & DHCPCD_BOOTP) {
		logdebugx("%s: ignoring DHCP reply (expecting BOOTP)",
		    ifp->name);
		return;
	}

#ifdef AUTH
	/* Authenticate the message */
	auth = get_option(ifp->ctx, bootp, bootp_len,
	    DHO_AUTHENTICATION, &auth_len);
	if (auth) {
		if (dhcp_auth_validate(&state->auth, &ifo->auth,
		    (uint8_t *)bootp, bootp_len, 4, type,
		    auth, auth_len) == NULL)
		{
			LOGDHCP0(LOG_ERR, "authentication failed");
			return;
		}
		if (state->auth.token)
			logdebugx("%s: validated using 0x%08" PRIu32,
			    ifp->name, state->auth.token->secretid);
		else
			loginfox("%s: accepted reconfigure key", ifp->name);
	} else if (ifo->auth.options & DHCPCD_AUTH_SEND) {
		if (ifo->auth.options & DHCPCD_AUTH_REQUIRE) {
			LOGDHCP0(LOG_ERR, "no authentication");
			return;
		}
		LOGDHCP0(LOG_WARNING, "no authentication");
	}
#endif

	/* RFC 3203 */
	if (type == DHCP_FORCERENEW) {
		if (from->s_addr == INADDR_ANY ||
		    from->s_addr == INADDR_BROADCAST)
		{
			LOGDHCP(LOG_ERR, "discarding Force Renew");
			return;
		}
#ifdef AUTH
		if (auth == NULL) {
			LOGDHCP(LOG_ERR, "unauthenticated Force Renew");
			if (ifo->auth.options & DHCPCD_AUTH_REQUIRE)
				return;
		}
		if (state->state != DHS_BOUND && state->state != DHS_INFORM) {
			LOGDHCP(LOG_DEBUG, "not bound, ignoring Force Renew");
			return;
		}
		LOGDHCP(LOG_INFO, "Force Renew from");
		/* The rebind and expire timings are still the same, we just
		 * enter the renew state early */
		if (state->state == DHS_BOUND)
			dhcp_renew(ifp);
		else {
			eloop_timeout_delete(ifp->ctx->eloop,
			    send_inform, ifp);
			dhcp_inform(ifp);
		}
#else
		LOGDHCP(LOG_ERR, "unauthenticated Force Renew");
#endif
		return;
	}

	if (state->state == DHS_BOUND) {
		LOGDHCP(LOG_DEBUG, "bound, ignoring");
		return;
	}

	if (state->state == DHS_PROBE) {
		/* Ignore any DHCP messages whilst probing a lease to bind. */
		LOGDHCP(LOG_DEBUG, "probing, ignoring");
		return;
	}

	/* reset the message counter */
	state->interval = 0;

	/* Ensure that no reject options are present */
	for (i = 1; i < 255; i++) {
		if (has_option_mask(ifo->rejectmask, i) &&
		    get_option_uint8(ifp->ctx, &tmp,
		    bootp, bootp_len, (uint8_t)i) == 0)
		{
			LOGDHCP(LOG_WARNING, "reject DHCP");
			return;
		}
	}

	if (type == DHCP_NAK) {
		/* For NAK, only check if we require the ServerID */
		if (has_option_mask(ifo->requiremask, DHO_SERVERID) &&
		    get_option_addr(ifp->ctx, &addr,
		    bootp, bootp_len, DHO_SERVERID) == -1)
		{
			LOGDHCP(LOG_WARNING, "reject NAK");
			return;
		}

		/* We should restart on a NAK */
		LOGDHCP(LOG_WARNING, "NAK:");
		if ((msg = get_option_string(ifp->ctx,
		    bootp, bootp_len, DHO_MESSAGE)))
		{
			logwarnx("%s: message: %s", ifp->name, msg);
			free(msg);
		}
		if (state->state == DHS_INFORM) /* INFORM should not be NAKed */
			return;
		if (!(ifp->ctx->options & DHCPCD_TEST)) {
			dhcp_drop(ifp, "NAK");
			unlink(state->leasefile);
		}

		/* If we constantly get NAKS then we should slowly back off */
		eloop_timeout_add_sec(ifp->ctx->eloop,
		    state->nakoff, dhcp_discover, ifp);
		if (state->nakoff == 0)
			state->nakoff = 1;
		else {
			state->nakoff *= 2;
			if (state->nakoff > NAKOFF_MAX)
				state->nakoff = NAKOFF_MAX;
		}
		return;
	}

	/* Ensure that all required options are present */
	for (i = 1; i < 255; i++) {
		if (has_option_mask(ifo->requiremask, i) &&
		    get_option_uint8(ifp->ctx, &tmp,
		    bootp, bootp_len, (uint8_t)i) != 0)
		{
			/* If we are BOOTP, then ignore the need for serverid.
			 * To ignore BOOTP, require dhcp_message_type.
			 * However, nothing really stops BOOTP from providing
			 * DHCP style options as well so the above isn't
			 * always true. */
			if (type == 0 && i == DHO_SERVERID)
				continue;
			LOGDHCP(LOG_WARNING, "reject DHCP");
			return;
		}
	}

	/* DHCP Auto-Configure, RFC 2563 */
	if (type == DHCP_OFFER && bootp->yiaddr == 0) {
		LOGDHCP(LOG_WARNING, "no address given");
		if ((msg = get_option_string(ifp->ctx,
		    bootp, bootp_len, DHO_MESSAGE)))
		{
			logwarnx("%s: message: %s", ifp->name, msg);
			free(msg);
		}
#ifdef IPV4LL
		if (state->state == DHS_DISCOVER &&
		    get_option_uint8(ifp->ctx, &tmp, bootp, bootp_len,
		    DHO_AUTOCONFIGURE) == 0)
		{
			switch (tmp) {
			case 0:
				LOGDHCP(LOG_WARNING, "IPv4LL disabled from");
				ipv4ll_drop(ifp);
#ifdef ARP
				arp_drop(ifp);
#endif
				break;
			case 1:
				LOGDHCP(LOG_WARNING, "IPv4LL enabled from");
				ipv4ll_start(ifp);
				break;
			default:
				logerrx("%s: unknown auto configuration "
				    "option %d",
				    ifp->name, tmp);
				break;
			}
			eloop_timeout_delete(ifp->ctx->eloop, NULL, ifp);
			eloop_timeout_add_sec(ifp->ctx->eloop,
			    DHCP_MAX, dhcp_discover, ifp);
		}
#endif
		return;
	}

	/* Ensure that the address offered is valid */
	if ((type == 0 || type == DHCP_OFFER || type == DHCP_ACK) &&
	    (bootp->ciaddr == INADDR_ANY || bootp->ciaddr == INADDR_BROADCAST)
	    &&
	    (bootp->yiaddr == INADDR_ANY || bootp->yiaddr == INADDR_BROADCAST))
	{
		LOGDHCP(LOG_WARNING, "reject invalid address");
		return;
	}

#ifdef IN_IFF_DUPLICATED
	ia = ipv4_iffindaddr(ifp, &lease->addr, NULL);
	if (ia && ia->addr_flags & IN_IFF_DUPLICATED) {
		LOGDHCP(LOG_WARNING, "declined duplicate address");
		if (type)
			dhcp_decline(ifp);
		ipv4_deladdr(ia, 0);
		eloop_timeout_delete(ifp->ctx->eloop, NULL, ifp);
		eloop_timeout_add_sec(ifp->ctx->eloop,
		    DHCP_RAND_MAX, dhcp_discover, ifp);
		return;
	}
#endif

	bootp_copied = false;
	if ((type == 0 || type == DHCP_OFFER) && state->state == DHS_DISCOVER) {
		lease->frominfo = 0;
		lease->addr.s_addr = bootp->yiaddr;
		memcpy(&lease->cookie, bootp->vend, sizeof(lease->cookie));
		if (type == 0 ||
		    get_option_addr(ifp->ctx,
		    &lease->server, bootp, bootp_len, DHO_SERVERID) != 0)
			lease->server.s_addr = INADDR_ANY;

		/* Test for rapid commit in the OFFER */
		if (!(ifp->ctx->options & DHCPCD_TEST) &&
		    has_option_mask(ifo->requestmask, DHO_RAPIDCOMMIT) &&
		    get_option(ifp->ctx, bootp, bootp_len,
		    DHO_RAPIDCOMMIT, NULL))
		{
			state->state = DHS_REQUEST;
			goto rapidcommit;
		}

		LOGDHCP(LOG_INFO, "offered");
		if (state->offer_len < bootp_len) {
			free(state->offer);
			if ((state->offer = malloc(bootp_len)) == NULL) {
				logerr(__func__);
				state->offer_len = 0;
				return;
			}
		}
		state->offer_len = bootp_len;
		memcpy(state->offer, bootp, bootp_len);
		bootp_copied = true;
		if (ifp->ctx->options & DHCPCD_TEST) {
			free(state->old);
			state->old = state->new;
			state->old_len = state->new_len;
			state->new = state->offer;
			state->new_len = state->offer_len;
			state->offer = NULL;
			state->offer_len = 0;
			state->reason = "TEST";
			script_runreason(ifp, state->reason);
			eloop_exit(ifp->ctx->eloop, EXIT_SUCCESS);
			state->bpf_flags |= BPF_EOF;
			return;
		}
		eloop_timeout_delete(ifp->ctx->eloop, send_discover, ifp);
		/* We don't request BOOTP addresses */
		if (type) {
			/* We used to ARP check here, but that seems to be in
			 * violation of RFC2131 where it only describes
			 * DECLINE after REQUEST.
			 * It also seems that some MS DHCP servers actually
			 * ignore DECLINE if no REQUEST, ie we decline a
			 * DISCOVER. */
			dhcp_request(ifp);
			return;
		}
	}

	if (type) {
		if (type == DHCP_OFFER) {
			LOGDHCP(LOG_WARNING, "ignoring offer of");
			return;
		}

		/* We should only be dealing with acks */
		if (type != DHCP_ACK) {
			LOGDHCP(LOG_ERR, "not ACK or OFFER");
			return;
		}

		if (state->state == DHS_DISCOVER) {
			/* We only allow ACK of rapid commit DISCOVER. */
			if (has_option_mask(ifo->requestmask,
			    DHO_RAPIDCOMMIT) &&
			    get_option(ifp->ctx, bootp, bootp_len,
			    DHO_RAPIDCOMMIT, NULL))
				state->state = DHS_REQUEST;
			else {
				LOGDHCP(LOG_DEBUG, "ignoring ack of");
				return;
			}
		}

rapidcommit:
		if (!(ifo->options & DHCPCD_INFORM))
			LOGDHCP(LOG_DEBUG, "acknowledged");
		else
		    ifo->options &= ~DHCPCD_STATIC;
	}

	/* No NAK, so reset the backoff
	 * We don't reset on an OFFER message because the server could
	 * potentially NAK the REQUEST. */
	state->nakoff = 0;

	/* BOOTP could have already assigned this above. */
	if (!bootp_copied) {
		if (state->offer_len < bootp_len) {
			free(state->offer);
			if ((state->offer = malloc(bootp_len)) == NULL) {
				logerr(__func__);
				state->offer_len = 0;
				return;
			}
		}
		state->offer_len = bootp_len;
		memcpy(state->offer, bootp, bootp_len);
	}

	lease->frominfo = 0;
	eloop_timeout_delete(ifp->ctx->eloop, NULL, ifp);

#if defined(ARP) || defined(KERNEL_RFC5227)
	dhcp_arp_bind(ifp);
#else
	dhcp_bind(ifp);
#endif
}

static void *
get_udp_data(void *packet, size_t *len)
{
	const struct ip *ip = packet;
	size_t ip_hl = (size_t)ip->ip_hl * 4;
	char *p = packet;

	p += ip_hl + sizeof(struct udphdr);
	*len = (size_t)ntohs(ip->ip_len) - sizeof(struct udphdr) - ip_hl;
	return p;
}

static bool
is_packet_udp_bootp(void *packet, size_t plen)
{
	struct ip *ip = packet;
	size_t ip_hlen;
	struct udphdr udp;

	if (plen < sizeof(*ip))
		return false;

	if (ip->ip_v != IPVERSION || ip->ip_p != IPPROTO_UDP)
		return false;

	/* Sanity. */
	if (ntohs(ip->ip_len) > plen)
		return false;

	ip_hlen = (size_t)ip->ip_hl * 4;
	if (ip_hlen < sizeof(*ip))
		return false;

	/* Check we have a UDP header and BOOTP. */
	if (ip_hlen + sizeof(udp) + offsetof(struct bootp, vend) > plen)
		return false;

	/* Sanity. */
	memcpy(&udp, (char *)ip + ip_hlen, sizeof(udp));
	if (ntohs(udp.uh_ulen) < sizeof(udp))
		return false;
	if (ip_hlen + ntohs(udp.uh_ulen) > plen)
		return false;

	/* Check it's to and from the right ports. */
	if (udp.uh_dport != htons(BOOTPC) || udp.uh_sport != htons(BOOTPS))
		return false;

	return true;
}

/* Lengths have already been checked. */
static bool
checksums_valid(void *packet,
    struct in_addr *from, unsigned int flags)
{
	struct ip *ip = packet;
	union pip {
		struct ip ip;
		uint16_t w[sizeof(struct ip) / 2];
	} pip = {
		.ip.ip_p = IPPROTO_UDP,
		.ip.ip_src = ip->ip_src,
		.ip.ip_dst = ip->ip_dst,
	};
	size_t ip_hlen;
	struct udphdr udp;
	char *udpp, *uh_sump;
	uint32_t csum;

	if (from != NULL)
		from->s_addr = ip->ip_src.s_addr;

	ip_hlen = (size_t)ip->ip_hl * 4;
	if (in_cksum(ip, ip_hlen, NULL) != 0)
		return false;

	if (flags & BPF_PARTIALCSUM)
		return true;

	udpp = (char *)ip + ip_hlen;
	memcpy(&udp, udpp, sizeof(udp));
	if (udp.uh_sum == 0)
		return true;

	/* UDP checksum is based on a pseudo IP header alongside
	 * the UDP header and payload. */
	pip.ip.ip_len = udp.uh_ulen;
	csum = 0;

	/* Need to zero the UDP sum in the packet for the checksum to work. */
	uh_sump = udpp + offsetof(struct udphdr, uh_sum);
	memset(uh_sump, 0, sizeof(udp.uh_sum));

	/* Checksum pseudo header and then UDP + payload. */
	in_cksum(pip.w, sizeof(pip.w), &csum);
	csum = in_cksum(udpp, ntohs(udp.uh_ulen), &csum);

#if 0	/* Not needed, just here for completeness. */
	/* Put the checksum back. */
	memcpy(uh_sump, &udp.uh_sum, sizeof(udp.uh_sum));
#endif

	return csum == udp.uh_sum;
}

static void
dhcp_handlebootp(struct interface *ifp, struct bootp *bootp, size_t len,
    struct in_addr *from)
{
	size_t v;

	if (len < offsetof(struct bootp, vend)) {
		logerrx("%s: truncated packet (%zu) from %s",
		    ifp->name, len, inet_ntoa(*from));
		return;
	}

	/* To make our IS_DHCP macro easy, ensure the vendor
	 * area has at least 4 octets. */
	v = len - offsetof(struct bootp, vend);
	while (v < 4) {
		bootp->vend[v++] = '\0';
		len++;
	}

	dhcp_handledhcp(ifp, bootp, len, from);
}

static void
dhcp_packet(struct interface *ifp, uint8_t *data, size_t len)
{
	struct bootp *bootp;
	struct in_addr from;
	size_t udp_len;
	const struct dhcp_state *state = D_CSTATE(ifp);

	/* Validate filter. */
	if (!is_packet_udp_bootp(data, len)) {
#ifdef BPF_DEBUG
		logerrx("%s: DHCP BPF validation failure", ifp->name);
#endif
		return;
	}

	if (!checksums_valid(data, &from, state->bpf_flags)) {
		logerrx("%s: checksum failure from %s",
		    ifp->name, inet_ntoa(from));
		return;
	}

	/*
	 * DHCP has a variable option area rather than a fixed vendor area.
	 * Because DHCP uses the BOOTP protocol it should still send BOOTP
	 * sized packets to be RFC compliant.
	 * However some servers send a truncated vendor area.
	 * dhcpcd can work fine without the vendor area being sent.
	 */
	bootp = get_udp_data(data, &udp_len);
	dhcp_handlebootp(ifp, bootp, udp_len, &from);
}

static void
dhcp_readbpf(void *arg)
{
	struct interface *ifp = arg;
	uint8_t buf[FRAMELEN_MAX];
	ssize_t bytes;
	struct dhcp_state *state = D_STATE(ifp);
	ssize_t fl = (ssize_t)bpf_frame_header_len(ifp);

	/* Some RAW mechanisms are generic file descriptors, not sockets.
	 * This means we have no kernel call to just get one packet,
	 * so we have to process the entire buffer. */
	state->bpf_flags &= ~BPF_EOF;
	state->bpf_flags |= BPF_READING;
	while (!(state->bpf_flags & BPF_EOF)) {
		bytes = bpf_read(ifp, state->bpf_fd, buf, sizeof(buf),
				 &state->bpf_flags);
		if (bytes == -1) {
			if (state->state != DHS_NONE) {
				logerr("%s: %s", __func__, ifp->name);
				dhcp_close(ifp);
			}
			break;
		}
		if (fl != 0) {
			if (bytes < fl) {
				logerrx("%s: %s: short frame header",
				    __func__, ifp->name);
				break;
			}
			bytes -= fl;
			memmove(buf, buf + fl, (size_t)bytes);
		}
		dhcp_packet(ifp, buf, (size_t)bytes);
		/* Check we still have a state after processing. */
		if ((state = D_STATE(ifp)) == NULL)
			break;
	}
	if (state != NULL)
		state->bpf_flags &= ~BPF_READING;
}

static void
dhcp_recvmsg(struct dhcpcd_ctx *ctx, struct msghdr *msg)
{
	struct sockaddr_in *from = (struct sockaddr_in *)msg->msg_name;
	struct iovec *iov = &msg->msg_iov[0];
	struct interface *ifp;
	const struct dhcp_state *state;

	ifp = if_findifpfromcmsg(ctx, msg, NULL);
	if (ifp == NULL) {
		logerr(__func__);
		return;
	}
	state = D_CSTATE(ifp);
	if (state == NULL) {
		logdebugx("%s: received BOOTP for inactive interface",
		    ifp->name);
		return;
	}

	if (state->bpf_fd != -1) {
		/* Avoid a duplicate read if BPF is open for the interface. */
		return;
	}

	dhcp_handlebootp(ifp, (struct bootp *)iov->iov_base, iov->iov_len,
	    &from->sin_addr);
}

static void
dhcp_readudp(struct dhcpcd_ctx *ctx, struct interface *ifp)
{
	const struct dhcp_state *state;
	struct sockaddr_in from;
	unsigned char buf[10 * 1024]; /* Maximum MTU */
	struct iovec iov = {
		.iov_base = buf,
		.iov_len = sizeof(buf),
	};
	union {
		struct cmsghdr hdr;
#ifdef IP_RECVIF
		uint8_t buf[CMSG_SPACE(sizeof(struct sockaddr_dl))];
#else
		uint8_t buf[CMSG_SPACE(sizeof(struct in_pktinfo))];
#endif
	} cmsgbuf = { .buf = { 0 } };
	struct msghdr msg = {
	    .msg_name = &from, .msg_namelen = sizeof(from),
	    .msg_iov = &iov, .msg_iovlen = 1,
	    .msg_control = cmsgbuf.buf, .msg_controllen = sizeof(cmsgbuf.buf),
	};
	int s;
	ssize_t bytes;

	if (ifp != NULL) {
		state = D_CSTATE(ifp);
		s = state->udp_fd;
	} else
		s = ctx->udp_fd;

	bytes = recvmsg(s, &msg, 0);
	if (bytes == -1) {
		logerr(__func__);
		return;
	}

	iov.iov_len = (size_t)bytes;
	dhcp_recvmsg(ctx, &msg);
}

static void
dhcp_handleudp(void *arg)
{
	struct dhcpcd_ctx *ctx = arg;

	dhcp_readudp(ctx, NULL);
}

static void
dhcp_handleifudp(void *arg)
{
	struct interface *ifp = arg;

	dhcp_readudp(ifp->ctx, ifp);
}

static int
dhcp_open(struct dhcpcd_ctx *ctx)
{

	if (ctx->udp_fd != -1 || (ctx->udp_fd = dhcp_openudp(NULL)) == -1)
		return ctx->udp_fd;

	eloop_event_add(ctx->eloop, ctx->udp_fd, dhcp_handleudp, ctx);
	return ctx->udp_fd;
}

static int
dhcp_openbpf(struct interface *ifp)
{
	struct dhcp_state *state;

	state = D_STATE(ifp);
	if (state->bpf_fd != -1)
		return 0;

	state->bpf_fd = bpf_open(ifp, bpf_bootp);
	if (state->bpf_fd == -1) {
		if (errno == ENOENT) {
			logerrx("%s not found", bpf_name);
			/* May as well disable IPv4 entirely at
			 * this point as we really need it. */
			ifp->options->options &= ~DHCPCD_IPV4;
		} else
			logerr("%s: %s", __func__, ifp->name);
		return -1;
	}

	eloop_event_add(ifp->ctx->eloop,
	    state->bpf_fd, dhcp_readbpf, ifp);
	return 0;
}

int
dhcp_dump(struct interface *ifp)
{
	struct dhcp_state *state;

	ifp->if_data[IF_DATA_DHCP] = state = calloc(1, sizeof(*state));
	if (state == NULL)
		goto eexit;
	state->bpf_fd = -1;
	dhcp_set_leasefile(state->leasefile, sizeof(state->leasefile),
	    AF_INET, ifp);
	state->new_len = read_lease(ifp, &state->new);
	if (state->new == NULL) {
		logerr("%s: %s",
		    *ifp->name ? ifp->name : state->leasefile, __func__);
		return -1;
	}
	state->reason = "DUMP";
	return script_runreason(ifp, state->reason);

eexit:
	logerr(__func__);
	return -1;
}

void
dhcp_free(struct interface *ifp)
{
	struct dhcp_state *state = D_STATE(ifp);
	struct dhcpcd_ctx *ctx;

	dhcp_close(ifp);
#ifdef ARP
	arp_drop(ifp);
#endif
	if (state) {
		state->state = DHS_NONE;
		free(state->old);
		free(state->new);
		free(state->offer);
		free(state->clientid);
		free(state);
	}

	ctx = ifp->ctx;
	/* If we don't have any more DHCP enabled interfaces,
	 * close the global socket and release resources */
	if (ctx->ifaces) {
		TAILQ_FOREACH(ifp, ctx->ifaces, next) {
			state = D_STATE(ifp);
			if (state != NULL && state->state != DHS_NONE)
				break;
		}
	}
	if (ifp == NULL) {
		if (ctx->udp_fd != -1) {
			eloop_event_delete(ctx->eloop, ctx->udp_fd);
			close(ctx->udp_fd);
			ctx->udp_fd = -1;
		}

		free(ctx->opt_buffer);
		ctx->opt_buffer = NULL;
	}
}

static int
dhcp_initstate(struct interface *ifp)
{
	struct dhcp_state *state;

	state = D_STATE(ifp);
	if (state != NULL)
		return 0;

	ifp->if_data[IF_DATA_DHCP] = calloc(1, sizeof(*state));
	state = D_STATE(ifp);
	if (state == NULL)
		return -1;

	state->state = DHS_NONE;
	/* 0 is a valid fd, so init to -1 */
	state->bpf_fd = -1;
	state->udp_fd = -1;
#ifdef ARPING
	state->arping_index = -1;
#endif
	return 1;
}

static int
dhcp_init(struct interface *ifp)
{
	struct dhcp_state *state;
	const struct if_options *ifo;
	uint8_t len;
	char buf[(sizeof(ifo->clientid) - 1) * 3];

	if (dhcp_initstate(ifp) == -1)
		return -1;

	state = D_STATE(ifp);
	state->state = DHS_INIT;
	state->reason = "PREINIT";
	state->nakoff = 0;
	dhcp_set_leasefile(state->leasefile, sizeof(state->leasefile),
	    AF_INET, ifp);

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
		logdebugx("%s: using ClientID %s", ifp->name,
		    hwaddr_ntoa(state->clientid + 1, state->clientid[0],
			buf, sizeof(buf)));
	else if (ifp->hwlen)
		logdebugx("%s: using hwaddr %s", ifp->name,
		    hwaddr_ntoa(ifp->hwaddr, ifp->hwlen, buf, sizeof(buf)));
	return 0;

eexit:
	logerr(__func__);
	return -1;
}

static void
dhcp_start1(void *arg)
{
	struct interface *ifp = arg;
	struct dhcpcd_ctx *ctx = ifp->ctx;
	struct if_options *ifo = ifp->options;
	struct dhcp_state *state;
	struct stat st;
	uint32_t l;
	int nolease;

	if (!(ifo->options & DHCPCD_IPV4))
		return;

	/* Listen on *.*.*.*:bootpc so that the kernel never sends an
	 * ICMP port unreachable message back to the DHCP server.
	 * Only do this in master mode so we don't swallow messages
	 * for dhcpcd running on another interface. */
	if (ctx->options & DHCPCD_MASTER) {
		if (dhcp_open(ctx) == -1) {
			/* Don't log an error if some other process
			 * is handling this. */
			if (errno != EADDRINUSE)
				logerr("%s: dhcp_open", __func__);
		}
	}

	if (dhcp_init(ifp) == -1) {
		logerr("%s: dhcp_init", ifp->name);
		return;
	}

	state = D_STATE(ifp);
	clock_gettime(CLOCK_MONOTONIC, &state->started);
	state->interval = 0;
	free(state->offer);
	state->offer = NULL;
	state->offer_len = 0;

#ifdef ARPING
	if (ifo->arping_len && state->arping_index < ifo->arping_len) {
		struct arp_state *astate;

		astate = dhcp_arp_new(ifp, NULL);
		if (astate)
			dhcp_arp_not_found(astate);
		return;
	}
#endif

	if (ifo->options & DHCPCD_STATIC) {
		dhcp_static(ifp);
		return;
	}

	if (ifo->options & DHCPCD_INFORM) {
		dhcp_inform(ifp);
		return;
	}

	/* We don't want to read the old lease if we NAK an old test */
	nolease = state->offer && ifp->ctx->options & DHCPCD_TEST;
	if (!nolease && ifo->options & DHCPCD_DHCP) {
		state->offer_len = read_lease(ifp, &state->offer);
		/* Check the saved lease matches the type we want */
		if (state->offer) {
#ifdef IN_IFF_DUPLICATED
			struct in_addr addr;
			struct ipv4_addr *ia;

			addr.s_addr = state->offer->yiaddr;
			ia = ipv4_iffindaddr(ifp, &addr, NULL);
#endif

			if ((!IS_DHCP(state->offer) &&
			    !(ifo->options & DHCPCD_BOOTP)) ||
#ifdef IN_IFF_DUPLICATED
			    (ia && ia->addr_flags & IN_IFF_DUPLICATED) ||
#endif
			    (IS_DHCP(state->offer) &&
			    ifo->options & DHCPCD_BOOTP))
			{
				free(state->offer);
				state->offer = NULL;
				state->offer_len = 0;
			}
		}
	}
	if (state->offer) {
		struct ipv4_addr *ia;

		get_lease(ifp, &state->lease, state->offer, state->offer_len);
		state->lease.frominfo = 1;
		if (state->new == NULL &&
		    (ia = ipv4_iffindaddr(ifp,
		    &state->lease.addr, &state->lease.mask)) != NULL)
		{
			/* We still have the IP address from the last lease.
			 * Fake add the address and routes from it so the lease
			 * can be cleaned up. */
			state->new = malloc(state->offer_len);
			if (state->new) {
				memcpy(state->new,
				    state->offer, state->offer_len);
				state->new_len = state->offer_len;
				state->addr = ia;
				state->added |= STATE_ADDED | STATE_FAKE;
				rt_build(ifp->ctx, AF_INET);
			} else
				logerr(__func__);
		}
		if (!IS_DHCP(state->offer)) {
			free(state->offer);
			state->offer = NULL;
			state->offer_len = 0;
		} else if (!(ifo->options & DHCPCD_LASTLEASE_EXTEND) &&
		    state->lease.leasetime != DHCP_INFINITE_LIFETIME &&
		    stat(state->leasefile, &st) == 0)
		{
			time_t now;

			/* Offset lease times and check expiry */
			now = time(NULL);
			if (now == -1 ||
			    (time_t)state->lease.leasetime < now - st.st_mtime)
			{
				logdebugx("%s: discarding expired lease",
				    ifp->name);
				free(state->offer);
				state->offer = NULL;
				state->offer_len = 0;
				state->lease.addr.s_addr = 0;
				/* Technically we should discard the lease
				 * as it's expired, just as DHCPv6 addresses
				 * would be by the kernel.
				 * However, this may violate POLA so
				 * we currently leave it be.
				 * If we get a totally different lease from
				 * the DHCP server we'll drop it anyway, as
				 * we will on any other event which would
				 * trigger a lease drop.
				 * This should only happen if dhcpcd stops
				 * running and the lease expires before
				 * dhcpcd starts again. */
#if 0
				if (state->new)
					dhcp_drop(ifp, "EXPIRE");
#endif
			} else {
				l = (uint32_t)(now - st.st_mtime);
				state->lease.leasetime -= l;
				state->lease.renewaltime -= l;
				state->lease.rebindtime -= l;
			}
		}
	}

#ifdef IPV4LL
	if (!(ifo->options & DHCPCD_DHCP)) {
		if (ifo->options & DHCPCD_IPV4LL)
			ipv4ll_start(ifp);
		return;
	}
#endif

	if (state->offer == NULL || !IS_DHCP(state->offer))
		dhcp_discover(ifp);
	else
		dhcp_reboot(ifp);
}

void
dhcp_start(struct interface *ifp)
{
	struct timespec tv;
#ifdef ARPING
	const struct dhcp_state *state;
#endif

	if (!(ifp->options->options & DHCPCD_IPV4))
		return;

	/* If we haven't been given a netmask for our requested address,
	 * set it now. */
	if (ifp->options->req_addr.s_addr != INADDR_ANY &&
	    ifp->options->req_mask.s_addr == INADDR_ANY)
		ifp->options->req_mask.s_addr =
		    ipv4_getnetmask(ifp->options->req_addr.s_addr);

	/* If we haven't specified a ClientID and our hardware address
	 * length is greater than BOOTP CHADDR then we enforce a ClientID
	 * of the hardware address family and the hardware address.
	 * If there is no hardware address and no ClientID set,
	 * force a DUID based ClientID. */
	if (ifp->hwlen > 16)
		ifp->options->options |= DHCPCD_CLIENTID;
	else if (ifp->hwlen == 0 && !(ifp->options->options & DHCPCD_CLIENTID))
		ifp->options->options |= DHCPCD_CLIENTID | DHCPCD_DUID;

	/* Firewire and InfiniBand interfaces require ClientID and
	 * the broadcast option being set. */
	switch (ifp->family) {
	case ARPHRD_IEEE1394:	/* FALLTHROUGH */
	case ARPHRD_INFINIBAND:
		ifp->options->options |= DHCPCD_CLIENTID | DHCPCD_BROADCAST;
		break;
	}

	/* If we violate RFC2131 section 3.7 then require ARP
	 * to detect if any other client wants our address. */
	if (ifp->options->options & DHCPCD_LASTLEASE_EXTEND)
		ifp->options->options |= DHCPCD_ARP;

	/* No point in delaying a static configuration */
	if (ifp->options->options & DHCPCD_STATIC ||
	    !(ifp->options->options & DHCPCD_INITIAL_DELAY))
	{
		dhcp_start1(ifp);
		return;
	}

#ifdef ARPING
	/* If we have arpinged then we have already delayed. */
	state = D_CSTATE(ifp);
	if (state != NULL && state->arping_index != -1) {
		dhcp_start1(ifp);
		return;
	}
#endif

	tv.tv_sec = DHCP_MIN_DELAY;
	tv.tv_nsec = (suseconds_t)arc4random_uniform(
	    (DHCP_MAX_DELAY - DHCP_MIN_DELAY) * NSEC_PER_SEC);
	timespecnorm(&tv);
	logdebugx("%s: delaying IPv4 for %0.1f seconds",
	    ifp->name, timespec_to_double(&tv));

	eloop_timeout_add_tv(ifp->ctx->eloop, &tv, dhcp_start1, ifp);
}

void
dhcp_abort(struct interface *ifp)
{
	struct dhcp_state *state;

	state = D_STATE(ifp);
#ifdef ARPING
	if (state != NULL)
		state->arping_index = -1;
#endif

	eloop_timeout_delete(ifp->ctx->eloop, dhcp_start1, ifp);

	if (state != NULL && state->added) {
		rt_build(ifp->ctx, AF_INET);
#ifdef ARP
		arp_announceaddr(ifp->ctx, &state->addr->addr);
#endif
	}
}

struct ipv4_addr *
dhcp_handleifa(int cmd, struct ipv4_addr *ia, pid_t pid)
{
	struct interface *ifp;
	struct dhcp_state *state;
	struct if_options *ifo;
	uint8_t i;

	ifp = ia->iface;
	state = D_STATE(ifp);
	if (state == NULL || state->state == DHS_NONE)
		return ia;

	if (cmd == RTM_DELADDR) {
		if (state->addr == ia) {
			loginfox("%s: pid %d deleted IP address %s",
			    ifp->name, pid, ia->saddr);
			state->addr = NULL;
			/* Don't clear the added state as we need
			 * to drop the lease. */
			dhcp_drop(ifp, "EXPIRE");
			dhcp_start1(ifp);
			return ia;
		}
	}

	if (cmd != RTM_NEWADDR)
		return ia;

#ifdef IN_IFF_NOTUSEABLE
	if (!(ia->addr_flags & IN_IFF_NOTUSEABLE))
		dhcp_finish_dad(ifp, &ia->addr);
	else if (ia->addr_flags & IN_IFF_DUPLICATED)
		return dhcp_addr_duplicated(ifp, &ia->addr) ? NULL : ia;
#endif

	ifo = ifp->options;
	if (ifo->options & DHCPCD_INFORM) {
		if (state->state != DHS_INFORM)
			dhcp_inform(ifp);
		return ia;
	}

	if (!(ifo->options & DHCPCD_STATIC))
		return ia;
	if (ifo->req_addr.s_addr != INADDR_ANY)
		return ia;

	free(state->old);
	state->old = state->new;
	state->new_len = dhcp_message_new(&state->new, &ia->addr, &ia->mask);
	if (state->new == NULL)
		return ia;
	if (ifp->flags & IFF_POINTOPOINT) {
		for (i = 1; i < 255; i++)
			if (i != DHO_ROUTER && has_option_mask(ifo->dstmask,i))
				dhcp_message_add_addr(state->new, i, ia->brd);
	}
	state->reason = "STATIC";
	rt_build(ifp->ctx, AF_INET);
	script_runreason(ifp, state->reason);
	if (ifo->options & DHCPCD_INFORM) {
		state->state = DHS_INFORM;
		dhcp_new_xid(ifp);
		state->lease.server.s_addr = INADDR_ANY;
		state->addr = ia;
		dhcp_inform(ifp);
	}

	return ia;
}
