#include <sys/cdefs.h>
 __RCSID("$NetBSD: dhcp6.c,v 1.1.1.9 2014/03/14 11:27:37 roy Exp $");

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

/* TODO: We should decline dupliate addresses detected */

#include <sys/stat.h>
#include <sys/utsname.h>

#include <netinet/in.h>
#ifdef __linux__
#  define _LINUX_IN6_H
#  include <linux/ipv6.h>
#endif

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>

#define ELOOP_QUEUE 3

#include "config.h"
#include "common.h"
#include "dhcp.h"
#include "dhcp6.h"
#include "duid.h"
#include "eloop.h"
#include "ipv6nd.h"
#include "platform.h"
#include "script.h"

#ifndef __UNCONST
#define __UNCONST(a) ((void *)(unsigned long)(const void *)(a))
#endif

/* DHCPCD Project has been assigned an IANA PEN of 40712 */
#define DHCPCD_IANA_PEN 40712

/* Unsure if I want this */
//#define VENDOR_SPLIT

struct dhcp6_op {
	uint16_t type;
	const char *name;
};

static const struct dhcp6_op dhcp6_ops[] = {
	{ DHCP6_SOLICIT, "SOLICIT6" },
	{ DHCP6_ADVERTISE, "ADVERTISE6" },
	{ DHCP6_REQUEST, "REQUEST6" },
	{ DHCP6_REPLY, "REPLY6" },
	{ DHCP6_RENEW, "RENEW6" },
	{ DHCP6_REBIND, "REBIND6" },
	{ DHCP6_CONFIRM, "CONFIRM6" },
	{ DHCP6_INFORMATION_REQ, "INFORM6" },
	{ DHCP6_RELEASE, "RELEASE6" },
	{ DHCP6_RECONFIGURE, "RECONFIURE6" },
	{ 0, NULL }
};

struct dhcp_compat {
	uint8_t dhcp_opt;
	uint16_t dhcp6_opt;
};

const struct dhcp_compat dhcp_compats[] = {
	{ DHO_DNSSERVER,	D6_OPTION_DNS_SERVERS },
	{ DHO_HOSTNAME,		D6_OPTION_FQDN },
	{ DHO_DNSDOMAIN,	D6_OPTION_FQDN },
	{ DHO_NISSERVER,	D6_OPTION_NIS_SERVERS },
	{ DHO_NTPSERVER,	D6_OPTION_SNTP_SERVERS },
	{ DHO_RAPIDCOMMIT,	D6_OPTION_RAPID_COMMIT },
	{ DHO_FQDN,		D6_OPTION_FQDN },
	{ DHO_VIVCO,		D6_OPTION_VENDOR_CLASS },
	{ DHO_VIVSO,		D6_OPTION_VENDOR_OPTS },
	{ DHO_DNSSEARCH,	D6_OPTION_DOMAIN_LIST },
	{ 0, 0 }
};

static const char * const dhcp6_statuses[] = {
	"Success",
	"Unspecified Failure",
	"No Addresses Available",
	"No Binding",
	"Not On Link",
	"Use Multicast"
};

void
dhcp6_printoptions(const struct dhcpcd_ctx *ctx)
{
	size_t i;
	const struct dhcp_opt *opt;

	for (i = 0, opt = ctx->dhcp6_opts;
	    i < ctx->dhcp6_opts_len; i++, opt++)
		printf("%05d %s\n", opt->option, opt->var);
}

static size_t
dhcp6_makevendor(struct dhcp6_option *o, const struct interface *ifp)
{
	const struct if_options *ifo;
	size_t len;
	uint8_t *p;
	uint16_t u16;
	uint32_t u32;
	size_t vlen, i;
	const struct vivco *vivco;
	char vendor[VENDORCLASSID_MAX_LEN];

	ifo = ifp->options;
	len = sizeof(uint32_t); /* IANA PEN */
	if (ifo->vivco_en) {
		for (i = 0, vivco = ifo->vivco;
		    i < ifo->vivco_len;
		    i++, vivco++)
			len += sizeof(uint16_t) + vivco->len;
		vlen = 0; /* silence bogus gcc warning */
	} else {
		vlen = dhcp_vendor(vendor, sizeof(vendor));
		len += sizeof(uint16_t) + vlen;
	}

	if (len > UINT16_MAX) {
		syslog(LOG_ERR, "%s: DHCPv6 Vendor Class too big", ifp->name);
		return 0;
	}

	if (o) {
		o->code = htons(D6_OPTION_VENDOR_CLASS);
		o->len = htons(len);
		p = D6_OPTION_DATA(o);
		u32 = htonl(ifo->vivco_en ? ifo->vivco_en : DHCPCD_IANA_PEN);
		memcpy(p, &u32, sizeof(u32));
		p += sizeof(u32);
		if (ifo->vivco_en) {
			for (i = 0, vivco = ifo->vivco;
			    i < ifo->vivco_len;
			    i++, vivco++)
			{
				u16 = htons(vivco->len);
				memcpy(p, &u16, sizeof(u16));
				p += sizeof(u16);
				memcpy(p, vivco->data, vivco->len);
				p += vivco->len;
			}
		} else {
			u16 = htons(vlen);
			memcpy(p, &u16, sizeof(u16));
			p += sizeof(u16);
			memcpy(p, vendor, vlen);
		}
	}

	return len;
}

static const struct dhcp6_option *
dhcp6_findoption(int code, const uint8_t *d, ssize_t len)
{
	const struct dhcp6_option *o;

	code = htons(code);
	for (o = (const struct dhcp6_option *)d;
	    len > (ssize_t)sizeof(*o);
	    o = D6_CNEXT_OPTION(o))
	{
		len -= sizeof(*o) + ntohs(o->len);
		if (len < 0) {
			errno = EINVAL;
			return NULL;
		}
		if (o->code == code)
			return o;
	}

	errno = ESRCH;
	return NULL;
}

static const uint8_t *
dhcp6_getoption(struct dhcpcd_ctx *ctx,
    unsigned int *os, unsigned int *code, unsigned int *len,
    const uint8_t *od, unsigned int ol, struct dhcp_opt **oopt)
{
	const struct dhcp6_option *o;
	size_t i;
	struct dhcp_opt *opt;

	if (od) {
		*os = sizeof(*o);
		if (ol < *os) {
			errno = EINVAL;
			return NULL;
		}
		o = (const struct dhcp6_option *)od;
		*len = ntohs(o->len);
		if (*len > ol) {
			errno = EINVAL;
			return NULL;
		}
		*code = ntohs(o->code);
	} else
		o = NULL;

	for (i = 0, opt = ctx->dhcp6_opts;
	    i < ctx->dhcp6_opts_len; i++, opt++)
	{
		if (opt->option == *code) {
			*oopt = opt;
			break;
		}
	}

	if (o)
		return D6_COPTION_DATA(o);
	return NULL;
}

static const struct dhcp6_option *
dhcp6_getmoption(int code, const struct dhcp6_message *m, ssize_t len)
{

	len -= sizeof(*m);
	return dhcp6_findoption(code,
	    (const uint8_t *)D6_CFIRST_OPTION(m), len);
}

static int
dhcp6_updateelapsed(struct interface *ifp, struct dhcp6_message *m, ssize_t len)
{
	struct dhcp6_state *state;
	const struct dhcp6_option *co;
	struct dhcp6_option *o;
	time_t up;
	uint16_t u16;

	co = dhcp6_getmoption(D6_OPTION_ELAPSED, m, len);
	if (co == NULL)
		return -1;

	o = __UNCONST(co);
	state = D6_STATE(ifp);
	up = uptime() - state->start_uptime;
	if (up < 0 || up > (time_t)UINT16_MAX)
		up = (time_t)UINT16_MAX;
	u16 = htons(up);
	memcpy(D6_OPTION_DATA(o), &u16, sizeof(u16));
	return 0;
}

static void
dhcp6_newxid(const struct interface *ifp, struct dhcp6_message *m)
{
	uint32_t xid;

	if (ifp->options->options & DHCPCD_XID_HWADDR &&
	    ifp->hwlen >= sizeof(xid))
		/* The lower bits are probably more unique on the network */
		memcpy(&xid, (ifp->hwaddr + ifp->hwlen) - sizeof(xid),
		    sizeof(xid));
	else
		xid = arc4random();

	m->xid[0] = (xid >> 16) & 0xff;
	m->xid[1] = (xid >> 8) & 0xff;
	m->xid[2] = xid & 0xff;
}

