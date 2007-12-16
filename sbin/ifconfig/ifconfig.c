/*	$NetBSD: ifconfig.c,v 1.181 2007/12/16 13:49:22 degroote Exp $	*/

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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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
__RCSID("$NetBSD: ifconfig.c,v 1.181 2007/12/16 13:49:22 degroote Exp $");
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

#include <ctype.h>
#include <err.h>
#include <errno.h>
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

#include "agr.h"
#include "carp.h"
#include "ieee80211.h"
#include "tunnel.h"
#include "vlan.h"

struct	ifreq		ifr, ridreq;
struct	ifaliasreq	addreq __attribute__((aligned(4)));

char	name[30];
u_short	flags;
int	setaddr, doalias;
u_long	metric, mtu, preference;
int	clearaddr, s;
int	newaddr = -1;
int	conflicting = 0;
int	check_up_state = -1;
int	af;
int	aflag, bflag, Cflag, dflag, lflag, mflag, sflag, uflag, vflag, zflag;
int	hflag;
int	have_preference = 0;
#ifdef INET6
int	Lflag;
#endif
int	explicit_prefix = 0;

struct ifcapreq g_ifcr;
int	g_ifcr_updated;

void 	notealias(const char *, int);
void 	notrailers(const char *, int);
void 	setifaddr(const char *, int);
void 	setifdstaddr(const char *, int);
void 	setifflags(const char *, int);
void	check_ifflags_up(const char *);
void	setifcaps(const char *, int);
void 	setifbroadaddr(const char *, int);
void 	setifipdst(const char *, int);
void 	setifmetric(const char *, int);
void	setifpreference(const char *, int);
void 	setifmtu(const char *, int);
void 	setifnetmask(const char *, int);
void	setifprefixlen(const char *, int);
void	setmedia(const char *, int);
void	setmediamode(const char *, int);
void	setmediaopt(const char *, int);
void	unsetmediaopt(const char *, int);
void	setmediainst(const char *, int);
void	clone_create(const char *, int);
void	clone_destroy(const char *, int);
int	main(int, char *[]);
void	do_setifpreference(void);

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

int	actions;			/* Actions performed */

#define	A_MEDIA		0x0001		/* media command */
#define	A_MEDIAOPTSET	0x0002		/* mediaopt command */
#define	A_MEDIAOPTCLR	0x0004		/* -mediaopt command */
#define	A_MEDIAOPT	(A_MEDIAOPTSET|A_MEDIAOPTCLR)
#define	A_MEDIAINST	0x0008		/* instance or inst command */
#define	A_MEDIAMODE	0x0010		/* mode command */

#define	NEXTARG		0xffffff
#define	NEXTARG2	0xfffffe

