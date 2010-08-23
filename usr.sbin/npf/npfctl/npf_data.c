/*	$NetBSD: npf_data.c,v 1.2 2010/08/23 06:01:04 jnemeth Exp $	*/

/*-
 * Copyright (c) 2009-2010 The NetBSD Foundation, Inc.
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
 * NPF proplib(9) dictionary producer.
 *
 * XXX: Needs some clean-up.
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>

#include <arpa/inet.h>
#include <prop/proplib.h>

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

static prop_dictionary_t	npf_dict, settings_dict;
static prop_array_t		nat_arr, tables_arr, rules_arr;

static pri_t			gr_prio_counter = 1;
static pri_t			rl_prio_counter = 1;
static pri_t			nat_prio_counter = 1;

void
npfctl_init_data(void)
{
	prop_number_t ver;

	if (getifaddrs(&ifs_list) == -1)
		err(EXIT_FAILURE, "getifaddrs");

	npf_dict = prop_dictionary_create();

	ver = prop_number_create_integer(NPF_VERSION);
	prop_dictionary_set(npf_dict, "version", ver);

	nat_arr = prop_array_create();
	prop_dictionary_set(npf_dict, "nat", nat_arr);

	settings_dict = prop_dictionary_create();
	prop_dictionary_set(npf_dict, "settings", settings_dict);

	tables_arr = prop_array_create();
	prop_dictionary_set(npf_dict, "tables", tables_arr);

	rules_arr = prop_array_create();
	prop_dictionary_set(npf_dict, "rules", rules_arr);
}

int
npfctl_ioctl_send(int fd)
{
	int ret = 0, errval;

#ifdef DEBUG
	prop_dictionary_externalize_to_file(npf_dict, "/tmp/npf.plist");
#else
	errval = prop_dictionary_send_ioctl(npf_dict, fd, IOC_NPF_RELOAD);
	if (errval) {
		errx(EXIT_FAILURE, "npf_ioctl_send: %s\n", strerror(errval));
		ret = -1;
	}
#endif
	prop_object_release(npf_dict);
	return ret;
}

/*
 * Helper routines:
 *
 *	npfctl_getif() - get interface addresses and index number from name.
 *	npfctl_servname2port() - get service ports from name.
 *	npfctl_parse_v4mask() - parse address/mask integers from CIDR block.
 */

static struct ifaddrs *
npfctl_getif(char *ifname, unsigned int *if_idx)
{
	struct ifaddrs *ifent;
	struct sockaddr_in *sin;

	for (ifent = ifs_list; ifent != NULL; ifent = ifent->ifa_next) {
		sin = (struct sockaddr_in *)ifent->ifa_addr;

		if (sin->sin_family != AF_INET)
			continue;
		if (strcmp(ifent->ifa_name, ifname) == 0)
			break;
	}
	if (ifent) {
		*if_idx = if_nametoindex(ifname);
	}
	return ifent;
}

static int
npfctl_servname2port(char *name)
{
	struct servent *se;

	se = getservbyname(name, NULL);
	return se ? se->s_port : -1;
}

bool
npfctl_parse_v4mask(char *str, in_addr_t *addr, in_addr_t *mask)
{
	char *p = strchr(str, '/');
	u_int bits;

	/* In network byte order. */
	if (p) {
		*p++ = '\0';
		bits = (u_int)atoi(p);
		*mask = bits ? htonl(0xffffffff << (32 - bits)) : 0;
	} else {
		*mask = 0xffffffff;
	}
	return inet_aton(str, (struct in_addr *)addr) != 0;
}

static void
npfctl_parse_cidr(char *str, in_addr_t *addr, in_addr_t *mask)
{

	if (isalpha((unsigned char)*str)) {
		struct ifaddrs *ifa;
		struct sockaddr_in *sin;
		u_int idx;

		if ((ifa = npfctl_getif(str, &idx)) == NULL) {
			errx(EXIT_FAILURE, "invalid interface '%s'", str);
		}
		/* Interface address. */
		sin = (struct sockaddr_in *)ifa->ifa_addr;
		*addr = sin->sin_addr.s_addr;
		*mask = 0xffffffff;

	} else if (!npfctl_parse_v4mask(str, addr, mask)) {
		errx(EXIT_FAILURE, "invalid CIDR '%s'\n", str);
	}
}

/*
 * NPF table creation and construction routines.
 */

prop_dictionary_t
npfctl_lookup_table(char *tidstr)
{
	prop_dictionary_t tl;
	prop_object_iterator_t it;
	prop_object_t obj;
	u_int tid;

	if ((it = prop_array_iterator(tables_arr)) == NULL)
		err(EXIT_FAILURE, "prop_array_iterator");

	tid = atoi(tidstr);
	while ((tl = prop_object_iterator_next(it)) != NULL) {
		obj = prop_dictionary_get(tl, "id");
		if (tid == prop_number_integer_value(obj))
			break;
	}
	return tl;
}

