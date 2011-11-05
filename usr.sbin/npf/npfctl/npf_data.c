/*	$NetBSD: npf_data.c,v 1.9 2011/11/05 19:19:29 jakllsch Exp $	*/

/*-
 * Copyright (c) 2009-2011 The NetBSD Foundation, Inc.
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
 * npfctl(8) helper routines.
 */

#include <sys/cdefs.h>
__RCSID("$NetBSD: npf_data.c,v 1.9 2011/11/05 19:19:29 jakllsch Exp $");

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <netinet/tcp.h>

#include <arpa/inet.h>

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <err.h>
#include <ifaddrs.h>
#include <netdb.h>
#include <assert.h>

#include "npfctl.h"

static struct ifaddrs *		ifs_list = NULL;
nl_config_t *			npf_conf = NULL;

void
npfctl_init_data(void)
{

	npf_conf = npf_config_create();
	if (npf_conf == NULL) {
		errx(EXIT_FAILURE, "npf_config_create");
	}
	if (getifaddrs(&ifs_list) == -1) {
		err(EXIT_FAILURE, "getifaddrs");
	}
}

int
npfctl_ioctl_send(int fd)
{
	int error = npf_config_submit(npf_conf, fd);
	npf_config_destroy(npf_conf);
	return error;
}

/*
 * Helper routines:
 *
 *	npfctl_getif() - get interface addresses and index number from name.
 *	npfctl_parse_v4mask() - parse address/mask integers from CIDR block.
 *	npfctl_parse_port() - parse port number (which may be a service name).
 *	npfctl_parse_tcpfl() - parse TCP flags.
 */

struct ifaddrs *
npfctl_getif(char *ifname, unsigned int *if_idx, bool reqaddr, sa_family_t addrtype)
{
	struct ifaddrs *ifent;
	struct sockaddr_in *sin;

	for (ifent = ifs_list; ifent != NULL; ifent = ifent->ifa_next) {
		sin = (struct sockaddr_in *)ifent->ifa_addr;
		if (sin->sin_family != addrtype && reqaddr)
			continue;
		if (strcmp(ifent->ifa_name, ifname) == 0)
			break;
	}
	if (ifent) {
		*if_idx = if_nametoindex(ifname);
	}
	return ifent;
}

bool
npfctl_parse_port(char *ostr, bool *range, in_port_t *fport, in_port_t *tport)
{
	char *str = xstrdup(ostr), *sep;

	*range = false;
	if ((sep = strchr(str, ':')) != NULL) {
		/* Port range (only numeric). */
		*range = true;
		*sep = '\0';

	} else if (isalpha((unsigned char)*str)) {
		struct servent *se;

		se = getservbyname(str, NULL);
		if (se == NULL) {
			free(str);
			return false;
		}
		*fport = se->s_port;
	} else {
		*fport = htons(atoi(str));
	}
	*tport = sep ? htons(atoi(sep + 1)) : *fport;
	free(str);
	return true;
}

void
npfctl_create_mask(sa_family_t family, u_int length, npf_addr_t *omask)
{
	uint32_t part;
	uint32_t *mask = (uint32_t*)omask;

	memset(omask, 0, sizeof(npf_addr_t));
	if (family == AF_INET) {
		part = htonl(0xffffffff << (32 - length));
		memcpy(mask, &part, 4);
	} else if (family == AF_INET6) {
		while (length > 32) {
			part = htonl(0xffffffff);
			memcpy(mask, &part, 4);
			mask += 1;
			length -= 32;
		}
		part = htonl(0xffffffff << (32 - length));
		memcpy(mask, &part, 4);
	}
}

sa_family_t
npfctl_get_addrfamily(const char *ostr)
{
	struct addrinfo hint, *res = NULL;
	int ret; 
	char *str = xstrdup(ostr);
	char *p = strchr(str, '/');
	sa_family_t family;

	if (p)
	    *p = '\0';
	memset(&hint, '\0', sizeof(hint));
	hint.ai_family = PF_UNSPEC;
	hint.ai_flags = AI_NUMERICHOST;
	ret = getaddrinfo(str, NULL, &hint, &res);
	if (ret) {
		family = AF_UNSPEC;
	} else {
		family = res->ai_family;
		freeaddrinfo(res);
	}
	free(str);
	return family;
}