const struct cmd {
	const char *c_name;
	int	c_parameter;	/* NEXTARG means next argv */
	int	c_action;	/* defered action */
	void	(*c_func)(const char *, int);
} cmds[] = {
	{ "up",		IFF_UP,		0,		setifflags } ,
	{ "down",	-IFF_UP,	0,		setifflags },
	{ "trailers",	-1,		0,		notrailers },
	{ "-trailers",	1,		0,		notrailers },
	{ "arp",	-IFF_NOARP,	0,		setifflags },
	{ "-arp",	IFF_NOARP,	0,		setifflags },
	{ "debug",	IFF_DEBUG,	0,		setifflags },
	{ "-debug",	-IFF_DEBUG,	0,		setifflags },
	{ "alias",	IFF_UP,		0,		notealias },
	{ "-alias",	-IFF_UP,	0,		notealias },
	{ "delete",	-IFF_UP,	0,		notealias },
#ifdef notdef
#define	EN_SWABIPS	0x1000
	{ "swabips",	EN_SWABIPS,	0,		setifflags },
	{ "-swabips",	-EN_SWABIPS,	0,		setifflags },
#endif
	{ "netmask",	NEXTARG,	0,		setifnetmask },
	{ "metric",	NEXTARG,	0,		setifmetric },
	{ "mtu",	NEXTARG,	0,		setifmtu },
	{ "bssid",	NEXTARG,	0,		setifbssid },
	{ "-bssid",	-1,		0,		setifbssid },
	{ "chan",	NEXTARG,	0,		setifchan },
	{ "-chan",	-1,		0,		setifchan },
	{ "frag",	NEXTARG,	0,		setiffrag },
	{ "-frag",	-1,		0,		setiffrag },
	{ "ssid",	NEXTARG,	0,		setifnwid },
	{ "nwid",	NEXTARG,	0,		setifnwid },
	{ "nwkey",	NEXTARG,	0,		setifnwkey },
	{ "-nwkey",	-1,		0,		setifnwkey },
	{ "powersave",	1,		0,		setifpowersave },
	{ "-powersave",	0,		0,		setifpowersave },
	{ "powersavesleep", NEXTARG,	0,		setifpowersavesleep },
	{ "hidessid",	1,		0,		sethidessid },
	{ "-hidessid",	0,		0,		sethidessid },
	{ "apbridge",	1,		0,		setapbridge },
	{ "-apbridge",	0,		0,		setapbridge },
	{ "broadcast",	NEXTARG,	0,		setifbroadaddr },
	{ "ipdst",	NEXTARG,	0,		setifipdst },
	{ "prefixlen",	NEXTARG,	0,		setifprefixlen},
	{ "preference",	NEXTARG,	0,		setifpreference},
	{ "list",	NEXTARG,	0,		setiflist},
#ifndef INET_ONLY
	/* CARP */
	{ "advbase",	NEXTARG,	0,		setcarp_advbase },
	{ "advskew",	NEXTARG,	0,		setcarp_advskew },
	{ "pass",	NEXTARG,	0,		setcarp_passwd },
	{ "vhid",	NEXTARG,	0,		setcarp_vhid },
	{ "state",	NEXTARG,	0,		setcarp_state },
	{ "carpdev",	NEXTARG,	0,		setcarpdev },
	{ "-carpdev",	1,		0,		unsetcarpdev },
#endif
#ifdef INET6
	{ "anycast",	IN6_IFF_ANYCAST,	0,	setia6flags },
	{ "-anycast",	-IN6_IFF_ANYCAST,	0,	setia6flags },
	{ "tentative",	IN6_IFF_TENTATIVE,	0,	setia6flags },
	{ "-tentative",	-IN6_IFF_TENTATIVE,	0,	setia6flags },
	{ "deprecated",	IN6_IFF_DEPRECATED,	0,	setia6flags },
	{ "-deprecated", -IN6_IFF_DEPRECATED,	0,	setia6flags },
	{ "pltime",	NEXTARG,	0,		setia6pltime },
	{ "vltime",	NEXTARG,	0,		setia6vltime },
	{ "eui64",	0,		0,		setia6eui64 },
#endif /*INET6*/
#ifndef INET_ONLY
	{ "range",	NEXTARG,	0,		setatrange },
	{ "phase",	NEXTARG,	0,		setatphase },
	{ "snpaoffset",	NEXTARG,	0,		setsnpaoffset },
	{ "nsellength",	NEXTARG,	0,		setnsellength },
#endif	/* INET_ONLY */
	{ "tunnel",	NEXTARG2,	0,	(void (*)(const char *, int))
							settunnel } ,
	{ "deletetunnel", 0,		0,		deletetunnel },
	{ "vlan",	NEXTARG,	0,		setvlan } ,
	{ "vlanif",	NEXTARG,	0,		setvlanif } ,
	{ "-vlanif",	0,		0,		unsetvlanif } ,
#if 0
	/* XXX `create' special-cased below */
	{ "create",	0,		0,		clone_create } ,
#endif
	{ "destroy",	0,		0,		clone_destroy } ,
	{ "link0",	IFF_LINK0,	0,		setifflags } ,
	{ "-link0",	-IFF_LINK0,	0,		setifflags } ,
	{ "link1",	IFF_LINK1,	0,		setifflags } ,
	{ "-link1",	-IFF_LINK1,	0,		setifflags } ,
	{ "link2",	IFF_LINK2,	0,		setifflags } ,
	{ "-link2",	-IFF_LINK2,	0,		setifflags } ,
	{ "media",	NEXTARG,	A_MEDIA,	setmedia },
	{ "mediaopt",	NEXTARG,	A_MEDIAOPTSET,	setmediaopt },
	{ "-mediaopt",	NEXTARG,	A_MEDIAOPTCLR,	unsetmediaopt },
	{ "mode",	NEXTARG,	A_MEDIAMODE,	setmediamode },
	{ "instance",	NEXTARG,	A_MEDIAINST,	setmediainst },
	{ "inst",	NEXTARG,	A_MEDIAINST,	setmediainst },
	{ "ip4csum-tx",	IFCAP_CSUM_IPv4_Tx,0,		setifcaps },
	{ "-ip4csum-tx",-IFCAP_CSUM_IPv4_Tx,0,		setifcaps },
	{ "ip4csum-rx",	IFCAP_CSUM_IPv4_Rx,0,		setifcaps },
	{ "-ip4csum-rx",-IFCAP_CSUM_IPv4_Rx,0,		setifcaps },
	{ "tcp4csum-tx",IFCAP_CSUM_TCPv4_Tx,0,		setifcaps },
	{ "-tcp4csum-tx",-IFCAP_CSUM_TCPv4_Tx,0,	setifcaps },
	{ "tcp4csum-rx",IFCAP_CSUM_TCPv4_Rx,0,		setifcaps },
	{ "-tcp4csum-rx",-IFCAP_CSUM_TCPv4_Rx,0,	setifcaps },
	{ "udp4csum-tx",IFCAP_CSUM_UDPv4_Tx,0,		setifcaps },
	{ "-udp4csum-tx",-IFCAP_CSUM_UDPv4_Tx,0,	setifcaps },
	{ "udp4csum-rx",IFCAP_CSUM_UDPv4_Rx,0,		setifcaps },
	{ "-udp4csum-rx",-IFCAP_CSUM_UDPv4_Rx,0,	setifcaps },
	{ "tcp6csum-tx",IFCAP_CSUM_TCPv6_Tx,0,		setifcaps },
	{ "-tcp6csum-tx",-IFCAP_CSUM_TCPv6_Tx,0,	setifcaps },
	{ "tcp6csum-rx",IFCAP_CSUM_TCPv6_Rx,0,		setifcaps },
	{ "-tcp6csum-rx",-IFCAP_CSUM_TCPv6_Rx,0,	setifcaps },
	{ "udp6csum-tx",IFCAP_CSUM_UDPv6_Tx,0,		setifcaps },
	{ "-udp6csum-tx",-IFCAP_CSUM_UDPv6_Tx,0,	setifcaps },
	{ "udp6csum-rx",IFCAP_CSUM_UDPv6_Rx,0,		setifcaps },
	{ "-udp6csum-rx",-IFCAP_CSUM_UDPv6_Rx,0,	setifcaps },
	{ "ip4csum",	IFCAP_CSUM_IPv4_Tx|IFCAP_CSUM_IPv4_Rx,
					0,		setifcaps },
	{ "-ip4csum",	-(IFCAP_CSUM_IPv4_Tx|IFCAP_CSUM_IPv4_Rx),
					0,		setifcaps },
	{ "tcp4csum",	IFCAP_CSUM_TCPv4_Tx|IFCAP_CSUM_TCPv4_Rx,
					0,		setifcaps },
	{ "-tcp4csum",	-(IFCAP_CSUM_TCPv4_Tx|IFCAP_CSUM_TCPv4_Rx),
					0,		setifcaps },
	{ "udp4csum",	IFCAP_CSUM_UDPv4_Tx|IFCAP_CSUM_UDPv4_Rx,
					0,		setifcaps },
	{ "-udp4csum",	-(IFCAP_CSUM_UDPv4_Tx|IFCAP_CSUM_UDPv4_Rx),
					0,		setifcaps },
	{ "tcp6csum",	IFCAP_CSUM_TCPv6_Tx|IFCAP_CSUM_TCPv6_Rx,
					0,		setifcaps },
	{ "-tcp6csum",	-(IFCAP_CSUM_TCPv6_Tx|IFCAP_CSUM_TCPv6_Rx),
					0,		setifcaps },
	{ "udp6csum",	IFCAP_CSUM_UDPv6_Tx|IFCAP_CSUM_UDPv6_Rx,
					0,		setifcaps },
	{ "-udp6csum",	-(IFCAP_CSUM_UDPv6_Tx|IFCAP_CSUM_UDPv6_Rx),
					0,		setifcaps },
	{ "tso4",	IFCAP_TSOv4,	0,		setifcaps },
	{ "-tso4",	-IFCAP_TSOv4,	0,		setifcaps },
	{ "tso6",	IFCAP_TSOv6,	0,		setifcaps },
	{ "-tso6",	-IFCAP_TSOv6,	0,		setifcaps },
	{ "agrport",	NEXTARG,	0,		agraddport } ,
	{ "-agrport",	NEXTARG,	0,		agrremport } ,
	{ 0,		0,		0,		setifaddr },
	{ 0,		0,		0,		setifdstaddr },
};

