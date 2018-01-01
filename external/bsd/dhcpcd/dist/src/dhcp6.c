/*
 * dhcpcd - DHCP client daemon
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

/* TODO: We should decline dupliate addresses detected */

#include <sys/stat.h>
#include <sys/utsname.h>

#include <netinet/in.h>

#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#define ELOOP_QUEUE 4
#include "config.h"
#include "common.h"
#include "dhcp.h"
#include "dhcp6.h"
#include "duid.h"
#include "eloop.h"
#include "if.h"
#include "if-options.h"
#include "ipv6nd.h"
#include "logerr.h"
#include "script.h"

#ifdef HAVE_SYS_BITOPS_H
#include <sys/bitops.h>
#else
#include "compat/bitops.h"
#endif

/* DHCPCD Project has been assigned an IANA PEN of 40712 */
#define DHCPCD_IANA_PEN 40712

/* Unsure if I want this */
//#define VENDOR_SPLIT

/* Support older systems with different defines */
#if !defined(IPV6_RECVPKTINFO) && defined(IPV6_PKTINFO)
#define IPV6_RECVPKTINFO IPV6_PKTINFO
#endif

#ifdef DHCP6

/* Assert the correct structure size for on wire */
struct dhcp6_message {
	uint8_t type;
	uint8_t xid[3];
	/* followed by options */
};
__CTASSERT(sizeof(struct dhcp6_message) == 4);

struct dhcp6_option {
	uint16_t code;
	uint16_t len;
	/* followed by data */
};
__CTASSERT(sizeof(struct dhcp6_option) == 4);

struct dhcp6_ia_na {
	uint8_t iaid[4];
	uint32_t t1;
	uint32_t t2;
};
__CTASSERT(sizeof(struct dhcp6_ia_na) == 12);

struct dhcp6_ia_addr {
	struct in6_addr addr;
	uint32_t pltime;
	uint32_t vltime;
};
__CTASSERT(sizeof(struct dhcp6_ia_addr) == 16 + 8);

/* XXX FIXME: This is the only packed structure and it does not align.
 * Maybe manually decode it? */
struct dhcp6_pd_addr {
	uint32_t pltime;
	uint32_t vltime;
	uint8_t prefix_len;
	struct in6_addr prefix;
} __packed;
__CTASSERT(sizeof(struct dhcp6_pd_addr) == 8 + 1 + 16);

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
	{ DHCP6_RECONFIGURE, "RECONFIGURE6" },
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
	"Use Multicast",
	"No Prefix Available"
};

static void dhcp6_bind(struct interface *, const char *);
static void dhcp6_failinform(void *);
static int dhcp6_listen(struct dhcpcd_ctx *, struct ipv6_addr *);
static void dhcp6_recvaddr(void *);

void
dhcp6_printoptions(const struct dhcpcd_ctx *ctx,
    const struct dhcp_opt *opts, size_t opts_len)
{
	size_t i, j;
	const struct dhcp_opt *opt, *opt2;
	int cols;

	for (i = 0, opt = ctx->dhcp6_opts;
	    i < ctx->dhcp6_opts_len; i++, opt++)
	{
		for (j = 0, opt2 = opts; j < opts_len; j++, opt2++)
			if (opt2->option == opt->option)
				break;
		if (j == opts_len) {
			cols = printf("%05d %s", opt->option, opt->var);
			dhcp_print_option_encoding(opt, cols);
		}
	}
	for (i = 0, opt = opts; i < opts_len; i++, opt++) {
		cols = printf("%05d %s", opt->option, opt->var);
		dhcp_print_option_encoding(opt, cols);
	}
}

static size_t
dhcp6_makevendor(void *data, const struct interface *ifp)
{
	const struct if_options *ifo;
	size_t len, i;
	uint8_t *p;
	ssize_t vlen;
	const struct vivco *vivco;
	char vendor[VENDORCLASSID_MAX_LEN];
	struct dhcp6_option o;

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
		if (vlen == -1)
			vlen = 0;
		else
			len += sizeof(uint16_t) + (size_t)vlen;
	}

	if (len > UINT16_MAX) {
		logerrx("%s: DHCPv6 Vendor Class too big", ifp->name);
		return 0;
	}

	if (data != NULL) {
		uint32_t pen;
		uint16_t hvlen;

		p = data;
		o.code = htons(D6_OPTION_VENDOR_CLASS);
		o.len = htons((uint16_t)len);
		memcpy(p, &o, sizeof(o));
		p += sizeof(o);
		pen = htonl(ifo->vivco_en ? ifo->vivco_en : DHCPCD_IANA_PEN);
		memcpy(p, &pen, sizeof(pen));
		p += sizeof(pen);

		if (ifo->vivco_en) {
			for (i = 0, vivco = ifo->vivco;
			    i < ifo->vivco_len;
			    i++, vivco++)
			{
				hvlen = htons((uint16_t)vivco->len);
				memcpy(p, &hvlen, sizeof(hvlen));
				p += sizeof(len);
				memcpy(p, vivco->data, vivco->len);
				p += vivco->len;
			}
		} else if (vlen) {
			hvlen = htons((uint16_t)vlen);
			memcpy(p, &hvlen, sizeof(hvlen));
			p += sizeof(hvlen);
			memcpy(p, vendor, (size_t)vlen);
		}
	}

	return sizeof(o) + len;
}

static void *
dhcp6_findoption(void *data, size_t data_len, uint16_t code, uint16_t *len)
{
	uint8_t *d;
	struct dhcp6_option o;

	code = htons(code);
	for (d = data; data_len != 0; d += o.len, data_len -= o.len) {
		if (data_len < sizeof(o)) {
			errno = EINVAL;
			return NULL;
		}
		memcpy(&o, d, sizeof(o));
		d += sizeof(o);
		data_len -= sizeof(o);
		o.len = htons(o.len);
		if (data_len < o.len) {
			errno = EINVAL;
			return NULL;
		}
		if (o.code == code) {
			if (len != NULL)
				*len = o.len;
			return d;
		}
	}

	errno = ENOENT;
	return NULL;
}

static void *
dhcp6_findmoption(void *data, size_t data_len, uint16_t code,
    uint16_t *len)
{
	uint8_t *d;

	if (data_len < sizeof(struct dhcp6_message)) {
		errno = EINVAL;
		return false;
	}
	d = data;
	d += sizeof(struct dhcp6_message);
	data_len -= sizeof(struct dhcp6_message);
	return dhcp6_findoption(d, data_len, code, len);
}

static const uint8_t *
dhcp6_getoption(struct dhcpcd_ctx *ctx,
    size_t *os, unsigned int *code, size_t *len,
    const uint8_t *od, size_t ol, struct dhcp_opt **oopt)
{
	struct dhcp6_option o;
	size_t i;
	struct dhcp_opt *opt;

	if (od != NULL) {
		*os = sizeof(o);
		if (ol < *os) {
			errno = EINVAL;
			return NULL;
		}
		memcpy(&o, od, sizeof(o));
		*len = ntohs(o.len);
		if (*len > ol - *os) {
			errno = ERANGE;
			return NULL;
		}
		*code = ntohs(o.code);
	}

	*oopt = NULL;
	for (i = 0, opt = ctx->dhcp6_opts;
	    i < ctx->dhcp6_opts_len; i++, opt++)
	{
		if (opt->option == *code) {
			*oopt = opt;
			break;
		}
	}

	if (od != NULL)
		return od + sizeof(o);
	return NULL;
}

static bool
dhcp6_updateelapsed(struct interface *ifp, struct dhcp6_message *m, size_t len)
{
	uint8_t *opt;
	uint16_t opt_len;
	struct dhcp6_state *state;
	struct timespec tv;
	time_t hsec;
	uint16_t sec;

	opt = dhcp6_findmoption(m, len, D6_OPTION_ELAPSED, &opt_len);
	if (opt == NULL)
		return false;
	if (opt_len != sizeof(sec)) {
		errno = EINVAL;
		return false;
	}

	state = D6_STATE(ifp);
	clock_gettime(CLOCK_MONOTONIC, &tv);
	if (state->RTC == 0) {
		/* An RTC of zero means we're the first message
		 * out of the door, so the elapsed time is zero. */
		state->started = tv;
		hsec = 0;
	} else {
		timespecsub(&tv, &state->started, &tv);
		/* Elapsed time is measured in centiseconds.
		 * We need to be sure it will not potentially overflow. */
		if (tv.tv_sec >= (UINT16_MAX / CSEC_PER_SEC) + 1)
			hsec = UINT16_MAX;
		else {
			hsec = (tv.tv_sec * CSEC_PER_SEC) +
			    (tv.tv_nsec / NSEC_PER_CSEC);
			if (hsec > UINT16_MAX)
				hsec = UINT16_MAX;
		}
	}
	sec = htons((uint16_t)hsec);
	memcpy(opt, &sec, sizeof(sec));
	return true;
}

static void
dhcp6_newxid(const struct interface *ifp, struct dhcp6_message *m)
{
	const struct interface *ifp1;
	const struct dhcp6_state *state1;
	uint32_t xid;

	if (ifp->options->options & DHCPCD_XID_HWADDR &&
	    ifp->hwlen >= sizeof(xid))
		/* The lower bits are probably more unique on the network */
		memcpy(&xid, (ifp->hwaddr + ifp->hwlen) - sizeof(xid),
		    sizeof(xid));
	else {
again:
		xid = arc4random();
	}

	m->xid[0] = (xid >> 16) & 0xff;
	m->xid[1] = (xid >> 8) & 0xff;
	m->xid[2] = xid & 0xff;

	/* Ensure it's unique */
	TAILQ_FOREACH(ifp1, ifp->ctx->ifaces, next) {
		if (ifp == ifp1)
			continue;
		if ((state1 = D6_CSTATE(ifp1)) == NULL)
			continue;
		if (state1->send != NULL &&
		    state1->send->xid[0] == m->xid[0] &&
		    state1->send->xid[1] == m->xid[1] &&
		    state1->send->xid[2] == m->xid[2])
			break;
	}

	if (ifp1 != NULL) {
		if (ifp->options->options & DHCPCD_XID_HWADDR &&
		    ifp->hwlen >= sizeof(xid))
		{
			logerrx("%s: duplicate xid on %s",
			    ifp->name, ifp1->name);
			    return;
		}
		goto again;
	}
}

#ifndef SMALL
static const struct if_sla *
dhcp6_findselfsla(struct interface *ifp, const uint8_t *iaid)
{
	size_t i, j;

	for (i = 0; i < ifp->options->ia_len; i++) {
		if (iaid == NULL ||
		    memcmp(&ifp->options->ia[i].iaid, iaid,
		    sizeof(ifp->options->ia[i].iaid)) == 0)
		{
			for (j = 0; j < ifp->options->ia[i].sla_len; j++) {
				if (strcmp(ifp->options->ia[i].sla[j].ifname,
				    ifp->name) == 0)
					return &ifp->options->ia[i].sla[j];
			}
		}
	}
	return NULL;
}

static int
dhcp6_delegateaddr(struct in6_addr *addr, struct interface *ifp,
    const struct ipv6_addr *prefix, const struct if_sla *sla, struct if_ia *ia)
{
	struct dhcp6_state *state;
	struct if_sla asla;
	char sabuf[INET6_ADDRSTRLEN];
	const char *sa;

	state = D6_STATE(ifp);
	if (state == NULL) {
		ifp->if_data[IF_DATA_DHCP6] = calloc(1, sizeof(*state));
		state = D6_STATE(ifp);
		if (state == NULL) {
			logerr(__func__);
			return -1;
		}

		TAILQ_INIT(&state->addrs);
		state->state = DH6S_DELEGATED;
		state->reason = "DELEGATED6";
	}

	if (sla == NULL || sla->sla_set == 0) {
		/* No SLA set, so make an assumption of
		 * desired SLA and prefix length. */
		asla.sla = ifp->index;
		asla.prefix_len = 0;
		asla.sla_set = 0;
		sla = &asla;
	} else if (sla->sla == 0 && sla->prefix_len == 0) {
		/* An SLA of 0 was set with no prefix length specified.
		 * This means we delegate the whole prefix. */
		asla.sla = sla->sla;
		asla.prefix_len = prefix->prefix_len;
		asla.sla_set = 0;
		sla = &asla;
	} else if (sla->prefix_len == 0) {
		/* An SLA was given, but prefix length was not.
		 * We need to work out a suitable prefix length for
		 * potentially more than one interface. */
		asla.sla = sla->sla;
		asla.prefix_len = 0;
		asla.sla_set = 0;
		sla = &asla;
	}

	if (sla->prefix_len == 0) {
		uint32_t sla_max;
		int bits;

		if (ia->sla_max == 0) {
			const struct interface *ifi;

			sla_max = 0;
			TAILQ_FOREACH(ifi, ifp->ctx->ifaces, next) {
				if (ifi->index > sla_max)
					sla_max = ifi->index;
			}
		} else
			sla_max = ia->sla_max;

		bits = fls32(sla_max);

		if (prefix->prefix_len + bits > (int)UINT8_MAX)
			asla.prefix_len = UINT8_MAX;
		else {
			asla.prefix_len = (uint8_t)(prefix->prefix_len + bits);

			/* Make a 64 prefix by default, as this makes SLAAC
			 * possible.
			 * Otherwise round up to the nearest 4 bits. */
			if (asla.prefix_len <= 64)
				asla.prefix_len = 64;
			else
				asla.prefix_len =
				    (uint8_t)ROUNDUP4(asla.prefix_len);
		}

#define BIT(n) (1UL << (n))
#define BIT_MASK(len) (BIT(len) - 1)
		if (ia->sla_max == 0)
			/* Work out the real sla_max from our bits used */
			ia->sla_max = (uint32_t)BIT_MASK(asla.prefix_len -
			    prefix->prefix_len);
	}

	if (ipv6_userprefix(&prefix->prefix, prefix->prefix_len,
		sla->sla, addr, sla->prefix_len) == -1)
	{
		sa = inet_ntop(AF_INET6, &prefix->prefix,
		    sabuf, sizeof(sabuf));
		logerr("%s: invalid prefix %s/%d + %d/%d",
		    ifp->name, sa, prefix->prefix_len,
		    sla->sla, sla->prefix_len);
		return -1;
	}

	if (prefix->prefix_exclude_len &&
	    IN6_ARE_ADDR_EQUAL(addr, &prefix->prefix_exclude))
	{
		sa = inet_ntop(AF_INET6, &prefix->prefix_exclude,
		    sabuf, sizeof(sabuf));
		logerrx("%s: cannot delegate excluded prefix %s/%d",
		    ifp->name, sa, prefix->prefix_exclude_len);
		return -1;
	}

	return sla->prefix_len;
}
#endif

