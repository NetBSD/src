/* $NetBSD: wlanctl.c,v 1.13.18.1 2014/08/10 07:00:35 tls Exp $ */
/*-
 * Copyright (c) 2005 David Young.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or
 * without modification, are permitted provided that the following
 * conditions are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer in the documentation and/or other materials provided
 *    with the distribution.
 * 3. The name of David Young may not be used to endorse or promote
 *    products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY David Young ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL David
 * Young BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 * TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
 * OF SUCH DAMAGE.
 */
#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>

#include <sys/param.h>
#include <sys/sysctl.h>
#include <sys/inttypes.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>

#include <net/if.h>
#include <net/if_media.h>
#include <netinet/in.h>
#include <netinet/if_ether.h>

#include <net80211/ieee80211.h>
#include <net80211/ieee80211_sysctl.h>

struct flagname {
	u_int32_t fn_flag;
	const char *fn_name;
};

struct cmdflags {
	int	cf_a;	/* all 802.11 interfaces */
  	int     cf_p;   /* public (i.e. non-private) dests */
};

static void		print_flags(u_int32_t, const struct flagname *, u_int);
static int		dump_nodes(const char *, int, struct cmdflags *);
static const char	*ether_string(u_int8_t *);
static void		parse_args(int *, char ***, struct cmdflags *);
static void		print_capinfo(u_int16_t);
static void		print_channel(u_int16_t, u_int16_t, u_int16_t);
static void		print_node_flags(u_int32_t);
static void		print_rateset(struct ieee80211_rateset *, int);
__dead static void	usage(void);

static void
print_rateset(struct ieee80211_rateset *rs, int txrate)
{
	int i, rate;
	const char *basic;

	printf("\trates");

	for (i = 0; i < rs->rs_nrates; i++) {

		if ((rs->rs_rates[i] & IEEE80211_RATE_BASIC) != 0)
			basic = "*";
		else
			basic = "";
		rate = 5 * (rs->rs_rates[i] & IEEE80211_RATE_VAL);
		if (i == txrate)
			printf(" [%s%d.%d]", basic, rate / 10, rate % 10);
		else
			printf(" %s%d.%d", basic, rate / 10, rate % 10);
	}
	printf("\n");
}

static void
print_flags(u_int32_t flags, const struct flagname *flagnames, u_int nname)
{
	u_int i;
	const char *delim;
	delim = "<";

	for (i = 0; i < nname; i++) {
		if ((flags & flagnames[i].fn_flag) != 0) {
			printf("%s%s", delim, flagnames[i].fn_name);
			delim = ",";
		}
	}

	printf("%s\n", (delim[0] == '<') ? "" : ">");
}

static void
print_node_flags(u_int32_t flags)
{
	static const struct flagname nodeflags[] = {
		  {IEEE80211_NODE_SYSCTL_F_BSS, "bss"}
		, {IEEE80211_NODE_SYSCTL_F_STA, "sta"}
		, {IEEE80211_NODE_SYSCTL_F_SCAN, "scan"}
	};
	printf("\tnode flags %04x", flags);

	print_flags(flags, nodeflags, __arraycount(nodeflags));
}

static void
print_capinfo(u_int16_t capinfo)
{
	static const struct flagname capflags[] = {
		{IEEE80211_CAPINFO_ESS, "ess"},
		{IEEE80211_CAPINFO_IBSS, "ibss"},
		{IEEE80211_CAPINFO_CF_POLLABLE, "cf pollable"},
		{IEEE80211_CAPINFO_CF_POLLREQ, "request cf poll"},
		{IEEE80211_CAPINFO_PRIVACY, "privacy"},
		{IEEE80211_CAPINFO_SHORT_PREAMBLE, "short preamble"},
		{IEEE80211_CAPINFO_PBCC, "pbcc"},
		{IEEE80211_CAPINFO_CHNL_AGILITY, "channel agility"},
		{IEEE80211_CAPINFO_SHORT_SLOTTIME, "short slot-time"},
		{IEEE80211_CAPINFO_RSN, "rsn"},
		{IEEE80211_CAPINFO_DSSSOFDM, "dsss-ofdm"}
	};

	printf("\tcapabilities %04x", capinfo);

	print_flags(capinfo, capflags, __arraycount(capflags));
}

static const char *
ether_string(u_int8_t *addr)
{
	struct ether_addr ea;
	(void)memcpy(ea.ether_addr_octet, addr, sizeof(ea.ether_addr_octet));
	return ether_ntoa(&ea);
}

static void
print_channel(u_int16_t chanidx, u_int16_t freq, u_int16_t flags)
{
	static const struct flagname chanflags[] = {
		{IEEE80211_CHAN_TURBO, "turbo"},
		{IEEE80211_CHAN_CCK, "cck"},
		{IEEE80211_CHAN_OFDM, "ofdm"},
		{IEEE80211_CHAN_2GHZ, "2.4GHz"},
		{IEEE80211_CHAN_5GHZ, "5GHz"},
		{IEEE80211_CHAN_PASSIVE, "passive scan"},
		{IEEE80211_CHAN_DYN, "dynamic cck-ofdm"},
		{IEEE80211_CHAN_GFSK, "gfsk"}
	};
	printf("\tchan %d freq %dMHz flags %04x", chanidx, freq, flags);

	print_flags(flags, chanflags, __arraycount(chanflags));
}

/*
 *
 * ifname:   dump nodes belonging to the given interface, or belonging
 *           to all interfaces if NULL
 * hdr_type: header type: IEEE80211_SYSCTL_T_NODE -> generic node,
 *                        IEEE80211_SYSCTL_T_RSSADAPT -> rssadapt(9) info,
 *                        IEEE80211_SYSCTL_T_DRVSPEC -> driver specific.
 * cf:       command flags: cf_a != 0 -> all 802.11 interfaces
 *                          cf_p != 0 -> public dests
 */