prop_dictionary_t
npfctl_mk_table(void)
{
	prop_dictionary_t tl;
	prop_array_t tlist;

	tl = prop_dictionary_create();
	tlist = prop_array_create();
	prop_dictionary_set(tl, "entries", tlist);

	return tl;
}

void
npfctl_table_setup(prop_dictionary_t tl, char *idstr, char *typestr)
{
	prop_number_t typenum;
	unsigned int id;

	id = atoi(idstr);
	/* TODO: 1. check ID range 2. check if not a duplicate */
	prop_dictionary_set(tl, "id", prop_number_create_integer(id));

	if (strcmp(typestr, "hash")) {
		typenum = prop_number_create_integer(NPF_TABLE_HASH);
	} else if (strcmp(typestr, "tree")) {
		typenum = prop_number_create_integer(NPF_TABLE_RBTREE);
	} else {
		errx(EXIT_FAILURE, "invalid table type '%s'\n", typestr);
	}
	prop_dictionary_set(tl, "type", typenum);
}

void
npfctl_construct_table(prop_dictionary_t tl, char *fname)
{
	prop_dictionary_t entdict;
	prop_array_t tblents;
	char *buf;
	FILE *fp;
	size_t n;
	int l;

	tblents = prop_dictionary_get(tl, "entries");
	assert(tblents != NULL);

	fp = fopen(fname, "r");
	if (fp == NULL) {
		err(EXIT_FAILURE, "fopen");
	}
	l = 1;
	buf = NULL;
	while (getline(&buf, &n, fp) != -1) {
		in_addr_t addr, mask;

		if (*buf == '\n' || *buf == '#')
			continue;

		/* IPv4 CIDR: a.b.c.d/mask */
		if (!npfctl_parse_v4mask(buf, &addr, &mask))
			errx(EXIT_FAILURE, "invalid table entry at line %d", l);

		/* Create and add table entry. */
		entdict = prop_dictionary_create();
		prop_dictionary_set(entdict, "addr",
		    prop_number_create_integer(addr));
		prop_dictionary_set(entdict, "mask",
		    prop_number_create_integer(mask));
		prop_array_add(tblents, entdict);
		l++;
	}
	if (buf != NULL) {
		free(buf);
	}
}

void
npfctl_add_table(prop_dictionary_t tl)
{

	prop_array_add(tables_arr, tl);
}

/*
 * npfctl_mk_rule: create a rule (or group) dictionary.
 *
 * Note: group is a rule containing subrules.  It has no n-code, however.
 */
prop_dictionary_t
npfctl_mk_rule(bool group)
{
	prop_dictionary_t rl;
	prop_array_t subrl;
	pri_t pri;

	rl = prop_dictionary_create();
	if (group) {
		subrl = prop_array_create();
		prop_dictionary_set(rl, "subrules", subrl);
		/* Give new priority, reset rule priority counter. */
		pri = gr_prio_counter++;
		rl_prio_counter = 1;
	} else {
		pri = rl_prio_counter++;
	}
	prop_dictionary_set(rl, "priority",
	    prop_number_create_integer(pri));

	return rl;
}

void
npfctl_add_rule(prop_dictionary_t rl, prop_dictionary_t parent)
{
	prop_array_t rlset;

	if (parent) {
		rlset = prop_dictionary_get(parent, "subrules");
		assert(rlset != NULL);
	} else {
		rlset = rules_arr;
	}
	prop_array_add(rlset, rl);
}

void
npfctl_rule_setattr(prop_dictionary_t rl, int attr, char *iface)
{
	prop_number_t attrnum;

	attrnum = prop_number_create_integer(attr);
	prop_dictionary_set(rl, "attributes", attrnum);
	if (iface) {
		prop_number_t ifnum;
		unsigned int if_idx;

		if (npfctl_getif(iface, &if_idx) == NULL) {
			errx(EXIT_FAILURE, "invalid interface '%s'", iface);
		}
		ifnum = prop_number_create_integer(if_idx);
		prop_dictionary_set(rl, "interface", ifnum);
	}
}

/*
 * Main rule generation routines.
 */

static void
npfctl_rulenc_v4cidr(void **nc, int nblocks[], var_t *dat, bool sd)
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

	/* Generate v4 CIDR matching blocks. */
	for (el = dat->v_elements; el != NULL; el = el->e_next) {
		in_addr_t addr, mask;

		npfctl_parse_cidr(el->e_data, &addr, &mask);

		nblocks[1]--;
		foff = npfctl_failure_offset(nblocks);
		npfctl_gennc_v4cidr(nc, foff, addr, mask, sd);
	}
}