static int
dhcp6_makemessage(struct interface *ifp)
{
	struct dhcp6_state *state;
	struct dhcp6_message *m;
	struct dhcp6_option o;
	uint8_t *p, *si, *unicast, IA;
	size_t n, l, len, ml, hl;
	uint8_t type;
	uint16_t si_len, uni_len, n_options;
	uint8_t *o_lenp;
	struct if_options *ifo;
	const struct dhcp_opt *opt, *opt2;
	const struct ipv6_addr *ap;
	char hbuf[HOSTNAME_MAX_LEN + 1];
	const char *hostname;
	int fqdn;
	struct dhcp6_ia_na ia_na;
	uint16_t ia_na_len;
#ifdef AUTH
	uint16_t auth_len;
#endif

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
	if (fqdn != FQDN_DISABLE)
		hostname = dhcp_get_hostname(hbuf, sizeof(hbuf), ifo);
	else
		hostname = NULL; /* appearse gcc */

	/* Work out option size first */
	n_options = 0;
	len = 0;
	si = NULL;
	hl = 0; /* Appease gcc */
	if (state->state != DH6S_RELEASE) {
		for (l = 0, opt = ifp->ctx->dhcp6_opts;
		    l < ifp->ctx->dhcp6_opts_len;
		    l++, opt++)
		{
			for (n = 0, opt2 = ifo->dhcp6_override;
			    n < ifo->dhcp6_override_len;
			    n++, opt2++)
			{
				if (opt->option == opt2->option)
					break;
			}
			if (n < ifo->dhcp6_override_len)
			    continue;
			if (!(opt->type & OT_NOREQ) &&
			    (opt->type & OT_REQUEST ||
			    has_option_mask(ifo->requestmask6, opt->option)))
			{
				n_options++;
				len += sizeof(o.len);
			}
		}
#ifndef SMALL
		for (l = 0, opt = ifo->dhcp6_override;
		    l < ifo->dhcp6_override_len;
		    l++, opt++)
		{
			if (!(opt->type & OT_NOREQ) &&
			    (opt->type & OT_REQUEST ||
			    has_option_mask(ifo->requestmask6, opt->option)))
			{
				n_options++;
				len += sizeof(o.len);
			}
		}
		if (dhcp6_findselfsla(ifp, NULL)) {
			n_options++;
			len += sizeof(o.len);
		}
#endif
		if (len)
			len += sizeof(o);

		if (fqdn != FQDN_DISABLE) {
			hl = encode_rfc1035(hostname, NULL);
			len += sizeof(o) + 1 + hl;
		}

		if (ifo->mudurl[0])
			len += sizeof(o) + ifo->mudurl[0];

#ifdef AUTH
		if ((ifo->auth.options & DHCPCD_AUTH_SENDREQUIRE) !=
		    DHCPCD_AUTH_SENDREQUIRE)
			len += sizeof(o); /* Reconfigure Accept */
#endif
	}

	len += sizeof(*state->send);
	len += sizeof(o) + ifp->ctx->duid_len;
	len += sizeof(o) + sizeof(uint16_t); /* elapsed */
	if (!has_option_mask(ifo->nomask6, D6_OPTION_VENDOR_CLASS))
		len += dhcp6_makevendor(NULL, ifp);

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
		si = dhcp6_findmoption(m, ml, D6_OPTION_SERVERID, &si_len);
		if (si == NULL)
			return -1;
		len += sizeof(o) + si_len;
		/* FALLTHROUGH */
	case DH6S_REBIND:
		/* FALLTHROUGH */
	case DH6S_CONFIRM:
		/* FALLTHROUGH */
	case DH6S_DISCOVER:
		if (m == NULL) {
			m = state->new;
			ml = state->new_len;
		}
		TAILQ_FOREACH(ap, &state->addrs, next) {
			if (ap->flags & IPV6_AF_STALE)
				continue;
			if (ap->prefix_vltime == 0 &&
			    !(ap->flags & IPV6_AF_REQUEST))
				continue;
			if (ap->ia_type == D6_OPTION_IA_PD) {
#ifndef SMALL
				len += sizeof(o) + sizeof(struct dhcp6_pd_addr);
				if (ap->prefix_exclude_len)
					len += sizeof(o) + 1 +
					    (uint8_t)((ap->prefix_exclude_len -
					    ap->prefix_len - 1) / NBBY) + 1;
#endif
			} else
				len += sizeof(o) + sizeof(struct dhcp6_ia_addr);
		}
		/* FALLTHROUGH */
	case DH6S_INIT:
		len += ifo->ia_len * (sizeof(o) + (sizeof(uint32_t) * 3));
		IA = 1;
		break;
	default:
		IA = 0;
	}

	if (state->state == DH6S_DISCOVER &&
	    !(ifp->ctx->options & DHCPCD_TEST) &&
	    has_option_mask(ifo->requestmask6, D6_OPTION_RAPID_COMMIT))
		len += sizeof(o);

	if (m == NULL) {
		m = state->new;
		ml = state->new_len;
	}
	unicast = NULL;
	/* Depending on state, get the unicast address */
	switch(state->state) {
	case DH6S_INIT: /* FALLTHROUGH */
	case DH6S_DISCOVER:
		type = DHCP6_SOLICIT;
		break;
	case DH6S_REQUEST:
		type = DHCP6_REQUEST;
		unicast = dhcp6_findmoption(m, ml, D6_OPTION_UNICAST, &uni_len);
		break;
	case DH6S_CONFIRM:
		type = DHCP6_CONFIRM;
		break;
	case DH6S_REBIND:
		type = DHCP6_REBIND;
		break;
	case DH6S_RENEW:
		type = DHCP6_RENEW;
		unicast = dhcp6_findmoption(m, ml, D6_OPTION_UNICAST, &uni_len);
		break;
	case DH6S_INFORM:
		type = DHCP6_INFORMATION_REQ;
		break;
	case DH6S_RELEASE:
		type = DHCP6_RELEASE;
		unicast = dhcp6_findmoption(m, ml, D6_OPTION_UNICAST, &uni_len);
		break;
	default:
		errno = EINVAL;
		return -1;
	}

#ifdef AUTH
	auth_len = 0;
	if (ifo->auth.options & DHCPCD_AUTH_SEND) {
		ssize_t alen = dhcp_auth_encode(&ifo->auth,
		    state->auth.token, NULL, 0, 6, type, NULL, 0);
		if (alen != -1 && alen > UINT16_MAX) {
			errno = ERANGE;
			alen = -1;
		}
		if (alen == -1)
			logerr("%s: %s: dhcp_auth_encode", __func__, ifp->name);
		else if (alen != 0) {
			auth_len = (uint16_t)alen;
			len += sizeof(o) + auth_len;
		}
	}
#endif

	state->send = malloc(len);
	if (state->send == NULL)
		return -1;

	state->send_len = len;
	state->send->type = type;

	/* If we found a unicast option, copy it to our state for sending */
	if (unicast && uni_len == sizeof(state->unicast))
		memcpy(&state->unicast, unicast, sizeof(state->unicast));
	else
		state->unicast = in6addr_any;

	dhcp6_newxid(ifp, state->send);

#define COPYIN1(_code, _len)		{	\
	o.code = htons((_code));		\
	o.len = htons((_len));			\
	memcpy(p, &o, sizeof(o));		\
	p += sizeof(o);				\
}
#define COPYIN(_code, _data, _len)	{	\
	COPYIN1((_code), (_len));		\
	if ((_len) != 0) {			\
		memcpy(p, (_data), (_len));	\
		p += (_len);			\
	}					\
}
#define NEXTLEN (p + offsetof(struct dhcp6_option, len))

	p = (uint8_t *)state->send + sizeof(*state->send);
	COPYIN(D6_OPTION_CLIENTID, ifp->ctx->duid,
	    (uint16_t)ifp->ctx->duid_len);

	if (si != NULL)
		COPYIN(D6_OPTION_SERVERID, si, si_len);

	si_len = 0;
	COPYIN(D6_OPTION_ELAPSED, &si_len, sizeof(si_len));

	if (!has_option_mask(ifo->nomask6, D6_OPTION_VENDOR_CLASS))
		p += dhcp6_makevendor(p, ifp);

	if (state->state == DH6S_DISCOVER &&
	    !(ifp->ctx->options & DHCPCD_TEST) &&
	    has_option_mask(ifo->requestmask6, D6_OPTION_RAPID_COMMIT))
		COPYIN1(D6_OPTION_RAPID_COMMIT, 0);

	for (l = 0; IA && l < ifo->ia_len; l++) {
		o_lenp = NEXTLEN;
		ia_na_len = sizeof(ia_na);
		memcpy(ia_na.iaid, ifo->ia[l].iaid, sizeof(ia_na.iaid));
		ia_na.t1 = 0;
		ia_na.t2 = 0;
		COPYIN(ifo->ia[l].ia_type, &ia_na, sizeof(ia_na));
		TAILQ_FOREACH(ap, &state->addrs, next) {
			if (ap->flags & IPV6_AF_STALE)
				continue;
			if (ap->prefix_vltime == 0 &&
			    !(ap->flags & IPV6_AF_REQUEST))
				continue;
			if (memcmp(ifo->ia[l].iaid, ap->iaid, sizeof(uint32_t)))
				continue;
			if (ap->ia_type == D6_OPTION_IA_PD) {
#ifndef SMALL
				struct dhcp6_pd_addr pdp;

				pdp.pltime = htonl(ap->prefix_pltime);
				pdp.vltime = htonl(ap->prefix_vltime);
				pdp.prefix_len = ap->prefix_len;
				/* pdp.prefix is not aligned, so copy it in. */
				memcpy(&pdp.prefix, &ap->prefix, sizeof(pdp.prefix));
				COPYIN(D6_OPTION_IAPREFIX, &pdp, sizeof(pdp));
				ia_na_len = (uint16_t)
				    (ia_na_len + sizeof(o) + sizeof(pdp));

				/* RFC6603 Section 4.2 */
				if (ap->prefix_exclude_len) {
					uint8_t exb[16], *ep, u8;
					const uint8_t *pp;

					n = (size_t)((ap->prefix_exclude_len -
					    ap->prefix_len - 1) / NBBY) + 1;
					ep = exb;
					*ep++ = (uint8_t)ap->prefix_exclude_len;
					pp = ap->prefix_exclude.s6_addr;
					pp += (size_t)
					    ((ap->prefix_len - 1) / NBBY) +
					    (n - 1);
					u8 = ap->prefix_len % NBBY;
					if (u8)
						n--;
					while (n-- > 0)
						*ep++ = *pp--;
					if (u8)
						*ep = (uint8_t)(*pp << u8);
					n++;
					COPYIN(D6_OPTION_PD_EXCLUDE, exb,
					    (uint16_t)n);
					ia_na_len = (uint16_t)
					    (ia_na_len + sizeof(o) + n);
				}
#endif
			} else {
				struct dhcp6_ia_addr ia;

				ia.addr = ap->addr;
				ia.pltime = htonl(ap->prefix_pltime);
				ia.vltime = htonl(ap->prefix_vltime);
				COPYIN(D6_OPTION_IA_ADDR, &ia, sizeof(ia));
				ia_na_len = (uint16_t)
				    (ia_na_len + sizeof(o) + sizeof(ia));
			}
		}

		/* Update the total option lenth. */
		ia_na_len = htons(ia_na_len);
		memcpy(o_lenp, &ia_na_len, sizeof(ia_na_len));
	}

	if (state->send->type != DHCP6_RELEASE) {
		if (fqdn != FQDN_DISABLE) {
			o_lenp = NEXTLEN;
			COPYIN1(D6_OPTION_FQDN, 0);
			if (hl == 0)
				*p = D6_FQDN_NONE;
			else {
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
			}
			p++;
			encode_rfc1035(hostname, p);
			p += hl;
			o.len = htons((uint16_t)(hl + 1));
			memcpy(o_lenp, &o.len, sizeof(o.len));
		}

		if (ifo->mudurl[0])
			COPYIN(D6_OPTION_MUDURL,
			    ifo->mudurl + 1, ifo->mudurl[0]);

#ifdef AUTH
		if ((ifo->auth.options & DHCPCD_AUTH_SENDREQUIRE) !=
		    DHCPCD_AUTH_SENDREQUIRE)
			COPYIN1(D6_OPTION_RECONF_ACCEPT, 0);
#endif

		if (n_options) {
			o_lenp = NEXTLEN;
			o.len = 0;
			COPYIN1(D6_OPTION_ORO, 0);
			for (l = 0, opt = ifp->ctx->dhcp6_opts;
			    l < ifp->ctx->dhcp6_opts_len;
			    l++, opt++)
			{
#ifndef SMALL
				for (n = 0, opt2 = ifo->dhcp6_override;
				    n < ifo->dhcp6_override_len;
				    n++, opt2++)
				{
					if (opt->option == opt2->option)
						break;
				}
				if (n < ifo->dhcp6_override_len)
				    continue;
#endif
				if (!(opt->type & OT_NOREQ) &&
				    (opt->type & OT_REQUEST ||
				    has_option_mask(ifo->requestmask6,
				        opt->option)))
				{
					o.code = htons((uint16_t)opt->option);
					memcpy(p, &o.code, sizeof(o.code));
					p += sizeof(o.code);
					o.len = (uint16_t)
					    (o.len + sizeof(o.code));
				}
			}
#ifndef SMALL
			for (l = 0, opt = ifo->dhcp6_override;
			    l < ifo->dhcp6_override_len;
			    l++, opt++)
			{
				if (!(opt->type & OT_NOREQ) &&
				    (opt->type & OT_REQUEST ||
				    has_option_mask(ifo->requestmask6,
				        opt->option)))
				{
					o.code = htons((uint16_t)opt->option);
					memcpy(p, &o.code, sizeof(o.code));
					p += sizeof(o.code);
					o.len = (uint16_t)
					    (o.len + sizeof(o.code));
				}
			}
			if (dhcp6_findselfsla(ifp, NULL)) {
				o.code = htons(D6_OPTION_PD_EXCLUDE);
				memcpy(p, &o.code, sizeof(o.code));
				p += sizeof(o.code);
				o.len = (uint16_t)(o.len + sizeof(o.code));
			}
#endif
			o.len = htons(o.len);
			memcpy(o_lenp, &o.len, sizeof(o.len));
		}
	}

#ifdef AUTH
	/* This has to be the last option */
	if (ifo->auth.options & DHCPCD_AUTH_SEND && auth_len != 0) {
		COPYIN1(D6_OPTION_AUTH, auth_len);
		/* data will be filled at send message time */
	}
#endif

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
			rt_build(ifp->ctx, AF_INET6);
	}
}

#ifndef SMALL
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
#endif

#ifdef AUTH
static ssize_t
dhcp6_update_auth(struct interface *ifp, struct dhcp6_message *m, size_t len)
{
	struct dhcp6_state *state;
	uint8_t *opt;
	uint16_t opt_len;

	opt = dhcp6_findmoption(m, len, D6_OPTION_AUTH, &opt_len);
	if (opt == NULL)
		return -1;

	state = D6_STATE(ifp);
	return dhcp_auth_encode(&ifp->options->auth, state->auth.token,
	    (uint8_t *)state->send, state->send_len,
	    6, state->send->type, opt, opt_len);
}
#endif

