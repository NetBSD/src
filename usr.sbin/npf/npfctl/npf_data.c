/*	$NetBSD: npf_data.c,v 1.18.2.2 2014/08/20 00:05:11 tls Exp $	*/

/*-
 * Copyright (c) 2009-2014 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * npfctl(8) data manipulation and helper routines.
 */

#include <sys/cdefs.h>
__RCSID("$NetBSD: npf_data.c,v 1.18.2.2 2014/08/20 00:05:11 tls Exp $");

#include <sys/types.h>
#include <sys/null.h>

#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#define ICMP_STRINGS
#include <netinet/ip_icmp.h>
#define ICMP6_STRINGS
#include <netinet/icmp6.h>
#include <netinet/tcp.h>
#include <net/if.h>

#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <ctype.h>
#include <err.h>
#include <errno.h>
#include <ifaddrs.h>
#include <netdb.h>

#include "npfctl.h"

static struct ifaddrs *		ifs_list = NULL;

void
npfctl_note_interface(const char *ifname)
{
	unsigned long if_idx = if_nametoindex(ifname);
	bool testif = npfctl_debug_addif(ifname);
	const char *p = ifname;

	/* If such interface exists or if it is a test interface - done. */
	if (if_idx || testif) {
		return;
	}

	/*
	 * Minimum sanity check.  The interface name shall be non-empty
	 * string shorter than IFNAMSIZ and alphanumeric only.
	 */
	if (*p == '\0') {
		goto invalid;
	}
	while (*p) {
		const size_t len = (ptrdiff_t)p - (ptrdiff_t)ifname;

		if (!isalnum((unsigned char)*p) || len > IFNAMSIZ) {
invalid:		yyerror("illegitimate interface name '%s'", ifname);
		}
		p++;
	}

	/* Throw a warning, so that the user could double check. */
	warnx("warning - unknown interface '%s'", ifname);
}

static unsigned long
npfctl_find_ifindex(const char *ifname)
{
	unsigned long if_idx = if_nametoindex(ifname);
	bool testif = npfctl_debug_addif(ifname);

	if (!if_idx) {
		if (testif) {
			static u_int dummy_if_idx = (1 << 15);
			return ++dummy_if_idx;
		}
		yyerror("unknown interface '%s'", ifname);
	}
	return if_idx;
}

static bool
npfctl_copy_address(sa_family_t fam, npf_addr_t *addr, const void *ptr)
{
	memset(addr, 0, sizeof(npf_addr_t));

	switch (fam) {
	case AF_INET: {
		const struct sockaddr_in *sin = ptr;
		memcpy(addr, &sin->sin_addr, sizeof(sin->sin_addr));
		return true;
	}
	case AF_INET6: {
		const struct sockaddr_in6 *sin6 = ptr;
		memcpy(addr, &sin6->sin6_addr, sizeof(sin6->sin6_addr));
		return true;
	}
	default:
		yyerror("unknown address family %u", fam);
		return false;
	}
}

static bool
npfctl_parse_fam_addr(const char *name, sa_family_t *fam, npf_addr_t *addr)
{
	static const struct addrinfo hint = {
		.ai_family = AF_UNSPEC,
		.ai_flags = AI_NUMERICHOST
	};
	struct addrinfo *ai;
	int ret;

	ret = getaddrinfo(name, NULL, &hint, &ai);
	if (ret) {
		yyerror("cannot parse '%s' (%s)", name, gai_strerror(ret));
		return false;
	}
	if (fam) {
		*fam = ai->ai_family;
	}
	if (!npfctl_copy_address(*fam, addr, ai->ai_addr)) {
		return false;
	}
	freeaddrinfo(ai);
	return true;
}

static bool
npfctl_parse_mask(const char *s, sa_family_t fam, npf_netmask_t *mask)
{
	char *ep = NULL;
	npf_addr_t addr;
	uint8_t *ap;

	if (s) {
		errno = 0;
		*mask = (npf_netmask_t)strtol(s, &ep, 0);
		if (*ep == '\0' && s != ep && errno != ERANGE)
			return true;
		if (!npfctl_parse_fam_addr(s, &fam, &addr))
			return false;
	}

	assert(fam == AF_INET || fam == AF_INET6);
	*mask = NPF_NO_NETMASK;
	if (ep == NULL) {
		return true;
	}

	ap = addr.s6_addr + (*mask / 8) - 1;
	while (ap >= addr.s6_addr) {
		for (int j = 8; j > 0; j--) {
			if (*ap & 1)
				return true;
			*ap >>= 1;
			(*mask)--;
			if (*mask == 0)
				return true;
		}
		ap--;
	}
	return true;
}

