/*	$NetBSD: brconfig.c,v 1.5 2003/03/19 10:34:33 bouyer Exp $	*/

/*
 * Copyright 2001 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Jason R. Thorpe for Wasabi Systems, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed for the NetBSD Project by
 *	Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * brconfig(8) --
 *
 *	Configuration utility for the bridge(4) driver.
 */

#include <sys/param.h>
#include <sys/socket.h>
#include <sys/ioctl.h>

#include <net/if.h>
#include <net/if_ether.h>
#include <net/if_bridgevar.h>

#include <ctype.h>
#include <err.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ifaddrs.h>

struct command {
	const char *cmd_keyword;
	int	cmd_argcnt;
	int	cmd_flags;
	void	(*cmd_func)(const struct command *, int, const char *,
		    char **);
};

#define	CMD_INVERT	0x01	/* "invert" the sense of the command */

void	cmd_add(const struct command *, int, const char *, char **);
void	cmd_delete(const struct command *, int, const char *, char **);
void	cmd_up(const struct command *, int, const char *, char **);
void	cmd_down(const struct command *, int, const char *, char **);
void	cmd_discover(const struct command *, int, const char *, char **);
void	cmd_learn(const struct command *, int, const char *, char **);
void	cmd_flush(const struct command *, int, const char *, char **);
void	cmd_flushall(const struct command *, int, const char *, char **);
void	cmd_static(const struct command *, int, const char *, char **);
void	cmd_deladdr(const struct command *, int, const char *, char **);
void	cmd_addr(const struct command *, int, const char *, char **);
void	cmd_maxaddr(const struct command *, int, const char *, char **);
void	cmd_hellotime(const struct command *, int, const char *, char **);
void	cmd_fwddelay(const struct command *, int, const char *, char **);
void	cmd_maxage(const struct command *, int, const char *, char **);
void	cmd_priority(const struct command *, int, const char *, char **);
void	cmd_ifpriority(const struct command *, int, const char *, char **);
void	cmd_ifpathcost(const struct command *, int, const char *, char **);
void	cmd_timeout(const struct command *, int, const char *, char **);
void	cmd_stp(const struct command *, int, const char *, char **);
void	cmd_ipf(const struct command *, int, const char *, char **);

const struct command command_table[] = {
	{ "add",		1,	0,		cmd_add },
	{ "delete",		1,	0,		cmd_delete },

	{ "up",			0,	0,		cmd_up },
	{ "down",		0,	0,		cmd_down },

	{ "discover",		1,	0,		cmd_discover },
	{ "-discover",		1,	CMD_INVERT,	cmd_discover },

	{ "learn",		1,	0,		cmd_learn },
	{ "-learn",		1,	CMD_INVERT,	cmd_learn },

	{ "flush",		0,	0,		cmd_flush },
	{ "flushall",		0,	0,		cmd_flushall },

	{ "static",		2,	0,		cmd_static },
	{ "deladdr",		1,	0,		cmd_deladdr },

	{ "addr",		0,	0,		cmd_addr },
	{ "maxaddr",		1,	0,		cmd_maxaddr },

	{ "hellotime",		1,	0,		cmd_hellotime },
	{ "fwddelay",		1,	0,		cmd_fwddelay },
	{ "maxage",		1,	0,		cmd_maxage },
	{ "priority",		1,	0,		cmd_priority },
	{ "ifpriority",		2,	0,		cmd_ifpriority },
	{ "ifpathcost",		2,	0,		cmd_ifpathcost },
	{ "timeout",		1,	0,		cmd_timeout },
	{ "stp",		1,	0,		cmd_stp },
	{ "-stp",		1,	CMD_INVERT,	cmd_stp },

        { "ipf",                0,      0,              cmd_ipf },
        { "-ipf",               0,      CMD_INVERT,     cmd_ipf },

	{ NULL,			0,	0,		NULL },
};