static int
dhcp6_makemessage(struct interface *ifp)
{
	struct dhcp6_state *state;
	struct dhcp6_message *m;
	struct dhcp6_option *o, *so;
	const struct dhcp6_option *si, *unicast;
	ssize_t len, ml;
	size_t l;
	int auth_len;
	uint8_t u8;
	uint16_t *u16, n_options, type;
	struct if_options *ifo;
	const struct dhcp_opt *opt;
	uint8_t IA, *p;
	uint32_t u32;
	const struct ipv6_addr *ap;
	char hbuf[HOSTNAME_MAX_LEN + 1];
	const char *hostname;
	int fqdn;

	state = D6_STATE(ifp);
	if (state->send) {
		free(state->send);
		state->send = NULL;
	}

	ifo = ifp->options;
	fqdn = ifo->fqdn;

	if (fqdn == FQDN_DISABLE && ifo->options & DHCPCD_HOSTNAME) {
		/* We're sending the DHCPv4 hostname option, so send FQDN as
		 * DHCPv6 has no FQDN option and DHCPv4 must not send
		 * hostname and FQDN according to RFC4702 */
		fqdn = FQDN_BOTH;
	}
	if (fqdn != FQDN_DISABLE) {
		if (ifo->hostname[0] == '\0')
			hostname = get_hostname(hbuf, sizeof(hbuf),
			    ifo->options & DHCPCD_HOSTNAME_SHORT ? 1 : 0);
		else
			hostname = ifo->hostname;
	} else
		hostname = NULL; /* appearse gcc */

	/* Work out option size first */
	n_options = 0;
	len = 0;
	si = NULL;
	if (state->state != DH6S_RELEASE) {
		for (l = 0, opt = ifp->ctx->dhcp6_opts;
		    l < ifp->ctx->dhcp6_opts_len;
		    l++, opt++)
		{
			if (!(opt->type & NOREQ) &&
			    (opt->type & REQUEST ||
			    has_option_mask(ifo->requestmask6, opt->option)))
			{
				n_options++;
				len += sizeof(*u16);
			}
		}
		if (len)
			len += sizeof(*o);

		if (fqdn != FQDN_DISABLE)
			len += sizeof(*o) + 1 + encode_rfc1035(hostname, NULL);

		if ((ifo->auth.options & DHCPCD_AUTH_SENDREQUIRE) !=
		    DHCPCD_AUTH_SENDREQUIRE)
			len += sizeof(*o); /* Reconfigure Accept */
	}

	len += sizeof(*state->send);
	len += sizeof(*o) + ifp->ctx->duid_len;
	len += sizeof(*o) + sizeof(uint16_t); /* elapsed */
	len += sizeof(*o) + dhcp6_makevendor(NULL, ifp);

	/* IA */
	m = NULL;
	ml = 0;
	switch(state->state) {
	case DH6S_REQUEST:
		m = state->recv;
		ml = state->recv_len;
		/* FALLTHROUGH */
	case DH6S_RELEASE:
		/* FALLTHROUGH */
	case DH6S_RENEW:
		if (m == NULL) {
			m = state->new;
			ml = state->new_len;
		}
		si = dhcp6_getmoption(D6_OPTION_SERVERID, m, ml);
		if (si == NULL) {
			errno = ESRCH;
			return -1;
		}
		len += sizeof(*si) + ntohs(si->len);
		/* FALLTHROUGH */
	case DH6S_REBIND:
		/* FALLTHROUGH */
	case DH6S_CONFIRM:
		if (m == NULL) {
			m = state->new;
			ml = state->new_len;
		}
		TAILQ_FOREACH(ap, &state->addrs, next) {
			if (ap->prefix_vltime == 0)
				continue;
			if (ifo->ia_type == D6_OPTION_IA_PD)
				len += sizeof(*o) + sizeof(u8) +
				    sizeof(u32) + sizeof(u32) +
				    sizeof(ap->prefix.s6_addr);
			else
				len += sizeof(*o) + sizeof(ap->addr.s6_addr) +
				    sizeof(u32) + sizeof(u32);
		}
		/* FALLTHROUGH */
	case DH6S_INIT: /* FALLTHROUGH */
	case DH6S_DISCOVER:
		len += ifo->ia_len * (sizeof(*o) + (sizeof(u32) * 3));
		IA = 1;
		break;
	default:
		IA = 0;
	}

	if (state->state == DH6S_DISCOVER &&
	    !(ifp->ctx->options & DHCPCD_TEST) &&
	    has_option_mask(ifo->requestmask6, D6_OPTION_RAPID_COMMIT))
		len += sizeof(*o);

	if (m == NULL) {
		m = state->new;
		ml = state->new_len;
	}
	unicast = NULL;
	/* Depending on state, get the unicast address */
	switch(state->state) {
		break;
	case DH6S_INIT: /* FALLTHROUGH */
	case DH6S_DISCOVER:
		type = DHCP6_SOLICIT;
		break;
	case DH6S_REQUEST:
		type = DHCP6_REQUEST;
		unicast = dhcp6_getmoption(D6_OPTION_UNICAST, m, ml);
		break;
	case DH6S_CONFIRM:
		type = DHCP6_CONFIRM;
		break;
	case DH6S_REBIND:
		type = DHCP6_REBIND;
		break;
	case DH6S_RENEW:
		type = DHCP6_RENEW;
		unicast = dhcp6_getmoption(D6_OPTION_UNICAST, m, ml);
		break;
	case DH6S_INFORM:
		type = DHCP6_INFORMATION_REQ;
		break;
	case DH6S_RELEASE:
		type = DHCP6_RELEASE;
		unicast = dhcp6_getmoption(D6_OPTION_UNICAST, m, ml);
		break;
	default:
		errno = EINVAL;
		return -1;
	}

	if (ifo->auth.options & DHCPCD_AUTH_SEND) {
		auth_len = dhcp_auth_encode(&ifo->auth, state->auth.token,
		    NULL, 0, 6, type, NULL, 0);
		if (auth_len > 0)
			len += sizeof(*o) + auth_len;
	} else
		auth_len = 0; /* appease GCC */

	state->send = malloc(len);
	if (state->send == NULL)
		return -1;

	state->send_len = len;
	state->send->type = type;

	/* If we found a unicast option, copy it to our state for sending */
	if (unicast && ntohs(unicast->len) == sizeof(state->unicast.s6_addr))
		memcpy(&state->unicast.s6_addr, D6_COPTION_DATA(unicast),
		    sizeof(state->unicast.s6_addr));
	else
		state->unicast = in6addr_any;

	dhcp6_newxid(ifp, state->send);

	o = D6_FIRST_OPTION(state->send);
	o->code = htons(D6_OPTION_CLIENTID);
	o->len = htons(ifp->ctx->duid_len);
	memcpy(D6_OPTION_DATA(o), ifp->ctx->duid, ifp->ctx->duid_len);

	if (si) {
		o = D6_NEXT_OPTION(o);
		memcpy(o, si, sizeof(*si) + ntohs(si->len));
	}

	o = D6_NEXT_OPTION(o);
	o->code = htons(D6_OPTION_ELAPSED);
	o->len = htons(sizeof(uint16_t));
	p = D6_OPTION_DATA(o);
	memset(p, 0, sizeof(u16));

	o = D6_NEXT_OPTION(o);
	dhcp6_makevendor(o, ifp);

	if (state->state == DH6S_DISCOVER &&
	    !(ifp->ctx->options & DHCPCD_TEST) &&
	    has_option_mask(ifo->requestmask6, D6_OPTION_RAPID_COMMIT))
	{
		o = D6_NEXT_OPTION(o);
		o->code = htons(D6_OPTION_RAPID_COMMIT);
		o->len = 0;
	}

	for (l = 0; IA && l < ifo->ia_len; l++) {
		o = D6_NEXT_OPTION(o);
		o->code = htons(ifo->ia_type);
		o->len = htons(sizeof(u32) + sizeof(u32) + sizeof(u32));
		p = D6_OPTION_DATA(o);
		memcpy(p, ifo->ia[l].iaid, sizeof(u32));
		p += sizeof(u32);
		memset(p, 0, sizeof(u32) + sizeof(u32));
		TAILQ_FOREACH(ap, &state->addrs, next) {
			if (ap->prefix_vltime == 0)
				continue;
			if (memcmp(ifo->ia[l].iaid, ap->iaid, sizeof(u32)))
				continue;
			so = D6_NEXT_OPTION(o);
			if (ifo->ia_type == D6_OPTION_IA_PD) {
				so->code = htons(D6_OPTION_IAPREFIX);
				so->len = htons(sizeof(ap->prefix.s6_addr) +
				    sizeof(u32) + sizeof(u32) + sizeof(u8));
				p = D6_OPTION_DATA(so);
				u32 = htonl(ap->prefix_pltime);
				memcpy(p, &u32, sizeof(u32));
				p += sizeof(u32);
				u32 = htonl(ap->prefix_vltime);
				memcpy(p, &u32, sizeof(u32));
				p += sizeof(u32);
				u8 = ap->prefix_len;
				memcpy(p, &u8, sizeof(u8));
				p += sizeof(u8);
				memcpy(p, &ap->prefix.s6_addr,
				    sizeof(ap->prefix.s6_addr));
				/* Avoid a shadowed declaration warning by
				 * moving our addition outside of the htons
				 * macro */
				u32 = ntohs(o->len) + sizeof(*so)
				    + ntohs(so->len);
				o->len = htons(u32);
			} else {
				so->code = htons(D6_OPTION_IA_ADDR);
				so->len = htons(sizeof(ap->addr.s6_addr) +
				    sizeof(u32) + sizeof(u32));
				p = D6_OPTION_DATA(so);
				memcpy(p, &ap->addr.s6_addr,
				    sizeof(ap->addr.s6_addr));
				p += sizeof(ap->addr.s6_addr);
				u32 = htonl(ap->prefix_pltime);
				memcpy(p, &u32, sizeof(u32));
				p += sizeof(u32);
				u32 = htonl(ap->prefix_vltime);
				memcpy(p, &u32, sizeof(u32));
				/* Avoid a shadowed declaration warning by
				 * moving our addition outside of the htons
				 * macro */
				u32 = ntohs(o->len) + sizeof(*so)
				    + ntohs(so->len);
				o->len = htons(u32);
			}
		}
	}

	if (state->send->type != DHCP6_RELEASE) {
		if (fqdn != FQDN_DISABLE) {
			o = D6_NEXT_OPTION(o);
			o->code = htons(D6_OPTION_FQDN);
			p = D6_OPTION_DATA(o);
			switch (fqdn) {
			case FQDN_BOTH:
				*p = D6_FQDN_BOTH;
				break;
			case FQDN_PTR:
				*p = D6_FQDN_PTR;
				break;
			default:
				*p = D6_FQDN_NONE;
				break;
			}
			l = encode_rfc1035(hostname, p + 1);
			if (l == 0)
				*p = D6_FQDN_NONE;
			o->len = htons(l + 1);
		}

		if ((ifo->auth.options & DHCPCD_AUTH_SENDREQUIRE) !=
		    DHCPCD_AUTH_SENDREQUIRE)
		{
			o = D6_NEXT_OPTION(o);
			o->code = htons(D6_OPTION_RECONF_ACCEPT);
			o->len = 0;
		}

		if (n_options) {
			o = D6_NEXT_OPTION(o);
			o->code = htons(D6_OPTION_ORO);
			o->len = 0;
			u16 = (uint16_t *)(void *)D6_OPTION_DATA(o);
			for (l = 0, opt = ifp->ctx->dhcp6_opts;
			    l < ifp->ctx->dhcp6_opts_len;
			    l++, opt++)
			{
				if (!(opt->type & NOREQ) &&
				    (opt->type & REQUEST ||
				    has_option_mask(ifo->requestmask6,
				        opt->option)))
				{
					*u16++ = htons(opt->option);
					o->len += sizeof(*u16);
				}
			}
			o->len = htons(o->len);
		}
	}

	/* This has to be the last option */
	if (ifo->auth.options & DHCPCD_AUTH_SEND && auth_len > 0) {
		o = D6_NEXT_OPTION(o);
		o->code = htons(D6_OPTION_AUTH);
		o->len = htons(auth_len);
		/* data will be filled at send message time */
	}

	return 0;
}

static const char *
dhcp6_get_op(uint16_t type)
{
	const struct dhcp6_op *d;

	for (d = dhcp6_ops; d->name; d++)
		if (d->type == type)
			return d->name;
	return NULL;
}

static void
dhcp6_freedrop_addrs(struct interface *ifp, int drop,
    const struct interface *ifd)
{
	struct dhcp6_state *state;

	state = D6_STATE(ifp);
	if (state) {
		ipv6_freedrop_addrs(&state->addrs, drop, ifd);
		if (drop)
			ipv6_buildroutes(ifp->ctx);
	}
}

static void dhcp6_delete_delegates(struct interface *ifp)
{
	struct interface *ifp0;

	if (ifp->ctx->ifaces) {
		TAILQ_FOREACH(ifp0, ifp->ctx->ifaces, next) {
			if (ifp0 != ifp)
				dhcp6_freedrop_addrs(ifp0, 1, ifp);
		}
	}
}


static int
dhcp6_update_auth(struct interface *ifp, struct dhcp6_message *m, ssize_t len)
{
	struct dhcp6_state *state;
	const struct dhcp6_option *co;
	struct dhcp6_option *o;

	co = dhcp6_getmoption(D6_OPTION_AUTH, m, len);
	if (co == NULL)
		return -1;

	o = __UNCONST(co);
	state = D6_STATE(ifp);

	return dhcp_auth_encode(&ifp->options->auth, state->auth.token,
	    (uint8_t *)state->send, state->send_len,
	    6, state->send->type,
	    D6_OPTION_DATA(o), ntohs(o->len));
}

