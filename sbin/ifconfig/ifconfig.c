/*	$NetBSD: ifconfig.c,v 1.184.2.1 2008/05/18 12:30:53 yamt Exp $	*/

/*-
 * Copyright (c) 1997, 1998, 2000 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe of the Numerical Aerospace Simulation Facility,
 * NASA Ames Research Center.
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
 * Copyright (c) 1983, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
#ifndef lint
__COPYRIGHT("@(#) Copyright (c) 1983, 1993\n\
	The Regents of the University of California.  All rights reserved.\n");
#endif /* not lint */

#ifndef lint
#if 0
static char sccsid[] = "@(#)ifconfig.c	8.2 (Berkeley) 2/16/94";
#else
__RCSID("$NetBSD: ifconfig.c,v 1.184.2.1 2008/05/18 12:30:53 yamt Exp $");
#endif
#endif /* not lint */

#include <sys/param.h>
#include <sys/socket.h>
#include <sys/ioctl.h>

#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_media.h>
#include <net/if_ether.h>
#include <netinet/in.h>		/* XXX */
#include <netinet/in_var.h>	/* XXX */
 
#include <netdb.h>

#include <sys/protosw.h>

#include <assert.h>
#include <ctype.h>
#include <err.h>
#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ifaddrs.h>
#include <util.h>

#include "extern.h"

#ifndef INET_ONLY
#include "af_atalk.h"
#include "af_iso.h"
#endif /* ! INET_ONLY */
#include "af_inet.h"
#ifdef INET6
#include "af_inet6.h"
#endif /* INET6 */
#include "af_link.h"

#include "agr.h"
#include "carp.h"
#include "ieee80211.h"
#include "tunnel.h"
#include "vlan.h"
#include "parse.h"
#include "env.h"

int	setaddr, doalias;
int	clearaddr;
int	newaddr = -1;
int	check_up_state = -1;
int	bflag, dflag, hflag, lflag, mflag, sflag, uflag, vflag, zflag;
#ifdef INET6
int	Lflag;
#endif

/*
 * Media stuff.  Whenever a media command is first performed, the
 * currently select media is grabbed for this interface.  If `media'
 * is given, the current media word is modifed.  `mediaopt' commands
 * only modify the set and clear words.  They then operate on the
 * current media word later.
 */
int	media_current;
int	mediaopt_set;
int	mediaopt_clear;

void	check_ifflags_up(prop_dictionary_t);

int 	notealias(prop_dictionary_t, prop_dictionary_t);
int 	notrailers(prop_dictionary_t, prop_dictionary_t);
int 	setifaddr(prop_dictionary_t, prop_dictionary_t);
int 	setifdstormask(prop_dictionary_t, prop_dictionary_t);
int 	setifflags(prop_dictionary_t, prop_dictionary_t);
static int	setifcaps(prop_dictionary_t, prop_dictionary_t);
int 	setifbroadaddr(prop_dictionary_t, prop_dictionary_t);
static int 	setifmetric(prop_dictionary_t, prop_dictionary_t);
static int 	setifmtu(prop_dictionary_t, prop_dictionary_t);
int 	setifnetmask(prop_dictionary_t, prop_dictionary_t);
int	setifprefixlen(prop_dictionary_t, prop_dictionary_t);
int	setmedia(prop_dictionary_t, prop_dictionary_t);
int	setmediamode(prop_dictionary_t, prop_dictionary_t);
int	setmediaopt(prop_dictionary_t, prop_dictionary_t);
int	unsetmediaopt(prop_dictionary_t, prop_dictionary_t);
int	setmediainst(prop_dictionary_t, prop_dictionary_t);
static int	clone_command(prop_dictionary_t, prop_dictionary_t);
static void	do_setifpreference(prop_dictionary_t);

static int no_cmds_exec(prop_dictionary_t, prop_dictionary_t);
static int media_status_exec(prop_dictionary_t, prop_dictionary_t);

int	carrier(prop_dictionary_t);
void	printall(const char *, prop_dictionary_t);
int	list_cloners(prop_dictionary_t, prop_dictionary_t);
void 	status(const struct sockaddr_dl *, prop_dictionary_t,
    prop_dictionary_t);
void 	usage(void);

void	print_media_word(int, const char *);
void	process_media_commands(prop_dictionary_t);
void	init_current_media(prop_dictionary_t, prop_dictionary_t);

/* Known address families */
static const struct afswtch afs[] = {
	  {.af_name = "inet", .af_af = AF_INET, .af_status = in_status,
	   .af_addr_commit = in_commit_address}

	, {.af_name = "link", .af_af = AF_LINK, .af_status = link_status,
	   .af_addr_commit = link_commit_address}
#ifdef INET6
	, {.af_name = "inet6", .af_af = AF_INET6, .af_status = in6_status,
	   .af_addr_commit = in6_commit_address}
#endif

#ifndef INET_ONLY	/* small version, for boot media */
	, {.af_name = "atalk", .af_af = AF_APPLETALK, .af_status = at_status,
	   .af_getaddr = at_getaddr, NULL, .af_difaddr = SIOCDIFADDR,
	   .af_aifaddr = SIOCAIFADDR, .af_gifaddr = SIOCGIFADDR,
	   .af_ridreq = &at_addreq, .af_addreq = &at_addreq}
	, {.af_name = "iso", .af_af = AF_ISO, .af_status = iso_status,
	   .af_getaddr = iso_getaddr, NULL, .af_difaddr = SIOCDIFADDR_ISO,
	   .af_aifaddr = SIOCAIFADDR_ISO, .af_gifaddr = SIOCGIFADDR_ISO,
	   .af_ridreq = &iso_ridreq, .af_addreq = &iso_addreq}
#endif	/* INET_ONLY */

	, {.af_name = NULL}	/* sentinel */
};

static const struct kwinst ifflagskw[] = {
	  IFKW("arp", IFF_NOARP)
	, IFKW("debug", IFF_DEBUG)
	, IFKW("link0", IFF_LINK0)
	, IFKW("link1", IFF_LINK1)
	, IFKW("link2", IFF_LINK2)
	, {.k_word = "down", .k_type = KW_T_NUM, .k_num = -IFF_UP}
	, {.k_word = "up", .k_type = KW_T_NUM, .k_num = IFF_UP}
};

static const struct kwinst ifcapskw[] = {
	  IFKW("ip4csum-tx",	IFCAP_CSUM_IPv4_Tx)
	, IFKW("ip4csum-rx",	IFCAP_CSUM_IPv4_Rx)
	, IFKW("tcp4csum-tx",	IFCAP_CSUM_TCPv4_Tx)
	, IFKW("tcp4csum-rx",	IFCAP_CSUM_TCPv4_Rx)
	, IFKW("udp4csum-tx",	IFCAP_CSUM_UDPv4_Tx)
	, IFKW("udp4csum-rx",	IFCAP_CSUM_UDPv4_Rx)
	, IFKW("tcp6csum-tx",	IFCAP_CSUM_TCPv6_Tx)
	, IFKW("tcp6csum-rx",	IFCAP_CSUM_TCPv6_Rx)
	, IFKW("udp6csum-tx",	IFCAP_CSUM_UDPv6_Tx)
	, IFKW("udp6csum-rx",	IFCAP_CSUM_UDPv6_Rx)
	, IFKW("ip4csum",	IFCAP_CSUM_IPv4_Tx|IFCAP_CSUM_IPv4_Rx)
	, IFKW("tcp4csum",	IFCAP_CSUM_TCPv4_Tx|IFCAP_CSUM_TCPv4_Rx)
	, IFKW("udp4csum",	IFCAP_CSUM_UDPv4_Tx|IFCAP_CSUM_UDPv4_Rx)
	, IFKW("tcp6csum",	IFCAP_CSUM_TCPv6_Tx|IFCAP_CSUM_TCPv6_Rx)
	, IFKW("udp6csum",	IFCAP_CSUM_UDPv6_Tx|IFCAP_CSUM_UDPv6_Rx)
	, IFKW("tso4",		IFCAP_TSOv4)
	, IFKW("tso6",		IFCAP_TSOv6)
};