void	printall(int);
void	status(int, const char *);
int	is_bridge(const char *);
void	show_config(int, const char *, const char *);
void	show_interfaces(int, const char *, const char *);
void	show_addresses(int, const char *, const char *);
int	get_val(const char *, u_long *);
int	do_cmd(int, const char *, u_long, void *, size_t, int);
void	do_ifflag(int, const char *, int, int);
void	do_bridgeflag(int, const char *, const char *, int, int);

void	printb(const char *, u_int, const char *);

int	main(int, char *[]);
void	usage(void);

int	aflag;

struct ifreq g_ifr;
int	g_ifr_updated;

#define	IFFBITS \
"\020\1UP\2BROADCAST\3DEBUG\4LOOPBACK\5POINTOPOINT\6NOTRAILERS\7RUNNING\10NOARP\
\11PROMISC\12ALLMULTI\13OACTIVE\14SIMPLEX\15LINK0\16LINK1\17LINK2\20MULTICAST"

int
main(int argc, char *argv[])
{
	const struct command *cmd;
	char *bridge;
	int sock, ch;

	if (argc < 2)
		usage();

	sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (sock < 0)
		err(1, "socket");

	while ((ch = getopt(argc, argv, "a")) != -1) {
		switch (ch) {
		case 'a':
			aflag = 1;
			break;

		default:
			usage();
		}
	}

	argc -= optind;
	argv += optind;

	if (aflag) {
		if (argc != 0)
			usage();
		printall(sock);
		exit(0);
	}

	if (argc == 0)
		usage();

	bridge = argv[0];

	if (is_bridge(bridge) == 0)
		errx(1, "%s is not a bridge", bridge);

	/* Get a copy of the interface flags. */
	strlcpy(g_ifr.ifr_name, bridge, sizeof(g_ifr.ifr_name));
	if (ioctl(sock, SIOCGIFFLAGS, &g_ifr) < 0)
		err(1, "unable to get interface flags");

	argc--;
	argv++;

	if (argc == 0) {
		status(sock, bridge);
		exit(0);
	}

	while (argc != 0) {
		for (cmd = command_table; cmd->cmd_keyword != NULL; cmd++) {
			if (strcmp(cmd->cmd_keyword, argv[0]) == 0)
				break;
		}
		if (cmd->cmd_keyword == NULL)
			errx(1, "unknown command: %s", argv[0]);

		argc--;
		argv++;

		if (argc < cmd->cmd_argcnt)
			errx(1, "command %s requires %d argument%s",
			    cmd->cmd_keyword, cmd->cmd_argcnt,
			    cmd->cmd_argcnt == 1 ? "" : "s");

		(*cmd->cmd_func)(cmd, sock, bridge, argv);

		argc -= cmd->cmd_argcnt;
		argv += cmd->cmd_argcnt;
	}

	/* If the flags changed, update them. */
	if (g_ifr_updated && ioctl(sock, SIOCSIFFLAGS, &g_ifr) < 0)
		err(1, "unable to set interface flags");

	exit (0);
}

void
usage(void)
{
	static const char *usage_strings[] = {
		"-a",
		"<bridge>",
		"<bridge> up|down",
		"<bridge> addr",
		"<bridge> add <interface>",
		"<bridge> delete <interface>",
		"<bridge> maxaddr <size>",
		"<bridge> timeout <time>",
		"<bridge> static <interface> <address>",
		"<bridge> deladdr <address>",
		"<bridge> flush",
		"<bridge> flushall",
		"<bridge> ipf|-ipf",
		"<bridge> discover|-discover <interface>",
		"<bridge> learn|-learn <interface>",
		"<bridge> stp|-stp <interface>",
		"<bridge> maxage <time>",
		"<bridge> fwddelay <time>",
		"<bridge> hellotime <time>",
		"<bridge> priority <value>",
		"<bridge> ifpriority <interface> <value>",
		"<bridge> ifpathcost <interface> <value>",
		NULL,
	};
	extern const char *__progname;
	int i;

	for (i = 0; usage_strings[i] != NULL; i++)
		fprintf(stderr, "%s %s %s\n",
		    i == 0 ? "usage:" : "      ",
		    __progname, usage_strings[i]);

	exit(1);
}