static int
dhcp6_sendmessage(struct interface *ifp, void (*callback)(void *))
{
	struct dhcp6_state *state;
	struct ipv6_ctx *ctx;
	struct sockaddr_in6 dst;
	struct cmsghdr *cm;
	struct in6_pktinfo pi;
	struct timeval RTprev;
	double rnd;
	suseconds_t ms;
	uint8_t neg;
	const char *broad_uni;
	const struct in6_addr alldhcp = IN6ADDR_LINKLOCAL_ALLDHCP_INIT;

	memset(&dst, 0, sizeof(dst));
	dst.sin6_family = AF_INET6;
	dst.sin6_port = htons(DHCP6_SERVER_PORT);
#ifdef SIN6_LEN
	dst.sin6_len = sizeof(dst);
#endif

	state = D6_STATE(ifp);
	/* We need to ensure we have sufficient scope to unicast the address */
	/* XXX FIXME: We should check any added addresses we have like from
	 * a Router Advertisement */
	if (IN6_IS_ADDR_UNSPECIFIED(&state->unicast) ||
	    (state->state == DH6S_REQUEST &&
	    (!IN6_IS_ADDR_LINKLOCAL(&state->unicast) || !ipv6_linklocal(ifp))))
	{
		dst.sin6_addr = alldhcp;
		broad_uni = "broadcasting";
	} else {
		dst.sin6_addr = state->unicast;
		broad_uni = "unicasting";
	}

	if (!callback)
		syslog(LOG_DEBUG, "%s: %s %s with xid 0x%02x%02x%02x",
		    ifp->name,
		    broad_uni,
		    dhcp6_get_op(state->send->type),
		    state->send->xid[0],
		    state->send->xid[1],
		    state->send->xid[2]);
	else {
		if (state->IMD) {
			/* Some buggy PPP servers close the link too early
			 * after sending an invalid status in their reply
			 * which means this host won't see it.
			 * 1 second grace seems to be the sweet spot. */
			if (ifp->flags & IFF_POINTOPOINT)
				state->RT.tv_sec = 1;
			else
				state->RT.tv_sec = 0;
			state->RT.tv_usec = arc4random() %
			    (state->IMD * 1000000);
			timernorm(&state->RT);
			broad_uni = "delaying";
			goto logsend;
		}
		if (state->RTC == 0) {
			RTprev.tv_sec = state->IRT;
			RTprev.tv_usec = 0;
			state->RT.tv_sec = RTprev.tv_sec;
			state->RT.tv_usec = 0;
		} else {
			RTprev = state->RT;
			timeradd(&state->RT, &state->RT, &state->RT);
		}

		rnd = DHCP6_RAND_MIN;
		rnd += arc4random() % (DHCP6_RAND_MAX - DHCP6_RAND_MIN);
		rnd /= 1000;
		neg = (rnd < 0.0);
		if (neg)
			rnd = -rnd;
		tv_to_ms(ms, &RTprev);
		ms *= rnd;
		ms_to_tv(&RTprev, ms);
		if (neg)
			timersub(&state->RT, &RTprev, &state->RT);
		else
			timeradd(&state->RT, &RTprev, &state->RT);

		if (state->RT.tv_sec > state->MRT) {
			RTprev.tv_sec = state->MRT;
			RTprev.tv_usec = 0;
			state->RT.tv_sec = state->MRT;
			state->RT.tv_usec = 0;
			tv_to_ms(ms, &RTprev);
			ms *= rnd;
			ms_to_tv(&RTprev, ms);
			if (neg)
				timersub(&state->RT, &RTprev, &state->RT);
			else
				timeradd(&state->RT, &RTprev, &state->RT);
		}

logsend:
		syslog(LOG_DEBUG,
		    "%s: %s %s (xid 0x%02x%02x%02x),"
		    " next in %0.1f seconds",
		    ifp->name,
		    broad_uni,
		    dhcp6_get_op(state->send->type),
		    state->send->xid[0],
		    state->send->xid[1],
		    state->send->xid[2],
		    timeval_to_double(&state->RT));

		/* Wait the initial delay */
		if (state->IMD) {
			state->IMD = 0;
			eloop_timeout_add_tv(ifp->ctx->eloop,
			    &state->RT, callback, ifp);
			return 0;
		}
	}

	/* Update the elapsed time */
	dhcp6_updateelapsed(ifp, state->send, state->send_len);
	if (ifp->options->auth.options & DHCPCD_AUTH_SEND &&
	    dhcp6_update_auth(ifp, state->send, state->send_len) == -1)
	{
		syslog(LOG_ERR, "%s: dhcp6_updateauth: %m", ifp->name);
		return -1;
	}

	ctx = ifp->ctx->ipv6;
	dst.sin6_scope_id = ifp->index;
	ctx->sndhdr.msg_name = (caddr_t)&dst;
	ctx->sndhdr.msg_iov[0].iov_base = (caddr_t)state->send;
	ctx->sndhdr.msg_iov[0].iov_len = state->send_len;

	/* Set the outbound interface */
	cm = CMSG_FIRSTHDR(&ctx->sndhdr);
	if (cm == NULL) /* unlikely */
		return -1;
	cm->cmsg_level = IPPROTO_IPV6;
	cm->cmsg_type = IPV6_PKTINFO;
	cm->cmsg_len = CMSG_LEN(sizeof(pi));
	memset(&pi, 0, sizeof(pi));
	pi.ipi6_ifindex = ifp->index;
	memcpy(CMSG_DATA(cm), &pi, sizeof(pi));

	if (sendmsg(ctx->dhcp_fd, &ctx->sndhdr, 0) == -1) {
		syslog(LOG_ERR, "%s: %s: sendmsg: %m", ifp->name, __func__);
		ifp->options->options &= ~DHCPCD_IPV6;
		dhcp6_drop(ifp, "EXPIRE6");
		return -1;
	}

	state->RTC++;
	if (callback) {
		if (state->MRC == 0 || state->RTC < state->MRC)
			eloop_timeout_add_tv(ifp->ctx->eloop,
			    &state->RT, callback, ifp);
		else if (state->MRC != 0 && state->MRCcallback)
			eloop_timeout_add_tv(ifp->ctx->eloop,
			    &state->RT, state->MRCcallback, ifp);
		else
			syslog(LOG_WARNING, "%s: sent %d times with no reply",
			    ifp->name, state->RTC);
	}
	return 0;
}

static void
dhcp6_sendinform(void *arg)
{

	dhcp6_sendmessage(arg, dhcp6_sendinform);
}

static void
dhcp6_senddiscover(void *arg)
{

	dhcp6_sendmessage(arg, dhcp6_senddiscover);
}

static void
dhcp6_sendrequest(void *arg)
{

	dhcp6_sendmessage(arg, dhcp6_sendrequest);
}

static void
dhcp6_sendrebind(void *arg)
{

	dhcp6_sendmessage(arg, dhcp6_sendrebind);
}

static void
dhcp6_sendrenew(void *arg)
{

	dhcp6_sendmessage(arg, dhcp6_sendrenew);
}

static void
dhcp6_sendconfirm(void *arg)
{

	dhcp6_sendmessage(arg, dhcp6_sendconfirm);
}

/*
static void
dhcp6_sendrelease(void *arg)
{

	dhcp6_sendmessage(arg, dhcp6_sendrelease);
}
*/

static void
dhcp6_startrenew(void *arg)
{
	struct interface *ifp;
	struct dhcp6_state *state;

	ifp = arg;
	state = D6_STATE(ifp);
	state->state = DH6S_RENEW;
	state->start_uptime = uptime();
	state->RTC = 0;
	state->IRT = REN_TIMEOUT;
	state->MRT = REN_MAX_RT;
	state->MRC = 0;

	if (dhcp6_makemessage(ifp) == -1)
		syslog(LOG_ERR, "%s: dhcp6_makemessage: %m", ifp->name);
	else
		dhcp6_sendrenew(ifp);
}

static void
dhcp6_startdiscover(void *arg)
{
	struct interface *ifp;
	struct dhcp6_state *state;

	ifp = arg;
	dhcp6_delete_delegates(ifp);
	syslog(LOG_INFO, "%s: soliciting a DHCPv6 lease", ifp->name);
	state = D6_STATE(ifp);
	state->state = DH6S_DISCOVER;
	state->start_uptime = uptime();
	state->RTC = 0;
	state->IMD = SOL_MAX_DELAY;
	state->IRT = SOL_TIMEOUT;
	state->MRT = state->sol_max_rt;
	state->MRC = 0;

	eloop_timeout_delete(ifp->ctx->eloop, NULL, ifp);
	free(state->new);
	state->new = NULL;
	state->new_len = 0;

	dhcp6_freedrop_addrs(ifp, 0, NULL);
	unlink(state->leasefile);

	if (dhcp6_makemessage(ifp) == -1)
		syslog(LOG_ERR, "%s: dhcp6_makemessage: %m", ifp->name);
	else
		dhcp6_senddiscover(ifp);
}

static void
dhcp6_failconfirm(void *arg)
{
	struct interface *ifp;

	ifp = arg;
	syslog(LOG_ERR, "%s: failed to confirm prior address", ifp->name);
	/* Section 18.1.2 says that we SHOULD use the last known
	 * IP address(s) and lifetimes if we didn't get a reply.
	 * I disagree with this. */
	dhcp6_startdiscover(ifp);
}

static void
dhcp6_failrequest(void *arg)
{
	struct interface *ifp;

	ifp = arg;
	syslog(LOG_ERR, "%s: failed to request address", ifp->name);
	/* Section 18.1.1 says that client local policy dictates
	 * what happens if a REQUEST fails.
	 * Of the possible scenarios listed, moving back to the
	 * DISCOVER phase makes more sense for us. */
	dhcp6_startdiscover(ifp);
}

static void
dhcp6_failrebind(void *arg)
{
	struct interface *ifp;

	ifp = arg;
	syslog(LOG_ERR, "%s: failed to rebind prior delegation", ifp->name);
	dhcp6_delete_delegates(ifp);
	/* Section 18.1.2 says that we SHOULD use the last known
	 * IP address(s) and lifetimes if we didn't get a reply.
	 * I disagree with this. */
	dhcp6_startdiscover(ifp);
}

static void
dhcp6_startrebind(void *arg)
{
	struct interface *ifp;
	struct dhcp6_state *state;

	ifp = arg;
	eloop_timeout_delete(ifp->ctx->eloop, dhcp6_sendrenew, ifp);
	state = D6_STATE(ifp);
	if (state->state == DH6S_RENEW)
		syslog(LOG_WARNING, "%s: failed to renew DHCPv6, rebinding",
		    ifp->name);
	state->state = DH6S_REBIND;
	state->RTC = 0;
	state->MRC = 0;

	/* RFC 3633 section 12.1 */
	if (ifp->options->ia_type == D6_OPTION_IA_PD) {
		syslog(LOG_INFO, "%s: confirming Prefix Delegation", ifp->name);
		state->IMD = CNF_MAX_DELAY;
		state->IRT = CNF_TIMEOUT;
		state->MRT = CNF_MAX_RT;
	} else {
		state->IRT = REB_TIMEOUT;
		state->MRT = REB_MAX_RT;
	}

	if (dhcp6_makemessage(ifp) == -1)
		syslog(LOG_ERR, "%s: dhcp6_makemessage: %m", ifp->name);
	else
		dhcp6_sendrebind(ifp);

	/* RFC 3633 section 12.1 */
	if (ifp->options->ia_type == D6_OPTION_IA_PD)
		eloop_timeout_add_sec(ifp->ctx->eloop,
		    CNF_MAX_RD, dhcp6_failrebind, ifp);
}


static void
dhcp6_startrequest(struct interface *ifp)
{
	struct dhcp6_state *state;

	eloop_timeout_delete(ifp->ctx->eloop, dhcp6_senddiscover, ifp);
	state = D6_STATE(ifp);
	state->state = DH6S_REQUEST;
	state->RTC = 0;
	state->IRT = REQ_TIMEOUT;
	state->MRT = REQ_MAX_RT;
	state->MRC = REQ_MAX_RC;
	state->MRCcallback = dhcp6_failrequest;

	if (dhcp6_makemessage(ifp) == -1) {
		syslog(LOG_ERR, "%s: dhcp6_makemessage: %m", ifp->name);
		return;
	}

	dhcp6_sendrequest(ifp);
}

static void
dhcp6_startconfirm(struct interface *ifp)
{
	struct dhcp6_state *state;

	state = D6_STATE(ifp);
	state->state = DH6S_CONFIRM;
	state->start_uptime = uptime();
	state->RTC = 0;
	state->IMD = CNF_MAX_DELAY;
	state->IRT = CNF_TIMEOUT;
	state->MRT = CNF_MAX_RT;
	state->MRC = 0;

	syslog(LOG_INFO, "%s: confirming prior DHCPv6 lease", ifp->name);
	if (dhcp6_makemessage(ifp) == -1) {
		syslog(LOG_ERR, "%s: dhcp6_makemessage: %m", ifp->name);
		return;
	}
	dhcp6_sendconfirm(ifp);
	eloop_timeout_add_sec(ifp->ctx->eloop,
	    CNF_MAX_RD, dhcp6_failconfirm, ifp);
}

static void
dhcp6_startinform(void *arg)
{
	struct interface *ifp;
	struct dhcp6_state *state;

	ifp = arg;
	state = D6_STATE(ifp);
	if (state->new == NULL || ifp->options->options & DHCPCD_DEBUG)
		syslog(LOG_INFO, "%s: requesting DHCPv6 information",
		    ifp->name);
	state->state = DH6S_INFORM;
	state->start_uptime = uptime();
	state->RTC = 0;
	state->IMD = INF_MAX_DELAY;
	state->IRT = INF_TIMEOUT;
	state->MRT = state->inf_max_rt;
	state->MRC = 0;

	if (dhcp6_makemessage(ifp) == -1)
		syslog(LOG_ERR, "%s: dhcp6_makemessage: %m", ifp->name);
	else
		dhcp6_sendinform(ifp);
}