extern struct pbranch command_root;
extern struct pbranch opt_command;
extern struct pbranch opt_family, opt_silent_family;
extern struct pkw cloning, silent_family, family, ifcaps, ifflags, misc;

struct pinteger parse_metric = PINTEGER_INITIALIZER(&parse_metric, "metric", 10,
    setifmetric, "metric", &command_root.pb_parser);

struct pinteger parse_mtu = PINTEGER_INITIALIZER(&parse_mtu, "mtu", 10,
    setifmtu, "mtu", &command_root.pb_parser);

struct pinteger parse_prefixlen = PINTEGER_INITIALIZER(&parse_prefixlen,
    "prefixlen", 10, setifprefixlen, "prefixlen", &command_root.pb_parser);

struct pinteger parse_preference = PINTEGER_INITIALIZER1(&parse_preference,
    "preference", INT16_MIN, INT16_MAX, 10, NULL, "preference",
    &command_root.pb_parser);

struct paddr parse_netmask = PADDR_INITIALIZER(&parse_netmask, "netmask",
    setifnetmask, "dstormask", NULL, NULL, NULL, &command_root.pb_parser);

struct paddr parse_broadcast = PADDR_INITIALIZER(&parse_broadcast,
    "broadcast address",
    setifbroadaddr, "broadcast", NULL, NULL, NULL, &command_root.pb_parser);

struct pstr mediamode = PSTR_INITIALIZER(&mediamode, "mediamode",
    setmediamode, "mediamode", &command_root.pb_parser);

struct pinteger mediainst = PINTEGER_INITIALIZER1(&mediainst, "mediainst",
    0, IFM_INST_MAX, 10, setmediainst, "mediainst", &command_root.pb_parser);

struct pstr unmediaopt = PSTR_INITIALIZER(&unmediaopt, "-mediaopt",
    unsetmediaopt, "unmediaopt", &command_root.pb_parser);

struct pstr mediaopt = PSTR_INITIALIZER(&mediaopt, "mediaopt",
    setmediaopt, "mediaopt", &command_root.pb_parser);

struct pstr media = PSTR_INITIALIZER(&media, "media",
    setmedia, "media", &command_root.pb_parser);

static const struct kwinst misckw[] = {
	  {.k_word = "active", .k_key = "active", .k_type = KW_T_BOOL,
	   .k_bool = true, .k_nextparser = &command_root.pb_parser}
	, {.k_word = "alias", .k_key = "alias", .k_deact = "alias",
	   .k_type = KW_T_BOOL, .k_neg = true,
	   .k_bool = true, .k_negbool = false,
	   .k_exec = notealias, .k_nextparser = &command_root.pb_parser}
	, {.k_word = "broadcast", .k_nextparser = &parse_broadcast.pa_parser}
	, {.k_word = "delete", .k_key = "alias", .k_deact = "alias",
	   .k_type = KW_T_BOOL, .k_bool = false, .k_exec = notealias,
	   .k_nextparser = &command_root.pb_parser}
	, {.k_word = "instance", .k_key = "anymedia", .k_type = KW_T_BOOL,
	   .k_bool = true, .k_act = "media", .k_deact = "mediainst",
	   .k_nextparser = &mediainst.pi_parser}
	, {.k_word = "inst", .k_key = "anymedia", .k_type = KW_T_BOOL,
	   .k_bool = true, .k_act = "media", .k_deact = "mediainst",
	   .k_nextparser = &mediainst.pi_parser}
	, {.k_word = "media", .k_key = "anymedia", .k_type = KW_T_BOOL,
	   .k_bool = true, .k_deact = "media", .k_altdeact = "anymedia",
	   .k_nextparser = &media.ps_parser}
	, {.k_word = "mediaopt", .k_key = "anymedia", .k_type = KW_T_BOOL,
	   .k_bool = true, .k_deact = "mediaopt", .k_altdeact = "instance",
	   .k_nextparser = &mediaopt.ps_parser}
	, {.k_word = "-mediaopt", .k_key = "anymedia", .k_type = KW_T_BOOL,
	   .k_bool = true, .k_deact = "unmediaopt", .k_altdeact = "media",
	   .k_nextparser = &unmediaopt.ps_parser}
	, {.k_word = "mode", .k_key = "anymedia", .k_type = KW_T_BOOL,
	   .k_bool = true, .k_deact = "mode",
	   .k_nextparser = &mediamode.ps_parser}
	, {.k_word = "metric", .k_nextparser = &parse_metric.pi_parser}
	, {.k_word = "mtu", .k_nextparser = &parse_mtu.pi_parser}
	, {.k_word = "netmask", .k_nextparser = &parse_netmask.pa_parser}
	, {.k_word = "preference", .k_act = "address",
	   .k_nextparser = &parse_preference.pi_parser}
	, {.k_word = "prefixlen", .k_nextparser = &parse_prefixlen.pi_parser}
	, {.k_word = "trailers", .k_neg = true,
	   .k_exec = notrailers, .k_nextparser = &command_root.pb_parser}
};

/* key: clonecmd */
static const struct kwinst clonekw[] = {
	{.k_word = "create", .k_type = KW_T_NUM, .k_num = SIOCIFCREATE,
	 .k_nextparser = &opt_silent_family.pb_parser},
	{.k_word = "destroy", .k_type = KW_T_NUM, .k_num = SIOCIFDESTROY}
};

static const struct kwinst familykw[] = {
	  {.k_word = "inet", .k_type = KW_T_NUM, .k_num = AF_INET,
	   .k_nextparser = NULL}
	, {.k_word = "link", .k_type = KW_T_NUM, .k_num = AF_LINK,
	   .k_nextparser = NULL}
#ifdef INET6
	, {.k_word = "inet6", .k_type = KW_T_NUM, .k_num = AF_INET6,
	   .k_nextparser = NULL}
#endif
#ifndef INET_ONLY	/* small version, for boot media */
	, {.k_word = "atalk", .k_type = KW_T_NUM, .k_num = AF_APPLETALK,
	   .k_nextparser = NULL}
	, {.k_word = "iso", .k_type = KW_T_NUM, .k_num = AF_ISO,
	   .k_nextparser = NULL}
#endif	/* INET_ONLY */
};

struct pterm cloneterm = PTERM_INITIALIZER(&cloneterm, "list cloners",
    list_cloners, "none");

struct pterm no_cmds = PTERM_INITIALIZER(&no_cmds, "no commands", no_cmds_exec,
    "none");

struct pkw family_only =
    PKW_INITIALIZER(&family_only, "family-only", NULL, "af", familykw,
	__arraycount(familykw), &no_cmds.pt_parser);

struct paddr address = PADDR_INITIALIZER(&address,
    "local address (address 1)",
    setifaddr, "address", "netmask", NULL, "address", &command_root.pb_parser);

struct paddr dstormask = PADDR_INITIALIZER(&dstormask,
    "destination/netmask (address 2)",
    setifdstormask, "dstormask", NULL, "address", "dstormask",
    &command_root.pb_parser);

struct paddr broadcast = PADDR_INITIALIZER(&broadcast,
    "broadcast address (address 3)",
    setifbroadaddr, "broadcast", NULL, "dstormask", "broadcast",
    &command_root.pb_parser);