int
is_bridge(const char *bridge)
{

	if (strncmp(bridge, "bridge", 6) != 0 ||
	    isdigit(bridge[6]) == 0)
		return (0);

	return (1);
}

void
printb(const char *s, u_int v, const char *bits)
{
	int i, any = 0;
	char c;

	if (bits && *bits == 8)
		printf("%s=%o", s, v);
	else
		printf("%s=%x", s, v);
	bits++;
	if (bits) { 
		putchar('<');
		while ((i = *bits++) != 0) {
			if (v & (1 << (i-1))) {
				if (any)
					putchar(',');
				any = 1;
				for (; (c = *bits) > 32; bits++)
					putchar(c);
			} else
				for (; *bits > 32; bits++)
					;
		}
		putchar('>');
	}
}

void
printall(int sock)
{
	struct ifaddrs *ifap, *ifa;
	char *p;

	if (getifaddrs(&ifap) != 0)
		err(1, "getifaddrs");
	p = NULL;
	for (ifa = ifap; ifa != NULL; ifa = ifa->ifa_next) {
		if (is_bridge(ifa->ifa_name) == 0)
			continue;
		if (p != NULL && strcmp(p, ifa->ifa_name) == 0)
			continue;
		p = ifa->ifa_name;
		status(sock, ifa->ifa_name);
	}

	freeifaddrs(ifap);
}

void
status(int sock, const char *bridge)
{
	struct ifreq ifr;
	struct ifbrparam bp1, bp2;

	memset(&ifr, 0, sizeof(ifr));

	strlcpy(ifr.ifr_name, bridge, sizeof(ifr.ifr_name));
	if (ioctl(sock, SIOCGIFFLAGS, &ifr) < 0)
		err(1, "unable to get flags");

	printf("%s: ", bridge);
	printb("flags", ifr.ifr_flags, IFFBITS);
	printf("\n");

	printf("\tConfiguration:\n");
	show_config(sock, bridge, "\t\t");

	printf("\tInterfaces:\n");
	show_interfaces(sock, bridge, "\t\t");

	if (do_cmd(sock, bridge, BRDGGCACHE, &bp1, sizeof(bp1), 0) < 0)
		err(1, "unable to get address cache size");
	if (do_cmd(sock, bridge, BRDGGTO, &bp2, sizeof(bp2), 0) < 0)
		err(1, "unable to get address timeout");

	printf("\tAddress cache (max cache: %u, timeout: %u):\n",
	    bp1.ifbrp_csize, bp2.ifbrp_ctime);
	show_addresses(sock, bridge, "\t\t");
}

void
show_config(int sock, const char *bridge, const char *prefix)
{
	struct ifbrparam param;
	u_int32_t ipfflags;
	u_int16_t pri;
	u_int8_t ht, fd, ma;

	if (do_cmd(sock, bridge, BRDGGPRI, &param, sizeof(param), 0) < 0)
		err(1, "unable to get bridge priority");
	pri = param.ifbrp_prio;

	if (do_cmd(sock, bridge, BRDGGHT, &param, sizeof(param), 0) < 0)
		err(1, "unable to get hellotime");
	ht = param.ifbrp_hellotime;

	if (do_cmd(sock, bridge, BRDGGFD, &param, sizeof(param), 0) < 0)
		err(1, "unable to get forward delay");
	fd = param.ifbrp_fwddelay;

	if (do_cmd(sock, bridge, BRDGGMA, &param, sizeof(param), 0) < 0)
		err(1, "unable to get max age");
	ma = param.ifbrp_maxage;

	printf("%spriority %u hellotime %u fwddelay %u maxage %u\n",
	    prefix, pri, ht, fd, ma);

	if (do_cmd(sock, bridge, BRDGGFILT, &param, sizeof(param), 0) < 0) {
		/* err(1, "unable to get ipfilter status"); */
		param.ifbrp_filter = 0;
	}

	ipfflags = param.ifbrp_filter;
	printf("%sipfilter %s flags 0x%x\n", prefix,
		(ipfflags & IFBF_FILT_USEIPF) ? "enabled" : "disabled",
		ipfflags);
}

