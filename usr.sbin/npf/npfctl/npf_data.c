/*	$NetBSD: npf_data.c,v 1.18 2012/08/12 03:35:13 rmind Exp $	*/

/*-
 * Copyright (c) 2009-2012 The NetBSD Foundation, Inc.
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
__RCSID("$NetBSD: npf_data.c,v 1.18 2012/08/12 03:35:13 rmind Exp $");

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
#include <string.h>
#include <err.h>
#include <errno.h>
#include <ifaddrs.h>
#include <netdb.h>

#include "npfctl.h"

static struct ifaddrs *		ifs_list = NULL;

unsigned long
npfctl_find_ifindex(const char *ifname)
{
	unsigned long if_idx = if_nametoindex(ifname);

	if (!if_idx) {
		if ((if_idx = npfctl_debug_addif(ifname)) != 0) {
			return if_idx;
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
	npfvar_t *vp = npfvar_create(".addr");
	fam_addr_mask_t fam;

	memset(&fam, 0, sizeof(fam));

	if (!npfctl_parse_fam_addr(addr, &fam.fam_family, &fam.fam_addr))
		goto out;

	/*
	 * Note: both mask and nummask may be NULL.  In such case,
	 * npfctl_parse_mask() will handle and will set full mask.
	 */
	if (nummask) {
		fam.fam_mask = *nummask;
	} else if (!npfctl_parse_mask(mask, fam.fam_family, &fam.fam_mask)) {
		goto out;
	}

	if (!npfvar_add_element(vp, NPFVAR_FAM, &fam, sizeof(fam)))
		goto out;

	return vp;
out:
	npfvar_destroy(vp);
	return NULL;
}

npfvar_t *
npfctl_parse_table_id(const char *id)
{
	npfvar_t *vp;

	if (!npfctl_table_exists_p(id)) {
		yyerror("table '%s' is not defined", id);
		return NULL;
	}
	vp = npfvar_create(".table");

	if (!npfvar_add_element(vp, NPFVAR_TABLE, id, strlen(id) + 1))
		goto out;

	return vp;
out:
	npfvar_destroy(vp);
	return NULL;
}

/*
 * npfctl_parse_port_range: create a port-range variable.  Note that the
 * passed port numbers should be in host byte order.
 */
npfvar_t *
npfctl_parse_port_range(in_port_t s, in_port_t e)
{
	npfvar_t *vp = npfvar_create(".port_range");
	port_range_t pr;

	pr.pr_start = htons(s);
	pr.pr_end = htons(e);

	if (!npfvar_add_element(vp, NPFVAR_PORT_RANGE, &pr, sizeof(pr)))
		goto out;

	return vp;
out:
	npfvar_destroy(vp);
	return NULL;
}

npfvar_t *
npfctl_parse_port_range_variable(const char *v)
{
	npfvar_t *vp = npfvar_lookup(v);
	size_t count = npfvar_get_count(vp);
	npfvar_t *pvp = npfvar_create(".port_range");
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
npfctl_parse_iface(const char *ifname)
{
	npfvar_t *vp = npfvar_create(".iface");
	struct ifaddrs *ifa;
	fam_addr_mask_t fam;
	bool gotif = false;

	if (ifs_list == NULL && getifaddrs(&ifs_list) == -1) {
		err(EXIT_FAILURE, "getifaddrs");
	}
	memset(&fam, 0, sizeof(fam));

	npfvar_t *ip = npfvar_create(".ifname");
	if (!npfvar_add_element(ip, NPFVAR_STRING, ifname, strlen(ifname) + 1))
		goto out;

	for (ifa = ifs_list; ifa != NULL; ifa = ifa->ifa_next) {
		struct sockaddr *sa;
		sa_family_t family;

		if (strcmp(ifa->ifa_name, ifname) != 0)
			continue;

		gotif = true;
		if ((ifa->ifa_flags & IFF_UP) == 0)
			warnx("interface '%s' is down", ifname);

		sa = ifa->ifa_addr;
		family = sa->sa_family;
		if (family != AF_INET && family != AF_INET6)
			continue;

		fam.fam_family = family;
		fam.fam_interface = ip;

		if (!npfctl_copy_address(family, &fam.fam_addr, sa))
			goto out;

		if (!npfctl_parse_mask(NULL, fam.fam_family, &fam.fam_mask))
			goto out;

		if (!npfvar_add_element(vp, NPFVAR_FAM, &fam, sizeof(fam)))
			goto out;

	}
	if (!gotif) {
		yyerror("interface '%s' not found", ifname);
		goto out;
	}
	if (npfvar_get_count(vp) == 0) {
		yyerror("no addresses matched for interface '%s'", ifname);
		goto out;
	}
	return vp;
out:
	npfvar_destroy(vp);
	npfvar_destroy(ip);
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

	npfvar_t *vp = npfvar_create(".tcp_flag");
	if (!npfvar_add_element(vp, NPFVAR_TCPFLAG, &tfl, sizeof(tfl))) {
		npfvar_destroy(vp);
		return NULL;
	}

	return vp;
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
				return (ul+128);
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
	npfvar_t *vp = npfvar_create(".icmp");
	int varnum;

	switch (proto) {
	case IPPROTO_ICMP:
		varnum = NPFVAR_ICMP;
		break;
	case IPPROTO_ICMPV6:
		varnum = NPFVAR_ICMP6;
		break;
	default:
		assert(false);
	}

	if (!npfvar_add_element(vp, varnum, &type, sizeof(type)))
		goto out;

	if (!npfvar_add_element(vp, varnum, &code, sizeof(code)))
		goto out;

	return vp;
out:
	npfvar_destroy(vp);
	return NULL;
}