static void
dhcp6_startexpire(void *arg)
{
	struct interface *ifp;

	ifp = arg;
	eloop_timeout_delete(ifp->ctx->eloop, dhcp6_sendrebind, ifp);

	syslog(LOG_ERR, "%s: DHCPv6 lease expired", ifp->name);
	dhcp6_freedrop_addrs(ifp, 1, NULL);
	dhcp6_delete_delegates(ifp);
	script_runreason(ifp, "EXPIRE6");
	dhcp6_startdiscover(ifp);
}

static void
dhcp6_startrelease(struct interface *ifp)
{
	struct dhcp6_state *state;

	state = D6_STATE(ifp);
	if (state->state != DH6S_BOUND)
		return;

	state->state = DH6S_RELEASE;
	state->start_uptime = uptime();
	state->RTC = 0;
	state->IRT = REL_TIMEOUT;
	state->MRT = 0;
	state->MRC = REL_MAX_RC;
	//state->MRCcallback = dhcp6_failrelease;
	state->MRCcallback = NULL;

	if (dhcp6_makemessage(ifp) == -1)
		syslog(LOG_ERR, "%s: dhcp6_makemessage: %m", ifp->name);
	else
		/* XXX: We should loop a few times
		 * Luckily RFC3315 section 18.1.6 says this is optional */
		//dhcp6_sendrelease(ifp);
		dhcp6_sendmessage(ifp, NULL);
}

static int
dhcp6_checkstatusok(const struct interface *ifp,
    const struct dhcp6_message *m, const uint8_t *p, size_t len)
{
	const struct dhcp6_option *o;
	uint16_t code;
	char *status;

	if (p)
		o = dhcp6_findoption(D6_OPTION_STATUS_CODE, p, len);
	else
		o = dhcp6_getmoption(D6_OPTION_STATUS_CODE, m, len);
	if (o == NULL) {
		//syslog(LOG_DEBUG, "%s: no status", ifp->name);
		return 0;
	}

	len = ntohs(o->len);
	if (len < sizeof(code)) {
		syslog(LOG_ERR, "%s: status truncated", ifp->name);
		return -1;
	}

	p = D6_COPTION_DATA(o);
	memcpy(&code, p, sizeof(code));
	code = ntohs(code);
	if (code == D6_STATUS_OK)
		return 1;

	len -= sizeof(code);

	if (len == 0) {
		if (code < sizeof(dhcp6_statuses) / sizeof(char *)) {
			p = (const uint8_t *)dhcp6_statuses[code];
			len = strlen((const char *)p);
		} else
			p = NULL;
	} else
		p += sizeof(code);

	status = malloc(len + 1);
	if (status == NULL) {
		syslog(LOG_ERR, "%s: %m", __func__);
		return -1;
	}
	if (p)
		memcpy(status, p, len);
	status[len] = '\0';
	syslog(LOG_ERR, "%s: DHCPv6 REPLY: %s", ifp->name, status);
	free(status);
	return -1;
}

static struct ipv6_addr *
dhcp6_findaddr(struct interface *ifp, const struct in6_addr *addr)
{
	const struct dhcp6_state *state;
	struct ipv6_addr *ap;

	state = D6_CSTATE(ifp);
	if (state) {
		TAILQ_FOREACH(ap, &state->addrs, next) {
			if (addr == NULL) {
				if ((ap->flags &
				    (IPV6_AF_ADDED | IPV6_AF_DADCOMPLETED)) ==
				    (IPV6_AF_ADDED | IPV6_AF_DADCOMPLETED))
					return ap;
			} else if (IN6_ARE_ADDR_EQUAL(&ap->addr, addr))
				return ap;
		}
	}
	return NULL;
}

int
dhcp6_addrexists(struct dhcpcd_ctx *ctx, const struct ipv6_addr *addr)
{
	struct interface *ifp;

	TAILQ_FOREACH(ifp, ctx->ifaces, next) {
		if (dhcp6_findaddr(ifp, addr == NULL ? NULL : &addr->addr))
			return 1;
	}
	return 0;
}

static void
dhcp6_dadcallback(void *arg)
{
	struct ipv6_addr *ap = arg;
	struct interface *ifp;
	struct dhcp6_state *state;
	int wascompleted;

	wascompleted = (ap->flags & IPV6_AF_DADCOMPLETED);
	ap->flags |= IPV6_AF_DADCOMPLETED;
	if (ap->flags & IPV6_AF_DUPLICATED)
		/* XXX FIXME
		 * We should decline the address */
		syslog(LOG_WARNING, "%s: DAD detected %s",
		    ap->iface->name, ap->saddr);

	if (!wascompleted) {
		ifp = ap->iface;
		state = D6_STATE(ifp);
		if (state->state == DH6S_BOUND ||
		    state->state == DH6S_DELEGATED)
		{
			TAILQ_FOREACH(ap, &state->addrs, next) {
				if ((ap->flags & IPV6_AF_DADCOMPLETED) == 0) {
					wascompleted = 1;
					break;
				}
			}
			if (!wascompleted) {
				syslog(LOG_DEBUG, "%s: DHCPv6 DAD completed",
				    ifp->name);
				script_runreason(ifp, state->reason);
				daemonise(ifp->ctx);
			}
		}
	}
}

static int
dhcp6_findna(struct interface *ifp, const uint8_t *iaid,
    const uint8_t *d, size_t l)
{
	struct dhcp6_state *state;
	const struct dhcp6_option *o;
	const uint8_t *p;
	struct in6_addr in6;
	struct ipv6_addr *a;
	char iabuf[INET6_ADDRSTRLEN];
	const char *ia;
	int i;
	uint32_t u32;

	i = 0;
	state = D6_STATE(ifp);
	while ((o = dhcp6_findoption(D6_OPTION_IA_ADDR, d, l))) {
		l -= ((const uint8_t *)o - d);
		d += ((const uint8_t *)o - d);
		u32 = ntohs(o->len);
		l -= sizeof(*o) + u32;
		d += sizeof(*o) + u32;
		if (u32 < 24) {
			errno = EINVAL;
			syslog(LOG_ERR, "%s: IA Address option truncated",
			    ifp->name);
			continue;
		}
		p = D6_COPTION_DATA(o);
		memcpy(&in6.s6_addr, p, sizeof(in6.s6_addr));
		p += sizeof(in6.s6_addr);
		a = dhcp6_findaddr(ifp, &in6);
		if (a == NULL) {
			a = calloc(1, sizeof(*a));
			if (a == NULL) {
				syslog(LOG_ERR, "%s: %m", __func__);
				break;
			}
			a->iface = ifp;
			a->flags = IPV6_AF_NEW | IPV6_AF_ONLINK;
			a->dadcallback = dhcp6_dadcallback;
			memcpy(a->iaid, iaid, sizeof(a->iaid));
			memcpy(&a->addr.s6_addr, &in6.s6_addr,
			    sizeof(in6.s6_addr));

			/*
			 * RFC 5942 Section 5
			 * We cannot assume any prefix length, nor tie the
			 * address to an existing one as it could expire
			 * before the address.
			 * As such we just give it a 128 prefix.
			 */
			a->prefix_len = 128;
			if (ipv6_makeprefix(&a->prefix, &a->addr,
			    a->prefix_len) == -1)
			{
				syslog(LOG_ERR, "%s: %m", __func__);
				free(a);
				continue;
			}
			ia = inet_ntop(AF_INET6, &a->addr.s6_addr,
			    iabuf, sizeof(iabuf));
			snprintf(a->saddr, sizeof(a->saddr),
			    "%s/%d", ia, a->prefix_len);
			TAILQ_INSERT_TAIL(&state->addrs, a, next);
		} else
			a->flags &= ~IPV6_AF_STALE;
		memcpy(&u32, p, sizeof(u32));
		a->prefix_pltime = ntohl(u32);
		p += sizeof(u32);
		memcpy(&u32, p, sizeof(u32));
		u32 = ntohl(u32);
		if (a->prefix_vltime != u32) {
			a->flags |= IPV6_AF_NEW;
			a->prefix_vltime = u32;
		}
		if (a->prefix_pltime && a->prefix_pltime < state->lowpl)
		    state->lowpl = a->prefix_pltime;
		if (a->prefix_vltime && a->prefix_vltime > state->expire)
		    state->expire = a->prefix_vltime;
		i++;
	}
	return i;
}

static int
dhcp6_findpd(struct interface *ifp, const uint8_t *iaid,
    const uint8_t *d, size_t l)
{
	struct dhcp6_state *state;
	const struct dhcp6_option *o;
	const uint8_t *p;
	struct ipv6_addr *a;
	char iabuf[INET6_ADDRSTRLEN];
	const char *ia;
	int i;
	uint8_t u8, len;
	uint32_t u32, pltime, vltime;
	struct in6_addr prefix;

	i = 0;
	state = D6_STATE(ifp);
	while ((o = dhcp6_findoption(D6_OPTION_IAPREFIX, d, l))) {
		l -= ((const uint8_t *)o - d);
		d += ((const uint8_t *)o - d);
		u32 = ntohs(o->len);
		l -= sizeof(*o) + u32;
		d += sizeof(*o) + u32;
		if (u32 < 25) {
			errno = EINVAL;
			syslog(LOG_ERR, "%s: IA Prefix option truncated",
			    ifp->name);
			continue;
		}
		p = D6_COPTION_DATA(o);
		memcpy(&u32, p, sizeof(u32));
		pltime = ntohl(u32);
		p += sizeof(u32);
		memcpy(&u32, p, sizeof(u32));
		p += sizeof(u32);
		vltime = ntohl(u32);
		memcpy(&u8, p, sizeof(u8));
		p += sizeof(u8);
		len = u8;
		memcpy(&prefix.s6_addr, p, sizeof(prefix.s6_addr));

		TAILQ_FOREACH(a, &state->addrs, next) {
			if (IN6_ARE_ADDR_EQUAL(&a->prefix, &prefix))
				break;
		}
		if (a == NULL) {
			a = calloc(1, sizeof(*a));
			if (a == NULL) {
				syslog(LOG_ERR, "%s: %m", __func__);
				break;
			}
			a->iface = ifp;
			a->flags = IPV6_AF_NEW;
			a->dadcallback = dhcp6_dadcallback;
			memcpy(a->iaid, iaid, sizeof(a->iaid));
			memcpy(&a->prefix.s6_addr,
			    &prefix.s6_addr, sizeof(a->prefix.s6_addr));
			a->prefix_len = len;
			ia = inet_ntop(AF_INET6, &a->prefix.s6_addr,
			    iabuf, sizeof(iabuf));
			snprintf(a->saddr, sizeof(a->saddr),
			    "%s/%d", ia, a->prefix_len);
			TAILQ_INSERT_TAIL(&state->addrs, a, next);
		} else {
			a->flags &= ~IPV6_AF_STALE;
			if (a->prefix_vltime != vltime)
				a->flags |= IPV6_AF_NEW;
		}

		a->prefix_pltime = pltime;
		a->prefix_vltime = vltime;
		if (a->prefix_pltime && a->prefix_pltime < state->lowpl)
			state->lowpl = a->prefix_pltime;
		if (a->prefix_vltime && a->prefix_vltime > state->expire)
			state->expire = a->prefix_vltime;
		i++;
	}
	return i;
}