int	getinfo(struct ifreq *);
int	carrier(void);
void	printall(const char *);
void	list_cloners(void);
void 	status(const struct sockaddr_dl *);
void 	usage(void);

void	print_media_word(int, const char *);
void	process_media_commands(void);
void	init_current_media(void);

/* Known address families */
const struct afswtch afs[] = {
	{ "inet", AF_INET, in_status, in_getaddr, in_getprefix,
	     SIOCDIFADDR, SIOCAIFADDR, SIOCGIFADDR, &ridreq, &in_addreq },
#ifdef INET6
	{ "inet6", AF_INET6, in6_status, in6_getaddr, in6_getprefix,
	     SIOCDIFADDR_IN6, SIOCAIFADDR_IN6,
	     /*
	      * Deleting the first address before setting new one is
	      * not prefered way in this protocol.
	      */
	     0,
	     &in6_ridreq, &in6_addreq },
#endif
#ifndef INET_ONLY	/* small version, for boot media */
	{ "atalk", AF_APPLETALK, at_status, at_getaddr, NULL,
	     SIOCDIFADDR, SIOCAIFADDR, SIOCGIFADDR, &addreq, &addreq },
	{ "iso", AF_ISO, iso_status, iso_getaddr, NULL,
	     SIOCDIFADDR_ISO, SIOCAIFADDR_ISO, SIOCGIFADDR_ISO,
	     &iso_ridreq, &iso_addreq },
#endif	/* INET_ONLY */
	{ 0,	0,	    0,		0, 0, 0, 0, 0, 0, 0 }
};

const struct afswtch *afp;	/*the address family being set or asked about*/