void
show_interfaces(int sock, const char *bridge, const char *prefix)
{
	static const char *stpstates[] = {
		"disabled",
		"listening",
		"learning",
		"forwarding",
		"blocking",
	};
	struct ifbifconf bifc;
	struct ifbreq *req;
	char *inbuf = NULL;
	int i, len = 8192;

	for (;;) {
		bifc.ifbic_len = len;
		bifc.ifbic_buf = inbuf = realloc(inbuf, len);
		if (inbuf == NULL)
			err(1, "unable to allocate interface buffer");
		if (do_cmd(sock, bridge, BRDGGIFS, &bifc, sizeof(bifc), 0) < 0)
			err(1, "unable to get interface list");
		if ((bifc.ifbic_len + sizeof(*req)) < len)
			break;
		len *= 2;
	}

	for (i = 0; i < bifc.ifbic_len / sizeof(*req); i++) {
		req = bifc.ifbic_req + i;
		printf("%s%s ", prefix, req->ifbr_ifsname);
		printb("flags", req->ifbr_ifsflags, IFBIFBITS);
		printf("\n");
		printf("%s\t", prefix);
		printf("port %u priority %u",
		    req->ifbr_portno, req->ifbr_priority);
		if (req->ifbr_ifsflags & IFBIF_STP) {
			printf(" path cost %u", req->ifbr_path_cost);
			if (req->ifbr_state <
			    sizeof(stpstates) / sizeof(stpstates[0]))
				printf(" %s", stpstates[req->ifbr_state]);
			else
				printf(" <unknown state %d>",
				    req->ifbr_state);
		}
		printf("\n");
	}

	free(inbuf);
}

void
show_addresses(int sock, const char *bridge, const char *prefix)
{
	struct ifbaconf ifbac;
	struct ifbareq *ifba;
	char *inbuf = NULL;
	int i, len = 8192;
	struct ether_addr ea;

	for (;;) {
		ifbac.ifbac_len = len;
		ifbac.ifbac_buf = inbuf = realloc(inbuf, len);
		if (inbuf == NULL)
			err(1, "unable to allocate address buffer");
		if (do_cmd(sock, bridge, BRDGRTS, &ifbac, sizeof(ifbac), 0) < 0)
			err(1, "unable to get address cache");
		if ((ifbac.ifbac_len + sizeof(*ifba)) < len)
			break;
		len *= 2;
	}

	for (i = 0; i < ifbac.ifbac_len / sizeof(*ifba); i++) {
		ifba = ifbac.ifbac_req + i;
		memcpy(ea.ether_addr_octet, ifba->ifba_dst,
		    sizeof(ea.ether_addr_octet));
		printf("%s%s %s %lu ", prefix, ether_ntoa(&ea),
		    ifba->ifba_ifsname, ifba->ifba_expire);
		printb("flags", ifba->ifba_flags, IFBAFBITS);
		printf("\n");
	}

	free(inbuf);
}

int
get_val(const char *cp, u_long *valp)
{
	char *endptr;
	u_long val;

	errno = 0;
	val = strtoul(cp, &endptr, 0);
	if (cp[0] == '\0' || endptr[0] != '\0' || errno == ERANGE)
		return (-1);

	*valp = val;
	return (0);
}