static int
dhcp6_findia(struct interface *ifp, const uint8_t *d, size_t l,
    const char *sfrom)
{
	struct dhcp6_state *state;
	const struct if_options *ifo;
	const struct dhcp6_option *o;
	const uint8_t *p;
	int i, e;
	uint32_t u32, renew, rebind;
	uint8_t iaid[4];
	size_t ol;
	struct ipv6_addr *ap, *nap;

	ifo = ifp->options;
	i = e = 0;
	state = D6_STATE(ifp);
	TAILQ_FOREACH(ap, &state->addrs, next) {
		ap->flags |= IPV6_AF_STALE;
	}
	while ((o = dhcp6_findoption(ifo->ia_type, d, l))) {
		l -= ((const uint8_t *)o - d);
		d += ((const uint8_t *)o - d);
		ol = ntohs(o->len);
		l -= sizeof(*o) + ol;
		d += sizeof(*o) + ol;
		u32 = ifo->ia_type == D6_OPTION_IA_TA ? 4 : 12;
		if (ol < u32) {
			errno = EINVAL;
			syslog(LOG_ERR, "%s: IA option truncated", ifp->name);
			continue;
		}

		p = D6_COPTION_DATA(o);
		memcpy(iaid, p, sizeof(iaid));
		p += sizeof(iaid);
		ol -= sizeof(iaid);
		if (ifo->ia_type != D6_OPTION_IA_TA) {
			memcpy(&u32, p, sizeof(u32));
			renew = ntohl(u32);
			p += sizeof(u32);
			ol -= sizeof(u32);
			memcpy(&u32, p, sizeof(u32));
			rebind = ntohl(u32);
			p += sizeof(u32);
			ol -= sizeof(u32);
		} else
			renew = rebind = 0; /* appease gcc */
		if (dhcp6_checkstatusok(ifp, NULL, p, ol) == -1) {
			e = 1;
			continue;
		}
		if (ifo->ia_type == D6_OPTION_IA_PD) {
			if (dhcp6_findpd(ifp, iaid, p, ol) == 0) {
				syslog(LOG_WARNING,
				    "%s: %s: DHCPv6 REPLY missing Prefix",
				    ifp->name, sfrom);
				continue;
			}
		} else {
			if (dhcp6_findna(ifp, iaid, p, ol) == 0) {
				syslog(LOG_WARNING,
				    "%s: %s: DHCPv6 REPLY missing IA Address",
				    ifp->name, sfrom);
				continue;
			}
		}
		if (ifo->ia_type != D6_OPTION_IA_TA) {
			if (renew > rebind && rebind > 0) {
				if (sfrom)
				    syslog(LOG_WARNING,
					"%s: T1 (%d) > T2 (%d) from %s",
					ifp->name, renew, rebind, sfrom);
				renew = 0;
				rebind = 0;
			}
			if (renew != 0 &&
			    (renew < state->renew || state->renew == 0))
				state->renew = renew;
			if (rebind != 0 &&
			    (rebind < state->rebind || state->rebind == 0))
				state->rebind = rebind;
		}
		i++;
	}
	TAILQ_FOREACH_SAFE(ap, &state->addrs, next, nap) {
		if (ap->flags & IPV6_AF_STALE) {
			TAILQ_REMOVE(&state->addrs, ap, next);
			if (ap->dadcallback)
				eloop_q_timeout_delete(ap->iface->ctx->eloop,
				    0, NULL, ap);
			free(ap);
		}
	}
	if (i == 0 && e)
		return -1;
	return i;
}

static int
dhcp6_validatelease(struct interface *ifp,
    const struct dhcp6_message *m, size_t len,
    const char *sfrom)
{
	struct dhcp6_state *state;
	const struct dhcp6_option *o;

	state = D6_STATE(ifp);
	o = dhcp6_getmoption(ifp->options->ia_type, m, len);
	if (o == NULL) {
		if (sfrom &&
		    dhcp6_checkstatusok(ifp, m, NULL, len) != -1)
			syslog(LOG_ERR, "%s: no IA in REPLY from %s",
			    ifp->name, sfrom);
		return -1;
	}

	if (dhcp6_checkstatusok(ifp, m, NULL, len) == -1)
		return -1;

	state->renew = state->rebind = state->expire = 0;
	state->lowpl = ND6_INFINITE_LIFETIME;
	len -= (const char *)o - (const char *)m;
	return dhcp6_findia(ifp, (const uint8_t *)o, len, sfrom);
}

static ssize_t
dhcp6_writelease(const struct interface *ifp)
{
	const struct dhcp6_state *state;
	int fd;
	ssize_t bytes;

	state = D6_CSTATE(ifp);
	syslog(LOG_DEBUG, "%s: writing lease `%s'",
	    ifp->name, state->leasefile);

	fd = open(state->leasefile, O_WRONLY | O_CREAT | O_TRUNC, 0644);
	if (fd == -1) {
		syslog(LOG_ERR, "%s: dhcp6_writelease: %m", ifp->name);
		return -1;
	}
	bytes = write(fd, state->new, state->new_len);
	close(fd);
	return bytes;
}

static int
dhcp6_readlease(struct interface *ifp)
{
	struct dhcp6_state *state;
	struct stat st;
	int fd;
	ssize_t bytes;
	struct timeval now;
	const struct dhcp6_option *o;

	state = D6_STATE(ifp);
	if (stat(state->leasefile, &st) == -1) {
		if (errno == ENOENT)
			return 0;
		return -1;
	}
	syslog(LOG_DEBUG, "%s: reading lease `%s'",
	    ifp->name, state->leasefile);
	state->new = malloc(st.st_size);
	if (state->new == NULL)
		return -1;
	state->new_len = st.st_size;
	fd = open(state->leasefile, O_RDONLY);
	if (fd == -1)
		return -1;
	bytes = read(fd, state->new, state->new_len);
	close(fd);
	if (bytes < (ssize_t)state->new_len) {
		syslog(LOG_ERR, "%s: read: %m", __func__);
		goto ex;
	}

	/* Check to see if the lease is still valid */
	fd = dhcp6_validatelease(ifp, state->new, state->new_len, NULL);
	if (fd == -1)
		goto ex;
	if (fd == 0) {
		syslog(LOG_INFO, "%s: lease was for different IA type",
		    ifp->name);
		goto ex;
	}

	if (state->expire != ND6_INFINITE_LIFETIME) {
		gettimeofday(&now, NULL);
		if ((time_t)state->expire < now.tv_sec - st.st_mtime) {
			syslog(LOG_DEBUG,"%s: discarding expired lease",
			    ifp->name);
			goto ex;
		}
	}

	/* Authenticate the message */
	o = dhcp6_getmoption(D6_OPTION_AUTH, state->new, state->new_len);
	if (o) {
		if (dhcp_auth_validate(&state->auth, &ifp->options->auth,
		    (uint8_t *)state->new, state->new_len, 6, state->new->type,
		    D6_COPTION_DATA(o), ntohs(o->len)) == NULL)
		{
			syslog(LOG_DEBUG, "%s: dhcp_auth_validate: %m",
			    ifp->name);
			syslog(LOG_ERR, "%s: authentication failed",
			    ifp->name);
			goto ex;
		}
		if (state->auth.token)
			syslog(LOG_DEBUG, "%s: validated using 0x%08" PRIu32,
			    ifp->name, state->auth.token->secretid);
		else
			syslog(LOG_DEBUG, "%s: accepted reconfigure key",
			    ifp->name);
	} else if (ifp->options->auth.options & DHCPCD_AUTH_REQUIRE) {
		syslog(LOG_ERR, "%s: authentication now required", ifp->name);
		goto ex;
	}

	return fd;

ex:
	dhcp6_freedrop_addrs(ifp, 0, NULL);
	free(state->new);
	state->new = NULL;
	state->new_len = 0;
	unlink(state->leasefile);
	return 0;
}

static void
dhcp6_startinit(struct interface *ifp)
{
	struct dhcp6_state *state;
	int r;

	state = D6_STATE(ifp);
	state->state = DH6S_INIT;
	state->expire = ND6_INFINITE_LIFETIME;
	state->lowpl = ND6_INFINITE_LIFETIME;
	if (!(ifp->ctx->options & DHCPCD_TEST) &&
	    ifp->options->ia_type != D6_OPTION_IA_TA &&
	    ifp->options->reboot != 0)
	{
		r = dhcp6_readlease(ifp);
		if (r == -1)
			syslog(LOG_ERR, "%s: dhcp6_readlease: %s: %m",
					ifp->name, state->leasefile);
		else if (r != 0) {
			/* RFC 3633 section 12.1 */
			if (ifp->options->ia_type == D6_OPTION_IA_PD)
				dhcp6_startrebind(ifp);
			else
				dhcp6_startconfirm(ifp);
			return;
		}
	}
	dhcp6_startdiscover(ifp);
}

static uint32_t
dhcp6_findsla(const struct dhcpcd_ctx *ctx)
{
	uint32_t sla;
	const struct interface *ifp;
	const struct dhcp6_state *state;

	/* Slow, but finding the lowest free SLA is needed if we get a
	 * /62 or /63 prefix from upstream */
	for (sla = 0; sla < UINT32_MAX; sla++) {
		TAILQ_FOREACH(ifp, ctx->ifaces, next) {
			state = D6_CSTATE(ifp);
			if (state && state->sla_set && state->sla == sla)
				break;
		}
		if (ifp == NULL)
			return sla;
	}

	errno = E2BIG;
	return 0;
}

static struct ipv6_addr *
dhcp6_delegate_addr(struct interface *ifp, const struct ipv6_addr *prefix,
    const struct if_sla *sla, struct interface *ifs)
{
	struct dhcp6_state *state;
	struct if_sla asla;
	struct in6_addr addr;
	struct ipv6_addr *a, *ap, *apn;
	char iabuf[INET6_ADDRSTRLEN];
	const char *ia;

	state = D6_STATE(ifp);
	if (state == NULL) {
		ifp->if_data[IF_DATA_DHCP6] = calloc(1, sizeof(*state));
		state = D6_STATE(ifp);
		if (state == NULL) {
			syslog(LOG_ERR, "%s: %m", __func__);
			return NULL;
		}

		TAILQ_INIT(&state->addrs);
		state->state = DH6S_DELEGATED;
		state->reason = "DELEGATED6";
	}

	if (sla == NULL || sla->sla_set == 0) {
		if (!state->sla_set) {
			errno = 0;
			state->sla = dhcp6_findsla(ifp->ctx);
			if (errno) {
				syslog(LOG_ERR, "%s: dhcp6_find_sla: %m",
				    ifp->name);
				return NULL;
			}
			syslog(LOG_DEBUG, "%s: set SLA %d",
			    ifp->name, state->sla);
			state->sla_set = 1;
		}
		asla.sla = state->sla;
		asla.prefix_len = 64;
		sla = &asla;
	}

	if (ipv6_userprefix(&prefix->prefix, prefix->prefix_len,
		sla->sla, &addr, sla->prefix_len) == -1)
	{
		ia = inet_ntop(AF_INET6, &prefix->prefix.s6_addr,
		    iabuf, sizeof(iabuf));
		syslog(LOG_ERR, "%s: invalid prefix %s/%d + %d/%d: %m",
			ifp->name, ia, prefix->prefix_len,
			sla->sla, sla->prefix_len);
		return NULL;
	}

	a = calloc(1, sizeof(*a));
	if (a == NULL) {
		syslog(LOG_ERR, "%s: %m", __func__);
		return NULL;
	}
	a->iface = ifp;
	a->flags = IPV6_AF_NEW | IPV6_AF_ONLINK;
	a->dadcallback = dhcp6_dadcallback;
	a->delegating_iface = ifs;
	memcpy(&a->iaid, &prefix->iaid, sizeof(a->iaid));
	a->prefix_pltime = prefix->prefix_pltime;
	a->prefix_vltime = prefix->prefix_vltime;
	memcpy(&a->prefix.s6_addr, &addr.s6_addr, sizeof(a->prefix.s6_addr));
	a->prefix_len = sla->prefix_len;

	if (ipv6_makeaddr(&a->addr, ifp, &a->prefix, a->prefix_len) == -1)
	{
		ia = inet_ntop(AF_INET6, &a->addr.s6_addr,
		    iabuf, sizeof(iabuf));
		syslog(LOG_ERR, "%s: %m (%s/%d)", __func__, ia, a->prefix_len);
		free(a);
		return NULL;
	}

	/* Remove any exiting address */
	TAILQ_FOREACH_SAFE(ap, &state->addrs, next, apn) {
		if (IN6_ARE_ADDR_EQUAL(&ap->addr, &a->addr)) {
			TAILQ_REMOVE(&state->addrs, ap, next);
			/* Keep our flags */
			a->flags |= ap->flags;
			a->flags &= ~IPV6_AF_NEW;
			free(ap);
		}
	}

	ia = inet_ntop(AF_INET6, &a->addr.s6_addr, iabuf, sizeof(iabuf));
	snprintf(a->saddr, sizeof(a->saddr), "%s/%d", ia, a->prefix_len);
	TAILQ_INSERT_TAIL(&state->addrs, a, next);
	return a;
}