static int
dhcp6_sendmessage(struct interface *ifp, void (*callback)(void *))
{
	struct dhcp6_state *state;
	struct dhcpcd_ctx *ctx;
	struct sockaddr_in6 dst;
	struct timespec RTprev;
	double rnd;
	time_t ms;
	uint8_t neg;
	const char *broad_uni;
	const struct in6_addr alldhcp = IN6ADDR_LINKLOCAL_ALLDHCP_INIT;

	if (!callback && ifp->carrier == LINK_DOWN)
		return 0;

	memset(&dst, 0, sizeof(dst));
	dst.sin6_family = AF_INET6;
	dst.sin6_port = htons(DHCP6_SERVER_PORT);
#ifdef HAVE_SA_LEN
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
		logdebugx("%s: %s %s with xid 0x%02x%02x%02x",
		    ifp->name,
		    broad_uni,
		    dhcp6_get_op(state->send->type),
		    state->send->xid[0],
		    state->send->xid[1],
		    state->send->xid[2]);
	else {
		if (state->IMD &&
		    !(ifp->options->options & DHCPCD_INITIAL_DELAY))
			state->IMD = 0;
		if (state->IMD) {
			/* Some buggy PPP servers close the link too early
			 * after sending an invalid status in their reply
			 * which means this host won't see it.
			 * 1 second grace seems to be the sweet spot. */
			if (ifp->flags & IFF_POINTOPOINT)
				state->RT.tv_sec = 1;
			else
				state->RT.tv_sec = 0;
			state->RT.tv_nsec = (suseconds_t)arc4random_uniform(
			    (uint32_t)(state->IMD * NSEC_PER_SEC));
			timespecnorm(&state->RT);
			broad_uni = "delaying";
			goto logsend;
		}
		if (state->RTC == 0) {
			RTprev.tv_sec = state->IRT;
			RTprev.tv_nsec = 0;
			state->RT.tv_sec = RTprev.tv_sec;
			state->RT.tv_nsec = 0;
		} else {
			RTprev = state->RT;
			timespecadd(&state->RT, &state->RT, &state->RT);
		}

		rnd = DHCP6_RAND_MIN;
		rnd += (suseconds_t)arc4random_uniform(
		    DHCP6_RAND_MAX - DHCP6_RAND_MIN);
		rnd /= MSEC_PER_SEC;
		neg = (rnd < 0.0);
		if (neg)
			rnd = -rnd;
		ts_to_ms(ms, &RTprev);
		ms = (time_t)((double)ms * rnd);
		ms_to_ts(&RTprev, ms);
		if (neg)
			timespecsub(&state->RT, &RTprev, &state->RT);
		else
			timespecadd(&state->RT, &RTprev, &state->RT);

		if (state->MRT != 0 && state->RT.tv_sec > state->MRT) {
			RTprev.tv_sec = state->MRT;
			RTprev.tv_nsec = 0;
			state->RT.tv_sec = state->MRT;
			state->RT.tv_nsec = 0;
			ts_to_ms(ms, &RTprev);
			ms = (time_t)((double)ms * rnd);
			ms_to_ts(&RTprev, ms);
			if (neg)
				timespecsub(&state->RT, &RTprev, &state->RT);
			else
				timespecadd(&state->RT, &RTprev, &state->RT);
		}

logsend:
		if (ifp->carrier != LINK_DOWN)
			logdebugx("%s: %s %s (xid 0x%02x%02x%02x),"
			    " next in %0.1f seconds",
			    ifp->name,
			    broad_uni,
			    dhcp6_get_op(state->send->type),
			    state->send->xid[0],
			    state->send->xid[1],
			    state->send->xid[2],
			    timespec_to_double(&state->RT));

		/* This sometimes happens when we delegate to this interface
		 * AND run DHCPv6 on it normally. */
		assert(timespec_to_double(&state->RT) != 0);

		/* Wait the initial delay */
		if (state->IMD != 0) {
			state->IMD = 0;
			eloop_timeout_add_tv(ifp->ctx->eloop,
			    &state->RT, callback, ifp);
			return 0;
		}
	}

	if (ifp->carrier == LINK_DOWN)
		return 0;

	/* Update the elapsed time */
	dhcp6_updateelapsed(ifp, state->send, state->send_len);
#ifdef AUTH
	if (ifp->options->auth.options & DHCPCD_AUTH_SEND &&
	    dhcp6_update_auth(ifp, state->send, state->send_len) == -1)
	{
		logerr("%s: %s: dhcp6_updateauth", __func__, ifp->name);
		if (errno != ESRCH)
			return -1;
	}
#endif

	ctx = ifp->ctx;
	ctx->sndhdr.msg_name = (void *)&dst;
	ctx->sndhdr.msg_iov[0].iov_base = state->send;
	ctx->sndhdr.msg_iov[0].iov_len = state->send_len;

	/* Set the outbound interface */
	if (IN6_ARE_ADDR_EQUAL(&dst.sin6_addr, &alldhcp)) {
		struct cmsghdr *cm;
		struct in6_pktinfo pi;

		dst.sin6_scope_id = ifp->index;
		cm = CMSG_FIRSTHDR(&ctx->sndhdr);
		if (cm == NULL) /* unlikely */
			return -1;
		cm->cmsg_level = IPPROTO_IPV6;
		cm->cmsg_type = IPV6_PKTINFO;
		cm->cmsg_len = CMSG_LEN(sizeof(pi));
		memset(&pi, 0, sizeof(pi));
		pi.ipi6_ifindex = ifp->index;
		memcpy(CMSG_DATA(cm), &pi, sizeof(pi));
	} else {
		/* Remove the control buffer as we're not dictating
		 * which interface to use for outgoing messages. */
		ctx->sndhdr.msg_control = NULL;
		ctx->sndhdr.msg_controllen = 0;
	}

	if (sendmsg(ctx->dhcp6_fd, &ctx->sndhdr, 0) == -1) {
		logerr("%s: %s: sendmsg", __func__, ifp->name);
		/* Allow DHCPv6 to continue .... the errors
		 * would be rate limited by the protocol.
		 * Generally the error is ENOBUFS when struggling to
		 * associate with an access point. */
	}

	/* Restore the control buffer assignment. */
	if (!IN6_ARE_ADDR_EQUAL(&dst.sin6_addr, &alldhcp)) {
		ctx->sndhdr.msg_control = ctx->sndbuf;
		ctx->sndhdr.msg_controllen = sizeof(ctx->sndbuf);
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
			logwarnx("%s: sent %d times with no reply",
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

static void
dhcp6_sendrelease(void *arg)
{

	dhcp6_sendmessage(arg, dhcp6_sendrelease);
}

static void
dhcp6_startrenew(void *arg)
{
	struct interface *ifp;
	struct dhcp6_state *state;

	ifp = arg;
	if ((state = D6_STATE(ifp)) == NULL)
		return;

	/* Only renew in the bound or renew states */
	if (state->state != DH6S_BOUND &&
	    state->state != DH6S_RENEW)
		return;

	/* Remove the timeout as the renew may have been forced. */
	eloop_timeout_delete(ifp->ctx->eloop, dhcp6_startrenew, ifp);

	state->state = DH6S_RENEW;
	state->RTC = 0;
	state->IMD = REN_MAX_DELAY;
	state->IRT = REN_TIMEOUT;
	state->MRT = REN_MAX_RT;
	state->MRC = 0;

	if (dhcp6_makemessage(ifp) == -1)
		logerr("%s: %s", __func__, ifp->name);
	else
		dhcp6_sendrenew(ifp);
}

void dhcp6_renew(struct interface *ifp)
{

	dhcp6_startrenew(ifp);
}

int
dhcp6_dadcompleted(const struct interface *ifp)
{
	const struct dhcp6_state *state;
	const struct ipv6_addr *ap;

	state = D6_CSTATE(ifp);
	TAILQ_FOREACH(ap, &state->addrs, next) {
		if (ap->flags & IPV6_AF_ADDED &&
		    !(ap->flags & IPV6_AF_DADCOMPLETED))
			return 0;
	}
	return 1;
}

static void
dhcp6_dadcallback(void *arg)
{
	struct ipv6_addr *ia = arg;
	struct interface *ifp;
	struct dhcp6_state *state;
	int wascompleted, valid;

	wascompleted = (ia->flags & IPV6_AF_DADCOMPLETED);
	ia->flags |= IPV6_AF_DADCOMPLETED;
	if (ia->flags & IPV6_AF_DUPLICATED) {
		/* XXX FIXME
		 * We should decline the address */
		logwarnx("%s: DAD detected %s", ia->iface->name, ia->saddr);
	}

	if (!wascompleted) {
		ifp = ia->iface;

		state = D6_STATE(ifp);
		if (state->state == DH6S_BOUND ||
		    state->state == DH6S_DELEGATED)
		{
			struct ipv6_addr *ia2;

#ifdef SMALL
			valid = true;
#else
			valid = (ia->delegating_prefix == NULL);
#endif
			TAILQ_FOREACH(ia2, &state->addrs, next) {
				if (ia2->flags & IPV6_AF_ADDED &&
				    !(ia2->flags & IPV6_AF_DADCOMPLETED))
				{
					wascompleted = 1;
					break;
				}
			}
			if (!wascompleted) {
				logdebugx("%s: DHCPv6 DAD completed",
				    ifp->name);
				script_runreason(ifp,
#ifndef SMALL
				    ia->delegating_prefix ? "DELEGATED6" :
#endif
				    state->reason);
				if (valid)
					dhcpcd_daemonise(ifp->ctx);
			}
		}
	}
}

static void
dhcp6_addrequestedaddrs(struct interface *ifp)
{
	struct dhcp6_state *state;
	size_t i;
	struct if_ia *ia;
	struct ipv6_addr *a;

	state = D6_STATE(ifp);
	/* Add any requested prefixes / addresses */
	for (i = 0; i < ifp->options->ia_len; i++) {
		ia = &ifp->options->ia[i];
		if (!((ia->ia_type == D6_OPTION_IA_PD && ia->prefix_len) ||
		    !IN6_IS_ADDR_UNSPECIFIED(&ia->addr)))
			continue;
		a = ipv6_newaddr(ifp, &ia->addr,
			/*
			 * RFC 5942 Section 5
			 * We cannot assume any prefix length, nor tie the
			 * address to an existing one as it could expire
			 * before the address.
			 * As such we just give it a 128 prefix.
			 */
		    ia->ia_type == D6_OPTION_IA_PD ? ia->prefix_len : 128,
		    IPV6_AF_REQUEST);
		if (a == NULL)
			continue;
		a->dadcallback = dhcp6_dadcallback;
		memcpy(&a->iaid, &ia->iaid, sizeof(a->iaid));
		a->ia_type = ia->ia_type;
		TAILQ_INSERT_TAIL(&state->addrs, a, next);
	}
}

static void
dhcp6_startdiscover(void *arg)
{
	struct interface *ifp;
	struct dhcp6_state *state;

	ifp = arg;
#ifndef SMALL
	dhcp6_delete_delegates(ifp);
#endif
	loginfox("%s: soliciting a DHCPv6 lease", ifp->name);
	state = D6_STATE(ifp);
	state->state = DH6S_DISCOVER;
	state->RTC = 0;
	state->IMD = SOL_MAX_DELAY;
	state->IRT = SOL_TIMEOUT;
	state->MRT = state->sol_max_rt;
	state->MRC = SOL_MAX_RC;

	eloop_timeout_delete(ifp->ctx->eloop, NULL, ifp);
	free(state->new);
	state->new = NULL;
	state->new_len = 0;

	if (dhcp6_makemessage(ifp) == -1)
		logerr("%s: %s", __func__, ifp->name);
	else
		dhcp6_senddiscover(ifp);
}

static void
dhcp6_startinform(void *arg)
{
	struct interface *ifp;
	struct dhcp6_state *state;

	ifp = arg;
	state = D6_STATE(ifp);
	if (state->new == NULL || ifp->options->options & DHCPCD_DEBUG)
		loginfox("%s: requesting DHCPv6 information", ifp->name);
	state->state = DH6S_INFORM;
	state->RTC = 0;
	state->IMD = INF_MAX_DELAY;
	state->IRT = INF_TIMEOUT;
	state->MRT = state->inf_max_rt;
	state->MRC = 0;

	eloop_timeout_delete(ifp->ctx->eloop, NULL, ifp);
	if (dhcp6_makemessage(ifp) == -1) {
		logerr("%s: %s", __func__, ifp->name);
		return;
	}
	dhcp6_sendinform(ifp);
	/* RFC3315 18.1.2 says that if CONFIRM failed then the prior addresses
	 * SHOULD be used. The wording here is poor, because the addresses are
	 * merely one facet of the lease as a whole.
	 * This poor wording might explain the lack of similar text for INFORM
	 * in 18.1.5 because there are no addresses in the INFORM message. */
	eloop_timeout_add_sec(ifp->ctx->eloop,
	    INF_MAX_RD, dhcp6_failinform, ifp);
}

static void
dhcp6_fail(struct interface *ifp)
{
	struct dhcp6_state *state = D6_STATE(ifp);

	/* RFC3315 18.1.2 says that prior addresses SHOULD be used on failure.
	 * RFC2131 3.2.3 says that MAY chose to use the prior address.
	 * Because dhcpcd was written first for RFC2131, we have the LASTLEASE
	 * option which defaults to off as that makes the most sense for
	 * mobile clients.
	 * dhcpcd also has LASTLEASE_EXTEND to extend this lease past it's
	 * expiry, but this is strictly not RFC compliant in any way or form. */
	if (state->new == NULL ||
	    !(ifp->options->options & DHCPCD_LASTLEASE))
	{
#ifndef SMALL
		dhcp6_delete_delegates(ifp);
#endif
		if (state->state != DH6S_INFORM)
			dhcp6_startdiscover(ifp);
		return;
	}

	switch (state->state) {
	case DH6S_INFORM:
	case DH6S_INFORMED:
		state->state = DH6S_ITIMEDOUT;
		break;
	default:
		state->state = DH6S_TIMEDOUT;
		break;
	}

	dhcp6_bind(ifp, NULL);

	switch (state->state) {
	case DH6S_BOUND:
	case DH6S_INFORMED:
		break;
	default:
		dhcp6_startdiscover(ifp);
		break;
	}
}

static void
dhcp6_failconfirm(void *arg)
{
	struct interface *ifp;

	ifp = arg;
	logerrx("%s: failed to confirm prior address", ifp->name);
	dhcp6_fail(ifp);
}

static void
dhcp6_failrequest(void *arg)
{
	struct interface *ifp;

	ifp = arg;
	logerrx("%s: failed to request address", ifp->name);
	dhcp6_fail(ifp);
}

static void
dhcp6_failinform(void *arg)
{
	struct interface *ifp;

	ifp = arg;
	logerrx("%s: failed to request information", ifp->name);
	dhcp6_fail(ifp);
}

#ifdef SMALL
#define dhcp6_hasprefixdelegation(a)	(0)
#else
static void
dhcp6_failrebind(void *arg)
{
	struct interface *ifp;

	ifp = arg;
	logerrx("%s: failed to rebind prior delegation", ifp->name);
	dhcp6_fail(ifp);
}

static int
dhcp6_hasprefixdelegation(struct interface *ifp)
{
	size_t i;
	uint16_t t;

	t = 0;
	for (i = 0; i < ifp->options->ia_len; i++) {
		if (t && t != ifp->options->ia[i].ia_type) {
			if (t == D6_OPTION_IA_PD ||
			    ifp->options->ia[i].ia_type == D6_OPTION_IA_PD)
				return 2;
		}
		t = ifp->options->ia[i].ia_type;
	}
	return t == D6_OPTION_IA_PD ? 1 : 0;
}
#endif

static void
dhcp6_startrebind(void *arg)
{
	struct interface *ifp;
	struct dhcp6_state *state;
#ifndef SMALL
	int pd;
#endif

	ifp = arg;
	eloop_timeout_delete(ifp->ctx->eloop, dhcp6_sendrenew, ifp);
	state = D6_STATE(ifp);
	if (state->state == DH6S_RENEW)
		logwarnx("%s: failed to renew DHCPv6, rebinding", ifp->name);
	else
		loginfox("%s: rebinding prior DHCPv6 lease", ifp->name);
	state->state = DH6S_REBIND;
	state->RTC = 0;
	state->MRC = 0;

#ifndef SMALL
	/* RFC 3633 section 12.1 */
	pd = dhcp6_hasprefixdelegation(ifp);
	if (pd) {
		state->IMD = CNF_MAX_DELAY;
		state->IRT = CNF_TIMEOUT;
		state->MRT = CNF_MAX_RT;
	} else
#endif
	{
		state->IMD = REB_MAX_DELAY;
		state->IRT = REB_TIMEOUT;
		state->MRT = REB_MAX_RT;
	}

	if (dhcp6_makemessage(ifp) == -1)
		logerr("%s: %s", __func__, ifp->name);
	else
		dhcp6_sendrebind(ifp);

#ifndef SMALL
	/* RFC 3633 section 12.1 */
	if (pd)
		eloop_timeout_add_sec(ifp->ctx->eloop,
		    CNF_MAX_RD, dhcp6_failrebind, ifp);
#endif
}


static void
dhcp6_startrequest(struct interface *ifp)
{
	struct dhcp6_state *state;

	eloop_timeout_delete(ifp->ctx->eloop, dhcp6_senddiscover, ifp);
	state = D6_STATE(ifp);
	state->state = DH6S_REQUEST;
	state->RTC = 0;
	state->IMD = 0;
	state->IRT = REQ_TIMEOUT;
	state->MRT = REQ_MAX_RT;
	state->MRC = REQ_MAX_RC;
	state->MRCcallback = dhcp6_failrequest;

	if (dhcp6_makemessage(ifp) == -1) {
		logerr("%s: %s", __func__, ifp->name);
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
	state->RTC = 0;
	state->IMD = CNF_MAX_DELAY;
	state->IRT = CNF_TIMEOUT;
	state->MRT = CNF_MAX_RT;
	state->MRC = CNF_MAX_RC;

	loginfox("%s: confirming prior DHCPv6 lease", ifp->name);
	if (dhcp6_makemessage(ifp) == -1) {
		logerr("%s: %s", __func__, ifp->name);
		return;
	}
	dhcp6_sendconfirm(ifp);
	eloop_timeout_add_sec(ifp->ctx->eloop,
	    CNF_MAX_RD, dhcp6_failconfirm, ifp);
}

static void
dhcp6_leaseextend(struct interface *ifp)
{
	struct dhcp6_state *state = D6_STATE(ifp);
	struct ipv6_addr *ia;

	logwarnx("%s: extending DHCPv6 lease", ifp->name);
	TAILQ_FOREACH(ia, &state->addrs, next) {
		ia->flags |= IPV6_AF_EXTENDED;
		/* Set infinite lifetimes. */
		ia->prefix_pltime = ND6_INFINITE_LIFETIME;
		ia->prefix_vltime = ND6_INFINITE_LIFETIME;
	}
}

static void
dhcp6_startexpire(void *arg)
{
	struct interface *ifp;

	ifp = arg;
	eloop_timeout_delete(ifp->ctx->eloop, dhcp6_sendrebind, ifp);

	logerrx("%s: DHCPv6 lease expired", ifp->name);
	if (ifp->options->options & DHCPCD_LASTLEASE_EXTEND) {
		struct dhcp6_state *state = D6_STATE(ifp);

		dhcp6_leaseextend(ifp);
		ipv6_addaddrs(&state->addrs);
	} else {
		dhcp6_freedrop_addrs(ifp, 1, NULL);
#ifndef SMALL
		dhcp6_delete_delegates(ifp);
#endif
		script_runreason(ifp, "EXPIRE6");
	}
	if (!(ifp->options->options & DHCPCD_IPV6RS) ||
	    ipv6nd_hasradhcp(ifp) ||
	    dhcp6_hasprefixdelegation(ifp))
		dhcp6_startdiscover(ifp);
	else
		logwarnx("%s: no advertising IPv6 router wants DHCP",ifp->name);
}

static void
dhcp6_finishrelease(void *arg)
{
	struct interface *ifp;
	struct dhcp6_state *state;

	ifp = (struct interface *)arg;
	if ((state = D6_STATE(ifp)) != NULL) {
		state->state = DH6S_RELEASED;
		dhcp6_drop(ifp, "RELEASE6");
	}
}

static void
dhcp6_startrelease(struct interface *ifp)
{
	struct dhcp6_state *state;

	state = D6_STATE(ifp);
	if (state->state != DH6S_BOUND)
		return;

	state->state = DH6S_RELEASE;
	state->RTC = 0;
	state->IMD = REL_MAX_DELAY;
	state->IRT = REL_TIMEOUT;
	state->MRT = REL_MAX_RT;
	/* MRC of REL_MAX_RC is optional in RFC 3315 18.1.6 */
#if 0
	state->MRC = REL_MAX_RC;
	state->MRCcallback = dhcp6_finishrelease;
#else
	state->MRC = 0;
	state->MRCcallback = NULL;
#endif

	if (dhcp6_makemessage(ifp) == -1)
		logerr("%s: %s", __func__, ifp->name);
	else {
		dhcp6_sendrelease(ifp);
		dhcp6_finishrelease(ifp);
	}
}

static int
dhcp6_checkstatusok(const struct interface *ifp,
    struct dhcp6_message *m, uint8_t *p, size_t len)
{
	uint8_t *opt;
	uint16_t opt_len, code;
	void * (*f)(void *, size_t, uint16_t, uint16_t *), *farg;
	char buf[32], *sbuf;
	const char *status;

	f = p ? dhcp6_findoption : dhcp6_findmoption;
	if (p)
		farg = p;
	else
		farg = m;
	if ((opt = f(farg, len, D6_OPTION_STATUS_CODE, &opt_len)) == NULL) {
		//logdebugx("%s: no status", ifp->name);
		return 0;
	}

	if (opt_len < sizeof(code)) {
		logerrx("%s: status truncated", ifp->name);
		return -1;
	}
	memcpy(&code, opt, sizeof(code));
	code = ntohs(code);
	if (code == D6_STATUS_OK)
		return 1;

	/* Anything after the code is a message. */
	opt += sizeof(code);
	opt_len = (uint16_t)(opt_len - sizeof(code));
	if (opt_len == 0) {
		sbuf = NULL;
		if (code < sizeof(dhcp6_statuses) / sizeof(char *))
			status = dhcp6_statuses[code];
		else {
			snprintf(buf, sizeof(buf), "Unknown Status (%d)", code);
			status = buf;
		}
	} else {
		if ((sbuf = malloc((size_t)opt_len + 1)) == NULL) {
			logerr(__func__);
			return -1;
		}
		memcpy(sbuf, opt, opt_len);
		sbuf[len] = '\0';
		status = sbuf;
	}

	logerrx("%s: DHCPv6 REPLY: %s", ifp->name, status);
	free(sbuf);
	return -1;
}

const struct ipv6_addr *
dhcp6_iffindaddr(const struct interface *ifp, const struct in6_addr *addr,
    unsigned int flags)
{
	const struct dhcp6_state *state;
	const struct ipv6_addr *ap;

	if ((state = D6_STATE(ifp)) != NULL) {
		TAILQ_FOREACH(ap, &state->addrs, next) {
			if (ipv6_findaddrmatch(ap, addr, flags))
				return ap;
		}
	}
	return NULL;
}

struct ipv6_addr *
dhcp6_findaddr(struct dhcpcd_ctx *ctx, const struct in6_addr *addr,
    unsigned int flags)
{
	struct interface *ifp;
	struct ipv6_addr *ap;
	struct dhcp6_state *state;

	TAILQ_FOREACH(ifp, ctx->ifaces, next) {
		if ((state = D6_STATE(ifp)) != NULL) {
			TAILQ_FOREACH(ap, &state->addrs, next) {
				if (ipv6_findaddrmatch(ap, addr, flags))
					return ap;
			}
		}
	}
	return NULL;
}

static int
dhcp6_findna(struct interface *ifp, uint16_t ot, const uint8_t *iaid,
    uint8_t *d, size_t l, const struct timespec *acquired)
{
	struct dhcp6_state *state;
	uint8_t *o, *nd;
	uint16_t ol;
	struct ipv6_addr *a;
	int i;
	struct dhcp6_ia_addr ia;

	i = 0;
	state = D6_STATE(ifp);
	while ((o = dhcp6_findoption(d, l, D6_OPTION_IA_ADDR, &ol))) {
		/* Set d and l first to ensure we find the next option. */
		nd = o + ol;
		l -= (size_t)(nd - d);
		d = nd;
		if (ol < 24) {
			errno = EINVAL;
			logerrx("%s: IA Address option truncated", ifp->name);
			continue;
		}
		memcpy(&ia, o, ol);
		ia.pltime = ntohl(ia.pltime);
		ia.vltime = ntohl(ia.vltime);
		/* RFC 3315 22.6 */
		if (ia.pltime > ia.vltime) {
			errno = EINVAL;
			logerr("%s: IA Address pltime %"PRIu32
			    " > vltime %"PRIu32,
			    ifp->name, ia.pltime, ia.vltime);
			continue;
		}
		TAILQ_FOREACH(a, &state->addrs, next) {
			if (ipv6_findaddrmatch(a, &ia.addr, 0))
				break;
		}
		if (a == NULL) {
			/*
			 * RFC 5942 Section 5
			 * We cannot assume any prefix length, nor tie the
			 * address to an existing one as it could expire
			 * before the address.
			 * As such we just give it a 128 prefix.
			 */
			a = ipv6_newaddr(ifp, &ia.addr, 128, IPV6_AF_ONLINK);
			a->dadcallback = dhcp6_dadcallback;
			a->ia_type = ot;
			memcpy(a->iaid, iaid, sizeof(a->iaid));
			a->created = *acquired;

			TAILQ_INSERT_TAIL(&state->addrs, a, next);
		} else {
			if (!(a->flags & IPV6_AF_ONLINK))
				a->flags |= IPV6_AF_ONLINK | IPV6_AF_NEW;
			a->flags &= ~(IPV6_AF_STALE | IPV6_AF_EXTENDED);
		}
		a->acquired = *acquired;
		a->prefix_pltime = ia.pltime;
		if (a->prefix_vltime != ia.vltime) {
			a->flags |= IPV6_AF_NEW;
			a->prefix_vltime = ia.vltime;
		}
		if (a->prefix_pltime && a->prefix_pltime < state->lowpl)
		    state->lowpl = a->prefix_pltime;
		if (a->prefix_vltime && a->prefix_vltime > state->expire)
		    state->expire = a->prefix_vltime;
		i++;
	}
	return i;
}

#ifndef SMALL
static int
dhcp6_findpd(struct interface *ifp, const uint8_t *iaid,
    uint8_t *d, size_t l, const struct timespec *acquired)
{
	struct dhcp6_state *state;
	uint8_t *o, *nd;
	struct ipv6_addr *a;
	int i;
	uint8_t nb, *pw;
	uint16_t ol;
	struct dhcp6_pd_addr pdp;
	struct in6_addr pdp_prefix;

	i = 0;
	state = D6_STATE(ifp);
	while ((o = dhcp6_findoption(d, l, D6_OPTION_IAPREFIX, &ol))) {
		/* Set d and l first to ensure we find the next option. */
		nd = o + ol;
		l -= (size_t)(nd - d);
		d = nd;
		if (ol < sizeof(pdp)) {
			errno = EINVAL;
			logerrx("%s: IA Prefix option truncated", ifp->name);
			continue;
		}

		memcpy(&pdp, o, sizeof(pdp));
		pdp.pltime = ntohl(pdp.pltime);
		pdp.vltime = ntohl(pdp.vltime);
		/* RFC 3315 22.6 */
		if (pdp.pltime > pdp.vltime) {
			errno = EINVAL;
			logerrx("%s: IA Prefix pltime %"PRIu32
			    " > vltime %"PRIu32,
			    ifp->name, pdp.pltime, pdp.vltime);
			continue;
		}

		o += sizeof(pdp);
		ol = (uint16_t)(ol - sizeof(pdp));

		/* pdp.prefix is not aligned so copy it out. */
		memcpy(&pdp_prefix, &pdp.prefix, sizeof(pdp_prefix));
		TAILQ_FOREACH(a, &state->addrs, next) {
			if (IN6_ARE_ADDR_EQUAL(&a->prefix, &pdp_prefix))
				break;
		}

		if (a == NULL) {
			a = ipv6_newaddr(ifp, &pdp_prefix, pdp.prefix_len,
			    IPV6_AF_DELEGATEDPFX);
			if (a == NULL)
				break;
			a->created = *acquired;
			a->dadcallback = dhcp6_dadcallback;
			a->ia_type = D6_OPTION_IA_PD;
			memcpy(a->iaid, iaid, sizeof(a->iaid));
			TAILQ_INIT(&a->pd_pfxs);
			TAILQ_INSERT_TAIL(&state->addrs, a, next);
		} else {
			if (!(a->flags & IPV6_AF_DELEGATEDPFX)) {
				a->flags |= IPV6_AF_NEW | IPV6_AF_DELEGATEDPFX;
				TAILQ_INIT(&a->pd_pfxs);
			}
			a->flags &= ~(IPV6_AF_STALE |
			              IPV6_AF_EXTENDED |
			              IPV6_AF_REQUEST);
			if (a->prefix_vltime != pdp.vltime)
				a->flags |= IPV6_AF_NEW;
		}

		a->acquired = *acquired;
		a->prefix_pltime = pdp.pltime;
		a->prefix_vltime = pdp.vltime;

		if (a->prefix_pltime && a->prefix_pltime < state->lowpl)
			state->lowpl = a->prefix_pltime;
		if (a->prefix_vltime && a->prefix_vltime > state->expire)
			state->expire = a->prefix_vltime;
		i++;

		o = dhcp6_findoption(o, ol, D6_OPTION_PD_EXCLUDE, &ol);
		a->prefix_exclude_len = 0;
		memset(&a->prefix_exclude, 0, sizeof(a->prefix_exclude));
#if 0
		if (ex == NULL) {
			struct dhcp6_option *w;
			uint8_t *wp;

			w = calloc(1, 128);
			w->len = htons(2);
			wp = D6_OPTION_DATA(w);
			*wp++ = 64;
			*wp++ = 0x78;
			ex = w;
		}
#endif
		if (o == NULL)
			continue;
		if (ol < 2) {
			logerrx("%s: truncated PD Exclude", ifp->name);
			continue;
		}
		a->prefix_exclude_len = *o++;
		ol--;
		if (((a->prefix_exclude_len - a->prefix_len - 1) / NBBY) + 1
		    != ol)
		{
			logerrx("%s: PD Exclude length mismatch", ifp->name);
			a->prefix_exclude_len = 0;
			continue;
		}
		nb = a->prefix_len % NBBY;
		memcpy(&a->prefix_exclude, &a->prefix,
		    sizeof(a->prefix_exclude));
		if (nb)
			ol--;
		pw = a->prefix_exclude.s6_addr +
		    (a->prefix_exclude_len / NBBY) - 1;
		while (ol-- > 0)
			*pw-- = *o++;
		if (nb)
			*pw = (uint8_t)(*pw | (*o >> nb));
	}
	return i;
}
#endif

static int
dhcp6_findia(struct interface *ifp, struct dhcp6_message *m, size_t l,
    const char *sfrom, const struct timespec *acquired)
{
	struct dhcp6_state *state;
	const struct if_options *ifo;
	struct dhcp6_option o;
	uint8_t *d, *p;
	struct dhcp6_ia_na ia;
	int i, e;
	size_t j;
	uint16_t nl;
	uint8_t iaid[4];
	char buf[sizeof(iaid) * 3];
	struct ipv6_addr *ap;

	if (l < sizeof(*m)) {
		/* Should be impossible with guards at packet in
		 * and reading leases */
		errno = EINVAL;
		return -1;
	}

	ifo = ifp->options;
	i = e = 0;
	state = D6_STATE(ifp);
	TAILQ_FOREACH(ap, &state->addrs, next) {
		if (!(ap->flags & IPV6_AF_DELEGATED))
			ap->flags |= IPV6_AF_STALE;
	}

	d = (uint8_t *)m + sizeof(*m);
	l -= sizeof(*m);
	while (l > sizeof(o)) {
		memcpy(&o, d, sizeof(o));
		o.len = ntohs(o.len);
		if (o.len > l || sizeof(o) + o.len > l) {
			errno = EINVAL;
			logerrx("%s: option overflow", ifp->name);
			break;
		}
		p = d + sizeof(o);
		d = p + o.len;
		l -= sizeof(o) + o.len;

		o.code = ntohs(o.code);
		switch(o.code) {
		case D6_OPTION_IA_TA:
			nl = 4;
			break;
		case D6_OPTION_IA_NA:
		case D6_OPTION_IA_PD:
			nl = 12;
			break;
		default:
			continue;
		}
		if (o.len < nl) {
			errno = EINVAL;
			logerrx("%s: IA option truncated", ifp->name);
			continue;
		}

		memcpy(&ia, p, nl);
		p += nl;
		o.len = (uint16_t)(o.len - nl);

		for (j = 0; j < ifo->ia_len; j++) {
			if (memcmp(&ifo->ia[j].iaid, ia.iaid,
			    sizeof(ia.iaid)) == 0)
				break;
		}
		if (j == ifo->ia_len &&
		    !(ifo->ia_len == 0 && ifp->ctx->options & DHCPCD_DUMPLEASE))
		{
			logdebugx("%s: ignoring unrequested IAID %s",
			    ifp->name,
			    hwaddr_ntoa(ia.iaid, sizeof(ia.iaid),
			    buf, sizeof(buf)));
			continue;
		}
		if ( j < ifo->ia_len && ifo->ia[j].ia_type != o.code) {
			logerrx("%s: IAID %s: option type mismatch",
			    ifp->name,
			    hwaddr_ntoa(ia.iaid, sizeof(ia.iaid),
			    buf, sizeof(buf)));
			continue;
		}

		if (o.code != D6_OPTION_IA_TA) {
			ia.t1 = ntohl(ia.t1);
			ia.t2 = ntohl(ia.t2);
			/* RFC 3315 22.4 */
			if (ia.t2 > 0 && ia.t1 > ia.t2) {
				logwarnx("%s: IAID %s T1(%d) > T2(%d) from %s",
				    ifp->name,
				    hwaddr_ntoa(iaid, sizeof(iaid), buf,
				                sizeof(buf)),
				    ia.t1, ia.t2, sfrom);
				continue;
			}
		} else
			ia.t1 = ia.t2 = 0; /* appease gcc */
		if (dhcp6_checkstatusok(ifp, NULL, p, o.len) == -1) {
			e = 1;
			continue;
		}
		if (o.code == D6_OPTION_IA_PD) {
#ifndef SMALL
			if (dhcp6_findpd(ifp, ia.iaid, p, o.len,
					 acquired) == 0)
			{
				logwarnx("%s: %s: DHCPv6 REPLY missing Prefix",
				    ifp->name, sfrom);
				continue;
			}
#endif
		} else {
			if (dhcp6_findna(ifp, o.code, ia.iaid, p, o.len,
					 acquired) == 0)
			{
				logwarnx("%s: %s: DHCPv6 REPLY missing "
				    "IA Address",
				    ifp->name, sfrom);
				continue;
			}
		}
		if (o.code != D6_OPTION_IA_TA) {
			if (ia.t1 != 0 &&
			    (ia.t1 < state->renew || state->renew == 0))
				state->renew = ia.t1;
			if (ia.t2 != 0 &&
			    (ia.t2 < state->rebind || state->rebind == 0))
				state->rebind = ia.t2;
		}
		i++;
	}

	if (i == 0 && e)
		return -1;
	return i;
}

static void
dhcp6_deprecateaddrs(struct ipv6_addrhead *addrs)
{
	struct ipv6_addr *ia, *ian;

	TAILQ_FOREACH_SAFE(ia, addrs, next, ian) {
		if (ia->flags & IPV6_AF_EXTENDED)
			;
		else if (ia->flags & IPV6_AF_STALE) {
			if (ia->prefix_vltime != 0)
				logdebugx("%s: %s: became stale",
				    ia->iface->name, ia->saddr);
			ia->prefix_pltime = 0;
		} else if (ia->prefix_vltime == 0)
			loginfox("%s: %s: no valid lifetime",
			    ia->iface->name, ia->saddr);
		else
			continue;

#ifndef SMALL
		/* If we delegated from this prefix, deprecate or remove
		 * the delegations. */
		if (ia->flags & IPV6_AF_DELEGATEDPFX) {
			struct ipv6_addr *da;
			bool touched = false;

			TAILQ_FOREACH(da, &ia->pd_pfxs, pd_next) {
				if (ia->prefix_vltime == 0) {
					if (da->prefix_vltime != 0) {
						da->prefix_vltime = 0;
						touched = true;
					}
				} else if (da->prefix_pltime != 0) {
					da->prefix_pltime = 0;
					touched = true;
				}
			}
			if (touched)
				ipv6_addaddrs(&ia->pd_pfxs);
		}
#endif

		if (ia->flags & IPV6_AF_REQUEST) {
			ia->prefix_vltime = ia->prefix_pltime = 0;
			eloop_q_timeout_delete(ia->iface->ctx->eloop,
			    0, NULL, ia);
			continue;
		}
		TAILQ_REMOVE(addrs, ia, next);
		if (ia->flags & IPV6_AF_EXTENDED)
			ipv6_deleteaddr(ia);
		ipv6_freeaddr(ia);
	}
}

static int
dhcp6_validatelease(struct interface *ifp,
    struct dhcp6_message *m, size_t len,
    const char *sfrom, const struct timespec *acquired)
{
	struct dhcp6_state *state;
	int ok, nia;
	struct timespec aq;

	if (len <= sizeof(*m)) {
		logerrx("%s: DHCPv6 lease truncated", ifp->name);
		return -1;
	}

	state = D6_STATE(ifp);
	if ((ok = dhcp6_checkstatusok(ifp, m, NULL, len) == -1))
		return -1;

	state->renew = state->rebind = state->expire = 0;
	state->lowpl = ND6_INFINITE_LIFETIME;
	if (!acquired) {
		clock_gettime(CLOCK_MONOTONIC, &aq);
		acquired = &aq;
	}
	nia = dhcp6_findia(ifp, m, len, sfrom, acquired);
	if (nia == 0) {
		if (state->state != DH6S_CONFIRM && ok != 1) {
			logerrx("%s: no useable IA found in lease", ifp->name);
			return -1;
		}

		/* We are confirming and have an OK,
		 * so look for ia's in our old lease.
		 * IA's must have existed here otherwise we would
		 * have rejected it earlier. */
		assert(state->new != NULL && state->new_len != 0);
		nia = dhcp6_findia(ifp, state->new, state->new_len,
		    sfrom, acquired);
	}
	return nia;
}

static ssize_t
dhcp6_writelease(const struct interface *ifp)
{
	const struct dhcp6_state *state;
	int fd;
	ssize_t bytes;

	state = D6_CSTATE(ifp);
	logdebugx("%s: writing lease `%s'", ifp->name, state->leasefile);

	fd = open(state->leasefile, O_WRONLY | O_CREAT | O_TRUNC, 0644);
	if (fd == -1) {
		logerr(__func__);
		return -1;
	}
	bytes = write(fd, state->new, state->new_len);
	close(fd);
	return bytes;
}

static int
dhcp6_readlease(struct interface *ifp, int validate)
{
	struct dhcp6_state *state;
	struct stat st;
	int fd;
	struct dhcp6_message *lease;
	time_t now;
	int retval;
	bool fd_opened;
#ifdef AUTH
	uint8_t *o;
	uint16_t ol;
#endif

	state = D6_STATE(ifp);
	if (state->leasefile[0] == '\0') {
		logdebugx("reading standard input");
		fd = fileno(stdin);
		fd_opened = false;
	} else {
		logdebugx("%s: reading lease `%s'", ifp->name, state->leasefile);
		fd = open(state->leasefile, O_RDONLY);
		if (fd != -1 && fstat(fd, &st) == -1) {
			close(fd);
			fd = -1;
		}
		fd_opened = true;
	}
	if (fd == -1)
		return -1;
	retval = -1;
	lease = NULL;
	free(state->new);
	state->new_len = dhcp_read_lease_fd(fd, (void **)&lease);
	state->new = lease;
	if (fd_opened)
		close(fd);
	if (state->new_len == 0)
		goto ex;

	if (ifp->ctx->options & DHCPCD_DUMPLEASE ||
	    state->leasefile[0] == '\0')
		return 0;

	/* If not validating IA's and if they have expired,
	 * skip to the auth check. */
	if (!validate) {
		fd = 0;
		goto auth;
	}

	clock_gettime(CLOCK_MONOTONIC, &state->acquired);
	if ((now = time(NULL)) == -1)
		goto ex;
	state->acquired.tv_sec -= now - st.st_mtime;

	/* Check to see if the lease is still valid */
	fd = dhcp6_validatelease(ifp, state->new, state->new_len, NULL,
	    &state->acquired);
	if (fd == -1)
		goto ex;

	if (state->expire != ND6_INFINITE_LIFETIME &&
	    state->leasefile[0] != '\0')
	{
		if ((time_t)state->expire < now - st.st_mtime &&
		    !(ifp->options->options & DHCPCD_LASTLEASE_EXTEND)) {
			logdebugx("%s: discarding expired lease", ifp->name);
			retval = 0;
			goto ex;
		}
	}

auth:
	retval = 0;
#ifdef AUTH
	/* Authenticate the message */
	o = dhcp6_findmoption(state->new, state->new_len, D6_OPTION_AUTH, &ol);
	if (o) {
		if (dhcp_auth_validate(&state->auth, &ifp->options->auth,
		    (uint8_t *)state->new, state->new_len, 6, state->new->type,
		    o, ol) == NULL)
		{
			logerr("%s: authentication failed", ifp->name);
			goto ex;
		}
		if (state->auth.token)
			logdebugx("%s: validated using 0x%08" PRIu32,
			    ifp->name, state->auth.token->secretid);
		else
			loginfox("%s: accepted reconfigure key", ifp->name);
	} else if ((ifp->options->auth.options & DHCPCD_AUTH_SENDREQUIRE) ==
	    DHCPCD_AUTH_SENDREQUIRE)
	{
		logerrx("%s: authentication now required", ifp->name);
		goto ex;
	}
#endif

	return fd;

ex:
	dhcp6_freedrop_addrs(ifp, 0, NULL);
	free(state->new);
	state->new = NULL;
	state->new_len = 0;
	if (!(ifp->ctx->options & DHCPCD_DUMPLEASE) &&
	    state->leasefile[0] != '\0')
		unlink(state->leasefile);
	return retval;
}

static void
dhcp6_startinit(struct interface *ifp)
{
	struct dhcp6_state *state;
	int r;
	uint8_t has_ta, has_non_ta;
	size_t i;

	state = D6_STATE(ifp);
	state->state = DH6S_INIT;
	state->expire = ND6_INFINITE_LIFETIME;
	state->lowpl = ND6_INFINITE_LIFETIME;

	dhcp6_addrequestedaddrs(ifp);
	has_ta = has_non_ta = 0;
	for (i = 0; i < ifp->options->ia_len; i++) {
		switch (ifp->options->ia[i].ia_type) {
		case D6_OPTION_IA_TA:
			has_ta = 1;
			break;
		default:
			has_non_ta = 1;
		}
	}

	if (!(ifp->ctx->options & DHCPCD_TEST) &&
	    !(has_ta && !has_non_ta) &&
	    ifp->options->reboot != 0)
	{
		r = dhcp6_readlease(ifp, 1);
		if (r == -1) {
			if (errno != ENOENT)
				logerr("%s: %s", __func__, state->leasefile);
		} else if (r != 0) {
			/* RFC 3633 section 12.1 */
#ifndef SMALL
			if (dhcp6_hasprefixdelegation(ifp))
				dhcp6_startrebind(ifp);
			else
#endif
				dhcp6_startconfirm(ifp);
			return;
		}
	}
	dhcp6_startdiscover(ifp);
}

#ifndef SMALL
static struct ipv6_addr *
dhcp6_ifdelegateaddr(struct interface *ifp, struct ipv6_addr *prefix,
    const struct if_sla *sla, struct if_ia *if_ia)
{
	struct dhcp6_state *state;
	struct in6_addr addr, daddr;
	struct ipv6_addr *ia;
	int pfxlen, dadcounter;
	uint64_t vl;

	/* RFC6603 Section 4.2 */
	if (strcmp(ifp->name, prefix->iface->name) == 0) {
		if (prefix->prefix_exclude_len == 0) {
			/* Don't spam the log automatically */
			if (sla != NULL)
				logwarnx("%s: DHCPv6 server does not support "
				    "OPTION_PD_EXCLUDE",
				    ifp->name);
			return NULL;
		}
		pfxlen = prefix->prefix_exclude_len;
		memcpy(&addr, &prefix->prefix_exclude, sizeof(addr));
	} else if ((pfxlen = dhcp6_delegateaddr(&addr, ifp, prefix,
	    sla, if_ia)) == -1)
		return NULL;

	if (sla != NULL && fls64(sla->suffix) > 128 - pfxlen) {
		logerrx("%s: suffix %" PRIu64 " + prefix_len %d > 128",
		    ifp->name, sla->suffix, pfxlen);
		return NULL;
	}

	/* Add our suffix */
	if (sla != NULL && sla->suffix != 0) {
		daddr = addr;
		vl = be64dec(addr.s6_addr + 8);
		vl |= sla->suffix;
		be64enc(daddr.s6_addr + 8, vl);
	} else {
		dadcounter = ipv6_makeaddr(&daddr, ifp, &addr, pfxlen);
		if (dadcounter == -1) {
			logerrx("%s: error adding slaac to prefix_len %d",
			    ifp->name, pfxlen);
			return NULL;
		}
	}

	/* Find an existing address */
	state = D6_STATE(ifp);
	TAILQ_FOREACH(ia, &state->addrs, next) {
		if (IN6_ARE_ADDR_EQUAL(&ia->addr, &daddr))
			break;
	}
	if (ia == NULL) {
		ia = ipv6_newaddr(ifp, &daddr, (uint8_t)pfxlen, IPV6_AF_ONLINK);
		if (ia == NULL)
			return NULL;
		ia->dadcallback = dhcp6_dadcallback;
		memcpy(&ia->iaid, &prefix->iaid, sizeof(ia->iaid));
		ia->created = prefix->acquired;

		TAILQ_INSERT_TAIL(&state->addrs, ia, next);
		TAILQ_INSERT_TAIL(&prefix->pd_pfxs, ia, pd_next);
	}
	ia->delegating_prefix = prefix;
	ia->prefix = addr;
	ia->prefix_len = (uint8_t)pfxlen;
	ia->acquired = prefix->acquired;
	ia->prefix_pltime = prefix->prefix_pltime;
	ia->prefix_vltime = prefix->prefix_vltime;

	/* If the prefix length hasn't changed,
	 * don't install a reject route. */
	if (prefix->prefix_len == pfxlen)
		prefix->flags |= IPV6_AF_NOREJECT;
	else
		prefix->flags &= ~IPV6_AF_NOREJECT;

	return ia;
}
#endif

static void
dhcp6_script_try_run(struct interface *ifp, int delegated)
{
	struct dhcp6_state *state;
	struct ipv6_addr *ap;
	int completed;

	state = D6_STATE(ifp);
	completed = 1;
	/* If all addresses have completed DAD run the script */
	TAILQ_FOREACH(ap, &state->addrs, next) {
		if (!(ap->flags & IPV6_AF_ADDED))
			continue;
		if (ap->flags & IPV6_AF_ONLINK) {
			if (!(ap->flags & IPV6_AF_DADCOMPLETED) &&
			    ipv6_iffindaddr(ap->iface, &ap->addr,
			                    IN6_IFF_TENTATIVE))
				ap->flags |= IPV6_AF_DADCOMPLETED;
			if ((ap->flags & IPV6_AF_DADCOMPLETED) == 0
#ifndef SMALL
			    && ((delegated && ap->delegating_prefix) ||
			    (!delegated && !ap->delegating_prefix))
#endif
			    )
			{
				completed = 0;
				break;
			}
		}
	}
	if (completed) {
		script_runreason(ifp, delegated ? "DELEGATED6" : state->reason);
		if (!delegated)
			dhcpcd_daemonise(ifp->ctx);
	} else
		logdebugx("%s: waiting for DHCPv6 DAD to complete", ifp->name);
}

#ifdef SMALL
size_t
dhcp6_find_delegates(__unused struct interface *ifp)
{

	return 0;
}
#else
static void
dhcp6_delegate_prefix(struct interface *ifp)
{
	struct if_options *ifo;
	struct dhcp6_state *state;
	struct ipv6_addr *ap;
	size_t i, j, k;
	struct if_ia *ia;
	struct if_sla *sla;
	struct interface *ifd;
	bool carrier_warned;

	ifo = ifp->options;
	state = D6_STATE(ifp);

	/* Clear the logged flag. */
	TAILQ_FOREACH(ap, &state->addrs, next) {
		ap->flags &= ~IPV6_AF_DELEGATEDLOG;
	}

	TAILQ_FOREACH(ifd, ifp->ctx->ifaces, next) {
		if (!ifd->active)
			continue;
		k = 0;
		carrier_warned = false;
		TAILQ_FOREACH(ap, &state->addrs, next) {
			if (!(ap->flags & IPV6_AF_DELEGATEDPFX))
				continue;
			if (!(ap->flags & IPV6_AF_DELEGATEDLOG)) {
				logfunc_t *logfunc;

				if (ap->flags & IPV6_AF_NEW)
					logfunc = loginfox;
				else
					logfunc = logdebugx;
				/* We only want to log this the once as we loop
				 * through many interfaces first. */
				ap->flags |= IPV6_AF_DELEGATEDLOG;
				logfunc("%s: delegated prefix %s",
				    ifp->name, ap->saddr);
				ap->flags &= ~IPV6_AF_NEW;
			}
			for (i = 0; i < ifo->ia_len; i++) {
				ia = &ifo->ia[i];
				if (memcmp(ia->iaid, ap->iaid,
				    sizeof(ia->iaid)))
					continue;
				if (ia->sla_len == 0) {
					/* no SLA configured, so lets
					 * automate it */
					if (ifd->carrier != LINK_UP) {
						logdebugx(
						    "%s: has no carrier, cannot"
						    " delegate addresses",
						    ifd->name);
						carrier_warned = true;
						break;
					}
					if (dhcp6_ifdelegateaddr(ifd, ap,
					    NULL, ia))
						k++;
				}
				for (j = 0; j < ia->sla_len; j++) {
					sla = &ia->sla[j];
					if (strcmp(ifd->name, sla->ifname))
						continue;
					if (ifd->carrier != LINK_UP) {
						logdebugx(
						    "%s: has no carrier, cannot"
						    " delegate addresses",
						    ifd->name);
						carrier_warned = true;
						break;
					}
					if (dhcp6_ifdelegateaddr(ifd, ap,
					    sla, ia))
						k++;
				}
				if (carrier_warned)
					break;
			}
			if (carrier_warned)
				break;
		}
		if (k && !carrier_warned) {
			struct dhcp6_state *s = D6_STATE(ifd);

			ipv6_addaddrs(&s->addrs);

			/*
			 * Can't add routes here because that will trigger
			 * interface sorting which may break the current
			 * enumeration.
			 * This doesn't really matter thanks to DaD because
			 * calling the script will be delayed and routes
			 * will get re-built if needed first.
			 * This only cause minor confusion when dhcpcd is
			 * restarted and confirms a lease where prior delegation
			 * has already been assigned, because it will log it
			 * added routes after the script has run.
			 * The routes should still be there and fine though.
			 */
			dhcp6_script_try_run(ifd, 1);
		}
	}

	/* Now all addresses have been added, rebuild the routing table. */
	if_initrt(ifp->ctx, AF_INET6);
	rt_build(ifp->ctx, AF_INET6);
}

static void
dhcp6_find_delegates1(void *arg)
{

	dhcp6_find_delegates(arg);
}

size_t
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
		state = D6_STATE(ifd);
		if (state == NULL || state->state != DH6S_BOUND)
			continue;
		TAILQ_FOREACH(ap, &state->addrs, next) {
			if (!(ap->flags & IPV6_AF_DELEGATEDPFX))
				continue;
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
						logdebugx(
						    "%s: delaying adding"
						    " delegated addresses for"
						    " LL address",
						    ifp->name);
						ipv6_addlinklocalcallback(ifp,
						    dhcp6_find_delegates1, ifp);
						return 1;
					}
					if (dhcp6_ifdelegateaddr(ifp, ap,
					    sla, ia))
					    k++;
				}
			}
		}
	}

	if (k) {
		loginfox("%s: adding delegated prefixes", ifp->name);
		state = D6_STATE(ifp);
		state->state = DH6S_DELEGATED;
		ipv6_addaddrs(&state->addrs);
		if_initrt(ifp->ctx, AF_INET6);
		rt_build(ifp->ctx, AF_INET6);
		dhcp6_script_try_run(ifp, 1);
	}
	return k;
}
#endif