struct branch opt_clone_brs[] = {
	  {.b_nextparser = &cloning.pk_parser}
	, {.b_nextparser = &opt_family.pb_parser}
}, opt_silent_family_brs[] = {
	  {.b_nextparser = &silent_family.pk_parser}
	, {.b_nextparser = &command_root.pb_parser}
}, opt_family_brs[] = {
	  {.b_nextparser = &family.pk_parser}
	, {.b_nextparser = &opt_command.pb_parser}
}, command_root_brs[] = {
	  {.b_nextparser = &ieee80211bool.pk_parser}
	, {.b_nextparser = &ifflags.pk_parser}
	, {.b_nextparser = &ifcaps.pk_parser}
#ifdef INET6
	, {.b_nextparser = &ia6flags.pk_parser}
	, {.b_nextparser = &inet6.pk_parser}
#endif /*INET6*/
	, {.b_nextparser = &misc.pk_parser}
	, {.b_nextparser = &tunnel.pk_parser}
	, {.b_nextparser = &vlan.pk_parser}
	, {.b_nextparser = &agr.pk_parser}
#ifndef INET_ONLY
	, {.b_nextparser = &carp.pk_parser}
	, {.b_nextparser = &atalk.pk_parser}
	, {.b_nextparser = &iso.pk_parser}
#endif
	, {.b_nextparser = &kw80211.pk_parser}
	, {.b_nextparser = &address.pa_parser}
	, {.b_nextparser = &dstormask.pa_parser}
	, {.b_nextparser = &broadcast.pa_parser}
	, {.b_nextparser = NULL}
}, opt_command_brs[] = {
	  {.b_nextparser = &no_cmds.pt_parser}
	, {.b_nextparser = &command_root.pb_parser}
};

struct branch opt_family_only_brs[] = {
	  {.b_nextparser = &no_cmds.pt_parser}
	, {.b_nextparser = &family_only.pk_parser}
};
struct pbranch opt_family_only = PBRANCH_INITIALIZER(&opt_family_only,
    "opt-family-only", opt_family_only_brs,
    __arraycount(opt_family_only_brs), true);
struct pbranch opt_command = PBRANCH_INITIALIZER(&opt_command,
    "optional command",
    opt_command_brs, __arraycount(opt_command_brs), true);

struct pbranch command_root = PBRANCH_INITIALIZER(&command_root,
    "command-root", command_root_brs, __arraycount(command_root_brs), true);

struct piface iface_opt_family_only =
    PIFACE_INITIALIZER(&iface_opt_family_only, "iface-opt-family-only",
    NULL, "if", &opt_family_only.pb_parser);

struct pkw family = PKW_INITIALIZER(&family, "family", NULL, "af",
    familykw, __arraycount(familykw), &opt_command.pb_parser);

struct pkw silent_family = PKW_INITIALIZER(&silent_family, "silent family",
    NULL, "af", familykw, __arraycount(familykw), &command_root.pb_parser);

struct pkw ifcaps = PKW_INITIALIZER(&ifcaps, "ifcaps", setifcaps,
    "ifcap", ifcapskw, __arraycount(ifcapskw), &command_root.pb_parser);

struct pkw ifflags = PKW_INITIALIZER(&ifflags, "ifflags", setifflags,
    "ifflag", ifflagskw, __arraycount(ifflagskw), &command_root.pb_parser);

struct pkw cloning = PKW_INITIALIZER(&cloning, "cloning", clone_command,
    "clonecmd", clonekw, __arraycount(clonekw), NULL);

struct pkw misc = PKW_INITIALIZER(&misc, "misc", NULL, NULL,
    misckw, __arraycount(misckw), NULL);

struct pbranch opt_clone = PBRANCH_INITIALIZER(&opt_clone,
    "opt-clone", opt_clone_brs, __arraycount(opt_clone_brs), true);

struct pbranch opt_silent_family = PBRANCH_INITIALIZER(&opt_silent_family,
    "optional silent family", opt_silent_family_brs,
    __arraycount(opt_silent_family_brs), true);

struct pbranch opt_family = PBRANCH_INITIALIZER(&opt_family,
    "opt-family", opt_family_brs, __arraycount(opt_family_brs), true);

struct piface iface_start = PIFACE_INITIALIZER(&iface_start,
    "iface-opt-family", NULL, "if", &opt_clone.pb_parser);

struct piface iface_only = PIFACE_INITIALIZER(&iface_only, "iface",
    media_status_exec, "if", NULL);

static struct parser *
init_parser(void)
{
	if (parser_init(&iface_opt_family_only.pif_parser) == -1)
		err(EXIT_FAILURE, "parser_init(iface_opt_family_only)");
	if (parser_init(&iface_only.pif_parser) == -1)
		err(EXIT_FAILURE, "parser_init(iface_only)");
	if (parser_init(&iface_start.pif_parser) == -1)
		err(EXIT_FAILURE, "parser_init(iface_start)");

	return &iface_start.pif_parser;
}

static int
no_cmds_exec(prop_dictionary_t env, prop_dictionary_t xenv)
{
	const char *ifname;
	unsigned short ignore;

	/* ifname == NULL is ok.  It indicates 'ifconfig -a'. */
	if ((ifname = getifname(env)) == NULL)
		;
	else if (getifflags(env, xenv, &ignore) == -1)
		err(EXIT_FAILURE, "SIOCGIFFLAGS %s", ifname);

	printall(ifname, env);
	exit(EXIT_SUCCESS);
}

static int
media_status_exec(prop_dictionary_t env, prop_dictionary_t xenv)
{
	const char *ifname;
	unsigned short ignore;

	/* ifname == NULL is ok.  It indicates 'ifconfig -a'. */
	if ((ifname = getifname(env)) == NULL)
		;
	else if (getifflags(env, xenv, &ignore) == -1)
		err(EXIT_FAILURE, "SIOCGIFFLAGS %s", ifname);

	exit(carrier(env));
}

static void
do_setifcaps(prop_dictionary_t env)
{
	struct ifcapreq ifcr;
	prop_data_t d;

	d = (prop_data_t )prop_dictionary_get(env, "ifcaps");
	if (d == NULL)
		return;

	assert(sizeof(ifcr) == prop_data_size(d));

	memcpy(&ifcr, prop_data_data_nocopy(d), sizeof(ifcr));
	if (direct_ioctl(env, SIOCSIFCAP, &ifcr) == -1)
		err(EXIT_FAILURE, "SIOCSIFCAP");
}