static void
dhcp6_delegate_prefix(struct interface *ifp)
{
	struct if_options *ifo;
	struct dhcp6_state *state, *ifd_state;
	struct ipv6_addr *ap;
	size_t i, j, k;
	struct if_ia *ia;
	struct if_sla *sla;
	struct interface *ifd;
	uint8_t carrier_warned;

	ifo = ifp->options;
	state = D6_STATE(ifp);
	TAILQ_FOREACH(ifd, ifp->ctx->ifaces, next) {
		k = 0;
		carrier_warned = 0;
		TAILQ_FOREACH(ap, &state->addrs, next) {
			if (ap->flags & IPV6_AF_NEW) {
				ap->flags &= ~IPV6_AF_NEW;
				syslog(LOG_DEBUG, "%s: delegated prefix %s",
				    ifp->name, ap->saddr);
			}
			for (i = 0; i < ifo->ia_len; i++) {
				ia = &ifo->ia[i];
				if (memcmp(ia->iaid, ap->iaid,
				    sizeof(ia->iaid)))
					continue;
				if (ia->sla_len == 0) {
					/* no SLA configured, so lets
					 * automate it */
					if (ifp == ifd)
						continue;
					if (ifd->carrier == LINK_DOWN) {
						syslog(LOG_DEBUG,
						    "%s: has no carrier, cannot"
						    " delegate addresses",
						    ifd->name);
						carrier_warned = 1;
						break;
					}
					if (dhcp6_delegate_addr(ifd, ap,
					    NULL, ifp))
						k++;
				}
				for (j = 0; j < ia->sla_len; j++) {
					sla = &ia->sla[j];
					if (strcmp(ifd->name, sla->ifname))
						continue;
					if (ifd->carrier == LINK_DOWN) {
						syslog(LOG_DEBUG,
						    "%s: has no carrier, cannot"
						    " delegate addresses",
						    ifd->name);
						carrier_warned = 1;
						break;
					}
					if (dhcp6_delegate_addr(ifd, ap,
					    sla, ifp))
						k++;
				}
				if (carrier_warned)
					break;
			}
			if (carrier_warned)
				break;
		}
		if (k && !carrier_warned) {
			ifd_state = D6_STATE(ifd);
			ipv6_addaddrs(&ifd_state->addrs);
		}
	}

	/* Warn about configured interfaces for delegation that do not exist */
	for (i = 0; i < ifo->ia_len; i++) {
		ia = &ifo->ia[i];
		for (j = 0; j < ia->sla_len; j++) {
			sla = &ia->sla[j];
			for (k = 0; k < i; j++)
				if (strcmp(sla->ifname, ia->sla[j].ifname) == 0)
					break;
			if (j >= i &&
			    find_interface(ifp->ctx, sla->ifname) == NULL)
				syslog(LOG_ERR,
				    "%s: interface does not exist"
				    " for delegation",
				    sla->ifname);
		}
	}
}

static void
dhcp6_find_delegates1(void *arg)
{

	dhcp6_find_delegates(arg);
}

int
dhcp6_find_delegates(struct interface *ifp)
{
	struct if_options *ifo;
	struct dhcp6_state *state;
	struct ipv6_addr *ap;
	size_t i, j, k;
	struct if_ia *ia;
	struct if_sla *sla;
	struct interface *ifd;

	k = 0;
	TAILQ_FOREACH(ifd, ifp->ctx->ifaces, next) {
		ifo = ifd->options;
		if (ifo->ia_type != D6_OPTION_IA_PD)
			continue;
		state = D6_STATE(ifd);
		if (state == NULL || state->state != DH6S_BOUND)
			continue;
		TAILQ_FOREACH(ap, &state->addrs, next) {
			for (i = 0; i < ifo->ia_len; i++) {
				ia = &ifo->ia[i];
				if (memcmp(ia->iaid, ap->iaid,
				    sizeof(ia->iaid)))
					continue;
				for (j = 0; j < ia->sla_len; j++) {
					sla = &ia->sla[j];
					if (strcmp(ifp->name, sla->ifname))
						continue;
					if (ipv6_linklocal(ifp) == NULL) {
						syslog(LOG_DEBUG,
						    "%s: delaying adding"
						    " delegated addresses for"
						    " LL address",
						    ifp->name);
						ipv6_addlinklocalcallback(ifp,
						    dhcp6_find_delegates1, ifp);
						return 1;
					}
					if (dhcp6_delegate_addr(ifp, ap,
					    sla, ifd))
					    k++;
				}
			}
		}
	}

	if (k) {
		syslog(LOG_INFO, "%s: adding delegated prefixes", ifp->name);
		state = D6_STATE(ifp);
		state->state = DH6S_DELEGATED;
		ipv6_addaddrs(&state->addrs);
		ipv6_buildroutes(ifp->ctx);
	}
	return k;
}

/* ARGSUSED */
static void
dhcp6_handledata(void *arg)
{
	struct dhcpcd_ctx *dhcpcd_ctx;
	struct ipv6_ctx *ctx;
	ssize_t len;
	size_t i;
	struct cmsghdr *cm;
	struct in6_pktinfo pkt;
	struct interface *ifp;
	const char *op;
	struct dhcp6_message *r;
	struct dhcp6_state *state;
	const struct dhcp6_option *o, *auth;
	const struct dhcp_opt *opt;
	const struct if_options *ifo;
	struct ipv6_addr *ap;
	uint8_t has_new;
	int error;
	uint32_t u32;

	dhcpcd_ctx = arg;
	ctx = dhcpcd_ctx->ipv6;
	ctx->rcvhdr.msg_controllen = CMSG_SPACE(sizeof(struct in6_pktinfo));
	len = recvmsg(ctx->dhcp_fd, &ctx->rcvhdr, 0);
	if (len == -1) {
		syslog(LOG_ERR, "recvmsg: %m");
		return;
	}
	ctx->sfrom = inet_ntop(AF_INET6, &ctx->from.sin6_addr,
	    ctx->ntopbuf, sizeof(ctx->ntopbuf));
	if ((size_t)len < sizeof(struct dhcp6_message)) {
		syslog(LOG_ERR, "DHCPv6 RA packet too short from %s",
		    ctx->sfrom);
		return;
	}

	pkt.ipi6_ifindex = 0;
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
		}
	}

	if (pkt.ipi6_ifindex == 0) {
		syslog(LOG_ERR,
		    "DHCPv6 reply did not contain index from %s",
		    ctx->sfrom);
		return;
	}

	TAILQ_FOREACH(ifp, dhcpcd_ctx->ifaces, next) {
		if (ifp->index == (unsigned int)pkt.ipi6_ifindex)
			break;
	}
	if (ifp == NULL) {
		syslog(LOG_DEBUG,
		    "DHCPv6 reply for unexpected interface from %s",
		    ctx->sfrom);
		return;
	}
	state = D6_STATE(ifp);
	if (state == NULL || state->send == NULL) {
		syslog(LOG_DEBUG, "%s: DHCPv6 reply received but not running",
		    ifp->name);
		return;
	}

	r = (struct dhcp6_message *)ctx->rcvhdr.msg_iov[0].iov_base;

	/* We're already bound and this message is for another machine */
	/* XXX DELEGATED? */
	if (r->type != DHCP6_RECONFIGURE &&
	    (state->state == DH6S_BOUND || state->state == DH6S_INFORMED))
		return;

	r = (struct dhcp6_message *)ctx->rcvhdr.msg_iov[0].iov_base;
	if (r->type != DHCP6_RECONFIGURE &&
	    (r->xid[0] != state->send->xid[0] ||
	    r->xid[1] != state->send->xid[1] ||
	    r->xid[2] != state->send->xid[2]))
	{
		syslog(LOG_DEBUG,
		    "%s: wrong xid 0x%02x%02x%02x"
		    " (expecting 0x%02x%02x%02x) from %s",
		    ifp->name,
		    r->xid[0], r->xid[1], r->xid[2],
		    state->send->xid[0], state->send->xid[1],
		    state->send->xid[2],
		    ctx->sfrom);
		return;
	}

	if (dhcp6_getmoption(D6_OPTION_SERVERID, r, len) == NULL) {
		syslog(LOG_DEBUG, "%s: no DHCPv6 server ID from %s",
		    ifp->name, ctx->sfrom);
		return;
	}

	o = dhcp6_getmoption(D6_OPTION_CLIENTID, r, len);
	if (o == NULL || ntohs(o->len) != dhcpcd_ctx->duid_len ||
	    memcmp(D6_COPTION_DATA(o),
	    dhcpcd_ctx->duid, dhcpcd_ctx->duid_len) != 0)
	{
		syslog(LOG_DEBUG, "%s: incorrect client ID from %s",
		    ifp->name, ctx->sfrom);
		return;
	}

	ifo = ifp->options;
	for (i = 0, opt = dhcpcd_ctx->dhcp6_opts;
	    i < dhcpcd_ctx->dhcp6_opts_len;
	    i++, opt++)
	{
		if (has_option_mask(ifo->requiremask6, opt->option) &&
		    dhcp6_getmoption(opt->option, r, len) == NULL)
		{
			syslog(LOG_WARNING,
			    "%s: reject DHCPv6 (no option %s) from %s",
			    ifp->name, opt->var, ctx->sfrom);
			return;
		}
	}

	/* Authenticate the message */
	auth = dhcp6_getmoption(D6_OPTION_AUTH, r, len);
	if (auth) {
		if (dhcp_auth_validate(&state->auth, &ifo->auth,
		    (uint8_t *)r, len, 6, r->type,
		    D6_COPTION_DATA(auth), ntohs(auth->len)) == NULL)
		{
			syslog(LOG_DEBUG, "dhcp_auth_validate: %m");
			syslog(LOG_ERR, "%s: authentication failed from %s",
			    ifp->name, ctx->sfrom);
			return;
		}
		if (state->auth.token)
			syslog(LOG_DEBUG, "%s: validated using 0x%08" PRIu32,
			    ifp->name, state->auth.token->secretid);
		else
			syslog(LOG_DEBUG, "%s: accepted reconfigure key",
			    ifp->name);
	} else if (ifo->auth.options & DHCPCD_AUTH_REQUIRE) {
		syslog(LOG_ERR, "%s: no authentication from %s",
		    ifp->name, ctx->sfrom);
		return;
	} else if (ifo->auth.options & DHCPCD_AUTH_SEND)
		syslog(LOG_WARNING,
		    "%s: no authentication from %s",
		    ifp->name, ctx->sfrom);

	op = dhcp6_get_op(r->type);
	switch(r->type) {
	case DHCP6_REPLY:
		switch(state->state) {
		case DH6S_INFORM:
			/* RFC4242 */
			o = dhcp6_getmoption(D6_OPTION_INFO_REFRESH_TIME,
			    r, len);
			if (o == NULL || ntohs(o->len) != sizeof(u32))
				state->renew = IRT_DEFAULT;
			else {
				memcpy(&u32, D6_COPTION_DATA(o), sizeof(u32));
				state->renew = ntohl(u32);
				if (state->renew < IRT_MINIMUM)
					state->renew = IRT_MINIMUM;
			}
			break;
		case DH6S_CONFIRM:
			error = dhcp6_checkstatusok(ifp, r, NULL, len);
			/* If we got an OK status the chances are that we
			 * didn't get the IA's returned, so preserve them
			 * from our saved response */
			if (error == 1)
				goto recv;
			if (error == -1 ||
			    dhcp6_validatelease(ifp, r, len, ctx->sfrom) == -1)
			{
				dhcp6_startdiscover(ifp);
				return;
			}
			break;
		case DH6S_DISCOVER:
			if (has_option_mask(ifo->requestmask6,
			    D6_OPTION_RAPID_COMMIT) &&
			    dhcp6_getmoption(D6_OPTION_RAPID_COMMIT, r, len))
				state->state = DH6S_REQUEST;
			else
				op = NULL;
		case DH6S_REQUEST: /* FALLTHROUGH */
		case DH6S_RENEW: /* FALLTHROUGH */
		case DH6S_REBIND:
			if (dhcp6_validatelease(ifp, r, len, ctx->sfrom) == -1){
				/* PD doesn't use CONFIRM, so REBIND could
				 * throw up an invalid prefix if we
				 * changed link */
				if (ifp->options->ia_type == D6_OPTION_IA_PD)
					dhcp6_startdiscover(ifp);
				return;
			}
			break;
		default:
			op = NULL;
		}
		break;
	case DHCP6_ADVERTISE:
		if (state->state != DH6S_DISCOVER) {
			op = NULL;
			break;
		}
		/* RFC7083 */
		o = dhcp6_getmoption(D6_OPTION_SOL_MAX_RT, r, len);
		if (o && ntohs(o->len) >= sizeof(u32)) {
			memcpy(&u32, D6_COPTION_DATA(o), sizeof(u32));
			u32 = ntohl(u32);
			if (u32 >= 60 && u32 <= 86400) {
				syslog(LOG_DEBUG, "%s: SOL_MAX_RT %d -> %d",
				    ifp->name, state->sol_max_rt, u32);
				state->sol_max_rt = u32;
			} else
				syslog(LOG_ERR, "%s: invalid SOL_MAX_RT %d",
				    ifp->name, u32);
		}
		o = dhcp6_getmoption(D6_OPTION_INF_MAX_RT, r, len);
		if (o && ntohs(o->len) >= sizeof(u32)) {
			memcpy(&u32, D6_COPTION_DATA(o), sizeof(u32));
			u32 = ntohl(u32);
			if (u32 >= 60 && u32 <= 86400) {
				syslog(LOG_DEBUG, "%s: INF_MAX_RT %d -> %d",
				    ifp->name, state->inf_max_rt, u32);
				state->inf_max_rt = u32;
			} else
				syslog(LOG_ERR, "%s: invalid INF_MAX_RT %d",
				    ifp->name, u32);
		}
		if (dhcp6_validatelease(ifp, r, len, ctx->sfrom) == -1)
			return;
		break;
	case DHCP6_RECONFIGURE:
		if (auth == NULL) {
			syslog(LOG_ERR,
			    "%s: unauthenticated %s from %s",
			    ifp->name, op, ctx->sfrom);
			return;
		}
		syslog(LOG_INFO, "%s: %s from %s",
		    ifp->name, op, ctx->sfrom);
		o = dhcp6_getmoption(D6_OPTION_RECONF_MSG, r, len);
		if (o == NULL) {
			syslog(LOG_ERR,
			    "%s: missing Reconfigure Message option",
			    ifp->name);
			return;
		}
		if (ntohs(o->len) != 1) {
			syslog(LOG_ERR,
			    "%s: missing Reconfigure Message type",
			    ifp->name);
			return;
		}
		switch(*D6_COPTION_DATA(o)) {
		case DHCP6_RENEW:
			if (state->state != DH6S_BOUND) {
				syslog(LOG_ERR,
				    "%s: not bound, ignoring %s",
				    ifp->name, op);
				return;
			}
			eloop_timeout_delete(ifp->ctx->eloop,
			    dhcp6_startrenew, ifp);
			dhcp6_startrenew(ifp);
			break;
		case DHCP6_INFORMATION_REQ:
			if (state->state != DH6S_INFORMED) {
				syslog(LOG_ERR,
				    "%s: not informed, ignoring %s",
				    ifp->name, op);
				return;
			}
			eloop_timeout_delete(ifp->ctx->eloop,
			    dhcp6_sendinform, ifp);
			dhcp6_startinform(ifp);
			break;
		default:
			syslog(LOG_ERR,
			    "%s: unsupported %s type %d",
			    ifp->name, op, *D6_COPTION_DATA(o));
			break;
		}
		return;
	default:
		syslog(LOG_ERR, "%s: invalid DHCP6 type %s (%d)",
		    ifp->name, op, r->type);
		return;
	}
	if (op == NULL) {
		syslog(LOG_WARNING, "%s: invalid state for DHCP6 type %s (%d)",
		    ifp->name, op, r->type);
		return;
	}

	if (state->recv_len < (size_t)len) {
		free(state->recv);
		state->recv = malloc(len);
		if (state->recv == NULL) {
			syslog(LOG_ERR, "%s: malloc recv: %m", ifp->name);
			return;
		}
	}
	memcpy(state->recv, r, len);
	state->recv_len = len;

	switch(r->type) {
	case DHCP6_ADVERTISE:
		if (state->state == DH6S_REQUEST) /* rapid commit */
			break;
		ap = TAILQ_FIRST(&state->addrs);
		syslog(LOG_INFO, "%s: ADV %s from %s",
		    ifp->name, ap->saddr, ctx->sfrom);
		if (ifp->ctx->options & DHCPCD_TEST)
			break;
		dhcp6_startrequest(ifp);
		return;
	}