int
main(int argc, char *argv[])
{
	int ch;

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
	if ((lflag || Cflag) && (aflag || mflag || vflag || argc || zflag))
		usage();
#ifdef INET6
	if ((lflag || Cflag) && Lflag)
		usage();
#endif
	if (lflag && Cflag)
		usage();
	if (Cflag) {
		if (argc)
			usage();
		list_cloners();
		exit(0);
	}
	if (aflag || lflag) {
		if (argc > 1)
			usage();
		else if (argc == 1) {
			afp = lookup_af_byname(argv[0]);
			if (afp == NULL)
				usage();
		}
		if (afp)
			af = ifr.ifr_addr.sa_family = afp->af_af;
		else
			af = ifr.ifr_addr.sa_family = afs[0].af_af;
		printall(NULL);
		exit(0);
	}

	/* Make sure there's an interface name. */
	if (argc < 1)
		usage();
	if (strlcpy(name, argv[0], sizeof(name)) >= sizeof(name))
		errx(1, "interface name '%s' too long", argv[0]);
	argc--; argv++;

	/*
	 * NOTE:  We must special-case the `create' command right
	 * here as we would otherwise fail in getinfo().
	 */
	if (argc > 0 && strcmp(argv[0], "create") == 0) {
		clone_create(argv[0], 0);
		argc--, argv++;
		if (argc == 0)
			exit(0);
	}

	/* Check for address family. */
	afp = NULL;
	if (argc > 0) {
		afp = lookup_af_byname(argv[0]);
		if (afp != NULL) {
			argv++;
			argc--;
		}
	}

	/* Initialize af, just for use in getinfo(). */
	if (afp == NULL)
		af = afs->af_af;
	else
		af = afp->af_af;

	/* Get information about the interface. */
	estrlcpy(ifr.ifr_name, name, sizeof(ifr.ifr_name));
	if (getinfo(&ifr) < 0)
		exit(1);

	if (sflag) {
		if (argc != 0)
			usage();
		else
			exit(carrier());
	}

	/* No more arguments means interface status. */
	if (argc == 0) {
		printall(name);
		exit(0);
	}

	/* The following operations assume inet family as the default. */
	if (afp == NULL)
		afp = afs;
	af = ifr.ifr_addr.sa_family = afp->af_af;

#ifdef INET6
	in6_init();
#endif

	/* Process commands. */
	while (argc > 0) {
		const struct cmd *p;

		for (p = cmds; p->c_name; p++)
			if (strcmp(argv[0], p->c_name) == 0)
				break;
		if (p->c_name == 0 && setaddr) {
			if ((flags & IFF_POINTOPOINT) == 0) {
				errx(EXIT_FAILURE,
				    "can't set destination address %s",
				     "on non-point-to-point link");
			}
			p++;	/* got src, do dst */
		}
		if (p->c_func != NULL) {
			if (p->c_parameter == NEXTARG) {
				if (argc < 2)
					errx(EXIT_FAILURE,
					    "'%s' requires argument",
					    p->c_name);
				(*p->c_func)(argv[1], 0);
				argc--, argv++;
			} else if (p->c_parameter == NEXTARG2) {
				if (argc < 3)
					errx(EXIT_FAILURE,
					    "'%s' requires 2 arguments",
					    p->c_name);
				((void (*)(const char *, const char *))
				    *p->c_func)(argv[1], argv[2]);
				argc -= 2, argv += 2;
			} else
				(*p->c_func)(argv[0], p->c_parameter);
			actions |= p->c_action;
		}
		argc--, argv++;
	}

	/* 
	 * See if multiple alias, -alias, or delete commands were
	 * specified. More than one constitutes an invalid command line
	 */

	if (conflicting > 1) 
		errx(EXIT_FAILURE,
		    "Only one use of alias, -alias or delete is valid.");

	/* Process any media commands that may have been issued. */
	process_media_commands();

	if (af == AF_INET6 && explicit_prefix == 0) {
		/*
		 * Aggregatable address architecture defines all prefixes
		 * are 64. So, it is convenient to set prefixlen to 64 if
		 * it is not specified.
		 */
		setifprefixlen("64", 0);
		/* in6_getprefix("64", MASK) if MASK is available here... */
	}

#ifndef INET_ONLY
	if (af == AF_ISO)
		adjust_nsellength();

	if (af == AF_APPLETALK)
		checkatrange(&addreq.ifra_addr);
#endif	/* INET_ONLY */

	if (clearaddr) {
		estrlcpy(afp->af_ridreq, name, sizeof ifr.ifr_name);
		if (ioctl(s, afp->af_difaddr, afp->af_ridreq) == -1)
			err(EXIT_FAILURE, "SIOCDIFADDR");
	}
	if (newaddr > 0) {
		estrlcpy(afp->af_addreq, name, sizeof ifr.ifr_name);
		if (ioctl(s, afp->af_aifaddr, afp->af_addreq) == -1)
			warn("SIOCAIFADDR");
		else if (check_up_state < 0)
			check_up_state = 1;
	}

	if (have_preference)
		do_setifpreference();
	if (g_ifcr_updated) {
		strlcpy(g_ifcr.ifcr_name, name,
		    sizeof(g_ifcr.ifcr_name));
		if (ioctl(s, SIOCSIFCAP, &g_ifcr) == -1)
			err(EXIT_FAILURE, "SIOCSIFCAP");
	}

	if (check_up_state == 1)
		check_ifflags_up(name);

	exit(0);
}

