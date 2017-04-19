/*	$NetBSD: canconfig.c,v 1.1.2.2 2017/04/19 17:51:16 bouyer Exp $	*/

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

#include <sys/cdefs.h>

#ifndef lint
__RCSID("$NetBSD: canconfig.c,v 1.1.2.2 2017/04/19 17:51:16 bouyer Exp $");
#endif


#include <sys/param.h>
#include <sys/socket.h>
#include <sys/ioctl.h>

#include <net/if.h>
#include <netcan/can.h>
#include <netcan/can_link.h>
#include <ifaddrs.h>

#include <ctype.h>
#include <err.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

struct command {
	const char *cmd_keyword;
	int	cmd_argcnt;
	int	cmd_flags;
	void	(*cmd_func)(const struct command *, int, const char *,
		    char **);
};

#define	CMD_INVERT	0x01	/* "invert" the sense of the command */

static void	cmd_up(const struct command *, int, const char *, char **);
static void	cmd_down(const struct command *, int, const char *, char **);
static void	cmd_brp(const struct command *, int, const char *, char **);
static void	cmd_prop_seg(const struct command *, int, const char *, char **);
static void	cmd_phase_seg1(const struct command *, int, const char *, char **);
static void	cmd_phase_seg2(const struct command *, int, const char *, char **);
static void	cmd_sjw(const struct command *, int, const char *, char **);
static void	cmd_3samples(const struct command *, int, const char *, char **);

static const struct command command_table[] = {
	{ "up",			0,	0,		cmd_up },
	{ "down",		0,	0,		cmd_down },

	{ "brp",		1,	0,		cmd_brp },
	{ "prop_seg",		1,	0,		cmd_prop_seg },
	{ "phase_seg1",		1,	0,		cmd_phase_seg1 },
	{ "phase_seg2",		1,	0,		cmd_phase_seg2 },
	{ "sjw",		1,	0,		cmd_sjw },

	{ "3samples",		0,	0,		cmd_3samples },
	{ "-3samples",		0,	CMD_INVERT,	cmd_3samples },

	{ NULL,			0,	0,		NULL },
};

static void	printall(int);
static void	status(int, const char *);
static void	show_timings(int, const char *, const char *);
static int	is_can(int s, const char *);
static int	get_val(const char *, u_long *);
#define	do_cmd(a,b,c,d,e,f)	do_cmd2((a),(b),(c),(d),(e),NULL,(f))
static int	do_cmd2(int, const char *, u_long, void *, size_t, size_t *, int);
__dead static void	usage(void);

static int	aflag;
static struct ifreq g_ifr;
static int	g_ifr_updated = 0;

struct can_link_timecaps g_cltc;
struct can_link_timings g_clt;
static int	g_clt_updated = 0;

int
main(int argc, char *argv[])
{
	const struct command *cmd;
	char *canifname;
	int sock, ch;

	if (argc < 2)
		usage();

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
		sock = socket(AF_CAN, SOCK_RAW, CAN_RAW);
		if (sock < 0)
			err(1, "socket");

		printall(sock);
		exit(0);
	}

	if (argc == 0)
		usage();

	sock = socket(AF_CAN, SOCK_RAW, CAN_RAW);
	if (sock < 0)
		err(1, "socket");

	canifname = argv[0];

	if (is_can(sock, canifname) == 0)
		errx(1, "%s is not a can interface", canifname);

	/* Get a copy of the interface flags. */
	strlcpy(g_ifr.ifr_name, canifname, sizeof(g_ifr.ifr_name));
	if (ioctl(sock, SIOCGIFFLAGS, &g_ifr) < 0)
		err(1, "unable to get interface flags");

	argc--;
	argv++;

	if (argc == 0) {
		status(sock, canifname);
		exit(0);
	}

	if (do_cmd(sock, canifname, CANGLINKTIMECAP, &g_cltc, sizeof(g_cltc), 0)
	    < 0)
		err(1, "unable to get can link timecaps");

	if (do_cmd(sock, canifname, CANGLINKTIMINGS, &g_clt, sizeof(g_clt), 0) < 0)
		err(1, "unable to get can link timings");

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

		(*cmd->cmd_func)(cmd, sock, canifname, argv);

		argc -= cmd->cmd_argcnt;
		argv += cmd->cmd_argcnt;
	}

	/* If the timings changed, update them. */
	if (g_clt_updated &&
	    do_cmd(sock, canifname, CANSLINKTIMINGS, &g_clt, sizeof(g_clt), 1) < 0) 
		err(1, "unable to set can link timings");

	/* If the flags changed, update them. */
	if (g_ifr_updated && ioctl(sock, SIOCSIFFLAGS, &g_ifr) < 0)
		err(1, "unable to set interface flags");

	exit (0);
}