static void
dhcp6_bind(struct interface *ifp, const char *op)
{
	struct dhcp6_state *state = D6_STATE(ifp);
	bool has_new = false;
	struct ipv6_addr *ia;
	logfunc_t *lognewinfo;
	struct timespec now;

	TAILQ_FOREACH(ia, &state->addrs, next) {
		if (ia->flags & IPV6_AF_NEW) {
			has_new = true;
			break;
		}
	}
	lognewinfo = has_new ? loginfox : logdebugx;
	if (op != NULL)
		lognewinfo("%s: %s received from %s",
		    ifp->name, op, ifp->ctx->sfrom);

	state->reason = NULL;
	if (state->state != DH6S_ITIMEDOUT)
		eloop_timeout_delete(ifp->ctx->eloop, NULL, ifp);
	switch(state->state) {
	case DH6S_INFORM:
		if (state->reason == NULL)
			state->reason = "INFORM6";
		/* FALLTHROUGH */
	case DH6S_ITIMEDOUT:
	{
		struct dhcp6_option *o;
		uint16_t ol;

		if (state->reason == NULL)
			state->reason = "ITIMEDOUT";
		o = dhcp6_findmoption(state->new, state->new_len,
		                      D6_OPTION_INFO_REFRESH_TIME, &ol);
		if (o == NULL || ol != sizeof(uint32_t))
			state->renew = IRT_DEFAULT;
		else {
			memcpy(&state->renew, o, ol);
			state->renew = ntohl(state->renew);
			if (state->renew < IRT_MINIMUM)
				state->renew = IRT_MINIMUM;
		}
		state->rebind = 0;
		state->expire = ND6_INFINITE_LIFETIME;
		state->lowpl = ND6_INFINITE_LIFETIME;
	}
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
		/* FALLTHROUGH */
	case DH6S_TIMEDOUT:
		if (state->reason == NULL)
			state->reason = "TIMEOUT6";
		if (state->renew != 0) {
			bool all_expired = true;

			TAILQ_FOREACH(ia, &state->addrs, next) {
				if (ia->flags & IPV6_AF_STALE)
					continue;
				if (ia->prefix_vltime <= state->renew)
					logwarnx(
					    "%s: %s will expire before renewal",
					    ifp->name, ia->saddr);
				else
					all_expired = false;
			}
			if (all_expired) {
				/* All address's vltime happens at or before
				 * the configured T1 in the IA.
				 * This is a badly configured server and we
				 * have to use our own notion of what
				 * T1 and T2 should be as a result.
				 *
				 * Doing this violates RFC 3315 22.4:
				 * In a message sent by a server to a client,
				 * the client MUST use the values in the T1
				 * and T2 fields for the T1 and T2 parameters,
				 * unless those values in those fields are 0.
				 */
				logwarnx("%s: ignoring T1 %"PRIu32
				    " to due address expiry",
				    ifp->name, state->renew);
				state->renew = state->rebind = 0;
			}
		}
		if (state->renew == 0 && state->lowpl != ND6_INFINITE_LIFETIME)
			state->renew = (uint32_t)(state->lowpl * 0.5);
		if (state->rebind == 0 && state->lowpl != ND6_INFINITE_LIFETIME)
			state->rebind = (uint32_t)(state->lowpl * 0.8);
		break;
	default:
		state->reason = "UNKNOWN6";
		break;
	}

	clock_gettime(CLOCK_MONOTONIC, &now);
	if (state->state == DH6S_TIMEDOUT || state->state == DH6S_ITIMEDOUT) {
		struct timespec diff;
		uint32_t diffsec;

		/* Reduce timers */
		timespecsub(&now, &state->acquired, &diff);
		diffsec = (uint32_t)diff.tv_sec;
		if (state->renew && state->renew != ND6_INFINITE_LIFETIME) {
			if (state->renew > diffsec)
				state->renew -= diffsec;
			else
				state->renew = 0;
		}
		if (state->rebind && state->rebind != ND6_INFINITE_LIFETIME) {
			if (state->rebind > diffsec)
				state->rebind -= diffsec;
			else
				state->rebind = 0;
		}
		if (state->expire && state->expire != ND6_INFINITE_LIFETIME) {
			if (state->expire > diffsec)
				state->expire -= diffsec;
			else {
				if (!(ifp->options->options &
				    DHCPCD_LASTLEASE_EXTEND))
					return;
				state->expire = ND6_INFINITE_LIFETIME;
			}
		}
		if (state->expire == ND6_INFINITE_LIFETIME &&
		    ifp->options->options & DHCPCD_LASTLEASE_EXTEND)
			dhcp6_leaseextend(ifp);

		/* Restart rebind or renew phases in a second. */
		if (state->expire != ND6_INFINITE_LIFETIME) {
			if (state->rebind == 0 &&
			    state->rebind != ND6_INFINITE_LIFETIME)
				state->rebind = 1;
			else if (state->renew == 0 &&
			    state->renew != ND6_INFINITE_LIFETIME)
				state->renew = 1;
		}
	} else
		state->acquired = now;

	switch (state->state) {
	case DH6S_CONFIRM:
	case DH6S_TIMEDOUT:
	case DH6S_ITIMEDOUT:
		break;
	default:
		free(state->old);
		state->old = state->new;
		state->old_len = state->new_len;
		state->new = state->recv;
		state->new_len = state->recv_len;
		state->recv = NULL;
		state->recv_len = 0;
		break;
	}

	if (ifp->ctx->options & DHCPCD_TEST)
		script_runreason(ifp, "TEST");
	else {
		bool timed_out;

		switch(state->state) {
		case DH6S_TIMEDOUT:
		case DH6S_ITIMEDOUT:
			timed_out = true;
			break;
		default:
			timed_out = false;
			break;
		}

		switch(state->state) {
		case DH6S_INFORM:
		case DH6S_ITIMEDOUT:
			state->state = DH6S_INFORMED;
			break;
		default:
			state->state = DH6S_BOUND;
			break;
		}

		if (state->renew && state->renew != ND6_INFINITE_LIFETIME)
			eloop_timeout_add_sec(ifp->ctx->eloop,
			    (time_t)state->renew,
			    state->state == DH6S_INFORMED ?
			    dhcp6_startinform : dhcp6_startrenew, ifp);
		if (state->rebind && state->rebind != ND6_INFINITE_LIFETIME)
			eloop_timeout_add_sec(ifp->ctx->eloop,
			    (time_t)state->rebind, dhcp6_startrebind, ifp);
		if (state->expire != ND6_INFINITE_LIFETIME)
			eloop_timeout_add_sec(ifp->ctx->eloop,
			    (time_t)state->expire, dhcp6_startexpire, ifp);
		else if (timed_out)
			eloop_timeout_add_sec(ifp->ctx->eloop,
			    (time_t)state->expire, dhcp6_startdiscover, ifp);

		ipv6_addaddrs(&state->addrs);
		dhcp6_deprecateaddrs(&state->addrs);

		if (state->state == DH6S_INFORMED)
			lognewinfo("%s: refresh in %"PRIu32" seconds",
			    ifp->name, state->renew);
		else if (state->renew || state->rebind)
			lognewinfo("%s: renew in %"PRIu32", "
			    "rebind in %"PRIu32", "
			    "expire in %"PRIu32" seconds",
			    ifp->name,
			    state->renew, state->rebind, state->expire);
		else if (state->expire == 0)
			lognewinfo("%s: will expire", ifp->name);
		else
			lognewinfo("%s: expire in %"PRIu32" seconds",
			    ifp->name, state->expire);
		if_initrt(ifp->ctx, AF_INET6);
		rt_build(ifp->ctx, AF_INET6);
		if (!timed_out)
			dhcp6_writelease(ifp);
#ifndef SMALL
		dhcp6_delegate_prefix(ifp);
#endif
		dhcp6_script_try_run(ifp, 0);
	}

	if (ifp->ctx->options & DHCPCD_TEST ||
	    (ifp->options->options & DHCPCD_INFORM &&
	    !(ifp->ctx->options & DHCPCD_MASTER)))
	{
		eloop_exit(ifp->ctx->eloop, EXIT_SUCCESS);
	}
}