recv:
	has_new = 0;
	TAILQ_FOREACH(ap, &state->addrs, next) {
		if (ap->flags & IPV6_AF_NEW) {
			has_new = 1;
			break;
		}
	}
	syslog(has_new ? LOG_INFO : LOG_DEBUG,
	    "%s: %s received from %s", ifp->name, op, ctx->sfrom);

	state->reason = NULL;
	eloop_timeout_delete(ifp->ctx->eloop, NULL, ifp);
	switch(state->state) {
	case DH6S_INFORM:
		state->rebind = 0;
		state->expire = ND6_INFINITE_LIFETIME;
		state->lowpl = ND6_INFINITE_LIFETIME;
		state->reason = "INFORM6";
		break;
	case DH6S_REQUEST:
		if (state->reason == NULL)
			state->reason = "BOUND6";
		/* FALLTHROUGH */
	case DH6S_RENEW:
		if (state->reason == NULL)
			state->reason = "RENEW6";
		/* FALLTHROUGH */
	case DH6S_REBIND:
		if (state->reason == NULL)
			state->reason = "REBIND6";
		/* FALLTHROUGH */
	case DH6S_CONFIRM:
		if (state->reason == NULL)
			state->reason = "REBOOT6";
		if (state->renew == 0) {
			if (state->expire == ND6_INFINITE_LIFETIME)
				state->renew = ND6_INFINITE_LIFETIME;
			else
				state->renew = state->lowpl * 0.5;
		}
		if (state->rebind == 0) {
			if (state->expire == ND6_INFINITE_LIFETIME)
				state->rebind = ND6_INFINITE_LIFETIME;
			else
				state->rebind = state->lowpl * 0.8;
		}
		break;
	default:
		state->reason = "UNKNOWN6";
		break;
	}

	if (state->state != DH6S_CONFIRM) {
		free(state->old);
		state->old = state->new;
		state->old_len = state->new_len;
		state->new = state->recv;
		state->new_len = state->recv_len;
		state->recv = NULL;
		state->recv_len = 0;
	}

	if (ifp->ctx->options & DHCPCD_TEST)
		script_runreason(ifp, "TEST");
	else {
		if (state->state == DH6S_INFORM)
			state->state = DH6S_INFORMED;
		else
			state->state = DH6S_BOUND;
		if (state->renew && state->renew != ND6_INFINITE_LIFETIME)
			eloop_timeout_add_sec(ifp->ctx->eloop, state->renew,
			    state->state == DH6S_INFORMED ?
			    dhcp6_startinform : dhcp6_startrenew, ifp);
		if (state->rebind && state->rebind != ND6_INFINITE_LIFETIME)
			eloop_timeout_add_sec(ifp->ctx->eloop, state->rebind,
			    dhcp6_startrebind, ifp);
		if (state->expire && state->expire != ND6_INFINITE_LIFETIME)
			eloop_timeout_add_sec(ifp->ctx->eloop, state->expire,
			    dhcp6_startexpire, ifp);
		if (ifp->options->ia_type == D6_OPTION_IA_PD)
			dhcp6_delegate_prefix(ifp);

		ipv6_addaddrs(&state->addrs);
		if (state->state == DH6S_INFORMED)
			syslog(has_new ? LOG_INFO : LOG_DEBUG,
			    "%s: refresh in %"PRIu32" seconds",
			    ifp->name, state->renew);
		else if (state->renew || state->rebind)
			syslog(has_new ? LOG_INFO : LOG_DEBUG,
			    "%s: renew in %"PRIu32" seconds,"
			    " rebind in %"PRIu32" seconds",
			    ifp->name, state->renew, state->rebind);
		ipv6_buildroutes(ifp->ctx);
		dhcp6_writelease(ifp);

		len = 1;
		/* If all addresses have completed DAD run the script */
		TAILQ_FOREACH(ap, &state->addrs, next) {
			if (ap->flags & IPV6_AF_ONLINK) {
				if (!(ap->flags & IPV6_AF_DADCOMPLETED) &&
				    ipv6_findaddr(ap->iface, &ap->addr))
					ap->flags |= IPV6_AF_DADCOMPLETED;
				if ((ap->flags & IPV6_AF_DADCOMPLETED) == 0) {
					len = 0;
					break;
				}
			}
		}
		if (len) {
			script_runreason(ifp, state->reason);
			daemonise(ifp->ctx);
		} else
			syslog(LOG_DEBUG,
			    "%s: waiting for DHCPv6 DAD to complete",
			    ifp->name);
	}

	if (ifp->ctx->options & DHCPCD_TEST ||
	    (ifp->options->options & DHCPCD_INFORM &&
	    !(ifp->ctx->options & DHCPCD_MASTER)))
	{
		eloop_exit(ifp->ctx->eloop, EXIT_SUCCESS);
	}
}

static int
dhcp6_open(struct dhcpcd_ctx *dctx)
{
	struct ipv6_ctx *ctx;
	struct sockaddr_in6 sa;
	int n;

	memset(&sa, 0, sizeof(sa));
	sa.sin6_family = AF_INET6;
	sa.sin6_port = htons(DHCP6_CLIENT_PORT);
#ifdef BSD
	sa.sin6_len = sizeof(sa);
#endif

	ctx = dctx->ipv6;
#ifdef SOCK_CLOEXEC
	ctx->dhcp_fd = socket(PF_INET6,
	    SOCK_DGRAM | SOCK_CLOEXEC | SOCK_NONBLOCK,
	    IPPROTO_UDP);
	if (ctx->dhcp_fd == -1)
		return -1;
#else
	if ((ctx->dhcp_fd = socket(PF_INET6, SOCK_DGRAM, IPPROTO_UDP)) == -1)
		return -1;
	if ((n = fcntl(ctx->dhcp_fd, F_GETFD, 0)) == -1 ||
	    fcntl(ctx->dhcp_fd, F_SETFD, n | FD_CLOEXEC) == -1)
	{
		close(ctx->dhcp_fd);
		ctx->dhcp_fd = -1;
	        return -1;
	}
	if ((n = fcntl(ctx->dhcp_fd, F_GETFL, 0)) == -1 ||
	    fcntl(ctx->dhcp_fd, F_SETFL, n | O_NONBLOCK) == -1)
	{
		close(ctx->dhcp_fd);
		ctx->dhcp_fd = -1;
	        return -1;
	}
#endif

	n = 1;
	if (setsockopt(ctx->dhcp_fd, SOL_SOCKET, SO_REUSEADDR,
	    &n, sizeof(n)) == -1)
		goto errexit;

	n = 1;
	if (setsockopt(ctx->dhcp_fd, SOL_SOCKET, SO_BROADCAST,
	    &n, sizeof(n)) == -1)
		goto errexit;

#ifdef SO_REUSEPORT
	n = 1;
	if (setsockopt(ctx->dhcp_fd, SOL_SOCKET, SO_REUSEPORT,
	    &n, sizeof(n)) == -1)
		goto errexit;
#endif

	if (bind(ctx->dhcp_fd, (struct sockaddr *)&sa, sizeof(sa)) == -1)
		goto errexit;

	n = 1;
	if (setsockopt(ctx->dhcp_fd, IPPROTO_IPV6, IPV6_RECVPKTINFO,
	    &n, sizeof(n)) == -1)
		goto errexit;

	eloop_event_add(dctx->eloop, ctx->dhcp_fd, dhcp6_handledata, dctx);
	return 0;

errexit:
	close(ctx->dhcp_fd);
	ctx->dhcp_fd = -1;
	return -1;
}