int
do_cmd(int sock, const char *bridge, u_long op, void *arg, size_t argsize,
    int set)
{
	struct ifdrv ifd;

	memset(&ifd, 0, sizeof(ifd));

	strlcpy(ifd.ifd_name, bridge, sizeof(ifd.ifd_name));
	ifd.ifd_cmd = op;
	ifd.ifd_len = argsize;
	ifd.ifd_data = arg;

	return (ioctl(sock, set ? SIOCSDRVSPEC : SIOCGDRVSPEC, &ifd));
}

void
do_ifflag(int sock, const char *bridge, int flag, int set)
{

	if (set)
		g_ifr.ifr_flags |= flag;
	else
		g_ifr.ifr_flags &= ~flag;

	g_ifr_updated = 1;
}

void
do_bridgeflag(int sock, const char *bridge, const char *ifs, int flag,
    int set)
{
	struct ifbreq req;

	strlcpy(req.ifbr_ifsname, ifs, sizeof(req.ifbr_ifsname));

	if (do_cmd(sock, bridge, BRDGGIFFLGS, &req, sizeof(req), 0) < 0)
		err(1, "unable to get bridge flags");

	if (set)
		req.ifbr_ifsflags |= flag;
	else
		req.ifbr_ifsflags &= ~flag;

	if (do_cmd(sock, bridge, BRDGSIFFLGS, &req, sizeof(req), 1) < 0)
		err(1, "unable to set bridge flags");
}

void
cmd_add(const struct command *cmd, int sock, const char *bridge,
    char **argv)
{
	struct ifbreq req;

	memset(&req, 0, sizeof(req));

	strlcpy(req.ifbr_ifsname, argv[0], sizeof(req.ifbr_ifsname));
	if (do_cmd(sock, bridge, BRDGADD, &req, sizeof(req), 1) < 0)
		err(1, "%s %s", cmd->cmd_keyword, argv[0]);
}

void
cmd_delete(const struct command *cmd, int sock, const char *bridge,
    char **argv)
{
	struct ifbreq req;

	memset(&req, 0, sizeof(req));
	strlcpy(req.ifbr_ifsname, argv[0], sizeof(req.ifbr_ifsname));
	if (do_cmd(sock, bridge, BRDGDEL, &req, sizeof(req), 1) < 0)
		err(1, "%s %s", cmd->cmd_keyword, argv[0]);
}

void
cmd_up(const struct command *cmd, int sock, const char *bridge,
    char **argv)
{

	do_ifflag(sock, bridge, IFF_UP, 1);
}

void
cmd_down(const struct command *cmd, int sock, const char *bridge,
    char **argv)
{

	do_ifflag(sock, bridge, IFF_UP, 0);
}

void
cmd_discover(const struct command *cmd, int sock, const char *bridge,
    char **argv)
{

	do_bridgeflag(sock, bridge, argv[0], IFBIF_DISCOVER,
	    (cmd->cmd_flags & CMD_INVERT) ? 0 : 1);
}

void
cmd_learn(const struct command *cmd, int sock, const char *bridge,
    char **argv)
{

	do_bridgeflag(sock, bridge, argv[0], IFBIF_LEARNING,
	    (cmd->cmd_flags & CMD_INVERT) ? 0 : 1);
}

void
cmd_stp(const struct command *cmd, int sock, const char *bridge,
    char **argv)
{

	do_bridgeflag(sock, bridge, argv[0], IFBIF_STP,
	    (cmd->cmd_flags & CMD_INVERT) ? 0 : 1);
}

void
cmd_flush(const struct command *cmd, int sock, const char *bridge,
    char **argv)
{
	struct ifbreq req;

	memset(&req, 0, sizeof(req));
	req.ifbr_ifsflags = IFBF_FLUSHDYN;
	if (do_cmd(sock, bridge, BRDGFLUSH, &req, sizeof(req), 1) < 0)
		err(1, "%s", cmd->cmd_keyword);
}