static int
dump_nodes(const char *ifname_arg, int hdr_type, struct cmdflags *cf)
{
#if 0
/*39*/	u_int8_t	ns_erp;		/* 11g only */
/*40*/	u_int32_t	ns_rstamp;	/* recv timestamp */
/*64*/	u_int16_t	ns_fhdwell;	/* FH only */
/*66*/	u_int8_t	ns_fhindex;	/* FH only */
/*68*/
#endif
	u_int i, ifindex;
	size_t namelen, nodes_len, totallen;
	int name[12];
	int *vname;
	char ifname[IFNAMSIZ];
	struct ieee80211_node_sysctl *pns, *ns;
	u_int64_t ts;

	namelen = __arraycount(name);

	if (sysctlnametomib("net.link.ieee80211.nodes", &name[0],
	    &namelen) != 0) {
		warn("sysctlnametomib");
		return -1;
	}

	if (ifname_arg == NULL)
		ifindex = 0;
	else if ((ifindex = if_nametoindex(ifname_arg)) == 0) {
		warn("if_nametoindex");
		return -1;
	}

	totallen = namelen + IEEE80211_SYSCTL_NODENAMELEN;
	if (totallen >= __arraycount(name)) {
		warnx("Internal error finding sysctl mib");
		return -1;
	}
	vname = &name[namelen];

	vname[IEEE80211_SYSCTL_NODENAME_IF] = ifindex;
	vname[IEEE80211_SYSCTL_NODENAME_OP] = IEEE80211_SYSCTL_OP_ALL;
	vname[IEEE80211_SYSCTL_NODENAME_ARG] = 0;
	vname[IEEE80211_SYSCTL_NODENAME_TYPE] = hdr_type;
	vname[IEEE80211_SYSCTL_NODENAME_ELTSIZE] = sizeof(*ns);
	vname[IEEE80211_SYSCTL_NODENAME_ELTCOUNT] = INT_MAX;

	/* how many? */
	if (sysctl(name, totallen, NULL, &nodes_len, NULL, 0) != 0) {
		warn("sysctl(count)");
		return -1;
	}

	ns = malloc(nodes_len);

	if (ns == NULL) {
		warn("malloc");
		return -1;
	}

	vname[IEEE80211_SYSCTL_NODENAME_ELTCOUNT] = nodes_len / sizeof(ns[0]);

	/* Get them. */
	if (sysctl(name, totallen, ns, &nodes_len, NULL, 0) != 0) {
		warn("sysctl(get)");
		return -1;
	}

	for (i = 0; i < nodes_len / sizeof(ns[0]); i++) {
		pns = &ns[i];
		if (if_indextoname(pns->ns_ifindex, ifname) == NULL) {
			warn("if_indextoname");
			return -1;
		}
		if (cf->cf_p && (pns->ns_capinfo & IEEE80211_CAPINFO_PRIVACY))
		  	continue;
		printf("%s: mac %s ", ifname, ether_string(pns->ns_macaddr));
		printf("bss %s\n", ether_string(pns->ns_bssid));
		print_node_flags(pns->ns_flags);

		/* TBD deal with binary ESSID */
		printf("\tess <%.*s>\n", pns->ns_esslen, pns->ns_essid);

		print_channel(pns->ns_chanidx, pns->ns_freq, pns->ns_chanflags);

		print_capinfo(pns->ns_capinfo);

		assert(sizeof(ts) == sizeof(pns->ns_tstamp));
		memcpy(&ts, &pns->ns_tstamp[0], sizeof(ts));
		printf("\tbeacon-interval %d TU tsft %" PRIu64 " us\n",
		    pns->ns_intval, (u_int64_t)le64toh(ts));

		print_rateset(&pns->ns_rates, pns->ns_txrate);

		printf("\tassoc-id %d assoc-failed %d inactivity %ds\n",
		    pns->ns_associd, pns->ns_fails, pns->ns_inact);

		printf("\trssi %d txseq %d rxseq %d\n",
		    pns->ns_rssi, pns->ns_txseq, pns->ns_rxseq);
	}
	return 0;
}

static void
usage(void)
{
	fprintf(stderr,
	    "Usage: %s [ -p ] -a\n"
	    "       %s [ -p ] interface [ interface ... ]\n",
	    getprogname(), getprogname());
	exit(EXIT_FAILURE);
}

static void
parse_args(int *argcp, char ***argvp, struct cmdflags *cf)
{
	int ch;

	(void)memset(cf, 0, sizeof(*cf));

	while ((ch = getopt(*argcp, *argvp, "ap")) != -1) {
		switch (ch) {
		case 'a':
			cf->cf_a = 1;
			break;
		case 'p':
			cf->cf_p = 1;
			break;
		default:
			warnx("unknown option -%c", ch);
			usage();
		}
	}

	*argcp -= optind;
	*argvp += optind;
}

#define	LOGICAL_XOR(x, y) (!(x) != !(y))

int
main(int argc, char **argv)
{
	int i;
	struct cmdflags cf;

	parse_args(&argc, &argv, &cf);

	if (!LOGICAL_XOR(argc > 0, cf.cf_a))
		usage();

	if (cf.cf_a) {
		if (dump_nodes(NULL, IEEE80211_SYSCTL_T_NODE, &cf) != 0)
			return EXIT_FAILURE;
		return EXIT_SUCCESS;
	}
	for (i = 0; i < argc; i++) {
		if (dump_nodes(argv[i], IEEE80211_SYSCTL_T_NODE, &cf) != 0)
			return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