static void
dhcp6_recvif(struct interface *ifp, struct dhcp6_message *r, size_t len)
{
	struct dhcpcd_ctx *ctx;
	size_t i;
	const char *op;
	struct dhcp6_state *state;
	uint8_t *o;
	uint16_t ol;
	const struct dhcp_opt *opt;
	const struct if_options *ifo;
	bool valid_op;
#ifdef AUTH
	uint8_t *auth;
	uint16_t auth_len;
#endif

	ctx = ifp->ctx;
	state = D6_STATE(ifp);
	if (state == NULL || state->send == NULL) {
		logdebug("%s: DHCPv6 reply received but not running",
		    ifp->name);
		return;
	}

	/* We're already bound and this message is for another machine */
	/* XXX DELEGATED? */
	if (r->type != DHCP6_RECONFIGURE &&
	    (state->state == DH6S_BOUND || state->state == DH6S_INFORMED))
	{
		logdebugx("%s: DHCPv6 reply received but already bound",
		    ifp->name);
		return;
	}

	if (dhcp6_findmoption(r, len, D6_OPTION_SERVERID, NULL) == NULL) {
		logdebugx("%s: no DHCPv6 server ID from %s",
		    ifp->name, ctx->sfrom);
		return;
	}

	ifo = ifp->options;
	for (i = 0, opt = ctx->dhcp6_opts;
	    i < ctx->dhcp6_opts_len;
	    i++, opt++)
	{
		if (has_option_mask(ifo->requiremask6, opt->option) &&
		    !dhcp6_findmoption(r, len, (uint16_t)opt->option, NULL))
		{
			logwarnx("%s: reject DHCPv6 (no option %s) from %s",
			    ifp->name, opt->var, ctx->sfrom);
			return;
		}
		if (has_option_mask(ifo->rejectmask6, opt->option) &&
		    dhcp6_findmoption(r, len, (uint16_t)opt->option, NULL))
		{
			logwarnx("%s: reject DHCPv6 (option %s) from %s",
			    ifp->name, opt->var, ctx->sfrom);
			return;
		}
	}

#ifdef AUTH
	/* Authenticate the message */
	auth = dhcp6_findmoption(r, len, D6_OPTION_AUTH, &auth_len);
	if (auth != NULL) {
		if (dhcp_auth_validate(&state->auth, &ifo->auth,
		    (uint8_t *)r, len, 6, r->type, auth, auth_len) == NULL)
		{
			logerr("%s: authentication failed from %s",
			    ifp->name, ctx->sfrom);
			return;
		}
		if (state->auth.token)
			logdebugx("%s: validated using 0x%08" PRIu32,
			    ifp->name, state->auth.token->secretid);
		else
			loginfox("%s: accepted reconfigure key", ifp->name);
	} else if (ifo->auth.options & DHCPCD_AUTH_SEND) {
		if (ifo->auth.options & DHCPCD_AUTH_REQUIRE) {
			logerr("%s: no authentication from %s",
			    ifp->name, ctx->sfrom);
			return;
		}
		logwarnx("%s: no authentication from %s",
		    ifp->name, ctx->sfrom);
	}
#endif

	op = dhcp6_get_op(r->type);
	valid_op = op != NULL;
	switch(r->type) {
	case DHCP6_REPLY:
		switch(state->state) {
		case DH6S_INFORM:
			if (dhcp6_checkstatusok(ifp, r, NULL, len) == -1)
				return;
			break;
		case DH6S_CONFIRM:
			if (dhcp6_validatelease(ifp, r, len,
						ctx->sfrom, NULL) == -1)
			{
				dhcp6_startdiscover(ifp);
				return;
			}
			break;
		case DH6S_DISCOVER:
			/* Only accept REPLY in DISCOVER for RAPID_COMMIT.
			 * Normally we get an ADVERTISE for a DISCOVER. */
			if (!has_option_mask(ifo->requestmask6,
			    D6_OPTION_RAPID_COMMIT) ||
			    !dhcp6_findmoption(r, len, D6_OPTION_RAPID_COMMIT,
					      NULL))
			{
				valid_op = false;
				break;
			}
			/* Validate lease before setting state to REQUEST. */
			/* FALLTHROUGH */
		case DH6S_REQUEST: /* FALLTHROUGH */
		case DH6S_RENEW: /* FALLTHROUGH */
		case DH6S_REBIND:
			if (dhcp6_validatelease(ifp, r, len,
			    ctx->sfrom, NULL) == -1)
			{
#ifndef SMALL
				/* PD doesn't use CONFIRM, so REBIND could
				 * throw up an invalid prefix if we
				 * changed link */
				if (state->state == DH6S_REBIND &&
				    dhcp6_hasprefixdelegation(ifp))
					dhcp6_startdiscover(ifp);
#endif
				return;
			}
			if (state->state == DH6S_DISCOVER)
				state->state = DH6S_REQUEST;
			break;
		default:
			valid_op = false;
			break;
		}
		break;
	case DHCP6_ADVERTISE:
		if (state->state != DH6S_DISCOVER) {
			valid_op = false;
			break;
		}
		/* RFC7083 */
		o = dhcp6_findmoption(r, len, D6_OPTION_SOL_MAX_RT, &ol);
		if (o && ol == sizeof(uint32_t)) {
			uint32_t max_rt;

			memcpy(&max_rt, o, sizeof(max_rt));
			max_rt = ntohl(max_rt);
			if (max_rt >= 60 && max_rt <= 86400) {
				logdebugx("%s: SOL_MAX_RT %llu -> %u",
				    ifp->name,
				    (unsigned long long)state->sol_max_rt,
				    max_rt);
				state->sol_max_rt = (time_t)max_rt;
			} else
				logerr("%s: invalid SOL_MAX_RT %u",
				    ifp->name, max_rt);
		}
		o = dhcp6_findmoption(r, len, D6_OPTION_INF_MAX_RT, &ol);
		if (o && ol == sizeof(uint32_t)) {
			uint32_t max_rt;

			memcpy(&max_rt, o, sizeof(max_rt));
			max_rt = ntohl(max_rt);
			if (max_rt >= 60 && max_rt <= 86400) {
				logdebugx("%s: INF_MAX_RT %llu -> %u",
				    ifp->name,
				    (unsigned long long)state->inf_max_rt,
				    max_rt);
				state->inf_max_rt = (time_t)max_rt;
			} else
				logerrx("%s: invalid INF_MAX_RT %u",
				    ifp->name, max_rt);
		}
		if (dhcp6_validatelease(ifp, r, len, ctx->sfrom, NULL) == -1)
			return;
		break;
	case DHCP6_RECONFIGURE:
#ifdef AUTH
		if (auth == NULL) {
#endif
			logerrx("%s: unauthenticated %s from %s",
			    ifp->name, op, ctx->sfrom);
			if (ifo->auth.options & DHCPCD_AUTH_REQUIRE)
				return;
#ifdef AUTH
		}
		loginfox("%s: %s from %s", ifp->name, op, ctx->sfrom);
		o = dhcp6_findmoption(r, len, D6_OPTION_RECONF_MSG, &ol);
		if (o == NULL) {
			logerrx("%s: missing Reconfigure Message option",
			    ifp->name);
			return;
		}
		if (ol != 1) {
			logerrx("%s: missing Reconfigure Message type",
			    ifp->name);
			return;
		}
		switch(*o) {
		case DHCP6_RENEW:
			if (state->state != DH6S_BOUND) {
				logerrx("%s: not bound, ignoring %s",
				    ifp->name, op);
				return;
			}
			dhcp6_startrenew(ifp);
			break;
		case DHCP6_INFORMATION_REQ:
			if (state->state != DH6S_INFORMED) {
				logerrx("%s: not informed, ignoring %s",
				    ifp->name, op);
				return;
			}
			eloop_timeout_delete(ifp->ctx->eloop,
			    dhcp6_sendinform, ifp);
			dhcp6_startinform(ifp);
			break;
		default:
			logerr("%s: unsupported %s type %d",
			    ifp->name, op, *o);
			break;
		}
		return;
#endif
	default:
		logerrx("%s: invalid DHCP6 type %s (%d)",
		    ifp->name, op, r->type);
		return;
	}
	if (!valid_op) {
		logwarnx("%s: invalid state for DHCP6 type %s (%d)",
		    ifp->name, op, r->type);
		return;
	}

	if (state->recv_len < (size_t)len) {
		free(state->recv);
		state->recv = malloc(len);
		if (state->recv == NULL) {
			logerr(__func__);
			return;
		}
	}
	memcpy(state->recv, r, len);
	state->recv_len = len;

	switch (r->type) {
	case DHCP6_ADVERTISE:
	{
		struct ipv6_addr *ia;

		if (state->state == DH6S_REQUEST) /* rapid commit */
			break;
		TAILQ_FOREACH(ia, &state->addrs, next) {
			if (!(ia->flags & (IPV6_AF_STALE | IPV6_AF_REQUEST)))
				break;
		}
		if (ia == NULL)
			ia = TAILQ_FIRST(&state->addrs);
		if (ia == NULL)
			loginfox("%s: ADV (no address) from %s",
			    ifp->name, ctx->sfrom);
		else
			loginfox("%s: ADV %s from %s",
			    ifp->name, ia->saddr, ctx->sfrom);
		if (ifp->ctx->options & DHCPCD_TEST)
			break;
		dhcp6_startrequest(ifp);
		return;
	}
	}

	dhcp6_bind(ifp, op);
}