/*
 * npfctl_parse_fam_addr_mask: return address family, address and mask.
 *
 * => Mask is optional and can be NULL.
 * => Returns true on success or false if unable to parse.
 */
npfvar_t *
npfctl_parse_fam_addr_mask(const char *addr, const char *mask,
    unsigned long *nummask)
{
	fam_addr_mask_t fam;

	memset(&fam, 0, sizeof(fam));

	if (!npfctl_parse_fam_addr(addr, &fam.fam_family, &fam.fam_addr))
		return NULL;

	/*
	 * Note: both mask and nummask may be NULL.  In such case,
	 * npfctl_parse_mask() will handle and will set full mask.
	 */
	if (nummask) {
		fam.fam_mask = *nummask;
	} else if (!npfctl_parse_mask(mask, fam.fam_family, &fam.fam_mask)) {
		return NULL;
	}
	return npfvar_create_element(NPFVAR_FAM, &fam, sizeof(fam));
}

npfvar_t *
npfctl_parse_table_id(const char *name)
{
	u_int tid;

	tid = npfctl_table_getid(name);
	if (tid == (unsigned)-1) {
		yyerror("table '%s' is not defined", name);
		return NULL;
	}
	return npfvar_create_element(NPFVAR_TABLE, &tid, sizeof(u_int));
}

/*
 * npfctl_parse_port_range: create a port-range variable.  Note that the
 * passed port numbers should be in host byte order.
 */
npfvar_t *
npfctl_parse_port_range(in_port_t s, in_port_t e)
{
	port_range_t pr;

	pr.pr_start = htons(s);
	pr.pr_end = htons(e);

	return npfvar_create_element(NPFVAR_PORT_RANGE, &pr, sizeof(pr));
}

npfvar_t *
npfctl_parse_port_range_variable(const char *v)
{
	npfvar_t *vp = npfvar_lookup(v);
	size_t count = npfvar_get_count(vp);
	npfvar_t *pvp = npfvar_create();
	port_range_t *pr;
	in_port_t p;

	for (size_t i = 0; i < count; i++) {
		int type = npfvar_get_type(vp, i);
		void *data = npfvar_get_data(vp, type, i);

		switch (type) {
		case NPFVAR_IDENTIFIER:
		case NPFVAR_STRING:
			p = npfctl_portno(data);
			npfvar_add_elements(pvp, npfctl_parse_port_range(p, p));
			break;
		case NPFVAR_PORT_RANGE:
			pr = data;
			npfvar_add_element(pvp, NPFVAR_PORT_RANGE, pr,
			    sizeof(*pr));
			break;
		case NPFVAR_NUM:
			p = *(unsigned long *)data;
			npfvar_add_elements(pvp, npfctl_parse_port_range(p, p));
			break;
		default:
			yyerror("wrong variable '%s' type '%s' for port range",
			    v, npfvar_type(type));
			npfvar_destroy(pvp);
			return NULL;
		}
	}
	return pvp;
}

npfvar_t *
npfctl_parse_ifnet(const char *ifname, const int family)
{
	struct ifaddrs *ifa;
	ifnet_addr_t ifna;
	npfvar_t *vpa;

	if (ifs_list == NULL && getifaddrs(&ifs_list) == -1) {
		err(EXIT_FAILURE, "getifaddrs");
	}

	vpa = npfvar_create();
	ifna.ifna_name = estrdup(ifname);
	ifna.ifna_addrs = vpa;
	ifna.ifna_index = npfctl_find_ifindex(ifname);
	assert(ifna.ifna_index != 0);

	for (ifa = ifs_list; ifa != NULL; ifa = ifa->ifa_next) {
		fam_addr_mask_t fam;
		struct sockaddr *sa;

		if (strcmp(ifa->ifa_name, ifname) != 0)
			continue;

		if ((ifa->ifa_flags & IFF_UP) == 0)
			warnx("interface '%s' is down", ifname);

		sa = ifa->ifa_addr;
		if (sa->sa_family != AF_INET && sa->sa_family != AF_INET6)
			continue;
		if (family != AF_UNSPEC && sa->sa_family != family)
			continue;

		memset(&fam, 0, sizeof(fam));
		fam.fam_family = sa->sa_family;
		fam.fam_ifindex = ifna.ifna_index;

		if (!npfctl_copy_address(sa->sa_family, &fam.fam_addr, sa))
			goto out;

		if (!npfctl_parse_mask(NULL, fam.fam_family, &fam.fam_mask))
			goto out;

		if (!npfvar_add_element(vpa, NPFVAR_FAM, &fam, sizeof(fam)))
			goto out;
	}
	if (npfvar_get_count(vpa) == 0) {
		yyerror("no addresses matched for interface '%s'", ifname);
		goto out;
	}

	return npfvar_create_element(NPFVAR_INTERFACE, &ifna, sizeof(ifna));
out:
	npfvar_destroy(ifna.ifna_addrs);
	return NULL;
}