int
main(int argc, char **argv)
{
	const struct afswtch *afp;
	int af, aflag = 0, Cflag = 0, s;
	struct match match[32];
	size_t nmatch;
	struct parser *start;
	int ch, narg = 0, rc;
	prop_dictionary_t env, xenv;
	const char *ifname;

	memset(match, 0, sizeof(match));

	start = init_parser();

	/* Parse command-line options */
	aflag = mflag = vflag = zflag = 0;
	while ((ch = getopt(argc, argv, "AabCdhlmsuvz"
#ifdef INET6
					"L"
#endif
			)) != -1) {
		switch (ch) {
		case 'A':
			warnx("-A is deprecated");
			break;

		case 'a':
			aflag = 1;
			break;

		case 'b':
			bflag = 1;
			break;
			
		case 'C':
			Cflag = 1;
			break;

		case 'd':
			dflag = 1;
			break;
		case 'h':
			hflag = 1;
			break;
#ifdef INET6
		case 'L':
			Lflag = 1;
			break;
#endif

		case 'l':
			lflag = 1;
			break;

		case 'm':
			mflag = 1;
			break;

		case 's':
			sflag = 1;
			break;

		case 'u':
			uflag = 1;
			break;

		case 'v':
			vflag = 1;
			break;

		case 'z':
			zflag = 1;
			break;

			
		default:
			usage();
			/* NOTREACHED */
		}
		switch (ch) {
		case 'a':
			start = &opt_family_only.pb_parser;
			break;

#ifdef INET6
		case 'L':
#endif
		case 'm':
		case 'v':
		case 'z':
			if (start != &opt_family_only.pb_parser)
				start = &iface_opt_family_only.pif_parser;
			break;
		case 'C':
			start = &cloneterm.pt_parser;
			break;
		case 'l':
			start = &no_cmds.pt_parser;
			break;
		case 's':
			if (start != &no_cmds.pt_parser &&
			    start != &opt_family_only.pb_parser)
				start = &iface_only.pif_parser;
			break;
		default:
			break;
		}
	}
	argc -= optind;
	argv += optind;

	/*
	 * -l means "list all interfaces", and is mutally exclusive with
	 * all other flags/commands.
	 *
	 * -C means "list all names of cloners", and it mutually exclusive
	 * with all other flags/commands.
	 *
	 * -a means "print status of all interfaces".
	 */
	if ((lflag || Cflag) && (aflag || mflag || vflag || zflag))
		usage();
#ifdef INET6
	if ((lflag || Cflag) && Lflag)
		usage();
#endif
	if (lflag && Cflag)
		usage();

	nmatch = __arraycount(match);

	rc = parse(argc, argv, start, match, &nmatch, &narg);
	if (rc != 0)
		usage();

	if ((xenv = prop_dictionary_create()) == NULL)
		err(EXIT_FAILURE, "%s: prop_dictionary_create", __func__);

	if (matches_exec(match, xenv, nmatch) == -1)
		err(EXIT_FAILURE, "exec_matches");

	argc -= narg;
	argv += narg;

	env = (nmatch > 0) ? match[(int)nmatch - 1].m_env : NULL;
	if (env == NULL)
		env = xenv;
	else
		env = prop_dictionary_augment(env, xenv);

	/* Process any media commands that may have been issued. */
	process_media_commands(env);

	af = getaf(env);
	switch (af) {
#ifndef INET_ONLY
	case AF_APPLETALK:
		checkatrange(&at_addreq.ifra_addr);
		break;
#endif	/* INET_ONLY */
	default:
		break;
	case -1:
		af = AF_INET;
		break;
	}

	if ((s = getsock(af)) == -1)
		err(EXIT_FAILURE, "%s: getsock", __func__);

	if ((ifname = getifname(env)) == NULL)
		err(EXIT_FAILURE, "%s: getifname", __func__);

	if ((afp = lookup_af_bynum(af)) == NULL)
		errx(EXIT_FAILURE, "%s: lookup_af_bynum", __func__);

	if (afp->af_addr_commit != NULL) {
		(*afp->af_addr_commit)(env, xenv);
	} else {
		if (clearaddr) {
			estrlcpy(afp->af_ridreq, ifname, IFNAMSIZ);
			if (ioctl(s, afp->af_difaddr, afp->af_ridreq) == -1)
				err(EXIT_FAILURE, "SIOCDIFADDR");
		}
		if (newaddr > 0) {
			estrlcpy(afp->af_addreq, ifname, IFNAMSIZ);
			if (ioctl(s, afp->af_aifaddr, afp->af_addreq) == -1)
				warn("SIOCAIFADDR");
			else if (check_up_state < 0)
				check_up_state = 1;
		}
	}

	do_setifpreference(env);
	do_setifcaps(env);

	if (check_up_state == 1)
		check_ifflags_up(env);

	exit(EXIT_SUCCESS);
}

const struct afswtch *
lookup_af_bynum(int afnum)
{
	const struct afswtch *a;

	for (a = afs; a->af_name != NULL; a++)
		if (a->af_af == afnum)
			return (a);
	return (NULL);
}

void
printall(const char *ifname, prop_dictionary_t env0)
{
	struct ifaddrs *ifap, *ifa;
	struct ifreq ifr;
	const struct sockaddr_dl *sdl = NULL;
	prop_dictionary_t env, oenv;
	int idx;
	char *p;

	if (env0 == NULL)
		env = prop_dictionary_create();
	else
		env = prop_dictionary_copy_mutable(env0);

	oenv = prop_dictionary_create();

	if (env == NULL || oenv == NULL)
		errx(EXIT_FAILURE, "%s: prop_dictionary_copy/create", __func__);

	if (getifaddrs(&ifap) != 0)
		err(EXIT_FAILURE, "getifaddrs");
	p = NULL;
	idx = 0;
	for (ifa = ifap; ifa; ifa = ifa->ifa_next) {
		memset(&ifr, 0, sizeof(ifr));
		estrlcpy(ifr.ifr_name, ifa->ifa_name, sizeof(ifr.ifr_name));
		if (sizeof(ifr.ifr_addr) >= ifa->ifa_addr->sa_len) {
			memcpy(&ifr.ifr_addr, ifa->ifa_addr,
			    ifa->ifa_addr->sa_len);
		}

		if (ifname != NULL && strcmp(ifname, ifa->ifa_name) != 0)
			continue;
		if (ifa->ifa_addr->sa_family == AF_LINK)
			sdl = (const struct sockaddr_dl *) ifa->ifa_addr;
		if (p && strcmp(p, ifa->ifa_name) == 0)
			continue;
		if (!prop_dictionary_set_cstring(env, "if", ifa->ifa_name))
			continue;
		p = ifa->ifa_name;

		if (bflag && (ifa->ifa_flags & IFF_BROADCAST) == 0)
			continue;
		if (dflag && (ifa->ifa_flags & IFF_UP) != 0)
			continue;
		if (uflag && (ifa->ifa_flags & IFF_UP) == 0)
			continue;

		if (sflag && carrier(env))
			continue;
		idx++;
		/*
		 * Are we just listing the interfaces?
		 */
		if (lflag) {
			if (idx > 1)
				printf(" ");
			fputs(ifa->ifa_name, stdout);
			continue;
		}

		status(sdl, env, oenv);
		sdl = NULL;
	}
	if (lflag)
		printf("\n");
	prop_object_release((prop_object_t)env);
	prop_object_release((prop_object_t)oenv);
	freeifaddrs(ifap);
}

int
list_cloners(prop_dictionary_t env, prop_dictionary_t oenv)
{
	struct if_clonereq ifcr;
	char *cp, *buf;
	int idx, s;

	memset(&ifcr, 0, sizeof(ifcr));

	s = getsock(AF_INET);

	if (ioctl(s, SIOCIFGCLONERS, &ifcr) == -1)
		err(EXIT_FAILURE, "SIOCIFGCLONERS for count");

	buf = malloc(ifcr.ifcr_total * IFNAMSIZ);
	if (buf == NULL)
		err(EXIT_FAILURE, "unable to allocate cloner name buffer");

	ifcr.ifcr_count = ifcr.ifcr_total;
	ifcr.ifcr_buffer = buf;

	if (ioctl(s, SIOCIFGCLONERS, &ifcr) == -1)
		err(EXIT_FAILURE, "SIOCIFGCLONERS for names");

	/*
	 * In case some disappeared in the mean time, clamp it down.
	 */
	if (ifcr.ifcr_count > ifcr.ifcr_total)
		ifcr.ifcr_count = ifcr.ifcr_total;

	for (cp = buf, idx = 0; idx < ifcr.ifcr_count; idx++, cp += IFNAMSIZ) {
		if (idx > 0)
			printf(" ");
		printf("%s", cp);
	}

	printf("\n");
	free(buf);
	exit(EXIT_SUCCESS);
}

static int
clone_command(prop_dictionary_t env, prop_dictionary_t xenv)
{
	int64_t cmd;

	if (!prop_dictionary_get_int64(env, "clonecmd", &cmd)) {
		errno = ENOENT;
		return -1;
	}

	if (indirect_ioctl(env, (unsigned long)cmd, NULL) == -1) {
		warn("%s", __func__);
		return -1;
	}
	return 0;
}