sa_family_t
npfctl_parse_cidr(char *str, sa_family_t addrfamily, npf_addr_t *addr, npf_netmask_t *mask)
{

	if (strcmp(str, "any") == 0) {
		memset(addr, 0, sizeof(npf_addr_t));
		memset(mask, 0, sizeof(npf_netmask_t));
	} else if (isalpha((unsigned char)*str)) {
		/* TODO: handle multiple addresses per interface */
		struct ifaddrs *ifa;
		struct sockaddr_in *sin;
		u_int idx;
		if ((ifa = npfctl_getif(str, &idx, true, AF_INET)) == NULL) {
			errx(EXIT_FAILURE, "invalid interface '%s'", str);
		}
		/* Interface address. */
		sin = (struct sockaddr_in *)ifa->ifa_addr;
		memcpy(addr, &(sin->sin_addr.s_addr), sizeof(struct in_addr));
		//v4mask = 0xffffffff; - TODO!
	} else {
		char *p = strchr(str, '/');
		if (p != NULL) {
			*p++ = '\0';
			*mask = atoi(p);
		} else {
			if (addrfamily == AF_INET)
				*mask = 32;
			else
				*mask = 128;
		}
		memset(addr, 0, sizeof(npf_addr_t));
		int ret = inet_pton(addrfamily, str, addr);
		if (ret != 1) {
			printf("TODO: error");
		}
	}

	return addrfamily;
}

static bool
npfctl_parse_tcpfl(char *s, uint8_t *tfl, uint8_t *tfl_mask)
{
	uint8_t tcpfl = 0;
	bool mask = false;

	while (*s) {
		switch (*s) {
		case 'F': tcpfl |= TH_FIN; break;
		case 'S': tcpfl |= TH_SYN; break;
		case 'R': tcpfl |= TH_RST; break;
		case 'P': tcpfl |= TH_PUSH; break;
		case 'A': tcpfl |= TH_ACK; break;
		case 'U': tcpfl |= TH_URG; break;
		case 'E': tcpfl |= TH_ECE; break;
		case 'W': tcpfl |= TH_CWR; break;
		case '/':
			*s = '\0';
			*tfl = tcpfl;
			tcpfl = 0;
			mask = true;
			break;
		default:
			return false;
		}
		s++;
	}
	if (!mask) {
		*tfl = tcpfl;
	}
	*tfl_mask = tcpfl;
	return true;
}

void
npfctl_fill_table(nl_table_t *tl, char *fname)
{
	char *buf;
	FILE *fp;
	size_t n;
	int l;

	fp = fopen(fname, "r");
	if (fp == NULL) {
		err(EXIT_FAILURE, "open '%s'", fname);
	}
	l = 1;
	buf = NULL;
	while (getline(&buf, &n, fp) != -1) {
		npf_addr_t addr;
		npf_netmask_t mask;

		if (*buf == '\n' || *buf == '#')
			continue;

		if (!npfctl_parse_cidr(buf, npfctl_get_addrfamily(buf), &addr, &mask)) {
			errx(EXIT_FAILURE, "invalid table entry at line %d", l);
		}

		/* Create and add table entry. */
		npf_table_add_entry(tl, &addr, mask);
		l++;
	}
	if (buf != NULL) {
		free(buf);
	}
}

/*
 * N-code generation helpers.
 */

static void
npfctl_rulenc_cidr(void **nc, int nblocks[], var_t *dat, bool sd, sa_family_t addrfamily)
{
	element_t *el = dat->v_elements;
	int foff;

	/* If table, generate a single table matching block. */
	if (dat->v_type == VAR_TABLE) {
		u_int tid = atoi(el->e_data);

		nblocks[0]--;
		foff = npfctl_failure_offset(nblocks);
		npfctl_gennc_tbl(nc, foff, tid, sd);
		return;
	}

	/* Generate v4/v6 CIDR matching blocks. */
	for (el = dat->v_elements; el != NULL; el = el->e_next) {
		npf_addr_t addr;
		npf_netmask_t mask;

		npfctl_parse_cidr(el->e_data, addrfamily, &addr, &mask);
		if (addrfamily == AF_INET)
		    nblocks[1]--;
		else if (addrfamily == AF_INET6)
		    nblocks[3]--;
		foff = npfctl_failure_offset(nblocks);
		if (addrfamily == AF_INET) 
			npfctl_gennc_v4cidr(nc, foff, &addr, mask, sd);
		else if (addrfamily == AF_INET6)
			npfctl_gennc_v6cidr(nc, foff, &addr, mask, sd);
	}
}

static void
npfctl_rulenc_ports(void **nc, int nblocks[], var_t *dat, bool tcpudp,
    bool both, bool sd)
{
	element_t *el = dat->v_elements;
	int foff;

	assert(dat->v_type != VAR_TABLE);

	/* Generate TCP/UDP port matching blocks. */
	for (el = dat->v_elements; el != NULL; el = el->e_next) {
		in_port_t fport, tport;
		bool range;

		if (!npfctl_parse_port(el->e_data, &range, &fport, &tport)) {
			errx(EXIT_FAILURE, "invalid service '%s'", el->e_data);
		}
		nblocks[0]--;
		foff = both ? 0 : npfctl_failure_offset(nblocks);
		npfctl_gennc_ports(nc, foff, fport, tport, tcpudp, sd);
	}
}