const struct afswtch *
lookup_af_byname(const char *cp)
{
	const struct afswtch *a;

	for (a = afs; a->af_name != NULL; a++)
		if (strcmp(a->af_name, cp) == 0)
			return (a);
	return (NULL);
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
getsock(int naf)
{
	static int oaf = -1;

	if (oaf == naf)
		return;
	if (oaf != -1)
		close(s);
	s = socket(naf, SOCK_DGRAM, 0);
	if (s < 0)
		oaf = -1;
	else
		oaf = naf;
}

int
getinfo(struct ifreq *giifr)
{

	getsock(af);
	if (s < 0)
		err(EXIT_FAILURE, "socket");
	if (ioctl(s, SIOCGIFFLAGS, giifr) == -1) {
		warn("SIOCGIFFLAGS %s", giifr->ifr_name);
		return (-1);
	}
	flags = giifr->ifr_flags;
	if (ioctl(s, SIOCGIFMETRIC, giifr) == -1) {
		warn("SIOCGIFMETRIC %s", giifr->ifr_name);
		metric = 0;
	} else
		metric = giifr->ifr_metric;
	if (ioctl(s, SIOCGIFMTU, giifr) == -1)
		mtu = 0;
	else
		mtu = giifr->ifr_mtu;

	memset(&g_ifcr, 0, sizeof(g_ifcr));
	estrlcpy(g_ifcr.ifcr_name, giifr->ifr_name, sizeof(g_ifcr.ifcr_name));
	(void) ioctl(s, SIOCGIFCAP, &g_ifcr);

	return (0);
}

void
printall(const char *ifname)
{
	struct ifaddrs *ifap, *ifa;
	struct ifreq paifr;
	const struct sockaddr_dl *sdl = NULL;
	int idx;
	char *p;

	if (getifaddrs(&ifap) != 0)
		err(EXIT_FAILURE, "getifaddrs");
	p = NULL;
	idx = 0;
	for (ifa = ifap; ifa; ifa = ifa->ifa_next) {
		memset(&paifr, 0, sizeof(paifr));
		estrlcpy(paifr.ifr_name, ifa->ifa_name, sizeof(paifr.ifr_name));
		if (sizeof(paifr.ifr_addr) >= ifa->ifa_addr->sa_len) {
			memcpy(&paifr.ifr_addr, ifa->ifa_addr,
			    ifa->ifa_addr->sa_len);
		}

		if (ifname && strcmp(ifname, ifa->ifa_name) != 0)
			continue;
		if (ifa->ifa_addr->sa_family == AF_LINK)
			sdl = (const struct sockaddr_dl *) ifa->ifa_addr;
		if (p && strcmp(p, ifa->ifa_name) == 0)
			continue;
		if (strlcpy(name, ifa->ifa_name, sizeof(name)) >= sizeof(name))
			continue;
		p = ifa->ifa_name;

		if (getinfo(&paifr) < 0)
			continue;
		if (bflag && (ifa->ifa_flags & IFF_BROADCAST) == 0)
			continue;
		if (dflag && (ifa->ifa_flags & IFF_UP) != 0)
			continue;
		if (uflag && (ifa->ifa_flags & IFF_UP) == 0)
			continue;

		if (sflag && carrier())
			continue;
		idx++;
		/*
		 * Are we just listing the interfaces?
		 */
		if (lflag) {
			if (idx > 1)
				printf(" ");
			fputs(name, stdout);
			continue;
		}

		status(sdl);
		sdl = NULL;
	}
	if (lflag)
		printf("\n");
	freeifaddrs(ifap);
}

void
list_cloners(void)
{
	struct if_clonereq ifcr;
	char *cp, *buf;
	int idx;

	memset(&ifcr, 0, sizeof(ifcr));

	getsock(AF_INET);

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
	return;
}

/*ARGSUSED*/
void
clone_create(const char *addr, int param)
{

	/* We're called early... */
	getsock(AF_INET);

	estrlcpy(ifr.ifr_name, name, sizeof(ifr.ifr_name));
	if (ioctl(s, SIOCIFCREATE, &ifr) == -1)
		err(EXIT_FAILURE, "SIOCIFCREATE");
}

/*ARGSUSED*/
void
clone_destroy(const char *addr, int param)
{

	estrlcpy(ifr.ifr_name, name, sizeof(ifr.ifr_name));
	if (ioctl(s, SIOCIFDESTROY, &ifr) == -1)
		err(EXIT_FAILURE, "SIOCIFDESTROY");
}

/*ARGSUSED*/
void
setifaddr(const char *addr, int param)
{
	struct ifreq *siifr;		/* XXX */

	/*
	 * Delay the ioctl to set the interface addr until flags are all set.
	 * The address interpretation may depend on the flags,
	 * and the flags may change when the address is set.
	 */
	setaddr++;
	if (newaddr == -1)
		newaddr = 1;
	if (doalias == 0 && afp->af_gifaddr != 0) {
		siifr = (struct ifreq *)afp->af_ridreq;
		estrlcpy(siifr->ifr_name, name, sizeof(siifr->ifr_name));
		siifr->ifr_addr.sa_family = afp->af_af;
		if (ioctl(s, afp->af_gifaddr, afp->af_ridreq) == 0)
			clearaddr = 1;
		else if (errno == EADDRNOTAVAIL)
			/* No address was assigned yet. */
			;
		else
			err(EXIT_FAILURE, "SIOCGIFADDR");
	}

	(*afp->af_getaddr)(addr, (doalias >= 0 ? ADDR : RIDADDR));
}

void
setifnetmask(const char *addr, int d)
{
	(*afp->af_getaddr)(addr, MASK);
}

void
setifbroadaddr(const char *addr, int d)
{
	(*afp->af_getaddr)(addr, DSTADDR);
}

#define rqtosa(x) (&(((struct ifreq *)(afp->x))->ifr_addr))
/*ARGSUSED*/
void
notealias(const char *addr, int param)
{
	if (setaddr && doalias == 0 && param < 0)
		(void) memcpy(rqtosa(af_ridreq), rqtosa(af_addreq),
		    rqtosa(af_addreq)->sa_len);
	doalias = param;
	if (param < 0) {
		clearaddr = 1;
		newaddr = 0;
		conflicting++;
	} else {
		clearaddr = 0;
		conflicting++;
	}
}

/*ARGSUSED*/
void
notrailers(const char *vname, int value)
{
	puts("Note: trailers are no longer sent, but always received");
}

/*ARGSUSED*/
void
setifdstaddr(const char *addr, int param)
{
	(*afp->af_getaddr)(addr, DSTADDR);
}

void
check_ifflags_up(const char *vname)
{
	struct ifreq ifreq;

	estrlcpy(ifreq.ifr_name, name, sizeof(ifreq.ifr_name));
 	if (ioctl(s, SIOCGIFFLAGS, &ifreq) == -1)
		err(EXIT_FAILURE, "SIOCGIFFLAGS");
	if (ifreq.ifr_flags & IFF_UP)
		return;
	ifreq.ifr_flags |= IFF_UP;
	if (ioctl(s, SIOCSIFFLAGS, &ifreq) == -1)
		err(EXIT_FAILURE, "SIOCSIFFLAGS");
}

void
setifflags(const char *vname, int value)
{
	struct ifreq ifreq;

	estrlcpy(ifreq.ifr_name, name, sizeof(ifreq.ifr_name));
 	if (ioctl(s, SIOCGIFFLAGS, &ifreq) == -1)
		err(EXIT_FAILURE, "SIOCGIFFLAGS");
 	flags = ifreq.ifr_flags;

	if (value < 0) {
		value = -value;
		if (value == IFF_UP)
			check_up_state = 0;
		flags &= ~value;
	} else
		flags |= value;
	ifreq.ifr_flags = flags;
	if (ioctl(s, SIOCSIFFLAGS, &ifreq) == -1)
		err(EXIT_FAILURE, "SIOCSIFFLAGS");
}

void
setifcaps(const char *vname, int value)
{

	if (value < 0) {
		value = -value;
		g_ifcr.ifcr_capenable &= ~value;
	} else
		g_ifcr.ifcr_capenable |= value;

	g_ifcr_updated = 1;
}

void
setifmetric(const char *val, int d)
{
	char *ep = NULL;

	estrlcpy(ifr.ifr_name, name, sizeof (ifr.ifr_name));
	ifr.ifr_metric = strtoul(val, &ep, 10);
	if (!ep || *ep)
		errx(EXIT_FAILURE, "%s: invalid metric", val);
	if (ioctl(s, SIOCSIFMETRIC, &ifr) == -1)
		warn("SIOCSIFMETRIC");
}

void
setifpreference(const char *val, int d)
{
	char *end = NULL;
	if (setaddr <= 0) {
		errx(EXIT_FAILURE,
		    "set address preference: first specify an address");
	}
	preference = strtoul(val, &end, 10);
	if (end == NULL || *end != '\0' || preference > UINT16_MAX)
		errx(EXIT_FAILURE, "invalid preference %s", val);
	have_preference = 1;
}

void
do_setifpreference(void)
{
	struct if_addrprefreq ifap;
	(void)strncpy(ifap.ifap_name, name, sizeof(ifap.ifap_name));
	ifap.ifap_preference = (uint16_t)preference;
	(void)memcpy(&ifap.ifap_addr, rqtosa(af_addreq),
	    MIN(sizeof(ifap.ifap_addr), rqtosa(af_addreq)->sa_len));
	if (ioctl(s, SIOCSIFADDRPREF, &ifap) == -1)
		warn("SIOCSIFADDRPREF");
}

void
setifmtu(const char *val, int d)
{
	char *ep = NULL;

	estrlcpy(ifr.ifr_name, name, sizeof(ifr.ifr_name));
	ifr.ifr_mtu = strtoul(val, &ep, 10);
	if (!ep || *ep)
		errx(EXIT_FAILURE, "%s: invalid mtu", val);
	if (ioctl(s, SIOCSIFMTU, &ifr) == -1)
		warn("SIOCSIFMTU");
}

const char *
get_string(const char *val, const char *sep, u_int8_t *buf, int *lenp)
{
	int len;
	int hexstr;
	u_int8_t *p;

	len = *lenp;
	p = buf;
	hexstr = (val[0] == '0' && tolower((u_char)val[1]) == 'x');
	if (hexstr)
		val += 2;
	for (;;) {
		if (*val == '\0')
			break;
		if (sep != NULL && strchr(sep, *val) != NULL) {
			val++;
			break;
		}
		if (hexstr) {
			if (!isxdigit((u_char)val[0]) ||
			    !isxdigit((u_char)val[1])) {
				warnx("bad hexadecimal digits");
				return NULL;
			}
		}
		if (p > buf + len) {
			if (hexstr)
				warnx("hexadecimal digits too long");
			else
				warnx("strings too long");
			return NULL;
		}
		if (hexstr) {
#define	tohex(x)	(isdigit(x) ? (x) - '0' : tolower(x) - 'a' + 10)
			*p++ = (tohex((u_char)val[0]) << 4) |
			    tohex((u_char)val[1]);
#undef tohex
			val += 2;
		} else
			*p++ = *val++;
	}
	len = p - buf;
	if (len < *lenp)
		memset(p, 0, *lenp - len);
	*lenp = len;
	return val;
}

void
print_string(const u_int8_t *buf, int len)
{
	int i;
	int hasspc;

	i = 0;
	hasspc = 0;
	if (len < 2 || buf[0] != '0' || tolower(buf[1]) != 'x') {
		for (; i < len; i++) {
			if (!isprint(buf[i]))
				break;
			if (isspace(buf[i]))
				hasspc++;
		}
	}
	if (i == len) {
		if (hasspc || len == 0)
			printf("\"%.*s\"", len, buf);
		else
			printf("%.*s", len, buf);
	} else {
		printf("0x");
		for (i = 0; i < len; i++)
			printf("%02x", buf[i]);
	}
}

static void
media_error(int type, const char *val, const char *opt)
{
	errx(EXIT_FAILURE, "unknown %s media %s: %s",
		get_media_type_string(type), opt, val);
}

void
init_current_media(void)
{
	struct ifmediareq ifmr;

	/*
	 * If we have not yet done so, grab the currently-selected
	 * media.
	 */
	if ((actions & (A_MEDIA|A_MEDIAOPT|A_MEDIAMODE)) == 0) {
		(void) memset(&ifmr, 0, sizeof(ifmr));
		estrlcpy(ifmr.ifm_name, name, sizeof(ifmr.ifm_name));

		if (ioctl(s, SIOCGIFMEDIA, &ifmr) == -1) {
			/*
			 * If we get E2BIG, the kernel is telling us
			 * that there are more, so we can ignore it.
			 */
			if (errno != E2BIG)
				err(EXIT_FAILURE, "SGIOCGIFMEDIA");
		}

		media_current = ifmr.ifm_current;
	}

	/* Sanity. */
	if (IFM_TYPE(media_current) == 0)
		errx(EXIT_FAILURE, "%s: no link type?", name);
}

void
process_media_commands(void)
{

	if ((actions & (A_MEDIA|A_MEDIAOPT|A_MEDIAMODE)) == 0) {
		/* Nothing to do. */
		return;
	}

	/*
	 * Media already set up, and commands sanity-checked.  Set/clear
	 * any options, and we're ready to go.
	 */
	media_current |= mediaopt_set;
	media_current &= ~mediaopt_clear;

	estrlcpy(ifr.ifr_name, name, sizeof(ifr.ifr_name));
	ifr.ifr_media = media_current;

	if (ioctl(s, SIOCSIFMEDIA, &ifr) == -1)
		err(EXIT_FAILURE, "SIOCSIFMEDIA");
}

void
setmedia(const char *val, int d)
{
	int type, subtype, inst;

	init_current_media();

	/* Only one media command may be given. */
	if (actions & A_MEDIA)
		errx(EXIT_FAILURE, "only one `media' command may be issued");

	/* Must not come after mode commands */
	if (actions & A_MEDIAMODE)
		errx(EXIT_FAILURE,
		    "may not issue `media' after `mode' commands");

	/* Must not come after mediaopt commands */
	if (actions & A_MEDIAOPT)
		errx(EXIT_FAILURE,
		    "may not issue `media' after `mediaopt' commands");

	/*
	 * No need to check if `instance' has been issued; setmediainst()
	 * craps out if `media' has not been specified.
	 */

	type = IFM_TYPE(media_current);
	inst = IFM_INST(media_current);

	/* Look up the subtype. */
	subtype = get_media_subtype(type, val);
	if (subtype == -1)
		media_error(type, val, "subtype");

	/* Build the new current media word. */
	media_current = IFM_MAKEWORD(type, subtype, 0, inst);

	/* Media will be set after other processing is complete. */
}

void
setmediaopt(const char *val, int d)
{
	char *invalid;

	init_current_media();

	/* Can only issue `mediaopt' once. */
	if (actions & A_MEDIAOPTSET)
		errx(EXIT_FAILURE, "only one `mediaopt' command may be issued");

	/* Can't issue `mediaopt' if `instance' has already been issued. */
	if (actions & A_MEDIAINST)
		errx(EXIT_FAILURE, "may not issue `mediaopt' after `instance'");

	mediaopt_set = get_media_options(media_current, val, &invalid);
	if (mediaopt_set == -1)
		media_error(media_current, invalid, "option");

	/* Media will be set after other processing is complete. */
}

void
unsetmediaopt(const char *val, int d)
{
	char *invalid;

	init_current_media();

	/* Can only issue `-mediaopt' once. */
	if (actions & A_MEDIAOPTCLR)
		errx(EXIT_FAILURE,
		    "only one `-mediaopt' command may be issued");

	/* May not issue `media' and `-mediaopt'. */
	if (actions & A_MEDIA)
		errx(EXIT_FAILURE,
		    "may not issue both `media' and `-mediaopt'");

	/*
	 * No need to check for A_MEDIAINST, since the test for A_MEDIA
	 * implicitly checks for A_MEDIAINST.
	 */

	mediaopt_clear = get_media_options(media_current, val, &invalid);
	if (mediaopt_clear == -1)
		media_error(media_current, invalid, "option");

	/* Media will be set after other processing is complete. */
}

void
setmediainst(const char *val, int d)
{
	int type, subtype, options, inst;

	init_current_media();

	/* Can only issue `instance' once. */
	if (actions & A_MEDIAINST)
		errx(EXIT_FAILURE, "only one `instance' command may be issued");

	/* Must have already specified `media' */
	if ((actions & A_MEDIA) == 0)
		errx(EXIT_FAILURE, "must specify `media' before `instance'");

	type = IFM_TYPE(media_current);
	subtype = IFM_SUBTYPE(media_current);
	options = IFM_OPTIONS(media_current);

	inst = atoi(val);
	if (inst < 0 || inst > IFM_INST_MAX)
		errx(EXIT_FAILURE, "invalid media instance: %s", val);

	media_current = IFM_MAKEWORD(type, subtype, options, inst);

	/* Media will be set after other processing is complete. */
}

void
setmediamode(const char *val, int d)
{
	int type, subtype, options, inst, mode;

	init_current_media();

	/* Can only issue `mode' once. */
	if (actions & A_MEDIAMODE)
		errx(EXIT_FAILURE, "only one `mode' command may be issued");

	type = IFM_TYPE(media_current);
	subtype = IFM_SUBTYPE(media_current);
	options = IFM_OPTIONS(media_current);
	inst = IFM_INST(media_current);

	mode = get_media_mode(type, val);
	if (mode == -1)
		media_error(type, val, "mode");

	media_current = IFM_MAKEWORD(type, subtype, options, inst) | mode;

	/* Media will be set after other processing is complete. */
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
carrier(void)
{
	struct ifmediareq ifmr;

	(void) memset(&ifmr, 0, sizeof(ifmr));
	estrlcpy(ifmr.ifm_name, name, sizeof(ifmr.ifm_name));

	if (ioctl(s, SIOCGIFMEDIA, &ifmr) == -1) {
		/*
		 * Interface doesn't support SIOC{G,S}IFMEDIA;
		 * assume ok.
		 */
		return 0;
	}
	if ((ifmr.ifm_status & IFM_AVALID) == 0) {
		/*
		 * Interface doesn't report media-valid status.
		 * assume ok.
		 */
		return 0;
	}
	/* otherwise, return ok for active, not-ok if not active. */
	return !(ifmr.ifm_status & IFM_ACTIVE);
}


const int ifm_status_valid_list[] = IFM_STATUS_VALID_LIST;

const struct ifmedia_status_description ifm_status_descriptions[] =
    IFM_STATUS_DESCRIPTIONS;

/*
 * Print the status of the interface.  If an address family was
 * specified, show it and it only; otherwise, show them all.
 */
void
status(const struct sockaddr_dl *sdl)
{
	const struct afswtch *p = afp;
	struct ifmediareq ifmr;
	struct ifdatareq ifdr;
	int *media_list, i;
	char hbuf[NI_MAXHOST];
	char fbuf[BUFSIZ];

	(void)snprintb(fbuf, sizeof(fbuf), IFFBITS, flags);
	printf("%s: flags=%s", name, &fbuf[2]);
	if (metric)
		printf(" metric %lu", metric);
	if (mtu)
		printf(" mtu %lu", mtu);
	printf("\n");

	if (g_ifcr.ifcr_capabilities) {
		(void)snprintb(fbuf, sizeof(fbuf), IFCAPBITS,
		    g_ifcr.ifcr_capabilities);
		printf("\tcapabilities=%s\n", &fbuf[2]);
		(void)snprintb(fbuf, sizeof(fbuf), IFCAPBITS,
		    g_ifcr.ifcr_capenable);
		printf("\tenabled=%s\n", &fbuf[2]);
	}

	ieee80211_status();
	vlan_status();
#ifndef INET_ONLY
	carp_status();
#endif
	tunnel_status();
	agr_status();

	if (sdl != NULL &&
	    getnameinfo((const struct sockaddr *)sdl, sdl->sdl_len,
		hbuf, sizeof(hbuf), NULL, 0, NI_NUMERICHOST) == 0 &&
	    hbuf[0] != '\0')
		printf("\taddress: %s\n", hbuf);

	(void) memset(&ifmr, 0, sizeof(ifmr));
	estrlcpy(ifmr.ifm_name, name, sizeof(ifmr.ifm_name));

	if (ioctl(s, SIOCGIFMEDIA, &ifmr) == -1) {
		/*
		 * Interface doesn't support SIOC{G,S}IFMEDIA.
		 */
		goto iface_stats;
	}

	if (ifmr.ifm_count == 0) {
		warnx("%s: no media types?", name);
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

	estrlcpy(ifdr.ifdr_name, name, sizeof(ifdr.ifdr_name));

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

	ieee80211_statistics();

 proto_status:
	if ((p = afp) != NULL) {
		(*p->af_status)(1);
	} else for (p = afs; p->af_name; p++) {
		ifr.ifr_addr.sa_family = p->af_af;
		(*p->af_status)(0);
	}
}

void
setifprefixlen(const char *addr, int d)
{
	if (*afp->af_getprefix)
		(*afp->af_getprefix)(addr, MASK);
	explicit_prefix = 1;
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
	exit(1);
}