/*ARGSUSED*/
int
setifaddr(prop_dictionary_t env, prop_dictionary_t xenv)
{
	struct ifreq *ifr;		/* XXX */
	const struct paddr_prefix *pfx0;
	struct paddr_prefix *pfx;
	prop_data_t d;
	const char *ifname;
	int af, s;
	const struct afswtch *afp;

	if ((af = getaf(env)) == -1)
		af = AF_INET;

	if ((afp = lookup_af_bynum(af)) == NULL)
		return -1;

	d = (prop_data_t)prop_dictionary_get(env, "address");
	assert(d != NULL);
	pfx0 = prop_data_data_nocopy(d);

	if (pfx0->pfx_len >= 0) {
		pfx = prefixlen_to_mask(af, pfx0->pfx_len);
		if (pfx == NULL)
			err(EXIT_FAILURE, "prefixlen_to_mask");

		if (afp->af_getaddr != NULL)
			(*afp->af_getaddr)(pfx, MASK);
		free(pfx);
	}

	if (afp->af_addr_commit != NULL)
		return 0;

	if ((ifname = getifname(env)) == NULL)
		return -1;

	/*
	 * Delay the ioctl to set the interface addr until flags are all set.
	 * The address interpretation may depend on the flags,
	 * and the flags may change when the address is set.
	 */
	setaddr++;
	if (newaddr == -1)
		newaddr = 1;
	if (doalias == 0 && afp->af_gifaddr != 0) {
		ifr = (struct ifreq *)afp->af_ridreq;
		estrlcpy(ifr->ifr_name, ifname, sizeof(ifr->ifr_name));
		ifr->ifr_addr.sa_family = af;

		if ((s = getsock(af)) == -1)
			err(EXIT_FAILURE, "getsock");

		if (ioctl(s, afp->af_gifaddr, afp->af_ridreq) == 0)
			clearaddr = 1;
		else if (errno == EADDRNOTAVAIL)
			/* No address was assigned yet. */
			;
		else
			err(EXIT_FAILURE, "SIOCGIFADDR");
	}

#if 0
	int i;
	for (i = 0; i < pfx0->pfx_addr.sa_len; i++)
		printf(" %02x", ((const uint8_t *)&pfx->pfx_addr)[i]);
	printf("\n");
#endif
	if (afp->af_getaddr != NULL)
		(*afp->af_getaddr)(pfx0, (doalias >= 0 ? ADDR : RIDADDR));
	return 0;
}

int
setifnetmask(prop_dictionary_t env, prop_dictionary_t xenv)
{
	const struct paddr_prefix *pfx;
	prop_data_t d;
	int af;
	const struct afswtch *afp;

	if ((af = getaf(env)) == -1)
		af = AF_INET;

	if ((afp = lookup_af_bynum(af)) == NULL)
		return -1;

	d = (prop_data_t)prop_dictionary_get(env, "dstormask");
	assert(d != NULL);
	pfx = prop_data_data_nocopy(d);

	if (!prop_dictionary_set(xenv, "netmask", (prop_object_t)d))
		return -1;

	if (afp->af_getaddr != NULL)
		(*afp->af_getaddr)(pfx, MASK);
	return 0;
}

int
setifbroadaddr(prop_dictionary_t env, prop_dictionary_t xenv)
{
	const struct paddr_prefix *pfx;
	prop_data_t d;
	int af;
	const struct afswtch *afp;
	unsigned short flags;

	if ((af = getaf(env)) == -1)
		af = AF_INET;

	if ((afp = lookup_af_bynum(af)) == NULL)
		return -1;

	if (getifflags(env, xenv, &flags) == -1)
		err(EXIT_FAILURE, "%s: getifflags", __func__);

	if ((flags & IFF_BROADCAST) == 0)
		errx(EXIT_FAILURE, "not a broadcast interface");

	d = (prop_data_t)prop_dictionary_get(env, "broadcast");
	assert(d != NULL);
	pfx = prop_data_data_nocopy(d);

	if (!prop_dictionary_set(xenv, "broadcast", (prop_object_t)d))
		return -1;

	if (afp->af_getaddr != NULL)
		(*afp->af_getaddr)(pfx, DSTADDR);

	return 0;
}

#define rqtosa(__afp, __x) (&(((struct ifreq *)(__afp->__x))->ifr_addr))

int
notealias(prop_dictionary_t env, prop_dictionary_t xenv)
{
	bool alias, delete;
	int af;
	const struct afswtch *afp;

	if ((af = getaf(env)) == -1)
		af = AF_INET;

	if ((afp = lookup_af_bynum(af)) == NULL)
		return -1;

	if (afp->af_addr_commit != NULL)
		return 0;

	if (!prop_dictionary_get_bool(env, "alias", &alias)) {
		errno = ENOENT;
		return -1;
	}
	delete = !alias;
	if (setaddr && doalias == 0 && delete)
		memcpy(rqtosa(afp, af_ridreq), rqtosa(afp, af_addreq),
		    rqtosa(afp, af_addreq)->sa_len);
	doalias = delete ? -1 : 1;
	if (delete) {
		clearaddr = 1;
		newaddr = 0;
	} else {
		clearaddr = 0;
	}
	return 0;
}

/*ARGSUSED*/
int
notrailers(prop_dictionary_t env, prop_dictionary_t xenv)
{
	puts("Note: trailers are no longer sent, but always received");
	return 0;
}

/*ARGSUSED*/
int
setifdstormask(prop_dictionary_t env, prop_dictionary_t xenv)
{
	const char *key;
	const struct paddr_prefix *pfx;
	prop_data_t d;
	int af, which;
	const struct afswtch *afp;
	unsigned short flags;

	if ((af = getaf(env)) == -1)
		af = AF_INET;

	if ((afp = lookup_af_bynum(af)) == NULL)
		return -1;

	if (getifflags(env, xenv, &flags) == -1)
		err(EXIT_FAILURE, "%s: getifflags", __func__);

	d = (prop_data_t)prop_dictionary_get(env, "dstormask");
	assert(d != NULL);
	pfx = prop_data_data_nocopy(d);

	if ((flags & IFF_BROADCAST) == 0) {
		key = "dst";
		which = DSTADDR;
	} else {
		key = "netmask";
		which = MASK;
	}

	if (!prop_dictionary_set(xenv, key, (prop_object_t)d))
		return -1;

	if (afp->af_getaddr != NULL)
		(*afp->af_getaddr)(pfx, which);
	return 0;
}

void
check_ifflags_up(prop_dictionary_t env)
{
	struct ifreq ifr;

 	if (direct_ioctl(env, SIOCGIFFLAGS, &ifr) == -1)
		err(EXIT_FAILURE, "SIOCGIFFLAGS");
	if (ifr.ifr_flags & IFF_UP)
		return;
	ifr.ifr_flags |= IFF_UP;
	if (direct_ioctl(env, SIOCSIFFLAGS, &ifr) == -1)
		err(EXIT_FAILURE, "SIOCSIFFLAGS");
}

int
setifflags(prop_dictionary_t env, prop_dictionary_t xenv)
{
	struct ifreq ifr;
	int64_t ifflag;
	bool rc;

	rc = prop_dictionary_get_int64(env, "ifflag", &ifflag);
	assert(rc);

 	if (direct_ioctl(env, SIOCGIFFLAGS, &ifr) == -1)
		return -1;

	if (ifflag < 0) {
		ifflag = -ifflag;
		if (ifflag == IFF_UP)
			check_up_state = 0;
		ifr.ifr_flags &= ~ifflag;
	} else
		ifr.ifr_flags |= ifflag;

	if (direct_ioctl(env, SIOCSIFFLAGS, &ifr) == -1)
		return -1;

	return 0; 
}

static int
getifcaps(prop_dictionary_t env, prop_dictionary_t oenv, struct ifcapreq *oifcr)
{
	bool rc;
	struct ifcapreq ifcr;
	const struct ifcapreq *tmpifcr;
	prop_data_t capdata;

	capdata = (prop_data_t)prop_dictionary_get(env, "ifcaps");

	if (capdata != NULL) {
		tmpifcr = prop_data_data_nocopy(capdata);
		*oifcr = *tmpifcr;
		return 0;
	}

	(void)direct_ioctl(env, SIOCGIFCAP, &ifcr);
	*oifcr = ifcr;

	capdata = prop_data_create_data(&ifcr, sizeof(ifcr));

	rc = prop_dictionary_set(oenv, "ifcaps", capdata);

	prop_object_release((prop_object_t)capdata);

	return rc ? 0 : -1;
}