void
cmd_flushall(const struct command *cmd, int sock, const char *bridge,
    char **argv)
{
	struct ifbreq req;

	memset(&req, 0, sizeof(req));
	req.ifbr_ifsflags = IFBF_FLUSHALL;
	if (do_cmd(sock, bridge, BRDGFLUSH, &req, sizeof(req), 1) < 0)
		err(1, "%s", cmd->cmd_keyword);
}

void
cmd_static(const struct command *cmd, int sock, const char *bridge,
    char **argv)
{
	struct ifbareq req;
	struct ether_addr *ea;

	memset(&req, 0, sizeof(req));
	strlcpy(req.ifba_ifsname, argv[0], sizeof(req.ifba_ifsname));

	ea = ether_aton(argv[1]);
	if (ea == NULL)
		errx(1, "%s: invalid address: %s", cmd->cmd_keyword, argv[1]);

	memcpy(req.ifba_dst, ea->ether_addr_octet, sizeof(req.ifba_dst));
	req.ifba_flags = IFBAF_STATIC;

	if (do_cmd(sock, bridge, BRDGSADDR, &req, sizeof(req), 1) < 0)
		err(1, "%s %s %s", cmd->cmd_keyword, argv[0], argv[1]);
}

void
cmd_deladdr(const struct command *cmd, int sock, const char *bridge,
    char **argv)
{
	struct ifbareq req;
	struct ether_addr *ea;

	memset(&req, 0, sizeof(req));

	ea = ether_aton(argv[0]);
	if (ea == NULL)
		errx(1, "%s: invalid address: %s", cmd->cmd_keyword, argv[0]);

	memcpy(req.ifba_dst, ea->ether_addr_octet, sizeof(req.ifba_dst));

	if (do_cmd(sock, bridge, BRDGDADDR, &req, sizeof(req), 1) < 0)
		err(1, "%s %s", cmd->cmd_keyword, argv[0]);
}

void
cmd_addr(const struct command *cmd, int sock, const char *bridge,
    char **argv)
{

	show_addresses(sock, bridge, "\t");
}

void
cmd_maxaddr(const struct command *cmd, int sock, const char *bridge,
    char **argv)
{
	struct ifbrparam param;
	u_long val;

	if (get_val(argv[0], &val) < 0 || (val & ~0xffffffff) != 0)
		errx(1, "%s: invalid value: %s", cmd->cmd_keyword, argv[0]);

	param.ifbrp_csize = val & 0xffffffff;

	if (do_cmd(sock, bridge, BRDGSCACHE, &param, sizeof(param), 1) < 0)
		err(1, "%s %s", cmd->cmd_keyword, argv[0]);
}

void
cmd_hellotime(const struct command *cmd, int sock, const char *bridge,
    char **argv)
{
	struct ifbrparam param;
	u_long val;

	if (get_val(argv[0], &val) < 0 || (val & ~0xff) != 0)
		errx(1, "%s: invalid value: %s", cmd->cmd_keyword, argv[0]);

	param.ifbrp_hellotime = val & 0xff;

	if (do_cmd(sock, bridge, BRDGSHT, &param, sizeof(param), 1) < 0)
		err(1, "%s %s", cmd->cmd_keyword, argv[0]);
}

void
cmd_fwddelay(const struct command *cmd, int sock, const char *bridge,
    char **argv)
{
	struct ifbrparam param;
	u_long val;

	if (get_val(argv[0], &val) < 0 || (val & ~0xff) != 0)
		errx(1, "%s: invalid value: %s", cmd->cmd_keyword, argv[0]);

	param.ifbrp_fwddelay = val & 0xff;

	if (do_cmd(sock, bridge, BRDGSFD, &param, sizeof(param), 1) < 0)
		err(1, "%s %s", cmd->cmd_keyword, argv[0]);
}