static void
npfctl_rulenc_ports(void **nc, int nblocks[], var_t *dat, bool tcpudp, bool sd)
{
	element_t *el = dat->v_elements;
	int foff;

	assert(dat->v_type != VAR_TABLE);

	/* Generate TCP/UDP port matching blocks. */
	for (el = dat->v_elements; el != NULL; el = el->e_next) {
		int pfrom, pto;
		char *sep;

		if ((sep = strchr(el->e_data, ':')) != NULL) {
			/* Port range (only numeric). */
			*sep = '\0';
		}
		if (isalpha((unsigned char)*el->e_data)) {
			pfrom = npfctl_servname2port(el->e_data);
			if (pfrom == -1) {
				errx(EXIT_FAILURE, "invalid service '%s'",
				    el->e_data);
			}
		} else {
			pfrom = htons(atoi(el->e_data));
		}
		pto = sep ? htons(atoi(sep + 1)) : pfrom;

		nblocks[0]--;
		foff = npfctl_failure_offset(nblocks);
		npfctl_gennc_ports(nc, foff, pfrom, pto, tcpudp, sd);
	}
}

static void
npfctl_rulenc_block(void **nc, int nblocks[], var_t *cidr, var_t *ports,
    bool both, bool tcpudp, bool sd)
{

	npfctl_rulenc_v4cidr(nc, nblocks, cidr, sd);
	if (ports == NULL) {
		return;
	}
	npfctl_rulenc_ports(nc, nblocks, ports, tcpudp, sd);
	if (!both) {
		return;
	}
	npfctl_rulenc_ports(nc, nblocks, ports, !tcpudp, sd);
}

void
npfctl_rule_protodata(prop_dictionary_t rl, char *proto, var_t *from,
    var_t *fports, var_t *to, var_t *tports)
{
	prop_data_t ncdata;
	bool icmp, tcpudp, both;
	int nblocks[2] = { 0, 0 };
	void *ncptr, *nc;
	size_t sz;

	/*
	 * Default: both TCP and UDP.
	 */
	icmp = false;
	tcpudp = true;
	both = false;
	if (proto == NULL) {
		goto skip_proto;
	}

	if (strcmp(proto, "icmp") == 0) {
		/* ICMP case. */
		fports = NULL;
		tports = NULL;
		icmp = true;
		nblocks[0] += 1;

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

	/* Calculate how blocks to determince n-code. */
	if (from && from->v_count) {
		if (from->v_type == VAR_TABLE)
			nblocks[0] += 1;
		else
			nblocks[1] += from->v_count;
		if (fports && fports->v_count)
			nblocks[0] += fports->v_count * (both ? 2 : 1);
	}
	if (to && to->v_count) {
		if (to->v_type == VAR_TABLE)
			nblocks[0] += 1;
		else
			nblocks[1] += to->v_count;
		if (tports && tports->v_count)
			nblocks[0] += tports->v_count * (both ? 2 : 1);
	}

	/* Allocate memory for the n-code. */
	sz = npfctl_calc_ncsize(nblocks);
	ncptr = malloc(sz);
	if (ncptr == NULL) {
		perror("malloc");
		exit(EXIT_FAILURE);
	}
	nc = ncptr;

	/* Ethernet fragment (ETHERTYPE_IP), XXX. */
	npfctl_gennc_ether(&nc, npfctl_failure_offset(nblocks), htons(0x0800));

	/* Generate v4 CIDR matching blocks and TCP/UDP port matching. */
	if (from) {
		npfctl_rulenc_block(&nc, nblocks, from, fports,
		    both, tcpudp, true);
	}
	if (to) {
		npfctl_rulenc_block(&nc, nblocks, to, tports,
		    both, tcpudp, false);
	}
	/* ICMP case. */
	if (icmp) {
		const int foff = npfctl_failure_offset(nblocks);
		npfctl_gennc_icmp(&nc, foff, -1, -1);
	}
	npfctl_gennc_complete(&nc);

	if ((uintptr_t)nc - (uintptr_t)ncptr != sz)
		errx(EXIT_FAILURE, "n-code size got wrong (%tu != %zu)",
		    (uintptr_t)nc - (uintptr_t)ncptr, sz);

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
	ncdata = prop_data_create_data(ncptr, sz);
	if (ncdata == NULL) {
		perror("prop_data_create_data");
		exit(EXIT_FAILURE);
	}
	prop_dictionary_set(rl, "ncode", ncdata);
	free(ncptr);
}

/*
 * NAT policy construction routines.
 */

prop_dictionary_t
npfctl_mk_nat(void)
{
	prop_dictionary_t rl;
	pri_t pri;

	/* NAT policy is rule with extra info. */
	rl = prop_dictionary_create();
	pri = nat_prio_counter++;
	prop_dictionary_set(rl, "priority",
	    prop_number_create_integer(pri));
	return rl;
}

void
npfctl_add_nat(prop_dictionary_t nat)
{
	prop_array_add(nat_arr, nat);
}

void
npfctl_nat_setup(prop_dictionary_t rl, char *iface, char *gwip)
{
	const int attr = NPF_RULE_PASS | NPF_RULE_OUT | NPF_RULE_FINAL;
	in_addr_t addr, mask;

	/* Interface and attributes. */
	npfctl_rule_setattr(rl, attr, iface);

	/* Gateway IP, XXX should be no mask. */
	npfctl_parse_cidr(gwip, &addr, &mask);
	prop_dictionary_set(rl, "gateway_ip", prop_number_create_integer(addr));
}