static void
dhcp6_recv(struct dhcpcd_ctx *ctx, struct ipv6_addr *ia)
{
	int s;
	size_t len;
	ssize_t bytes;
	struct interface *ifp;
	struct dhcp6_message *r;
	const struct dhcp6_state *state;
	uint8_t *o;
	uint16_t ol;

	ctx->rcvhdr.msg_controllen = CMSG_SPACE(sizeof(struct in6_pktinfo));
	s = ia != NULL ? ia->dhcp6_fd : ctx->dhcp6_fd;
	bytes = recvmsg_realloc(s, &ctx->rcvhdr, 0);
	if (bytes == -1) {
		logerr("%s: recvmsg_realloc", __func__);
		close(s);
		eloop_event_delete(ctx->eloop, s);
		if (ia != NULL)
			ia->dhcp6_fd = -1;
		else
			ctx->dhcp6_fd = -1;
		eloop_exit(ctx->eloop, 1);
		return;
	}
	len = (size_t)bytes;
	ctx->sfrom = inet_ntop(AF_INET6, &ctx->from.sin6_addr,
	    ctx->ntopbuf, sizeof(ctx->ntopbuf));
	if (len < sizeof(struct dhcp6_message)) {
		logerrx("DHCPv6 packet too short from %s", ctx->sfrom);
		return;
	}

	if (ia != NULL)
		ifp = ia->iface;
	else {
		struct cmsghdr *cm;
		struct in6_pktinfo pi;

		pi.ipi6_ifindex = 0;
		for (cm = (struct cmsghdr *)CMSG_FIRSTHDR(&ctx->rcvhdr);
		    cm;
		    cm = (struct cmsghdr *)CMSG_NXTHDR(&ctx->rcvhdr, cm))
		{
			if (cm->cmsg_level != IPPROTO_IPV6)
				continue;
			switch(cm->cmsg_type) {
			case IPV6_PKTINFO:
				if (cm->cmsg_len == CMSG_LEN(sizeof(pi)))
					memcpy(&pi, CMSG_DATA(cm), sizeof(pi));
				break;
			}
		}
		if (pi.ipi6_ifindex == 0) {
			logerrx("DHCPv6 reply did not contain index from %s",
			    ctx->sfrom);
			return;
		}

		TAILQ_FOREACH(ifp, ctx->ifaces, next) {
			if (ifp->index == (unsigned int)pi.ipi6_ifindex)
				break;
		}
		if (ifp == NULL) {
			logerrx("DHCPv6 reply for unexpected interface from %s",
			    ctx->sfrom);
			return;
		}
	}

	r = (struct dhcp6_message *)ctx->rcvhdr.msg_iov[0].iov_base;
	o = dhcp6_findmoption(r, len, D6_OPTION_CLIENTID, &ol);
	if (o == NULL || ol != ctx->duid_len ||
	    memcmp(o, ctx->duid, ol) != 0)
	{
		logdebugx("%s: incorrect client ID from %s",
		    ifp->name, ctx->sfrom);
		return;
	}

	if (dhcp6_findmoption(r, len, D6_OPTION_SERVERID, NULL) == NULL) {
		logdebugx("%s: no DHCPv6 server ID from %s",
		    ifp->name, ctx->sfrom);
		return;
	}

	if (r->type == DHCP6_RECONFIGURE) {
		logdebugx("%s: RECONFIGURE recv from %s,"
		    " sending to all interfaces",
		    ifp->name, ctx->sfrom);
		TAILQ_FOREACH(ifp, ctx->ifaces, next) {
			if (D6_CSTATE(ifp) != NULL)
				dhcp6_recvif(ifp, r, len);
		}
		return;
	}

	state = D6_CSTATE(ifp);
	if (state == NULL ||
	    r->xid[0] != state->send->xid[0] ||
	    r->xid[1] != state->send->xid[1] ||
	    r->xid[2] != state->send->xid[2])
	{
		struct interface *ifp1;
		const struct dhcp6_state *state1;

		/* Find an interface with a matching xid. */
		TAILQ_FOREACH(ifp1, ctx->ifaces, next) {
			state1 = D6_CSTATE(ifp1);
			if (state1 == NULL || state1->send == NULL)
				continue;
			if (r->xid[0] == state1->send->xid[0] &&
			    r->xid[1] == state1->send->xid[1] &&
			    r->xid[2] == state1->send->xid[2])
				break;
		}

		if (ifp1 == NULL) {
			if (state != NULL)
				logdebugx("%s: wrong xid 0x%02x%02x%02x"
				    " (expecting 0x%02x%02x%02x) from %s",
				    ifp->name,
				    r->xid[0], r->xid[1], r->xid[2],
				    state->send->xid[0],
				    state->send->xid[1],
				    state->send->xid[2],
				    ctx->sfrom);
			return;
		}
		logdebugx("%s: redirecting DHCP6 message to %s",
		    ifp->name, ifp1->name);
		ifp = ifp1;
	}

	dhcp6_recvif(ifp, r, len);
}