static int
setifcaps(prop_dictionary_t env, prop_dictionary_t oenv)
{
	int64_t ifcap;
	int s;
	bool rc;
	prop_data_t capdata;
	const char *ifname;
	struct ifcapreq ifcr;

	s = getsock(AF_INET);

	rc = prop_dictionary_get_int64(env, "ifcap", &ifcap);
	assert(rc);

	if ((ifname = getifname(env)) == NULL)
		return -1;

	if (getifcaps(env, oenv, &ifcr) == -1)
		return -1;

	if (ifcap < 0) {
		ifcap = -ifcap;
		ifcr.ifcr_capenable &= ~ifcap;
	} else
		ifcr.ifcr_capenable |= ifcap;

	if ((capdata = prop_data_create_data(&ifcr, sizeof(ifcr))) == NULL)
		return -1;

	rc = prop_dictionary_set(oenv, "ifcaps", capdata);
	prop_object_release((prop_object_t)capdata);

	return rc ? 0 : -1;
}

static int
setifmetric(prop_dictionary_t env, prop_dictionary_t xenv)
{
	struct ifreq ifr;
	bool rc;
	int64_t metric;

	rc = prop_dictionary_get_int64(env, "metric", &metric);
	assert(rc);

	ifr.ifr_metric = metric;
	if (direct_ioctl(env, SIOCSIFMETRIC, &ifr) == -1)
		warn("SIOCSIFMETRIC");
	return 0;
}

static void
do_setifpreference(prop_dictionary_t env)
{
	struct if_addrprefreq ifap;
	int af;
	const struct afswtch *afp;
	prop_data_t d;
	const struct paddr_prefix *pfx;

	if ((af = getaf(env)) == -1)
		af = AF_INET;

	if ((afp = lookup_af_bynum(af)) == NULL)
		errx(EXIT_FAILURE, "%s: lookup_af_bynum", __func__);

	memset(&ifap, 0, sizeof(ifap));

	if (!prop_dictionary_get_int16(env, "preference",
	    &ifap.ifap_preference))
		return; 

	d = (prop_data_t)prop_dictionary_get(env, "address");
	assert(d != NULL);

	pfx = prop_data_data_nocopy(d);

	memcpy(&ifap.ifap_addr, &pfx->pfx_addr,
	    MIN(sizeof(ifap.ifap_addr), pfx->pfx_addr.sa_len));
	if (direct_ioctl(env, SIOCSIFADDRPREF, &ifap) == -1)
		warn("SIOCSIFADDRPREF");
}

static int
setifmtu(prop_dictionary_t env, prop_dictionary_t xenv)
{
	int64_t mtu;
	bool rc;
	struct ifreq ifr;

	rc = prop_dictionary_get_int64(env, "mtu", &mtu);
	assert(rc);

	ifr.ifr_mtu = mtu;
	if (direct_ioctl(env, SIOCSIFMTU, &ifr) == -1)
		warn("SIOCSIFMTU");

	return 0;
}

static void
media_error(int type, const char *val, const char *opt)
{
	errx(EXIT_FAILURE, "unknown %s media %s: %s",
		get_media_type_string(type), opt, val);
}

void
init_current_media(prop_dictionary_t env, prop_dictionary_t oenv)
{
	const char *ifname;
	struct ifmediareq ifmr;

	if ((ifname = getifname(env)) == NULL)
		err(EXIT_FAILURE, "getifname");

	/*
	 * If we have not yet done so, grab the currently-selected
	 * media.
	 */

	if (prop_dictionary_get(env, "initmedia") == NULL) {
		memset(&ifmr, 0, sizeof(ifmr));

		if (direct_ioctl(env, SIOCGIFMEDIA, &ifmr) == -1) {
			/*
			 * If we get E2BIG, the kernel is telling us
			 * that there are more, so we can ignore it.
			 */
			if (errno != E2BIG)
				err(EXIT_FAILURE, "SIOCGIFMEDIA");
		}

		if (!prop_dictionary_set_bool(oenv, "initmedia", true)) {
			err(EXIT_FAILURE, "%s: prop_dictionary_set_bool",
			    __func__);
		}
		media_current = ifmr.ifm_current;
	}

	/* Sanity. */
	if (IFM_TYPE(media_current) == 0)
		errx(EXIT_FAILURE, "%s: no link type?", ifname);
}

void
process_media_commands(prop_dictionary_t env)
{
	struct ifreq ifr;

	if (prop_dictionary_get(env, "media") == NULL &&
	    prop_dictionary_get(env, "mediaopt") == NULL &&
	    prop_dictionary_get(env, "unmediaopt") == NULL &&
	    prop_dictionary_get(env, "mediamode") == NULL) {
		/* Nothing to do. */
		return;
	}

	/*
	 * Media already set up, and commands sanity-checked.  Set/clear
	 * any options, and we're ready to go.
	 */
	media_current |= mediaopt_set;
	media_current &= ~mediaopt_clear;

	memset(&ifr, 0, sizeof(ifr));
	ifr.ifr_media = media_current;

	if (direct_ioctl(env, SIOCSIFMEDIA, &ifr) == -1)
		err(EXIT_FAILURE, "SIOCSIFMEDIA");
}

int
setmedia(prop_dictionary_t env, prop_dictionary_t xenv)
{
	int type, subtype, inst;
	prop_data_t data;
	char *val;

	init_current_media(env, xenv);

	data = (prop_data_t)prop_dictionary_get(env, "media");
	assert(data != NULL);

	/* Only one media command may be given. */
	/* Must not come after mode commands */
	/* Must not come after mediaopt commands */

	/*
	 * No need to check if `instance' has been issued; setmediainst()
	 * craps out if `media' has not been specified.
	 */

	type = IFM_TYPE(media_current);
	inst = IFM_INST(media_current);

	val = strndup(prop_data_data_nocopy(data), prop_data_size(data));
	if (val == NULL)
		return -1;

	/* Look up the subtype. */
	subtype = get_media_subtype(type, val);
	if (subtype == -1)
		media_error(type, val, "subtype");

	/* Build the new current media word. */
	media_current = IFM_MAKEWORD(type, subtype, 0, inst);

	/* Media will be set after other processing is complete. */
	return 0;
}

int
setmediaopt(prop_dictionary_t env, prop_dictionary_t xenv)
{
	char *invalid;
	prop_data_t data;
	char *val;

	init_current_media(env, xenv);

	data = (prop_data_t)prop_dictionary_get(env, "mediaopt");
	assert(data != NULL);

	/* Can only issue `mediaopt' once. */
	/* Can't issue `mediaopt' if `instance' has already been issued. */

	val = strndup(prop_data_data_nocopy(data), prop_data_size(data));
	if (val == NULL)
		return -1;

	mediaopt_set = get_media_options(media_current, val, &invalid);
	free(val);
	if (mediaopt_set == -1)
		media_error(media_current, invalid, "option");

	/* Media will be set after other processing is complete. */
	return 0;
}

int
unsetmediaopt(prop_dictionary_t env, prop_dictionary_t xenv)
{
	char *invalid, *val;
	prop_data_t data;

	init_current_media(env, xenv);

	data = (prop_data_t)prop_dictionary_get(env, "unmediaopt");
	if (data == NULL) {
		errno = ENOENT;
		return -1;
	}

	val = strndup(prop_data_data_nocopy(data), prop_data_size(data));
	if (val == NULL)
		return -1;

	/*
	 * No need to check for A_MEDIAINST, since the test for A_MEDIA
	 * implicitly checks for A_MEDIAINST.
	 */

	mediaopt_clear = get_media_options(media_current, val, &invalid);
	free(val);
	if (mediaopt_clear == -1)
		media_error(media_current, invalid, "option");

	/* Media will be set after other processing is complete. */
	return 0;
}