static void
usage(void)
{
	static const char *usage_strings[] = {
		"-a",
		"<canif>",
		"<canif> up|down",
		"<canif> brp <value>",
		"<canif> prop_seg <value>",
		"<canif> phase_seg1 <value>",
		"<canif> phase_seg2 <value>",
		"<canif> sjw <value>",
		"<canif> 3samples | -3samples",
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

static int
is_can(int s, const char *canif)
{
	uint32_t linkmode;

	if (do_cmd(s, canif, CANGLINKMODE, &linkmode, sizeof(linkmode), 0) < 0)
		return (0);

	return (1);
}

static void
printb(const char *s, u_int v, const char *bits)
{
	int i, any = 0;
	char c;

	if (bits && *bits == 8)
		printf("%s=%o", s, v);
	else
		printf("%s=%x", s, v);
	if (bits) { 
		bits++;
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

static void
printall(int sock)
{
	struct ifaddrs *ifap, *ifa;
	char *p;

	if (getifaddrs(&ifap) != 0)
		err(1, "getifaddrs");
	p = NULL;
	for (ifa = ifap; ifa != NULL; ifa = ifa->ifa_next) {
		if (is_can(sock, ifa->ifa_name) == 0)
			continue;
		if (p != NULL && strcmp(p, ifa->ifa_name) == 0)
			continue;
		p = ifa->ifa_name;
		status(sock, ifa->ifa_name);
	}

	freeifaddrs(ifap);
}

static void
status(int sock, const char *canifname)
{
	struct ifreq ifr;

	memset(&ifr, 0, sizeof(ifr));

	strlcpy(ifr.ifr_name, canifname, sizeof(ifr.ifr_name));
	if (ioctl(sock, SIOCGIFFLAGS, &ifr) < 0)
		err(1, "unable to get flags");

	printf("%s: ", canifname);
	printb("flags", ifr.ifr_flags, IFFBITS);
	printf("\n");

	show_timings(sock, canifname, "\t");

}

static void
show_timings(int sock, const char *canifname, const char *prefix)
{
	struct can_link_timecaps cltc;
	struct can_link_timings clt;
	u_int32_t linkmode;
	char hbuf[8];

	if (do_cmd(sock, canifname, CANGLINKTIMECAP, &cltc, sizeof(cltc), 0)
	    < 0)
		err(1, "unable to get can link timecaps");

	if (do_cmd(sock, canifname, CANGLINKTIMINGS, &clt, sizeof(clt), 0) < 0)
		err(1, "unable to get can link timings");

	if (do_cmd(sock, canifname, CANGLINKMODE, &linkmode, sizeof(linkmode),
	    0) < 0)
		err(1, "unable to get can link mode");

	humanize_number(hbuf, sizeof(hbuf), cltc.cltc_clock_freq, "Hz", 0,
	    HN_AUTOSCALE | HN_NOSPACE | HN_DIVISOR_1000);

	printf("%stiming caps:\n", prefix);
	printf("%s  clock %s, brp [%d..%d]/%d, prop_seg [%d..%d]\n",
	    prefix, hbuf,
	    cltc.cltc_brp_min, cltc.cltc_brp_max, cltc.cltc_brp_inc,
	    cltc.cltc_prop_min, cltc.cltc_prop_max);
	printf("%s  phase_seg1 [%d..%d], phase_seg2 [%d..%d], sjw [0..%d]\n",
	    prefix,
	    cltc.cltc_ps1_min, cltc.cltc_ps1_max,
	    cltc.cltc_ps2_min, cltc.cltc_ps2_max,
	    cltc.cltc_sjw_max);
	printf("%s  ", prefix);
	printb("capabilities", cltc.cltc_linkmode_caps, CAN_IFFBITS);
	printf("\n");
	printf("%soperational timings:\n", prefix);
	printf("%s  brp %d prop_seg %d, phase_seg1 %d, phase_seg2 %d, sjw %d\n",
	    prefix,
	    clt.clt_brp, clt.clt_prop, clt.clt_ps1, clt.clt_ps2, clt.clt_sjw);
	printf("%s  ", prefix);
	printb("mode", linkmode, CAN_IFFBITS);
	printf("\n");
}

static int
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

static int
do_cmd2(int sock, const char *canifname, u_long op, void *arg, size_t argsize,
    size_t *outsizep, int set)
{
	struct ifdrv ifd;
	int error;

	memset(&ifd, 0, sizeof(ifd));

	strlcpy(ifd.ifd_name, canifname, sizeof(ifd.ifd_name));
	ifd.ifd_cmd = op;
	ifd.ifd_len = argsize;
	ifd.ifd_data = arg;

	error = ioctl(sock, set ? SIOCSDRVSPEC : SIOCGDRVSPEC, &ifd);

	if (outsizep)
		*outsizep = ifd.ifd_len;

	return error;
}

static void
do_ifflag(int sock, const char *canifname, int flag, int set)
{

	if (set)
		g_ifr.ifr_flags |= flag;
	else
		g_ifr.ifr_flags &= ~flag;

	g_ifr_updated = 1;
}

static int
do_canflag(int sock, const char *canifname, uint32_t flag, int set)
{
	int cmd;
	if (set)
		cmd = CANSLINKMODE;
	else
		cmd = CANCLINKMODE;
	return do_cmd(sock, canifname, cmd, &flag, sizeof(flag), set);
}

static void
cmd_up(const struct command *cmd, int sock, const char *canifname,
    char **argv)
{

	do_ifflag(sock, canifname, IFF_UP, 1);
}

static void
cmd_down(const struct command *cmd, int sock, const char *canifname,
    char **argv)
{

	do_ifflag(sock, canifname, IFF_UP, 0);
}

static void
cmd_brp(const struct command *cmd, int sock, const char *bridge,
    char **argv)
{
	u_long val;

	if (get_val(argv[0], &val) < 0 || (val & ~0xffffffff) != 0)
		errx(1, "%s: invalid value: %s", cmd->cmd_keyword, argv[0]);
	if (val <  g_cltc.cltc_brp_min || val > g_cltc.cltc_brp_max) 
		errx(1, "%s: out of range value: %s", cmd->cmd_keyword, argv[0]);
	g_clt.clt_brp = val;
	g_clt_updated=1;
}

static void
cmd_prop_seg(const struct command *cmd, int sock, const char *bridge,
    char **argv)
{
	u_long val;

	if (get_val(argv[0], &val) < 0 || (val & ~0xffffffff) != 0)
		errx(1, "%s: invalid value: %s", cmd->cmd_keyword, argv[0]);
	if (val <  g_cltc.cltc_prop_min || val > g_cltc.cltc_prop_max) 
		errx(1, "%s: out of range value: %s", cmd->cmd_keyword, argv[0]);
	g_clt.clt_prop = val;
	g_clt_updated=1;
}

static void
cmd_phase_seg1(const struct command *cmd, int sock, const char *bridge,
    char **argv)
{
	u_long val;

	if (get_val(argv[0], &val) < 0 || (val & ~0xffffffff) != 0)
		errx(1, "%s: invalid value: %s", cmd->cmd_keyword, argv[0]);
	if (val <  g_cltc.cltc_ps1_min || val > g_cltc.cltc_ps1_max) 
		errx(1, "%s: out of range value: %s", cmd->cmd_keyword, argv[0]);
	g_clt.clt_ps1 = val;
	g_clt_updated=1;
}

static void
cmd_phase_seg2(const struct command *cmd, int sock, const char *bridge,
    char **argv)
{
	u_long val;

	if (get_val(argv[0], &val) < 0 || (val & ~0xffffffff) != 0)
		errx(1, "%s: invalid value: %s", cmd->cmd_keyword, argv[0]);
	if (val <  g_cltc.cltc_ps2_min || val > g_cltc.cltc_ps2_max) 
		errx(1, "%s: out of range value: %s", cmd->cmd_keyword, argv[0]);
	g_clt.clt_ps2 = val;
	g_clt_updated=1;
}

static void
cmd_sjw(const struct command *cmd, int sock, const char *bridge,
    char **argv)
{
	u_long val;

	if (get_val(argv[0], &val) < 0 || (val & ~0xffffffff) != 0)
		errx(1, "%s: invalid value: %s", cmd->cmd_keyword, argv[0]);
	if (val > g_cltc.cltc_sjw_max) 
		errx(1, "%s: out of range value: %s", cmd->cmd_keyword, argv[0]);
	g_clt.clt_sjw = val;
	g_clt_updated=1;
}

static void
cmd_3samples(const struct command *cmd, int sock, const char *canifname,
    char **argv)
{
        if (do_canflag(sock, canifname, CAN_LINKMODE_3SAMPLES,
	    (cmd->cmd_flags & CMD_INVERT) ? 0 : 1) < 0)
		err(1, "%s", cmd->cmd_keyword);

}