void
cmd_maxage(const struct command *cmd, int sock, const char *bridge,
    char **argv)
{
	struct ifbrparam param;
	u_long val;

	if (get_val(argv[0], &val) < 0 || (val & ~0xff) != 0)
		errx(1, "%s: invalid value: %s", cmd->cmd_keyword, argv[0]);

	param.ifbrp_maxage = val & 0xff;

	if (do_cmd(sock, bridge, BRDGSMA, &param, sizeof(param), 1) < 0)
		err(1, "%s %s", cmd->cmd_keyword, argv[0]);
}

void
cmd_priority(const struct command *cmd, int sock, const char *bridge,
    char **argv)
{
	struct ifbrparam param;
	u_long val;

	if (get_val(argv[0], &val) < 0 || (val & ~0xffff) != 0)
		errx(1, "%s: invalid value: %s", cmd->cmd_keyword, argv[0]);

	param.ifbrp_prio = val & 0xffff;

	if (do_cmd(sock, bridge, BRDGSPRI, &param, sizeof(param), 1) < 0)
		err(1, "%s %s", cmd->cmd_keyword, argv[0]);
}

void
cmd_ifpriority(const struct command *cmd, int sock, const char *bridge,
    char **argv)
{
	struct ifbreq req;
	u_long val;

	memset(&req, 0, sizeof(req));

	if (get_val(argv[1], &val) < 0 || (val & ~0xff) != 0)
		errx(1, "%s: invalid value: %s", cmd->cmd_keyword, argv[1]);

	strlcpy(req.ifbr_ifsname, argv[0], sizeof(req.ifbr_ifsname));
	req.ifbr_priority = val & 0xff;

	if (do_cmd(sock, bridge, BRDGSIFPRIO, &req, sizeof(req), 1) < 0)
		err(1, "%s %s", cmd->cmd_keyword, argv[0]);
}

void
cmd_ifpathcost(const struct command *cmd, int sock, const char *bridge,
    char **argv)
{
	struct ifbreq req;
	u_long val;

	memset(&req, 0, sizeof(req));

	if (get_val(argv[1], &val) < 0 || (val & ~0xff) != 0)
		errx(1, "%s: invalid value: %s", cmd->cmd_keyword, argv[1]);

	strlcpy(req.ifbr_ifsname, argv[0], sizeof(req.ifbr_ifsname));
	req.ifbr_path_cost = val & 0xffff;

	if (do_cmd(sock, bridge, BRDGSIFCOST, &req, sizeof(req), 1) < 0)
		err(1, "%s %s", cmd->cmd_keyword, argv[0]);
}

void
cmd_timeout(const struct command *cmd, int sock, const char *bridge,
    char **argv)
{
	struct ifbrparam param;
	u_long val;

	if (get_val(argv[0], &val) < 0 || (val & ~0xffffffff) != 0)
		errx(1, "%s: invalid value: %s", cmd->cmd_keyword, argv[0]);

	param.ifbrp_ctime = val & 0xffffffff;

	if (do_cmd(sock, bridge, BRDGSTO, &param, sizeof(param), 1) < 0)
		err(1, "%s %s", cmd->cmd_keyword, argv[0]);
}

void
cmd_ipf(const struct command *cmd, int sock, const char *bridge,
    char **argv)
{
        struct ifbrparam param;

        if (do_cmd(sock, bridge, BRDGGFILT, &param, sizeof(param), 0) < 0)
		err(1, "%s", cmd->cmd_keyword);

        param.ifbrp_filter &= ~IFBF_FILT_USEIPF;
        param.ifbrp_filter |= (cmd->cmd_flags & CMD_INVERT) ? 0 : IFBF_FILT_USEIPF;
        if (do_cmd(sock, bridge, BRDGSFILT, &param, sizeof(param), 1) < 0)
		err(1, "%s %x", cmd->cmd_keyword, param.ifbrp_filter);
}