int
setmediainst(prop_dictionary_t env, prop_dictionary_t xenv)
{
	int type, subtype, options;
	int64_t inst;
	bool rc;

	init_current_media(env, xenv);

	rc = prop_dictionary_get_int64(env, "mediainst", &inst);
	assert(rc);

	/* Can only issue `instance' once. */
	/* Must have already specified `media' */

	type = IFM_TYPE(media_current);
	subtype = IFM_SUBTYPE(media_current);
	options = IFM_OPTIONS(media_current);

	media_current = IFM_MAKEWORD(type, subtype, options, inst);

	/* Media will be set after other processing is complete. */
	return 0;
}

int
setmediamode(prop_dictionary_t env, prop_dictionary_t xenv)
{
	int type, subtype, options, inst, mode;
	prop_data_t data;
	char *val;

	init_current_media(env, xenv);

	data = (prop_data_t)prop_dictionary_get(env, "mediamode");
	assert(data != NULL);

	type = IFM_TYPE(media_current);
	subtype = IFM_SUBTYPE(media_current);
	options = IFM_OPTIONS(media_current);
	inst = IFM_INST(media_current);

	val = strndup(prop_data_data_nocopy(data), prop_data_size(data));
	if (val == NULL)
		return -1;

	mode = get_media_mode(type, val);
	if (mode == -1)
		media_error(type, val, "mode");

	free(val);

	media_current = IFM_MAKEWORD(type, subtype, options, inst) | mode;

	/* Media will be set after other processing is complete. */
	return 0;
}

void
print_media_word(int ifmw, const char *opt_sep)
{
	const char *str;

	printf("%s", get_media_subtype_string(ifmw));

	/* Find mode. */
	if (IFM_MODE(ifmw) != 0) {
		str = get_media_mode_string(ifmw);
		if (str != NULL)
			printf(" mode %s", str);
	}

	/* Find options. */
	for (; (str = get_media_option_string(&ifmw)) != NULL; opt_sep = ",")
		printf("%s%s", opt_sep, str);

	if (IFM_INST(ifmw) != 0)
		printf(" instance %d", IFM_INST(ifmw));
}

int
carrier(prop_dictionary_t env)
{
	struct ifmediareq ifmr;

	memset(&ifmr, 0, sizeof(ifmr));

	if (direct_ioctl(env, SIOCGIFMEDIA, &ifmr) == -1) {
		/*
		 * Interface doesn't support SIOC{G,S}IFMEDIA;
		 * assume ok.
		 */
		return EXIT_SUCCESS;
	}
	if ((ifmr.ifm_status & IFM_AVALID) == 0) {
		/*
		 * Interface doesn't report media-valid status.
		 * assume ok.
		 */
		return EXIT_SUCCESS;
	}
	/* otherwise, return ok for active, not-ok if not active. */
	if (ifmr.ifm_status & IFM_ACTIVE)
		return EXIT_SUCCESS;
	else
		return EXIT_FAILURE;
}

const int ifm_status_valid_list[] = IFM_STATUS_VALID_LIST;

const struct ifmedia_status_description ifm_status_descriptions[] =
    IFM_STATUS_DESCRIPTIONS;

/*
 * Print the status of the interface.  If an address family was
 * specified, show it and it only; otherwise, show them all.
 */