bool
npfctl_parse_cidr(char *cidr, fam_addr_mask_t *fam, int *alen)
{
	char *mask, *p;

	p = strchr(cidr, '\n');
	if (p) {
		*p = '\0';
	}
	mask = strchr(cidr, '/');
	if (mask) {
		*mask++ = '\0';
	}

	memset(fam, 0, sizeof(*fam));
	if (!npfctl_parse_fam_addr(cidr, &fam->fam_family, &fam->fam_addr)) {
		return false;
	}
	if (!npfctl_parse_mask(mask, fam->fam_family, &fam->fam_mask)) {
		return false;
	}
	switch (fam->fam_family) {
	case AF_INET:
		*alen = sizeof(struct in_addr);
		break;
	case AF_INET6:
		*alen = sizeof(struct in6_addr);
		break;
	default:
		return false;
	}
	return true;
}

int
npfctl_protono(const char *proto)
{
	struct protoent *pe;

	pe = getprotobyname(proto);
	if (pe == NULL) {
		yyerror("unknown protocol '%s'", proto);
		return -1;
	}
	return pe->p_proto;
}

/*
 * npfctl_portno: convert port identifier (string) to a number.
 *
 * => Returns port number in host byte order.
 */
in_port_t
npfctl_portno(const char *port)
{
	struct addrinfo *ai, *rai;
	in_port_t p = 0;
	int e;

	e = getaddrinfo(NULL, port, NULL, &rai);
	if (e != 0) {
		yyerror("invalid port name '%s' (%s)", port, gai_strerror(e));
		return 0;
	}

	for (ai = rai; ai; ai = ai->ai_next) {
		switch (ai->ai_family) {
		case AF_INET: {
			struct sockaddr_in *sin = (void *)ai->ai_addr;
			p = sin->sin_port;
			goto out;
		}
		case AF_INET6: {
			struct sockaddr_in6 *sin6 = (void *)ai->ai_addr;
			p = sin6->sin6_port;
			goto out;
		}
		default:
			break;
		}
	}
out:
	freeaddrinfo(rai);
	return ntohs(p);
}

npfvar_t *
npfctl_parse_tcpflag(const char *s)
{
	uint8_t tfl = 0;

	while (*s) {
		switch (*s) {
		case 'F': tfl |= TH_FIN; break;
		case 'S': tfl |= TH_SYN; break;
		case 'R': tfl |= TH_RST; break;
		case 'P': tfl |= TH_PUSH; break;
		case 'A': tfl |= TH_ACK; break;
		case 'U': tfl |= TH_URG; break;
		case 'E': tfl |= TH_ECE; break;
		case 'W': tfl |= TH_CWR; break;
		default:
			yyerror("invalid flag '%c'", *s);
			return NULL;
		}
		s++;
	}
	return npfvar_create_element(NPFVAR_TCPFLAG, &tfl, sizeof(tfl));
}

uint8_t
npfctl_icmptype(int proto, const char *type)
{
	uint8_t ul;

	switch (proto) {
	case IPPROTO_ICMP:
		for (ul = 0; icmp_type[ul]; ul++)
			if (strcmp(icmp_type[ul], type) == 0)
				return ul;
		break;
	case IPPROTO_ICMPV6:
		for (ul = 0; icmp6_type_err[ul]; ul++)
			if (strcmp(icmp6_type_err[ul], type) == 0)
				return ul;
		for (ul = 0; icmp6_type_info[ul]; ul++)
			if (strcmp(icmp6_type_info[ul], type) == 0)
				return ul + 128;
		break;
	default:
		assert(false);
	}

	yyerror("unknown icmp-type %s", type);
	return ~0;
}