static void
npfctl_rulenc_block(void **nc, int nblocks[], var_t *cidr, var_t *ports,
    bool both, bool tcpudp, bool sd, sa_family_t addrfamily)
{

	npfctl_rulenc_cidr(nc, nblocks, cidr, sd, addrfamily);
	if (ports == NULL) {
		return;
	}
	npfctl_rulenc_ports(nc, nblocks, ports, tcpudp, both, sd);
	if (!both) {
		return;
	}
	npfctl_rulenc_ports(nc, nblocks, ports, !tcpudp, false, sd);
}

void
npfctl_rule_ncode(nl_rule_t *rl, char *proto, char *tcpfl, int icmp_type,
    int icmp_code, var_t *from, sa_family_t addrfamily, var_t *fports, var_t *to, var_t *tports)
{
	int nblocks[4] = { 0, 0, 0, 0 };
	bool icmp, tcpudp, both;
	void *ncptr, *nc;
	size_t sz, foff;

	/*
	 * Default: both TCP and UDP.
	 */
	icmp = false;
	tcpudp = true;
	if (proto == NULL) {
		both = true;
		goto skip_proto;
	}
	both = false;

	if (strcmp(proto, "icmp") == 0) {
		/* ICMP case. */
		fports = NULL;
		tports = NULL;
		icmp = true;

	} else if (strcmp(proto, "tcp") == 0) {
		/* Just TCP. */
		tcpudp = true;

	} else if (strcmp(proto, "udp") == 0) {
		/* Just UDP. */
		tcpudp = false;

	} else {
		/* Default. */
	}
skip_proto:
	if (icmp || icmp_type != -1) {
		assert(tcpfl == NULL);
		icmp = true;
		nblocks[2] += 1;
	}
	if (tcpudp && tcpfl) {
		assert(icmp_type == -1 && icmp_code == -1);
		nblocks[2] += 1;
	}

	/* Calculate how blocks to determince n-code. */
	if (from && from->v_count) {
		if (from->v_type == VAR_TABLE)
			nblocks[0] += 1;
		else {
			if (addrfamily == AF_INET)
				nblocks[1] += from->v_count;
			else
				nblocks[3] += from->v_count;
		}
		if (fports && fports->v_count)
			nblocks[0] += fports->v_count * (both ? 2 : 1);
	}
	if (to && to->v_count) {
		if (to->v_type == VAR_TABLE)
			nblocks[0] += 1;
		else {
			if (addrfamily == AF_INET)
				nblocks[1] += to->v_count;
			else
				nblocks[3] += to->v_count;
		}
		if (tports && tports->v_count)
			nblocks[0] += tports->v_count * (both ? 2 : 1);
	}

	/* Any n-code to generate? */
	if (!icmp && (nblocks[0] + nblocks[1] + nblocks[2] + nblocks[3]) == 0) {
		/* Done, if none. */
		return;
	}

	/* Allocate memory for the n-code. */
	sz = npfctl_calc_ncsize(nblocks);
	ncptr = malloc(sz);
	if (ncptr == NULL) {
		err(EXIT_FAILURE, "malloc");
	}
	nc = ncptr;

	/*
	 * Generate v4/v6 CIDR matching blocks and TCP/UDP port matching.
	 */
	if (from) {
		npfctl_rulenc_block(&nc, nblocks, from, fports,
		    both, tcpudp, true, addrfamily);
	}
	if (to) {
		npfctl_rulenc_block(&nc, nblocks, to, tports,
		    both, tcpudp, false, addrfamily);
	}

	if (icmp) {
		/*
		 * ICMP case.
		 */
		nblocks[2]--;
		foff = npfctl_failure_offset(nblocks);
		npfctl_gennc_icmp(&nc, foff, icmp_type, icmp_code);

	} else if (tcpudp && tcpfl) {
		/*
		 * TCP case, flags.
		 */
		uint8_t tfl = 0, tfl_mask;

		nblocks[2]--;
		foff = npfctl_failure_offset(nblocks);
		if (!npfctl_parse_tcpfl(tcpfl, &tfl, &tfl_mask)) {
			errx(EXIT_FAILURE, "invalid TCP flags '%s'", tcpfl);
		}
		npfctl_gennc_tcpfl(&nc, foff, tfl, tfl_mask);
	}
	npfctl_gennc_complete(&nc);

	if ((uintptr_t)nc - (uintptr_t)ncptr != sz) {
		errx(EXIT_FAILURE, "n-code size got wrong (%tu != %zu)",
		    (uintptr_t)nc - (uintptr_t)ncptr, sz);
	}

#ifdef DEBUG
	uint32_t *op = ncptr;
	size_t n = sz;
	do {
		DPRINTF(("\t> |0x%02x|\n", (u_int)*op));
		op++;
		n -= sizeof(*op);
	} while (n);
#endif

	/* Create a final memory block of data, ready to send. */
	if (npf_rule_setcode(rl, NPF_CODE_NCODE, ncptr, sz) == -1) {
		errx(EXIT_FAILURE, "npf_rule_setcode");
	}
	free(ncptr);
}