void
status(const struct sockaddr_dl *sdl, prop_dictionary_t env,
    prop_dictionary_t oenv)
{
	struct ifmediareq ifmr;
	struct ifdatareq ifdr;
	struct ifreq ifr;
	int *media_list, i;
	char hbuf[NI_MAXHOST];
	char fbuf[BUFSIZ];
	int af, s;
	const char *ifname;
	struct ifcapreq ifcr;
	unsigned short flags;
	const struct afswtch *afp;

	if ((af = getaf(env)) == -1) {
		afp = NULL;
		af = AF_UNSPEC;
	} else
		afp = lookup_af_bynum(af);

	/* get out early if the family is unsupported by the kernel */
	if ((s = getsock(af)) == -1)
		err(EXIT_FAILURE, "%s: getsock", __func__);

	if ((ifname = getifinfo(env, oenv, &flags)) == NULL)
		err(EXIT_FAILURE, "%s: getifinfo", __func__);

	(void)snprintb(fbuf, sizeof(fbuf), IFFBITS, flags);
	printf("%s: flags=%s", ifname, &fbuf[2]);

	estrlcpy(ifr.ifr_name, ifname, sizeof(ifr.ifr_name));
	if (ioctl(s, SIOCGIFMETRIC, &ifr) == -1)
		warn("SIOCGIFMETRIC %s", ifr.ifr_name);
	else if (ifr.ifr_metric != 0)
		printf(" metric %d", ifr.ifr_metric);

	estrlcpy(ifr.ifr_name, ifname, sizeof(ifr.ifr_name));
	if (ioctl(s, SIOCGIFMTU, &ifr) != -1 && ifr.ifr_mtu != 0)
		printf(" mtu %d", ifr.ifr_mtu);
	printf("\n");

	if (getifcaps(env, oenv, &ifcr) == -1)
		err(EXIT_FAILURE, "%s: getifcaps", __func__);

	if (ifcr.ifcr_capabilities != 0) {
		(void)snprintb(fbuf, sizeof(fbuf), IFCAPBITS,
		    ifcr.ifcr_capabilities);
		printf("\tcapabilities=%s\n", &fbuf[2]);
		(void)snprintb(fbuf, sizeof(fbuf), IFCAPBITS,
		    ifcr.ifcr_capenable);
		printf("\tenabled=%s\n", &fbuf[2]);
	}

	ieee80211_status(env, oenv);
	vlan_status(env, oenv);
#ifndef INET_ONLY
	carp_status(env, oenv);
#endif
	tunnel_status(env, oenv);
	agr_status(env, oenv);

	if (sdl != NULL &&
	    getnameinfo((const struct sockaddr *)sdl, sdl->sdl_len,
		hbuf, sizeof(hbuf), NULL, 0, NI_NUMERICHOST) == 0 &&
	    hbuf[0] != '\0')
		printf("\taddress: %s\n", hbuf);

	(void) memset(&ifmr, 0, sizeof(ifmr));
	estrlcpy(ifmr.ifm_name, ifname, sizeof(ifmr.ifm_name));

	if (ioctl(s, SIOCGIFMEDIA, &ifmr) == -1) {
		/*
		 * Interface doesn't support SIOC{G,S}IFMEDIA.
		 */
		goto iface_stats;
	}

	if (ifmr.ifm_count == 0) {
		warnx("%s: no media types?", ifname);
		goto iface_stats;
	}

	media_list = (int *)malloc(ifmr.ifm_count * sizeof(int));
	if (media_list == NULL)
		err(EXIT_FAILURE, "malloc");
	ifmr.ifm_ulist = media_list;

	if (ioctl(s, SIOCGIFMEDIA, &ifmr) == -1)
		err(EXIT_FAILURE, "SIOCGIFMEDIA");

	printf("\tmedia: %s ", get_media_type_string(ifmr.ifm_current));
	print_media_word(ifmr.ifm_current, " ");
	if (ifmr.ifm_active != ifmr.ifm_current) {
		printf(" (");
		print_media_word(ifmr.ifm_active, " ");
		printf(")");
	}
	printf("\n");

	if (ifmr.ifm_status & IFM_STATUS_VALID) {
		const struct ifmedia_status_description *ifms;
		int bitno, found = 0;

		printf("\tstatus: ");
		for (bitno = 0; ifm_status_valid_list[bitno] != 0; bitno++) {
			for (ifms = ifm_status_descriptions;
			     ifms->ifms_valid != 0; ifms++) {
				if (ifms->ifms_type !=
				      IFM_TYPE(ifmr.ifm_current) ||
				    ifms->ifms_valid !=
				      ifm_status_valid_list[bitno])
					continue;
				printf("%s%s", found ? ", " : "",
				    IFM_STATUS_DESC(ifms, ifmr.ifm_status));
				found = 1;

				/*
				 * For each valid indicator bit, there's
				 * only one entry for each media type, so
				 * terminate the inner loop now.
				 */
				break;
			}
		}

		if (found == 0)
			printf("unknown");
		printf("\n");
	}

	if (mflag) {
		int type, printed_type;

		for (type = IFM_NMIN; type <= IFM_NMAX; type += IFM_NMIN) {
			for (i = 0, printed_type = 0; i < ifmr.ifm_count; i++) {
				if (IFM_TYPE(media_list[i]) != type)
					continue;
				if (printed_type == 0) {
					printf("\tsupported %s media:\n",
					    get_media_type_string(type));
					printed_type = 1;
				}
				printf("\t\tmedia ");
				print_media_word(media_list[i], " mediaopt ");
				printf("\n");
			}
		}
	}

	free(media_list);

 iface_stats:
	if (!vflag && !zflag)
		goto proto_status;

	estrlcpy(ifdr.ifdr_name, ifname, sizeof(ifdr.ifdr_name));

	if (ioctl(s, zflag ? SIOCZIFDATA:SIOCGIFDATA, &ifdr) == -1) {
		err(EXIT_FAILURE, zflag ? "SIOCZIFDATA" : "SIOCGIFDATA");
	} else {
		struct if_data * const ifi = &ifdr.ifdr_data;
		char buf[5];

#define	PLURAL(n)	((n) == 1 ? "" : "s")
#define PLURALSTR(s)	((atof(s)) == 1.0 ? "" : "s")
		printf("\tinput: %llu packet%s, ", 
		    (unsigned long long) ifi->ifi_ipackets,
		    PLURAL(ifi->ifi_ipackets));
		if (hflag) {
			(void) humanize_number(buf, sizeof(buf),
			    (int64_t) ifi->ifi_ibytes, "", HN_AUTOSCALE, 
			    HN_NOSPACE | HN_DECIMAL);
			printf("%s byte%s", buf,
			    PLURALSTR(buf));
		} else
			printf("%llu byte%s",
			    (unsigned long long) ifi->ifi_ibytes,
		            PLURAL(ifi->ifi_ibytes));
		if (ifi->ifi_imcasts)
			printf(", %llu multicast%s",
			    (unsigned long long) ifi->ifi_imcasts,
			    PLURAL(ifi->ifi_imcasts));
		if (ifi->ifi_ierrors)
			printf(", %llu error%s",
			    (unsigned long long) ifi->ifi_ierrors,
			    PLURAL(ifi->ifi_ierrors));
		if (ifi->ifi_iqdrops)
			printf(", %llu queue drop%s",
			    (unsigned long long) ifi->ifi_iqdrops,
			    PLURAL(ifi->ifi_iqdrops));
		if (ifi->ifi_noproto)
			printf(", %llu unknown protocol",
			    (unsigned long long) ifi->ifi_noproto);
		printf("\n\toutput: %llu packet%s, ",
		    (unsigned long long) ifi->ifi_opackets,
		    PLURAL(ifi->ifi_opackets));
		if (hflag) {
			(void) humanize_number(buf, sizeof(buf),
			    (int64_t) ifi->ifi_obytes, "", HN_AUTOSCALE,
			    HN_NOSPACE | HN_DECIMAL);
			printf("%s byte%s", buf,
			    PLURALSTR(buf));
		} else
			printf("%llu byte%s",
			    (unsigned long long) ifi->ifi_obytes,
			    PLURAL(ifi->ifi_obytes));
		if (ifi->ifi_omcasts)
			printf(", %llu multicast%s",
			    (unsigned long long) ifi->ifi_omcasts,
			    PLURAL(ifi->ifi_omcasts));
		if (ifi->ifi_oerrors)
			printf(", %llu error%s",
			    (unsigned long long) ifi->ifi_oerrors,
			    PLURAL(ifi->ifi_oerrors));
		if (ifi->ifi_collisions)
			printf(", %llu collision%s",
			    (unsigned long long) ifi->ifi_collisions,
			    PLURAL(ifi->ifi_collisions));
		printf("\n");
#undef PLURAL
#undef PLURALSTR
	}

	ieee80211_statistics(env);

 proto_status:

	if (afp != NULL)
		(*afp->af_status)(env, oenv, true);
	else for (afp = afs; afp->af_name != NULL; afp++)
		(*afp->af_status)(env, oenv, false);
}

int
setifprefixlen(prop_dictionary_t env, prop_dictionary_t xenv)
{
	const char *ifname;
	bool rc;
	int64_t plen;
	int af;
	const struct afswtch *afp;
	unsigned short flags;
	struct paddr_prefix *pfx;
	prop_data_t d;

	if ((af = getaf(env)) == -1)
		af = AF_INET;

	if ((afp = lookup_af_bynum(af)) == NULL)
		return -1;

	if ((ifname = getifinfo(env, xenv, &flags)) == NULL)
		err(EXIT_FAILURE, "getifinfo");

	rc = prop_dictionary_get_int64(env, "prefixlen", &plen);
	assert(rc);

	pfx = prefixlen_to_mask(af, plen);
	if (pfx == NULL)
		err(EXIT_FAILURE, "prefixlen_to_mask");

	d = prop_data_create_data(pfx,
	    offsetof(struct paddr_prefix, pfx_addr) + pfx->pfx_addr.sa_len);
	if (d == NULL)
		err(EXIT_FAILURE, "%s: prop_data_create_data", __func__);

	if (!prop_dictionary_set(xenv, "netmask", (prop_object_t)d))
		err(EXIT_FAILURE, "%s: prop_dictionary_set", __func__);

	if (afp->af_getaddr != NULL)
		(*afp->af_getaddr)(pfx, MASK);

	free(pfx);
	return 0;
}

void
usage(void)
{
	const char *progname = getprogname();

	fprintf(stderr,
	    "usage: %s [-h] [-m] [-v] [-z] "
#ifdef INET6
		"[-L] "
#endif
		"interface\n"
		"\t[ af [ address [ dest_addr ] ] [ netmask mask ] [ prefixlen n ]\n"
		"\t\t[ alias | -alias ] ]\n"
		"\t[ up ] [ down ] [ metric n ] [ mtu n ]\n"
		"\t[ nwid network_id ] [ nwkey network_key | -nwkey ]\n"
		"\t[ list scan ]\n"
		"\t[ powersave | -powersave ] [ powersavesleep duration ]\n"
		"\t[ hidessid | -hidessid ] [ apbridge | -apbridge ]\n"
		"\t[ [ af ] tunnel src_addr dest_addr ] [ deletetunnel ]\n"
		"\t[ arp | -arp ]\n"
		"\t[ media type ] [ mediaopt opts ] [ -mediaopt opts ] "
		"[ instance minst ]\n"
		"\t[ preference n ]\n"
		"\t[ vlan n vlanif i ]\n"
		"\t[ agrport i ] [ -agrport i ]\n"
		"\t[ anycast | -anycast ] [ deprecated | -deprecated ]\n"
		"\t[ tentative | -tentative ] [ pltime n ] [ vltime n ] [ eui64 ]\n"
		"\t[ link0 | -link0 ] [ link1 | -link1 ] [ link2 | -link2 ]\n"
		"       %s -a [-b] [-h] [-m] [-d] [-u] [-v] [-z] [ af ]\n"
		"       %s -l [-b] [-d] [-u] [-s]\n"
		"       %s -C\n"
		"       %s interface create\n"
		"       %s interface destroy\n",
		progname, progname, progname, progname, progname, progname);
	exit(EXIT_FAILURE);
}