static void
dhcp6_recvaddr(void *arg)
{
	struct ipv6_addr *ia = arg;

	dhcp6_recv(ia->iface->ctx, ia);
}

static void
dhcp6_recvctx(void *arg)
{
	struct dhcpcd_ctx *ctx = arg;

	dhcp6_recv(ctx, NULL);
}

static int
dhcp6_listen(struct dhcpcd_ctx *ctx, struct ipv6_addr *ia)
{
	struct sockaddr_in6 sa;
	int n, s;

#define SOCK_FLAGS	SOCK_CLOEXEC | SOCK_NONBLOCK
	s = xsocket(PF_INET6, SOCK_DGRAM | SOCK_FLAGS, IPPROTO_UDP);
#undef SOCK_FLAGS
	if (s == -1)
		goto errexit;

	n = 1;
	if (setsockopt(s, SOL_SOCKET, SO_BROADCAST, &n, sizeof(n)) == -1)
		goto errexit;

	memset(&sa, 0, sizeof(sa));
	sa.sin6_family = AF_INET6;
	sa.sin6_port = htons(DHCP6_CLIENT_PORT);
#ifdef BSD
	sa.sin6_len = sizeof(sa);
#endif

	if (ia != NULL) {
		memcpy(&sa.sin6_addr, &ia->addr, sizeof(sa.sin6_addr));
		sa.sin6_scope_id = ia->iface->index;
	} else if (!(ctx->options & DHCPCD_MASTER))
		/* This socket is only used for sending. */
		return s;

	if (bind(s, (struct sockaddr *)&sa, sizeof(sa)) == -1)
		goto errexit;

	if (ia == NULL) {
		n = 1;
		if (setsockopt(s, IPPROTO_IPV6, IPV6_RECVPKTINFO,
		    &n, sizeof(n)) == -1)
			goto errexit;
	} else {
		ia->dhcp6_fd = s;
		eloop_event_add(ctx->eloop, s, dhcp6_recvaddr, ia);
	}

	return s;

errexit:
	logerr(__func__);
	if (s != -1)
		close(s);
	return -1;
}