uint8_t
npfctl_icmpcode(int proto, uint8_t type, const char *code)
{
	const char * const *arr;

	switch (proto) {
	case IPPROTO_ICMP:
		switch (type) {
		case ICMP_ECHOREPLY:
		case ICMP_SOURCEQUENCH:
		case ICMP_ALTHOSTADDR:
		case ICMP_ECHO:
		case ICMP_ROUTERSOLICIT:
		case ICMP_TSTAMP:
		case ICMP_TSTAMPREPLY:
		case ICMP_IREQ:
		case ICMP_IREQREPLY:
		case ICMP_MASKREQ:
		case ICMP_MASKREPLY:
			arr = icmp_code_none;
			break;
		case ICMP_ROUTERADVERT:
			arr = icmp_code_routeradvert;
			break;
		case ICMP_UNREACH:
			arr = icmp_code_unreach;
			break;
		case ICMP_REDIRECT:
			arr = icmp_code_redirect;
			break;
		case ICMP_TIMXCEED:
			arr = icmp_code_timxceed;
			break;
		case ICMP_PARAMPROB:
			arr = icmp_code_paramprob;
			break;
		case ICMP_PHOTURIS:
			arr = icmp_code_photuris;
			break;
		default:
			yyerror("unknown icmp-type %d while parsing code %s",
				type, code);
			return ~0;
		}
		break;
	case IPPROTO_ICMPV6:
		switch (type) {
		case ICMP6_DST_UNREACH:
			arr = icmp6_code_unreach;
			break;
		case ICMP6_TIME_EXCEEDED:
			arr = icmp6_code_timxceed;
			break;
		case ICMP6_PARAM_PROB:
			arr = icmp6_code_paramprob;
			break;
		case ICMP6_PACKET_TOO_BIG:
		/* code-less info ICMPs */
		case ICMP6_ECHO_REQUEST:
		case ICMP6_ECHO_REPLY:
		case MLD_LISTENER_QUERY:
		case MLD_LISTENER_REPORT:
		case MLD_LISTENER_DONE:
		case ND_ROUTER_SOLICIT:
		case ND_ROUTER_ADVERT:
		case ND_NEIGHBOR_SOLICIT:
		case ND_NEIGHBOR_ADVERT:
		case ND_REDIRECT:
			arr = icmp6_code_none;
			break;
		/* XXX TODO: info ICMPs with code values */
		default:
			yyerror("unknown icmp-type %d while parsing code %s",
				type, code);
			return ~0;
		}
		break;
	default:
		assert(false);
	}

	for (uint8_t ul = 0; arr[ul]; ul++) {
		if (strcmp(arr[ul], code) == 0)
			return ul;
	}
	yyerror("unknown code %s for icmp-type %d", code, type);
	return ~0;
}

npfvar_t *
npfctl_parse_icmp(int proto, int type, int code)
{
	npfvar_t *vp = npfvar_create();

	if (!npfvar_add_element(vp, NPFVAR_ICMP, &type, sizeof(type)))
		goto out;

	if (!npfvar_add_element(vp, NPFVAR_ICMP, &code, sizeof(code)))
		goto out;

	return vp;
out:
	npfvar_destroy(vp);
	return NULL;
}

/*
 * npfctl_npt66_calcadj: calculate the adjustment for NPTv6 as per RFC 6296.
 */
uint16_t
npfctl_npt66_calcadj(npf_netmask_t len, const npf_addr_t *pref_in,
    const npf_addr_t *pref_out)
{
	const uint16_t *addr6_in = (const uint16_t *)pref_in;
	const uint16_t *addr6_out = (const uint16_t *)pref_out;
	unsigned i, remnant, wordmask, preflen = len >> 4;
	uint32_t adj, isum = 0, osum = 0;

	/*
	 * Extract the bits within a 16-bit word (when prefix length is
	 * not dividable by 16) and include them into the sum.
	 */
	remnant = len - (preflen << 4);
	wordmask = (1U << remnant) - 1;
	assert(wordmask == 0 || (len % 16) != 0);

	/* Inner prefix - sum and fold. */
	for (i = 0; i < preflen; i++) {
		isum += addr6_in[i];
	}
	isum += addr6_in[i] & wordmask;
	while (isum >> 16) {
		isum = (isum >> 16) + (isum & 0xffff);
	}

	/* Outer prefix - sum and fold. */
	for (i = 0; i < preflen; i++) {
		osum += addr6_out[i];
	}
	osum += addr6_out[i] & wordmask;
	while (osum >> 16) {
		osum = (osum >> 16) + (osum & 0xffff);
	}

	/* Calculate 1's complement difference. */
	adj = isum + ~osum;
	while (adj >> 16) {
		adj = (adj >> 16) + (adj & 0xffff);
	}
	return (uint16_t)adj;
}