static void
dhcp6_start1(void *arg)
{
	struct interface *ifp = arg;
	struct if_options *ifo = ifp->options;
	struct dhcp6_state *state;
	size_t i;
	const struct dhcp_compat *dhc;

	state = D6_STATE(ifp);
	/* Match any DHCPv4 opton to DHCPv6 options if given for easy
	 * configuration */
	for (i = 0; i < sizeof(ifo->requestmask6); i++) {
		if (ifo->requestmask6[i] != '\0')
			break;
	}
	if (i == sizeof(ifo->requestmask6)) {
		for (dhc = dhcp_compats; dhc->dhcp_opt; dhc++) {
			if (has_option_mask(ifo->requestmask, dhc->dhcp_opt))
				add_option_mask(ifo->requestmask6,
				    dhc->dhcp6_opt);
		}
		if (ifo->fqdn != FQDN_DISABLE ||
		    ifo->options & DHCPCD_HOSTNAME)
			add_option_mask(ifo->requestmask6, D6_OPTION_FQDN);
	}

	if (state->state == DH6S_INFORM) {
		add_option_mask(ifo->requestmask6, D6_OPTION_INFO_REFRESH_TIME);
		dhcp6_startinform(ifp);
	} else {
		del_option_mask(ifo->requestmask6, D6_OPTION_INFO_REFRESH_TIME);
		dhcp6_startinit(ifp);
	}
}

int
dhcp6_start(struct interface *ifp, enum DH6S init_state)
{
	struct dhcp6_state *state;

	state = D6_STATE(ifp);
	if (state) {
		if (state->state == DH6S_DELEGATED) {
			dhcp6_find_delegates(ifp);
			return 0;
		}
		if (state->state == DH6S_INFORMED &&
		    init_state == DH6S_INFORM)
		{
			dhcp6_startinform(ifp);
			return 0;
		}
		/* We're already running DHCP6 */
		/* XXX: What if the managed flag changes? */
		return 0;
	}

	if (!(ifp->options->options & DHCPCD_DHCP6))
		return 0;

	if (ifp->ctx->ipv6->dhcp_fd == -1 && dhcp6_open(ifp->ctx) == -1)
		return -1;

	ifp->if_data[IF_DATA_DHCP6] = calloc(1, sizeof(*state));
	state = D6_STATE(ifp);
	if (state == NULL)
		return -1;

	state->sol_max_rt = SOL_MAX_RT;
	state->inf_max_rt = INF_MAX_RT;

	TAILQ_INIT(&state->addrs);
	if (dhcp6_find_delegates(ifp))
		return 0;

	state->state = init_state;
	snprintf(state->leasefile, sizeof(state->leasefile),
	    LEASEFILE6, ifp->name);

	if (ipv6_linklocal(ifp) == NULL) {
		syslog(LOG_DEBUG,
		    "%s: delaying DHCPv6 soliciation for LL address",
		    ifp->name);
		ipv6_addlinklocalcallback(ifp, dhcp6_start1, ifp);
		return 0;
	}

	dhcp6_start1(ifp);
	return 0;
}

void
dhcp6_reboot(struct interface *ifp)
{
	struct dhcp6_state *state;

	state = D6_STATE(ifp);
	if (state) {
		switch (state->state) {
		case DH6S_BOUND:
			dhcp6_startrebind(ifp);
			break;
		case DH6S_INFORMED:
			dhcp6_startinform(ifp);
			break;
		default:
			dhcp6_startdiscover(ifp);
			break;
		}
	}
}

static void
dhcp6_freedrop(struct interface *ifp, int drop, const char *reason)
{
	struct dhcp6_state *state;
	struct dhcpcd_ctx *ctx;
	unsigned long long options;

	if (ifp->ctx->eloop)
		eloop_timeout_delete(ifp->ctx->eloop, NULL, ifp);

	if (ifp->options)
		options = ifp->options->options;
	else
		options = 0;

	/*
	 * As the interface is going away from dhcpcd we need to
	 * remove the delegated addresses, otherwise we lose track
	 * of which interface is delegating as we remeber it by pointer.
	 * So if we need to change this behaviour, we need to change
	 * how we remember which interface delegated.
	 *
	 * XXX The below is no longer true due to the change of the
	 * default IAID, but do PPP links have stable ethernet
	 * addresses?
	 *
	 * To make it more interesting, on some OS's with PPP links
	 * there is no guarantee the delegating interface will have
	 * the same name or index so think very hard before changing
	 * this.
	 */
	if (options & (DHCPCD_STOPPING | DHCPCD_RELEASE) &&
	    (options &
	    (DHCPCD_EXITING | DHCPCD_PERSISTENT)) !=
	    (DHCPCD_EXITING | DHCPCD_PERSISTENT))
		dhcp6_delete_delegates(ifp);

	state = D6_STATE(ifp);
	if (state) {
		dhcp_auth_reset(&state->auth);
		if (options & DHCPCD_RELEASE) {
			if (ifp->carrier != LINK_DOWN)
				dhcp6_startrelease(ifp);
			unlink(state->leasefile);
		}
		dhcp6_freedrop_addrs(ifp, drop, NULL);
		if (drop && state->new &&
		    (options &
		    (DHCPCD_EXITING | DHCPCD_PERSISTENT)) !=
		    (DHCPCD_EXITING | DHCPCD_PERSISTENT))
		{
			if (reason == NULL)
				reason = "STOP6";
			script_runreason(ifp, reason);
		}
		free(state->send);
		free(state->recv);
		free(state->new);
		free(state->old);
		free(state);
		ifp->if_data[IF_DATA_DHCP6] = NULL;
	}

	/* If we don't have any more DHCP6 enabled interfaces,
	 * close the global socket and release resources */
	ctx = ifp->ctx;
	if (ctx->ifaces) {
		TAILQ_FOREACH(ifp, ctx->ifaces, next) {
			if (D6_STATE(ifp))
				break;
		}
	}
	if (ifp == NULL && ctx->ipv6) {
		if (ctx->ipv6->dhcp_fd != -1) {
			eloop_event_delete(ctx->eloop, ctx->ipv6->dhcp_fd);
			close(ctx->ipv6->dhcp_fd);
			ctx->ipv6->dhcp_fd = -1;
		}
	}
}

void
dhcp6_drop(struct interface *ifp, const char *reason)
{

	dhcp6_freedrop(ifp, 1, reason);
}

void
dhcp6_free(struct interface *ifp)
{

	dhcp6_freedrop(ifp, 0, NULL);
}

void
dhcp6_handleifa(struct dhcpcd_ctx *ctx, int cmd, const char *ifname,
    const struct in6_addr *addr, int flags)
{
	struct interface *ifp;
	struct dhcp6_state *state;

	if (ctx->ifaces == NULL)
		return;

	TAILQ_FOREACH(ifp, ctx->ifaces, next) {
		state = D6_STATE(ifp);
		if (state == NULL || strcmp(ifp->name, ifname))
			continue;
		ipv6_handleifa_addrs(cmd, &state->addrs, addr, flags);
	}

}

ssize_t
dhcp6_env(char **env, const char *prefix, const struct interface *ifp,
    const struct dhcp6_message *m, ssize_t len)
{
	const struct dhcp6_state *state;
	const struct if_options *ifo;
	struct dhcp_opt *opt, *vo;
	const struct dhcp6_option *o;
	size_t i, l, n;
	uint16_t ol, oc;
	char *v, *val, *pfx;
	const struct ipv6_addr *ap;
	uint32_t en;
	const struct dhcpcd_ctx *ctx;

	state = D6_CSTATE(ifp);
	n = 0;
	ifo = ifp->options;
	ctx = ifp->ctx;

	/* Zero our indexes */
	if (env) {
		for (i = 0, opt = ctx->dhcp6_opts;
		    i < ctx->dhcp6_opts_len;
		    i++, opt++)
			dhcp_zero_index(opt);
		for (i = 0, opt = ifp->options->dhcp6_override;
		    i < ifp->options->dhcp6_override_len;
		    i++, opt++)
			dhcp_zero_index(opt);
		for (i = 0, opt = ctx->vivso;
		    i < ctx->vivso_len;
		    i++, opt++)
			dhcp_zero_index(opt);
		i = strlen(prefix) + strlen("_dhcp6") + 1;
		pfx = malloc(i);
		if (pfx == NULL) {
			syslog(LOG_ERR, "%s: %m", __func__);
			return 0;
		}
		snprintf(pfx, i, "%s_dhcp6", prefix);
	} else
		pfx = NULL;

	/* Unlike DHCP, DHCPv6 options *may* occur more than once.
	 * There is also no provision for option concatenation unlike DHCP. */
	for (o = D6_CFIRST_OPTION(m);
	    len > (ssize_t)sizeof(*o);
	    o = D6_CNEXT_OPTION(o))
	{
		ol = ntohs(o->len);
		len -= sizeof(*o) + ol;
		if (len < 0) {
			errno = EINVAL;
			break;
		}
		oc = ntohs(o->code);
		if (has_option_mask(ifo->nomask6, oc))
			continue;
		for (i = 0, opt = ifo->dhcp6_override;
		    i < ifo->dhcp6_override_len;
		    i++, opt++)
			if (opt->option == oc)
				break;
		if (i == ifo->dhcp6_override_len &&
		    oc == D6_OPTION_VENDOR_OPTS &&
		    ol > sizeof(en))
		{
			memcpy(&en, D6_COPTION_DATA(o), sizeof(en));
			en = ntohl(en);
			vo = vivso_find(en, ifp);
		} else
			vo = NULL;
		if (i == ifo->dhcp6_override_len) {
			for (i = 0, opt = ctx->dhcp6_opts;
			    i < ctx->dhcp6_opts_len;
			    i++, opt++)
				if (opt->option == oc)
					break;
			if (i == ctx->dhcp6_opts_len)
				opt = NULL;
		}
		if (opt) {
			n += dhcp_envoption(ifp->ctx,
			    env == NULL ? NULL : &env[n],
			    pfx, ifp->name,
			    opt, dhcp6_getoption, D6_COPTION_DATA(o), ol);
		}
		if (vo) {
			n += dhcp_envoption(ifp->ctx,
			    env == NULL ? NULL : &env[n],
			    pfx, ifp->name,
			    vo, dhcp6_getoption,
			    D6_COPTION_DATA(o) + sizeof(en),
			    ol - sizeof(en));
		}
	}
	free(pfx);

	/* It is tempting to remove this section.
	 * However, we need it at least for Delegated Prefixes
	 * (they don't have a DHCPv6 message to parse to get the addressses)
	 * and it's easier for shell scripts to see which addresses have
	 * been added */
	if (TAILQ_FIRST(&state->addrs)) {
		if (env) {
			if (ifo->ia_type == D6_OPTION_IA_PD) {
				l = strlen(prefix) +
				    strlen("_dhcp6_prefix=");
				TAILQ_FOREACH(ap, &state->addrs, next) {
					l += strlen(ap->saddr) + 1;
				}
				v = val = env[n] = malloc(l);
				if (v == NULL) {
					syslog(LOG_ERR, "%s: %m", __func__);
					return -1;
				}
				i = snprintf(v, l, "%s_dhcp6_prefix=",
				    prefix);
				v += i;
				l -= i;
				TAILQ_FOREACH(ap, &state->addrs, next) {
					i = strlen(ap->saddr);
					strlcpy(v, ap->saddr, l);
					v += i;
					l -= i;
					*v++ = ' ';
				}
				*--v = '\0';
			} else {
				l = strlen(prefix) +
				    strlen("_dhcp6_ip_address=");
				TAILQ_FOREACH(ap, &state->addrs, next) {
					l += strlen(ap->saddr) + 1;
				}
				v = val = env[n] = malloc(l);
				if (v == NULL) {
					syslog(LOG_ERR, "%s: %m", __func__);
					return -1;
				}
				i = snprintf(v, l, "%s_dhcp6_ip_address=",
				    prefix);
				v += i;
				l -= i;
				TAILQ_FOREACH(ap, &state->addrs, next) {
					i = strlen(ap->saddr);
					strlcpy(v, ap->saddr, l);
					v += i;
					l -= i;
					*v++ = ' ';
				}
				*--v = '\0';
			}
		}
		n++;
	}

	return n;
}