static int
dhcp6_open(struct dhcpcd_ctx *ctx)
{

	/* Open an unbound socket to send from. */
	ctx->dhcp6_fd = dhcp6_listen(ctx, NULL);
	if (ctx->dhcp6_fd != -1 && (ctx->options & DHCPCD_MASTER))
		eloop_event_add(ctx->eloop, ctx->dhcp6_fd, dhcp6_recvctx, ctx);

	return ctx->dhcp6_fd;
}

#ifndef SMALL
static void
dhcp6_activateinterfaces(struct interface *ifp)
{
	struct interface *ifd;
	size_t i, j;
	struct if_ia *ia;
	struct if_sla *sla;

	for (i = 0; i < ifp->options->ia_len; i++) {
		ia = &ifp->options->ia[i];
		for (j = 0; j < ia->sla_len; j++) {
			sla = &ia->sla[j];
			ifd = if_find(ifp->ctx->ifaces, sla->ifname);
			if (ifd == NULL) {
				logwarn("%s: cannot delegate to %s",
				    ifp->name, sla->ifname);
				continue;
			}
			if (!ifd->active) {
				loginfox("%s: activating for delegation",
				    sla->ifname);
				dhcpcd_activateinterface(ifd,
				    DHCPCD_IPV6 | DHCPCD_DHCP6);
			}
		}
	}
}
#endif

static void
dhcp6_start1(void *arg)
{
	struct interface *ifp = arg;
	struct if_options *ifo = ifp->options;
	struct dhcp6_state *state;
	size_t i;
	const struct dhcp_compat *dhc;

	if (ifp->ctx->dhcp6_fd == -1 && dhcp6_open(ifp->ctx) == -1)
		return;

	state = D6_STATE(ifp);
	/* If no DHCPv6 options are configured,
	   match configured DHCPv4 options to DHCPv6 equivalents. */
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

#ifndef SMALL
	/* Rapid commit won't work with Prefix Delegation Exclusion */
	if (dhcp6_findselfsla(ifp, NULL))
		del_option_mask(ifo->requestmask6, D6_OPTION_RAPID_COMMIT);
#endif

	if (state->state == DH6S_INFORM) {
		add_option_mask(ifo->requestmask6, D6_OPTION_INFO_REFRESH_TIME);
		dhcp6_startinform(ifp);
	} else {
		del_option_mask(ifo->requestmask6, D6_OPTION_INFO_REFRESH_TIME);
		dhcp6_startinit(ifp);
	}

#ifndef SMALL
	dhcp6_activateinterfaces(ifp);
#endif
}

int
dhcp6_start(struct interface *ifp, enum DH6S init_state)
{
	struct dhcp6_state *state;

	state = D6_STATE(ifp);
	if (state != NULL) {
		switch (init_state) {
		case DH6S_INIT:
			goto gogogo;
		case DH6S_INFORM:
			if (state->state == DH6S_INFORMED)
				dhcp6_startinform(ifp);
			break;
		case DH6S_REQUEST:
			if (ifp->options->options & DHCPCD_DHCP6 &&
			    (state->state == DH6S_INFORM ||
			     state->state == DH6S_INFORMED ||
			     state->state == DH6S_DELEGATED))
			{
				/* Change from stateless to stateful */
				init_state = DH6S_INIT;
				goto gogogo;
			}
			break;
		case DH6S_CONFIRM:
			init_state = DH6S_INIT;
			goto gogogo;
		default:
			/* Not possible, but sushes some compiler warnings. */
			break;
		}
		return 0;
	} else {
		switch (init_state) {
		case DH6S_CONFIRM:
			/* No DHCPv6 config, no existing state
			 * so nothing to do. */
			return 0;
		default:
			init_state = DH6S_INIT;
			break;
		}
	}

	if (!(ifp->options->options & DHCPCD_DHCP6))
		return 0;

	ifp->if_data[IF_DATA_DHCP6] = calloc(1, sizeof(*state));
	state = D6_STATE(ifp);
	if (state == NULL)
		return -1;

	state->sol_max_rt = SOL_MAX_RT;
	state->inf_max_rt = INF_MAX_RT;
	TAILQ_INIT(&state->addrs);

gogogo:
	state->state = init_state;
	dhcp_set_leasefile(state->leasefile, sizeof(state->leasefile),
	    AF_INET6, ifp);
	if (ipv6_linklocal(ifp) == NULL) {
		logdebugx("%s: delaying DHCPv6 soliciation for LL address",
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

	if (ifp->options)
		options = ifp->options->options;
	else
		options = ifp->ctx->options;

	if (ifp->ctx->eloop)
		eloop_timeout_delete(ifp->ctx->eloop, NULL, ifp);

#ifndef SMALL
	/* If we're dropping the lease, drop delegated addresses.
	 * If, for whatever reason, we don't drop them in the future
	 * then they should at least be marked as deprecated (pltime 0). */
	if (drop && (options & DHCPCD_NODROP) != DHCPCD_NODROP)
		dhcp6_delete_delegates(ifp);
#endif

	state = D6_STATE(ifp);
	if (state) {
		/* Failure to send the release may cause this function to
		 * re-enter */
		if (state->state == DH6S_RELEASE) {
			dhcp6_finishrelease(ifp);
			return;
		}

		if (drop && options & DHCPCD_RELEASE &&
		    state->state != DH6S_DELEGATED)
		{
			if (ifp->carrier == LINK_UP &&
			    state->state != DH6S_RELEASED &&
			    state->state != DH6S_INFORMED)
			{
				dhcp6_startrelease(ifp);
				return;
			}
			unlink(state->leasefile);
		}
		dhcp6_freedrop_addrs(ifp, drop, NULL);
		free(state->old);
		state->old = state->new;
		state->old_len = state->new_len;
		state->new = NULL;
		state->new_len = 0;
		if (drop && state->old &&
		    (options & DHCPCD_NODROP) != DHCPCD_NODROP)
		{
			if (reason == NULL)
				reason = "STOP6";
			script_runreason(ifp, reason);
		}
		free(state->old);
		free(state->send);
		free(state->recv);
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
	if (ifp == NULL && ctx->dhcp6_fd != -1) {
		eloop_event_delete(ctx->eloop, ctx->dhcp6_fd);
		close(ctx->dhcp6_fd);
		ctx->dhcp6_fd = -1;
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
dhcp6_dropnondelegates(struct interface *ifp)
{
#ifndef SMALL
	struct dhcp6_state *state;
	struct ipv6_addr *ia;

	if ((state = D6_STATE(ifp)) == NULL)
		return;
	TAILQ_FOREACH(ia, &state->addrs, next) {
		if (ia->flags & (IPV6_AF_DELEGATED | IPV6_AF_DELEGATEDPFX))
			return;
	}
#endif
	dhcp6_drop(ifp, "EXPIRE6");
}

void
dhcp6_handleifa(int cmd, struct ipv6_addr *ia)
{
	struct dhcp6_state *state;
	struct interface *ifp = ia->iface;

	/* If not running in master mode, listen to this address */
	if (cmd == RTM_NEWADDR &&
	    !(ia->addr_flags & IN6_IFF_NOTUSEABLE) &&
	    ifp->active == IF_ACTIVE_USER &&
	    !(ifp->ctx->options & DHCPCD_MASTER) &&
	    ifp->options->options & DHCPCD_DHCP6 &&
	    ia->dhcp6_fd == -1)
		dhcp6_listen(ia->iface->ctx, ia);

	if ((state = D6_STATE(ifp)) != NULL)
		ipv6_handleifa_addrs(cmd, &state->addrs, ia);
}

ssize_t
dhcp6_env(char **env, const char *prefix, const struct interface *ifp,
    const struct dhcp6_message *m, size_t len)
{
	const struct if_options *ifo;
	struct dhcp_opt *opt, *vo;
	const uint8_t *p;
	struct dhcp6_option o;
	size_t i, n;
	char *pfx;
	uint32_t en;
	const struct dhcpcd_ctx *ctx;
#ifndef SMALL
	const struct dhcp6_state *state;
	const struct ipv6_addr *ap;
	char *v, *val;
#endif

	n = 0;
	if (m == NULL)
		goto delegated;

	if (len < sizeof(*m)) {
		/* Should be impossible with guards at packet in
		 * and reading leases */
		errno = EINVAL;
		return -1;
	}

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
			logerr(__func__);
			return -1;
		}
		snprintf(pfx, i, "%s_dhcp6", prefix);
	} else
		pfx = NULL;

	/* Unlike DHCP, DHCPv6 options *may* occur more than once.
	 * There is also no provision for option concatenation unlike DHCP. */
	p = (const uint8_t *)m + sizeof(*m);
	len -= sizeof(*m);
	for (; len != 0; p += o.len, len -= o.len) {
		if (len < sizeof(o)) {
			errno = EINVAL;
			break;
		}
		memcpy(&o, p, sizeof(o));
		p += sizeof(o);
		len -= sizeof(o);
		o.len = ntohs(o.len);
		if (len < o.len) {
			errno =	EINVAL;
			break;
		}
		o.code = ntohs(o.code);
		if (has_option_mask(ifo->nomask6, o.code))
			continue;
		for (i = 0, opt = ifo->dhcp6_override;
		    i < ifo->dhcp6_override_len;
		    i++, opt++)
			if (opt->option == o.code)
				break;
		if (i == ifo->dhcp6_override_len &&
		    o.code == D6_OPTION_VENDOR_OPTS &&
		    o.len > sizeof(en))
		{
			memcpy(&en, p, sizeof(en));
			en = ntohl(en);
			vo = vivso_find(en, ifp);
		} else
			vo = NULL;
		if (i == ifo->dhcp6_override_len) {
			for (i = 0, opt = ctx->dhcp6_opts;
			    i < ctx->dhcp6_opts_len;
			    i++, opt++)
				if (opt->option == o.code)
					break;
			if (i == ctx->dhcp6_opts_len)
				opt = NULL;
		}
		if (opt) {
			n += dhcp_envoption(ifp->ctx,
			    env == NULL ? NULL : &env[n],
			    pfx, ifp->name,
			    opt, dhcp6_getoption, p, o.len);
		}
		if (vo) {
			n += dhcp_envoption(ifp->ctx,
			    env == NULL ? NULL : &env[n],
			    pfx, ifp->name,
			    vo, dhcp6_getoption,
			    p + sizeof(en),
			    o.len - sizeof(en));
		}
	}
	free(pfx);

delegated:
#ifndef SMALL
        /* Needed for Delegated Prefixes */
	state = D6_CSTATE(ifp);
	i = 0;
	TAILQ_FOREACH(ap, &state->addrs, next) {
		if (ap->delegating_prefix) {
			i += strlen(ap->saddr) + 1;
		}
	}
	if (env && i) {
		i += strlen(prefix) + strlen("_delegated_dhcp6_prefix=");
                v = val = env[n] = malloc(i);
		if (v == NULL) {
			logerr(__func__);
			return -1;
		}
		v += snprintf(val, i, "%s_delegated_dhcp6_prefix=", prefix);
		TAILQ_FOREACH(ap, &state->addrs, next) {
			if (ap->delegating_prefix) {
				/* Can't use stpcpy(3) due to "security" */
				const char *sap = ap->saddr;

				do
					*v++ = *sap;
				while (*++sap != '\0');
				*v++ = ' ';
			}
		}
		*--v = '\0';
        }
	if (i)
		n++;
#endif

	return (ssize_t)n;
}

int
dhcp6_dump(struct interface *ifp)
{
	struct dhcp6_state *state;

	ifp->if_data[IF_DATA_DHCP6] = state = calloc(1, sizeof(*state));
	if (state == NULL) {
		logerr(__func__);
		return -1;
	}
	TAILQ_INIT(&state->addrs);
	dhcp_set_leasefile(state->leasefile, sizeof(state->leasefile),
	    AF_INET6, ifp);
	if (dhcp6_readlease(ifp, 0) == -1) {
		logerr("%s: %s", __func__,
		    *ifp->name ? ifp->name : state->leasefile);
		return -1;
	}
	state->reason = "DUMP6";
	return script_runreason(ifp, state->reason);
}
#endif
