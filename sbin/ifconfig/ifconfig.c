/*	$NetBSD: ifconfig.c,v 1.141.4.2 2005/07/24 01:58:38 snj Exp $	*/

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
__RCSID("$NetBSD: ifconfig.c,v 1.141.4.2 2005/07/24 01:58:38 snj Exp $");
#endif
#endif /* not lint */

#include <sys/param.h>
#include <sys/socket.h>
#include <sys/ioctl.h>

#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_media.h>
#include <net/if_ether.h>
#include <net80211/ieee80211.h>
#include <net80211/ieee80211_ioctl.h>
#include <net/if_vlanvar.h>
#include <netinet/in.h>
#include <netinet/in_var.h>
#ifdef INET6
#include <netinet6/nd6.h>
#endif
#include <arpa/inet.h>

#include <netatalk/at.h>

#define	NSIP
#include <netns/ns.h>
#include <netns/ns_if.h>
#include <netdb.h>

#define EON
#include <netiso/iso.h>
#include <netiso/iso_var.h>
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

struct	ifreq		ifr, ridreq;
struct	ifaliasreq	addreq __attribute__((aligned(4)));
struct	in_aliasreq	in_addreq;
#ifdef INET6
struct	in6_ifreq	ifr6;
struct	in6_ifreq	in6_ridreq;
struct	in6_aliasreq	in6_addreq;
#endif
struct	iso_ifreq	iso_ridreq;
struct	iso_aliasreq	iso_addreq;
struct	sockaddr_in	netmask;
struct	netrange	at_nr;		/* AppleTalk net range */

char	name[30];
u_short	flags;
int	setaddr, setipdst, doalias;
u_long	metric, mtu;
int	clearaddr, s;
int	newaddr = -1;
int	conflicting = 0;
int	nsellength = 1;
int	af;
int	aflag, bflag, Cflag, dflag, lflag, mflag, sflag, uflag, vflag, zflag;
#ifdef INET6
int	Lflag;
#endif
int	explicit_prefix = 0;
u_int	vlan_tag = (u_int)-1;

struct ifcapreq g_ifcr;
int	g_ifcr_updated;

void 	notealias(const char *, int);
void 	notrailers(const char *, int);
void 	setifaddr(const char *, int);
void 	setifdstaddr(const char *, int);
void 	setifflags(const char *, int);
void	setifcaps(const char *, int);
void 	setifbroadaddr(const char *, int);
void 	setifipdst(const char *, int);
void 	setifmetric(const char *, int);
void 	setifmtu(const char *, int);
void	setifnwid(const char *, int);
void	setifnwkey(const char *, int);
void	setifbssid(const char *, int);
void	setifchan(const char *, int);
void	setifpowersave(const char *, int);
void	setifpowersavesleep(const char *, int);
void 	setifnetmask(const char *, int);
void	setifprefixlen(const char *, int);
void 	setnsellength(const char *, int);
void 	setsnpaoffset(const char *, int);
void	setatrange(const char *, int);
void	setatphase(const char *, int);
void	settunnel(const char *, const char *);
void	deletetunnel(const char *, int);
#ifdef INET6
void 	setia6flags(const char *, int);
void	setia6pltime(const char *, int);
void	setia6vltime(const char *, int);
void	setia6lifetime(const char *, const char *);
void	setia6eui64(const char *, int);
#endif
void	checkatrange(struct sockaddr_at *);
void	setmedia(const char *, int);
void	setmediamode(const char *, int);
void	setmediaopt(const char *, int);
void	unsetmediaopt(const char *, int);
void	setmediainst(const char *, int);
void	clone_create(const char *, int);
void	clone_destroy(const char *, int);
void	fixnsel(struct sockaddr_iso *);
void	setvlan(const char *, int);
void	setvlanif(const char *, int);
void	unsetvlanif(const char *, int);
int	main(int, char *[]);

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
	void	(*c_func2)(const char *, const char *);
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
	{ "ssid",	NEXTARG,	0,		setifnwid },
	{ "nwid",	NEXTARG,	0,		setifnwid },
	{ "nwkey",	NEXTARG,	0,		setifnwkey },
	{ "-nwkey",	-1,		0,		setifnwkey },
	{ "powersave",	1,		0,		setifpowersave },
	{ "-powersave",	0,		0,		setifpowersave },
	{ "powersavesleep", NEXTARG,	0,		setifpowersavesleep },
	{ "broadcast",	NEXTARG,	0,		setifbroadaddr },
	{ "ipdst",	NEXTARG,	0,		setifipdst },
	{ "prefixlen",	NEXTARG,	0,		setifprefixlen},
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
	{ "tunnel",	NEXTARG2,	0,		NULL,
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
	{ "ip4csum",	IFCAP_CSUM_IPv4,0,		setifcaps },
	{ "-ip4csum",	-IFCAP_CSUM_IPv4,0,		setifcaps },
	{ "tcp4csum",	IFCAP_CSUM_TCPv4,0,		setifcaps },
	{ "-tcp4csum",	-IFCAP_CSUM_TCPv4,0,		setifcaps },
	{ "udp4csum",	IFCAP_CSUM_UDPv4,0,		setifcaps },
	{ "-udp4csum",	-IFCAP_CSUM_UDPv4,0,		setifcaps },
	{ "tcp6csum",	IFCAP_CSUM_TCPv6,0,		setifcaps },
	{ "-tcp6csum",	-IFCAP_CSUM_TCPv6,0,		setifcaps },
	{ "udp6csum",	IFCAP_CSUM_UDPv6,0,		setifcaps },
	{ "-udp6csum",	-IFCAP_CSUM_UDPv6,0,		setifcaps },
	{ "tcp4csum-rx",IFCAP_CSUM_TCPv4_Rx,0,		setifcaps },
	{ "-tcp4csum-rx",-IFCAP_CSUM_TCPv4_Rx,0,	setifcaps },
	{ "udp4csum-rx",IFCAP_CSUM_UDPv4_Rx,0,		setifcaps },
	{ "-udp4csum-rx",-IFCAP_CSUM_UDPv4_Rx,0,	setifcaps },
	{ 0,		0,		0,		setifaddr },
	{ 0,		0,		0,		setifdstaddr },
};

void 	adjust_nsellength(void);
int	getinfo(struct ifreq *);
int	carrier(void);
void	getsock(int);
void	printall(const char *);
void	list_cloners(void);
int	prefix(void *, int);
void 	status(const struct sockaddr_dl *);
void 	usage(void);
const char *get_string(const char *, const char *, u_int8_t *, int *);
void	print_string(const u_int8_t *, int);
char	*sec2str(time_t);

void	print_media_word(int, const char *);
void	process_media_commands(void);
void	init_current_media(void);

/*
 * XNS support liberally adapted from code written at the University of
 * Maryland principally by James O'Toole and Chris Torek.
 */
void	in_alias(struct ifreq *);
void	in_status(int);
void 	in_getaddr(const char *, int);
void 	in_getprefix(const char *, int);
#ifdef INET6
void	in6_fillscopeid(struct sockaddr_in6 *sin6);
void	in6_alias(struct in6_ifreq *);
void	in6_status(int);
void 	in6_getaddr(const char *, int);
void 	in6_getprefix(const char *, int);
#endif
void	at_status(int);
void	at_getaddr(const char *, int);
void 	xns_status(int);
void 	xns_getaddr(const char *, int);
void 	iso_status(int);
void 	iso_getaddr(const char *, int);

void	ieee80211_status(void);
void	tunnel_status(void);
void	vlan_status(void);

/* Known address families */
struct afswtch {
	const char *af_name;
	short af_af;
	void (*af_status)(int);
	void (*af_getaddr)(const char *, int);
	void (*af_getprefix)(const char *, int);
	u_long af_difaddr;
	u_long af_aifaddr;
	u_long af_gifaddr;
	void *af_ridreq;
	void *af_addreq;
} afs[] = {
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
	{ "ns", AF_NS, xns_status, xns_getaddr, NULL,
	     SIOCDIFADDR, SIOCAIFADDR, SIOCGIFADDR, &ridreq, &addreq },
	{ "iso", AF_ISO, iso_status, iso_getaddr, NULL,
	     SIOCDIFADDR_ISO, SIOCAIFADDR_ISO, SIOCGIFADDR_ISO,
	     &iso_ridreq, &iso_addreq },
#endif	/* INET_ONLY */
	{ 0,	0,	    0,		0 }
};

struct afswtch *afp;	/*the address family being set or asked about*/

struct afswtch *lookup_af(const char *);

int
main(int argc, char *argv[])
{
	int ch;

	/* Parse command-line options */
	aflag = mflag = vflag = zflag = 0;
	while ((ch = getopt(argc, argv, "AabCdlmsuvz"
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
			afp = lookup_af(argv[0]);
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
		afp = lookup_af(argv[0]);
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
	(void) strncpy(ifr.ifr_name, name, sizeof(ifr.ifr_name));
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
	/* initialization */
	in6_addreq.ifra_lifetime.ia6t_pltime = ND6_INFINITE_LIFETIME;
	in6_addreq.ifra_lifetime.ia6t_vltime = ND6_INFINITE_LIFETIME;
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
		if (p->c_func != NULL || p->c_func2 != NULL) {
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
				(*p->c_func2)(argv[1], argv[2]);
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
		checkatrange((struct sockaddr_at *) &addreq.ifra_addr);

	if (setipdst && af==AF_NS) {
		struct nsip_req rq;
		int size = sizeof(rq);

		rq.rq_ns = addreq.ifra_addr;
		rq.rq_ip = addreq.ifra_dstaddr;

		if (setsockopt(s, 0, SO_NSIP_ROUTE, &rq, size) < 0)
			warn("encapsulation routing");
	}

#endif	/* INET_ONLY */

	if (clearaddr) {
		(void) strncpy(afp->af_ridreq, name, sizeof ifr.ifr_name);
		if (ioctl(s, afp->af_difaddr, afp->af_ridreq) == -1)
			err(EXIT_FAILURE, "SIOCDIFADDR");
	}
	if (newaddr > 0) {
		(void) strncpy(afp->af_addreq, name, sizeof ifr.ifr_name);
		if (ioctl(s, afp->af_aifaddr, afp->af_addreq) == -1)
			warn("SIOCAIFADDR");
	}

	if (g_ifcr_updated) {
		(void) strncpy(g_ifcr.ifcr_name, name,
		    sizeof(g_ifcr.ifcr_name));
		if (ioctl(s, SIOCSIFCAP, &g_ifcr) == -1)
			err(EXIT_FAILURE, "SIOCSIFCAP");
	}

	exit(0);
}

struct afswtch *
lookup_af(const char *cp)
{
	struct afswtch *a;

	for (a = afs; a->af_name != NULL; a++)
		if (strcmp(a->af_name, cp) == 0)
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
	strcpy(g_ifcr.ifcr_name, giifr->ifr_name);
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
		strncpy(paifr.ifr_name, ifa->ifa_name, sizeof(paifr.ifr_name));
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

	(void) strncpy(ifr.ifr_name, name, sizeof(ifr.ifr_name));
	if (ioctl(s, SIOCIFCREATE, &ifr) == -1)
		err(EXIT_FAILURE, "SIOCIFCREATE");
}

/*ARGSUSED*/
void
clone_destroy(const char *addr, int param)
{

	(void) strncpy(ifr.ifr_name, name, sizeof(ifr.ifr_name));
	if (ioctl(s, SIOCIFDESTROY, &ifr) == -1)
		err(EXIT_FAILURE, "SIOCIFDESTROY");
}

#define RIDADDR 0
#define ADDR	1
#define MASK	2
#define DSTADDR	3

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
		(void) strncpy(siifr->ifr_name, name, sizeof(siifr->ifr_name));
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
settunnel(const char *src, const char *dst)
{
	struct addrinfo hints, *srcres, *dstres;
	int ecode;
	struct if_laddrreq req;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = afp->af_af;
	hints.ai_socktype = SOCK_DGRAM;	/*dummy*/

	if ((ecode = getaddrinfo(src, NULL, &hints, &srcres)) != 0)
		errx(EXIT_FAILURE, "error in parsing address string: %s",
		    gai_strerror(ecode));

	if ((ecode = getaddrinfo(dst, NULL, &hints, &dstres)) != 0)
		errx(EXIT_FAILURE, "error in parsing address string: %s",
		    gai_strerror(ecode));

	if (srcres->ai_addr->sa_family != dstres->ai_addr->sa_family)
		errx(EXIT_FAILURE,
		    "source and destination address families do not match");

	if (srcres->ai_addrlen > sizeof(req.addr) ||
	    dstres->ai_addrlen > sizeof(req.dstaddr))
		errx(EXIT_FAILURE, "invalid sockaddr");

	memset(&req, 0, sizeof(req));
	strncpy(req.iflr_name, name, sizeof(req.iflr_name));
	memcpy(&req.addr, srcres->ai_addr, srcres->ai_addrlen);
	memcpy(&req.dstaddr, dstres->ai_addr, dstres->ai_addrlen);

#ifdef INET6
	if (req.addr.ss_family == AF_INET6) {
		struct sockaddr_in6 *s6, *d;

		s6 = (struct sockaddr_in6 *)&req.addr;
		d = (struct sockaddr_in6 *)&req.dstaddr;
		if (s6->sin6_scope_id != d->sin6_scope_id) {
			errx(EXIT_FAILURE, "scope mismatch");
			/* NOTREACHED */
		}
#ifdef __KAME__
		/* embed scopeid */
		if (s6->sin6_scope_id && 
		    (IN6_IS_ADDR_LINKLOCAL(&s6->sin6_addr) ||
		     IN6_IS_ADDR_MC_LINKLOCAL(&s6->sin6_addr))) {
			*(u_int16_t *)&s6->sin6_addr.s6_addr[2] =
			    htons(s6->sin6_scope_id);
		}
		if (d->sin6_scope_id && 
		    (IN6_IS_ADDR_LINKLOCAL(&d->sin6_addr) ||
		     IN6_IS_ADDR_MC_LINKLOCAL(&d->sin6_addr))) {
			*(u_int16_t *)&d->sin6_addr.s6_addr[2] =
			    htons(d->sin6_scope_id);
		}
#endif
	}
#endif

	if (ioctl(s, SIOCSLIFPHYADDR, &req) == -1)
		warn("SIOCSLIFPHYADDR");

	freeaddrinfo(srcres);
	freeaddrinfo(dstres);
}

/* ARGSUSED */
void
deletetunnel(const char *vname, int param)
{

	if (ioctl(s, SIOCDIFPHYADDR, &ifr) == -1)
		err(EXIT_FAILURE, "SIOCDIFPHYADDR");
}

void
setvlan(const char *val, int d)
{
	struct vlanreq vlr;

	if (strncmp(ifr.ifr_name, "vlan", 4) != 0 ||
	    !isdigit((unsigned char)ifr.ifr_name[4]))
		errx(EXIT_FAILURE,
		    "``vlan'' valid only with vlan(4) interfaces");

	vlan_tag = atoi(val);

	memset(&vlr, 0, sizeof(vlr));
	ifr.ifr_data = (void *)&vlr;

	if (ioctl(s, SIOCGETVLAN, &ifr) == -1)
		err(EXIT_FAILURE, "SIOCGETVLAN");

	vlr.vlr_tag = vlan_tag;

	if (ioctl(s, SIOCSETVLAN, &ifr) == -1)
		err(EXIT_FAILURE, "SIOCSETVLAN");
}

void
setvlanif(const char *val, int d)
{
	struct vlanreq vlr;

	if (strncmp(ifr.ifr_name, "vlan", 4) != 0 ||
	    !isdigit((unsigned char)ifr.ifr_name[4]))
		errx(EXIT_FAILURE,
		    "``vlanif'' valid only with vlan(4) interfaces");

	if (vlan_tag == (u_int)-1)
		errx(EXIT_FAILURE,
		    "must specify both ``vlan'' and ``vlanif''");

	memset(&vlr, 0, sizeof(vlr));
	ifr.ifr_data = (void *)&vlr;

	if (ioctl(s, SIOCGETVLAN, &ifr) == -1)
		err(EXIT_FAILURE, "SIOCGETVLAN");

	strlcpy(vlr.vlr_parent, val, sizeof(vlr.vlr_parent));
	vlr.vlr_tag = vlan_tag;

	if (ioctl(s, SIOCSETVLAN, &ifr) == -1)
		err(EXIT_FAILURE, "SIOCSETVLAN");
}

void
unsetvlanif(const char *val, int d)
{
	struct vlanreq vlr;

	if (strncmp(ifr.ifr_name, "vlan", 4) != 0 ||
	    !isdigit((unsigned char)ifr.ifr_name[4]))
		errx(EXIT_FAILURE,
		    "``vlanif'' valid only with vlan(4) interfaces");

	memset(&vlr, 0, sizeof(vlr));
	ifr.ifr_data = (void *)&vlr;

	if (ioctl(s, SIOCGETVLAN, &ifr) == -1)
		err(EXIT_FAILURE, "SIOCGETVLAN");

	vlr.vlr_parent[0] = '\0';
	vlr.vlr_tag = 0;

	if (ioctl(s, SIOCSETVLAN, &ifr) == -1)
		err(EXIT_FAILURE, "SIOCSETVLAN");
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

void
setifipdst(const char *addr, int d)
{
	in_getaddr(addr, DSTADDR);
	setipdst++;
	clearaddr = 0;
	newaddr = 0;
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
setifflags(const char *vname, int value)
{
	struct ifreq ifreq;

	(void) strncpy(ifreq.ifr_name, name, sizeof(ifreq.ifr_name));
 	if (ioctl(s, SIOCGIFFLAGS, &ifreq) == -1)
		err(EXIT_FAILURE, "SIOCGIFFLAGS");
 	flags = ifreq.ifr_flags;

	if (value < 0) {
		value = -value;
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

#ifdef INET6
void
setia6flags(const char *vname, int value)
{

	if (value < 0) {
		value = -value;
		in6_addreq.ifra_flags &= ~value;
	} else
		in6_addreq.ifra_flags |= value;
}

void
setia6pltime(const char *val, int d)
{

	setia6lifetime("pltime", val);
}

void
setia6vltime(const char *val, int d)
{

	setia6lifetime("vltime", val);
}

void
setia6lifetime(const char *cmd, const char *val)
{
	time_t newval, t;
	char *ep;

	t = time(NULL);
	newval = (time_t)strtoul(val, &ep, 0);
	if (val == ep)
		errx(EXIT_FAILURE, "invalid %s", cmd);
	if (afp->af_af != AF_INET6)
		errx(EXIT_FAILURE, "%s not allowed for the AF", cmd);
	if (strcmp(cmd, "vltime") == 0) {
		in6_addreq.ifra_lifetime.ia6t_expire = t + newval;
		in6_addreq.ifra_lifetime.ia6t_vltime = newval;
	} else if (strcmp(cmd, "pltime") == 0) {
		in6_addreq.ifra_lifetime.ia6t_preferred = t + newval;
		in6_addreq.ifra_lifetime.ia6t_pltime = newval;
	}
}

void
setia6eui64(const char *cmd, int val)
{
	struct ifaddrs *ifap, *ifa;
	const struct sockaddr_in6 *sin6 = NULL;
	const struct in6_addr *lladdr = NULL;
	struct in6_addr *in6;

	if (afp->af_af != AF_INET6)
		errx(EXIT_FAILURE, "%s not allowed for the AF", cmd);
 	in6 = (struct in6_addr *)&in6_addreq.ifra_addr.sin6_addr;
	if (memcmp(&in6addr_any.s6_addr[8], &in6->s6_addr[8], 8) != 0)
		errx(EXIT_FAILURE, "interface index is already filled");
	if (getifaddrs(&ifap) != 0)
		err(EXIT_FAILURE, "getifaddrs");
	for (ifa = ifap; ifa; ifa = ifa->ifa_next) {
		if (ifa->ifa_addr->sa_family == AF_INET6 &&
		    strcmp(ifa->ifa_name, name) == 0) {
			sin6 = (const struct sockaddr_in6 *)ifa->ifa_addr;
			if (IN6_IS_ADDR_LINKLOCAL(&sin6->sin6_addr)) {
				lladdr = &sin6->sin6_addr;
				break;
			}
		}
	}
	if (!lladdr)
		errx(EXIT_FAILURE, "could not determine link local address"); 

 	memcpy(&in6->s6_addr[8], &lladdr->s6_addr[8], 8);

	freeifaddrs(ifap);
}
#endif

void
setifmetric(const char *val, int d)
{
	char *ep = NULL;

	(void) strncpy(ifr.ifr_name, name, sizeof (ifr.ifr_name));
	ifr.ifr_metric = strtoul(val, &ep, 10);
	if (!ep || *ep)
		errx(EXIT_FAILURE, "%s: invalid metric", val);
	if (ioctl(s, SIOCSIFMETRIC, &ifr) == -1)
		warn("SIOCSIFMETRIC");
}

void
setifmtu(const char *val, int d)
{
	char *ep = NULL;

	(void)strncpy(ifr.ifr_name, name, sizeof(ifr.ifr_name));
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

void
setifnwid(const char *val, int d)
{
	struct ieee80211_nwid nwid;
	int len;

	len = sizeof(nwid.i_nwid);
	if (get_string(val, NULL, nwid.i_nwid, &len) == NULL)
		return;
	nwid.i_len = len;
	(void)strncpy(ifr.ifr_name, name, sizeof(ifr.ifr_name));
	ifr.ifr_data = (void *)&nwid;
	if (ioctl(s, SIOCS80211NWID, &ifr) == -1)
		warn("SIOCS80211NWID");
}

void
setifbssid(const char *val, int d)
{
	struct ieee80211_bssid bssid;
	struct ether_addr *ea;

	if (d != 0) {
		/* no BSSID is especially desired */
		memset(&bssid.i_bssid, 0, sizeof(bssid.i_bssid));
	} else {
		ea = ether_aton(val);
		if (ea == NULL) {
			warnx("malformed BSSID: %s", val);
			return;
		}
		memcpy(&bssid.i_bssid, ea->ether_addr_octet,
		    sizeof(bssid.i_bssid));
	}
	(void)strncpy(bssid.i_name, name, sizeof(bssid.i_name));
	if (ioctl(s, SIOCS80211BSSID, &bssid) == -1)
		warn("SIOCS80211BSSID");
}

void
setifchan(const char *val, int d)
{
	struct ieee80211chanreq channel;
	int chan;

	if (d != 0)
		chan = IEEE80211_CHAN_ANY;
	else {
		chan = atoi(val);
		if (chan < 0 || chan > 0xffff) {
			warnx("invalid channel: %s", val);
			return;
		}
	}

	(void)strncpy(channel.i_name, name, sizeof(channel.i_name));
	channel.i_channel = (u_int16_t) chan;
	if (ioctl(s, SIOCS80211CHANNEL, &channel) == -1)
		warn("SIOCS80211CHANNEL");
}

void
setifnwkey(const char *val, int d)
{
	struct ieee80211_nwkey nwkey;
	int i;
	u_int8_t keybuf[IEEE80211_WEP_NKID][16];

	nwkey.i_wepon = IEEE80211_NWKEY_WEP;
	nwkey.i_defkid = 1;
	for (i = 0; i < IEEE80211_WEP_NKID; i++) {
		nwkey.i_key[i].i_keylen = sizeof(keybuf[i]);
		nwkey.i_key[i].i_keydat = keybuf[i];
	}
	if (d != 0) {
		/* disable WEP encryption */
		nwkey.i_wepon = 0;
		i = 0;
	} else if (strcasecmp("persist", val) == 0) {
		/* use all values from persistent memory */
		nwkey.i_wepon |= IEEE80211_NWKEY_PERSIST;
		nwkey.i_defkid = 0;
		for (i = 0; i < IEEE80211_WEP_NKID; i++)
			nwkey.i_key[i].i_keylen = -1;
	} else if (strncasecmp("persist:", val, 8) == 0) {
		val += 8;
		/* program keys in persistent memory */
		nwkey.i_wepon |= IEEE80211_NWKEY_PERSIST;
		goto set_nwkey;
	} else {
  set_nwkey:
		if (isdigit((unsigned char)val[0]) && val[1] == ':') {
			/* specifying a full set of four keys */
			nwkey.i_defkid = val[0] - '0';
			val += 2;
			for (i = 0; i < IEEE80211_WEP_NKID; i++) {
				val = get_string(val, ",", keybuf[i],
				    &nwkey.i_key[i].i_keylen);
				if (val == NULL)
					return;
			}
			if (*val != '\0') {
				warnx("SIOCS80211NWKEY: too many keys.");
				return;
			}
		} else {
			val = get_string(val, NULL, keybuf[0],
			    &nwkey.i_key[0].i_keylen);
			if (val == NULL)
				return;
			i = 1;
		}
	}
	for (; i < IEEE80211_WEP_NKID; i++)
		nwkey.i_key[i].i_keylen = 0;
	(void)strncpy(nwkey.i_name, name, sizeof(nwkey.i_name));
	if (ioctl(s, SIOCS80211NWKEY, &nwkey) == -1)
		warn("SIOCS80211NWKEY");
}

void
setifpowersave(const char *val, int d)
{
	struct ieee80211_power power;

	(void)strncpy(power.i_name, name, sizeof(power.i_name));
	if (ioctl(s, SIOCG80211POWER, &power) == -1) {
		warn("SIOCG80211POWER");
		return;
	}

	power.i_enabled = d;
	if (ioctl(s, SIOCS80211POWER, &power) == -1)
		warn("SIOCS80211POWER");
}

void
setifpowersavesleep(const char *val, int d)
{
	struct ieee80211_power power;

	(void)strncpy(power.i_name, name, sizeof(power.i_name));
	if (ioctl(s, SIOCG80211POWER, &power) == -1) {
		warn("SIOCG80211POWER");
		return;
	}

	power.i_maxsleep = atoi(val);
	if (ioctl(s, SIOCS80211POWER, &power) == -1)
		warn("SIOCS80211POWER");
}

void
ieee80211_status(void)
{
	int i, nwkey_verbose;
	struct ieee80211_nwid nwid;
	struct ieee80211_nwkey nwkey;
	struct ieee80211_power power;
	u_int8_t keybuf[IEEE80211_WEP_NKID][16];
	struct ieee80211_bssid bssid;
	struct ieee80211chanreq channel;
	struct ether_addr ea;
	static const u_int8_t zero_macaddr[IEEE80211_ADDR_LEN];

	memset(&ifr, 0, sizeof(ifr));
	ifr.ifr_data = (void *)&nwid;
	(void)strncpy(ifr.ifr_name, name, sizeof(ifr.ifr_name));
	if (ioctl(s, SIOCG80211NWID, &ifr) == -1)
		return;
	if (nwid.i_len > IEEE80211_NWID_LEN) {
		warnx("SIOCG80211NWID: wrong length of nwid (%d)", nwid.i_len);
		return;
	}
	printf("\tssid ");
	print_string(nwid.i_nwid, nwid.i_len);
	memset(&nwkey, 0, sizeof(nwkey));
	(void)strncpy(nwkey.i_name, name, sizeof(nwkey.i_name));
	/* show nwkey only when WEP is enabled */
	if (ioctl(s, SIOCG80211NWKEY, &nwkey) == -1 ||
	    nwkey.i_wepon == 0) {
		printf("\n");
		goto skip_wep;
	}

	printf(" nwkey ");
	/* try to retrieve WEP keys */
	for (i = 0; i < IEEE80211_WEP_NKID; i++) {
		nwkey.i_key[i].i_keydat = keybuf[i];
		nwkey.i_key[i].i_keylen = sizeof(keybuf[i]);
	}
	if (ioctl(s, SIOCG80211NWKEY, &nwkey) == -1) {
		printf("*****");
	} else {
		nwkey_verbose = 0;
		/* check to see non default key or multiple keys defined */
		if (nwkey.i_defkid != 1) {
			nwkey_verbose = 1;
		} else {
			for (i = 1; i < IEEE80211_WEP_NKID; i++) {
				if (nwkey.i_key[i].i_keylen != 0) {
					nwkey_verbose = 1;
					break;
				}
			}
		}
		/* check extra ambiguity with keywords */
		if (!nwkey_verbose) {
			if (nwkey.i_key[0].i_keylen >= 2 &&
			    isdigit(nwkey.i_key[0].i_keydat[0]) &&
			    nwkey.i_key[0].i_keydat[1] == ':')
				nwkey_verbose = 1;
			else if (nwkey.i_key[0].i_keylen >= 7 &&
			    strncasecmp("persist", nwkey.i_key[0].i_keydat, 7)
			    == 0)
				nwkey_verbose = 1;
		}
		if (nwkey_verbose)
			printf("%d:", nwkey.i_defkid);
		for (i = 0; i < IEEE80211_WEP_NKID; i++) {
			if (i > 0)
				printf(",");
			if (nwkey.i_key[i].i_keylen < 0)
				printf("persist");
			else
				print_string(nwkey.i_key[i].i_keydat,
				    nwkey.i_key[i].i_keylen);
			if (!nwkey_verbose)
				break;
		}
	}
	printf("\n");

 skip_wep:
	(void)strncpy(power.i_name, name, sizeof(power.i_name));
	if (ioctl(s, SIOCG80211POWER, &power) == -1)
		goto skip_power;
	printf("\tpowersave ");
	if (power.i_enabled)
		printf("on (%dms sleep)", power.i_maxsleep);
	else
		printf("off");
	printf("\n");

 skip_power:
	(void)strncpy(bssid.i_name, name, sizeof(bssid.i_name));
	if (ioctl(s, SIOCG80211BSSID, &bssid) == -1)
		return;
	(void)strncpy(channel.i_name, name, sizeof(channel.i_name));
	if (ioctl(s, SIOCG80211CHANNEL, &channel) == -1)
		return;
	if (memcmp(bssid.i_bssid, zero_macaddr, IEEE80211_ADDR_LEN) == 0) {
		if (channel.i_channel != (u_int16_t)-1)
			printf("\tchan %d\n", channel.i_channel);
	} else {
		memcpy(ea.ether_addr_octet, bssid.i_bssid,
		    sizeof(ea.ether_addr_octet));
		printf("\tbssid %s", ether_ntoa(&ea));
		if (channel.i_channel != IEEE80211_CHAN_ANY)
			printf(" chan %d", channel.i_channel);
		printf("\n");
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
		(void) strncpy(ifmr.ifm_name, name, sizeof(ifmr.ifm_name));

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

	strncpy(ifr.ifr_name, name, sizeof(ifr.ifr_name));
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
	(void) strncpy(ifmr.ifm_name, name, sizeof(ifmr.ifm_name));

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


#define	IFFBITS \
"\020\1UP\2BROADCAST\3DEBUG\4LOOPBACK\5POINTOPOINT\6NOTRAILERS\7RUNNING\10NOARP\
\11PROMISC\12ALLMULTI\13OACTIVE\14SIMPLEX\15LINK0\16LINK1\17LINK2\20MULTICAST"

#define	IFCAPBITS \
"\020\1IP4CSUM\2TCP4CSUM\3UDP4CSUM\4TCP6CSUM\5UDP6CSUM\6TCP4CSUM_Rx\7UDP4CSUM_Rx"

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
	struct afswtch *p = afp;
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
	tunnel_status();

	if (sdl != NULL &&
	    getnameinfo((struct sockaddr *)sdl, sdl->sdl_len,
		hbuf, sizeof(hbuf), NULL, 0, NI_NUMERICHOST) == 0 &&
	    hbuf[0] != '\0')
		printf("\taddress: %s\n", hbuf);

	(void) memset(&ifmr, 0, sizeof(ifmr));
	(void) strncpy(ifmr.ifm_name, name, sizeof(ifmr.ifm_name));

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

	(void) strncpy(ifdr.ifdr_name, name, sizeof(ifdr.ifdr_name));

	if (ioctl(s, zflag ? SIOCZIFDATA:SIOCGIFDATA, &ifdr) == -1) {
		err(EXIT_FAILURE, zflag ? "SIOCZIFDATA" : "SIOCGIFDATA");
	} else {
		struct if_data * const ifi = &ifdr.ifdr_data;
#define	PLURAL(n)	((n) == 1 ? "" : "s")
		printf("\tinput: %llu packet%s, %llu byte%s",
		    (unsigned long long) ifi->ifi_ipackets,
		    PLURAL(ifi->ifi_ipackets),
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
		printf("\n\toutput: %llu packet%s, %llu byte%s",
		    (unsigned long long) ifi->ifi_opackets,
		    PLURAL(ifi->ifi_opackets),
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
	}

 proto_status:
	if ((p = afp) != NULL) {
		(*p->af_status)(1);
	} else for (p = afs; p->af_name; p++) {
		ifr.ifr_addr.sa_family = p->af_af;
		(*p->af_status)(0);
	}
}

void
tunnel_status(void)
{
	char psrcaddr[NI_MAXHOST];
	char pdstaddr[NI_MAXHOST];
	const char *ver = "";
#ifdef NI_WITHSCOPEID
	const int niflag = NI_NUMERICHOST | NI_WITHSCOPEID;
#else
	const int niflag = NI_NUMERICHOST;
#endif
	struct if_laddrreq req;

	psrcaddr[0] = pdstaddr[0] = '\0';

	memset(&req, 0, sizeof(req));
	strncpy(req.iflr_name, name, IFNAMSIZ);
	if (ioctl(s, SIOCGLIFPHYADDR, &req) == -1)
		return;
#ifdef INET6
	if (req.addr.ss_family == AF_INET6)
		in6_fillscopeid((struct sockaddr_in6 *)&req.addr);
#endif
	getnameinfo((struct sockaddr *)&req.addr, req.addr.ss_len,
	    psrcaddr, sizeof(psrcaddr), 0, 0, niflag);
#ifdef INET6
	if (req.addr.ss_family == AF_INET6)
		ver = "6";
#endif

#ifdef INET6
	if (req.dstaddr.ss_family == AF_INET6)
		in6_fillscopeid((struct sockaddr_in6 *)&req.dstaddr);
#endif
	getnameinfo((struct sockaddr *)&req.dstaddr, req.dstaddr.ss_len,
	    pdstaddr, sizeof(pdstaddr), 0, 0, niflag);

	printf("\ttunnel inet%s %s --> %s\n", ver, psrcaddr, pdstaddr);
}

void
vlan_status(void)
{
	struct vlanreq vlr;

	if (strncmp(ifr.ifr_name, "vlan", 4) != 0 ||
	    !isdigit((unsigned char)ifr.ifr_name[4]))
		return;

	memset(&vlr, 0, sizeof(vlr));
	ifr.ifr_data = (void *)&vlr;

	if (ioctl(s, SIOCGETVLAN, &ifr) == -1)
		return;

	if (vlr.vlr_tag || vlr.vlr_parent[0] != '\0')
		printf("\tvlan: %d parent: %s\n",
		    vlr.vlr_tag, vlr.vlr_parent[0] == '\0' ?
		    "<none>" : vlr.vlr_parent);
}

void
in_alias(struct ifreq *creq)
{
	struct sockaddr_in *iasin;
	int alias;

	if (lflag)
		return;

	alias = 1;

	/* Get the non-alias address for this interface. */
	getsock(AF_INET);
	if (s < 0) {
		if (errno == EPROTONOSUPPORT)
			return;
		err(EXIT_FAILURE, "socket");
	}
	(void) memset(&ifr, 0, sizeof(ifr));
	(void) strncpy(ifr.ifr_name, name, sizeof(ifr.ifr_name));
	if (ioctl(s, SIOCGIFADDR, &ifr) == -1) {
		if (errno == EADDRNOTAVAIL || errno == EAFNOSUPPORT) {
			return;
		} else
			warn("SIOCGIFADDR");
	}
	/* If creq and ifr are the same address, this is not an alias. */
	if (memcmp(&ifr.ifr_addr, &creq->ifr_addr,
		   sizeof(creq->ifr_addr)) == 0)
		alias = 0;
	(void) memset(&in_addreq, 0, sizeof(in_addreq));
	(void) strncpy(in_addreq.ifra_name, name, sizeof(in_addreq.ifra_name));
	memcpy(&in_addreq.ifra_addr, &creq->ifr_addr,
	    sizeof(in_addreq.ifra_addr));
	if (ioctl(s, SIOCGIFALIAS, &in_addreq) == -1) {
		if (errno == EADDRNOTAVAIL || errno == EAFNOSUPPORT) {
			return;
		} else
			warn("SIOCGIFALIAS");
	}

	iasin = &in_addreq.ifra_addr;
	printf("\tinet %s%s", alias ? "alias " : "", inet_ntoa(iasin->sin_addr));

	if (flags & IFF_POINTOPOINT) {
		iasin = &in_addreq.ifra_dstaddr;
		printf(" -> %s", inet_ntoa(iasin->sin_addr));
	}

	iasin = &in_addreq.ifra_mask;
	printf(" netmask 0x%x", ntohl(iasin->sin_addr.s_addr));

	if (flags & IFF_BROADCAST) {
		iasin = &in_addreq.ifra_broadaddr;
		printf(" broadcast %s", inet_ntoa(iasin->sin_addr));
	}
	printf("\n");
}

void
in_status(int force)
{
	struct ifaddrs *ifap, *ifa;
	struct ifreq isifr;

	if (getifaddrs(&ifap) != 0)
		err(EXIT_FAILURE, "getifaddrs");
	for (ifa = ifap; ifa; ifa = ifa->ifa_next) {
		if (strcmp(name, ifa->ifa_name) != 0)
			continue;
		if (ifa->ifa_addr->sa_family != AF_INET)
			continue;
		if (sizeof(isifr.ifr_addr) < ifa->ifa_addr->sa_len)
			continue;

		memset(&isifr, 0, sizeof(isifr));
		strncpy(isifr.ifr_name, ifa->ifa_name, sizeof(isifr.ifr_name));
		memcpy(&isifr.ifr_addr, ifa->ifa_addr, ifa->ifa_addr->sa_len);
		in_alias(&isifr);
	}
	freeifaddrs(ifap);
}

void
setifprefixlen(const char *addr, int d)
{
	if (*afp->af_getprefix)
		(*afp->af_getprefix)(addr, MASK);
	explicit_prefix = 1;
}

#ifdef INET6
void
in6_fillscopeid(struct sockaddr_in6 *sin6)
{
#if defined(__KAME__) && defined(KAME_SCOPEID)
	if (IN6_IS_ADDR_LINKLOCAL(&sin6->sin6_addr)) {
		sin6->sin6_scope_id =
			ntohs(*(u_int16_t *)&sin6->sin6_addr.s6_addr[2]);
		sin6->sin6_addr.s6_addr[2] = sin6->sin6_addr.s6_addr[3] = 0;
	}
#endif
}

/* XXX not really an alias */
void
in6_alias(struct in6_ifreq *creq)
{
	struct sockaddr_in6 *sin6;
	char hbuf[NI_MAXHOST];
	u_int32_t scopeid;
#ifdef NI_WITHSCOPEID
	const int niflag = NI_NUMERICHOST | NI_WITHSCOPEID;
#else
	const int niflag = NI_NUMERICHOST;
#endif

	/* Get the non-alias address for this interface. */
	getsock(AF_INET6);
	if (s < 0) {
		if (errno == EPROTONOSUPPORT)
			return;
		err(EXIT_FAILURE, "socket");
	}

	sin6 = (struct sockaddr_in6 *)&creq->ifr_addr;

	in6_fillscopeid(sin6);
	scopeid = sin6->sin6_scope_id;
	if (getnameinfo((struct sockaddr *)sin6, sin6->sin6_len,
			hbuf, sizeof(hbuf), NULL, 0, niflag))
		strlcpy(hbuf, "", sizeof(hbuf));	/* some message? */
	printf("\tinet6 %s", hbuf);

	if (flags & IFF_POINTOPOINT) {
		(void) memset(&ifr6, 0, sizeof(ifr6));
		(void) strncpy(ifr6.ifr_name, name, sizeof(ifr6.ifr_name));
		ifr6.ifr_addr = creq->ifr_addr;
		if (ioctl(s, SIOCGIFDSTADDR_IN6, &ifr6) == -1) {
			if (errno != EADDRNOTAVAIL)
				warn("SIOCGIFDSTADDR_IN6");
			(void) memset(&ifr6.ifr_addr, 0, sizeof(ifr6.ifr_addr));
			ifr6.ifr_addr.sin6_family = AF_INET6;
			ifr6.ifr_addr.sin6_len = sizeof(struct sockaddr_in6);
		}
		sin6 = (struct sockaddr_in6 *)&ifr6.ifr_addr;
		in6_fillscopeid(sin6);
		hbuf[0] = '\0';
		if (getnameinfo((struct sockaddr *)sin6, sin6->sin6_len,
				hbuf, sizeof(hbuf), NULL, 0, niflag))
			strlcpy(hbuf, "", sizeof(hbuf)); /* some message? */
		printf(" -> %s", hbuf);
	}

	(void) memset(&ifr6, 0, sizeof(ifr6));
	(void) strncpy(ifr6.ifr_name, name, sizeof(ifr6.ifr_name));
	ifr6.ifr_addr = creq->ifr_addr;
	if (ioctl(s, SIOCGIFNETMASK_IN6, &ifr6) == -1) {
		if (errno != EADDRNOTAVAIL)
			warn("SIOCGIFNETMASK_IN6");
	} else {
		sin6 = (struct sockaddr_in6 *)&ifr6.ifr_addr;
		printf(" prefixlen %d", prefix(&sin6->sin6_addr,
					       sizeof(struct in6_addr)));
	}

	(void) memset(&ifr6, 0, sizeof(ifr6));
	(void) strncpy(ifr6.ifr_name, name, sizeof(ifr6.ifr_name));
	ifr6.ifr_addr = creq->ifr_addr;
	if (ioctl(s, SIOCGIFAFLAG_IN6, &ifr6) == -1) {
		if (errno != EADDRNOTAVAIL)
			warn("SIOCGIFAFLAG_IN6");
	} else {
		if (ifr6.ifr_ifru.ifru_flags6 & IN6_IFF_ANYCAST)
			printf(" anycast");
		if (ifr6.ifr_ifru.ifru_flags6 & IN6_IFF_TENTATIVE)
			printf(" tentative");
		if (ifr6.ifr_ifru.ifru_flags6 & IN6_IFF_DUPLICATED)
			printf(" duplicated");
		if (ifr6.ifr_ifru.ifru_flags6 & IN6_IFF_DETACHED)
			printf(" detached");
		if (ifr6.ifr_ifru.ifru_flags6 & IN6_IFF_DEPRECATED)
			printf(" deprecated");
	}

	if (scopeid)
		printf(" scopeid 0x%x", scopeid);

	if (Lflag) {
		struct in6_addrlifetime *lifetime;
		(void) memset(&ifr6, 0, sizeof(ifr6));
		(void) strncpy(ifr6.ifr_name, name, sizeof(ifr6.ifr_name));
		ifr6.ifr_addr = creq->ifr_addr;
		lifetime = &ifr6.ifr_ifru.ifru_lifetime;
		if (ioctl(s, SIOCGIFALIFETIME_IN6, &ifr6) == -1) {
			if (errno != EADDRNOTAVAIL)
				warn("SIOCGIFALIFETIME_IN6");
		} else if (lifetime->ia6t_preferred || lifetime->ia6t_expire) {
			time_t t = time(NULL);
			printf(" pltime ");
			if (lifetime->ia6t_preferred) {
				printf("%s", lifetime->ia6t_preferred < t
					? "0"
					: sec2str(lifetime->ia6t_preferred - t));
			} else
				printf("infty");

			printf(" vltime ");
			if (lifetime->ia6t_expire) {
				printf("%s", lifetime->ia6t_expire < t
					? "0"
					: sec2str(lifetime->ia6t_expire - t));
			} else
				printf("infty");
		}
	}

	printf("\n");
}

void
in6_status(int force)
{
	struct ifaddrs *ifap, *ifa;
	struct in6_ifreq isifr;

	if (getifaddrs(&ifap) != 0)
		err(EXIT_FAILURE, "getifaddrs");
	for (ifa = ifap; ifa; ifa = ifa->ifa_next) {
		if (strcmp(name, ifa->ifa_name) != 0)
			continue;
		if (ifa->ifa_addr->sa_family != AF_INET6)
			continue;
		if (sizeof(isifr.ifr_addr) < ifa->ifa_addr->sa_len)
			continue;

		memset(&isifr, 0, sizeof(isifr));
		strncpy(isifr.ifr_name, ifa->ifa_name, sizeof(isifr.ifr_name));
		memcpy(&isifr.ifr_addr, ifa->ifa_addr, ifa->ifa_addr->sa_len);
		in6_alias(&isifr);
	}
	freeifaddrs(ifap);
}
#endif /*INET6*/

#ifndef INET_ONLY

void
at_status(int force)
{
	struct sockaddr_at *sat, null_sat;
	struct netrange *nr;

	getsock(AF_APPLETALK);
	if (s < 0) {
		if (errno == EPROTONOSUPPORT)
			return;
		err(EXIT_FAILURE, "socket");
	}
	(void) memset(&ifr, 0, sizeof(ifr));
	(void) strncpy(ifr.ifr_name, name, sizeof(ifr.ifr_name));
	if (ioctl(s, SIOCGIFADDR, &ifr) == -1) {
		if (errno == EADDRNOTAVAIL || errno == EAFNOSUPPORT) {
			if (!force)
				return;
			(void) memset(&ifr.ifr_addr, 0, sizeof(ifr.ifr_addr));
		} else
			warn("SIOCGIFADDR");
	}
	(void) strncpy(ifr.ifr_name, name, sizeof ifr.ifr_name);
	sat = (struct sockaddr_at *)&ifr.ifr_addr;

	(void) memset(&null_sat, 0, sizeof(null_sat));

	nr = (struct netrange *) &sat->sat_zero;
	printf("\tatalk %d.%d range %d-%d phase %d",
	    ntohs(sat->sat_addr.s_net), sat->sat_addr.s_node,
	    ntohs(nr->nr_firstnet), ntohs(nr->nr_lastnet), nr->nr_phase);
	if (flags & IFF_POINTOPOINT) {
		if (ioctl(s, SIOCGIFDSTADDR, &ifr) == -1) {
			if (errno == EADDRNOTAVAIL)
			    (void) memset(&ifr.ifr_addr, 0,
				sizeof(ifr.ifr_addr));
			else
			    warn("SIOCGIFDSTADDR");
		}
		(void) strncpy(ifr.ifr_name, name, sizeof (ifr.ifr_name));
		sat = (struct sockaddr_at *)&ifr.ifr_dstaddr;
		if (!sat)
			sat = &null_sat;
		printf("--> %d.%d",
		    ntohs(sat->sat_addr.s_net), sat->sat_addr.s_node);
	}
	if (flags & IFF_BROADCAST) {
		/* note RTAX_BRD overlap with IFF_POINTOPOINT */
		sat = (struct sockaddr_at *)&ifr.ifr_broadaddr;
		if (sat)
			printf(" broadcast %d.%d", ntohs(sat->sat_addr.s_net),
			    sat->sat_addr.s_node);
	}
	printf("\n");
}

void
xns_status(int force)
{
	struct sockaddr_ns *sns;

	getsock(AF_NS);
	if (s < 0) {
		if (errno == EPROTONOSUPPORT)
			return;
		err(EXIT_FAILURE, "socket");
	}
	(void) memset(&ifr, 0, sizeof(ifr));
	(void) strncpy(ifr.ifr_name, name, sizeof(ifr.ifr_name));
	if (ioctl(s, SIOCGIFADDR, &ifr) == -1) {
		if (errno == EADDRNOTAVAIL || errno == EAFNOSUPPORT) {
			if (!force)
				return;
			memset(&ifr.ifr_addr, 0, sizeof(ifr.ifr_addr));
		} else
			warn("SIOCGIFADDR");
	}
	(void) strncpy(ifr.ifr_name, name, sizeof ifr.ifr_name);
	sns = (struct sockaddr_ns *)&ifr.ifr_addr;
	printf("\tns %s ", ns_ntoa(sns->sns_addr));
	if (flags & IFF_POINTOPOINT) { /* by W. Nesheim@Cornell */
		if (ioctl(s, SIOCGIFDSTADDR, &ifr) == -1) {
			if (errno == EADDRNOTAVAIL)
			    memset(&ifr.ifr_addr, 0, sizeof(ifr.ifr_addr));
			else
			    warn("SIOCGIFDSTADDR");
		}
		(void) strncpy(ifr.ifr_name, name, sizeof (ifr.ifr_name));
		sns = (struct sockaddr_ns *)&ifr.ifr_dstaddr;
		printf("--> %s ", ns_ntoa(sns->sns_addr));
	}
	printf("\n");
}

void
iso_status(int force)
{
	struct sockaddr_iso *siso;
	struct iso_ifreq isoifr;

	getsock(AF_ISO);
	if (s < 0) {
		if (errno == EPROTONOSUPPORT)
			return;
		err(EXIT_FAILURE, "socket");
	}
	(void) memset(&isoifr, 0, sizeof(isoifr));
	(void) strncpy(isoifr.ifr_name, name, sizeof(isoifr.ifr_name));
	if (ioctl(s, SIOCGIFADDR_ISO, &isoifr) == -1) {
		if (errno == EADDRNOTAVAIL || errno == EAFNOSUPPORT) {
			if (!force)
				return;
			(void) memset(&isoifr.ifr_Addr, 0,
			    sizeof(isoifr.ifr_Addr));
		} else
			warn("SIOCGIFADDR_ISO");
	}
	(void) strncpy(isoifr.ifr_name, name, sizeof isoifr.ifr_name);
	siso = &isoifr.ifr_Addr;
	printf("\tiso %s ", iso_ntoa(&siso->siso_addr));
	if (ioctl(s, SIOCGIFNETMASK_ISO, &isoifr) == -1) {
		if (errno == EADDRNOTAVAIL)
			memset(&isoifr.ifr_Addr, 0, sizeof(isoifr.ifr_Addr));
		else
			warn("SIOCGIFNETMASK_ISO");
	} else {
		if (siso->siso_len > offsetof(struct sockaddr_iso, siso_addr))
			siso->siso_addr.isoa_len = siso->siso_len
			    - offsetof(struct sockaddr_iso, siso_addr);
		printf("\n\t\tnetmask %s ", iso_ntoa(&siso->siso_addr));
	}
	if (flags & IFF_POINTOPOINT) {
		if (ioctl(s, SIOCGIFDSTADDR_ISO, &isoifr) == -1) {
			if (errno == EADDRNOTAVAIL)
			    memset(&isoifr.ifr_Addr, 0,
				sizeof(isoifr.ifr_Addr));
			else
			    warn("SIOCGIFDSTADDR_ISO");
		}
		(void) strncpy(isoifr.ifr_name, name, sizeof (isoifr.ifr_name));
		siso = &isoifr.ifr_Addr;
		printf("--> %s ", iso_ntoa(&siso->siso_addr));
	}
	printf("\n");
}

#endif	/* INET_ONLY */

#define SIN(x) ((struct sockaddr_in *) &(x))
struct sockaddr_in *sintab[] = {
    SIN(ridreq.ifr_addr), SIN(in_addreq.ifra_addr),
    SIN(in_addreq.ifra_mask), SIN(in_addreq.ifra_broadaddr)};

void
in_getaddr(const char *str, int which)
{
	struct sockaddr_in *gasin = sintab[which];
	struct hostent *hp;
	struct netent *np;

	gasin->sin_len = sizeof(*gasin);
	if (which != MASK)
		gasin->sin_family = AF_INET;

	if (which == ADDR) {
		char *p = NULL;
		if ((p = strrchr(str, '/')) != NULL) {
			*p = '\0';
			in_getprefix(p + 1, MASK);
		}
	}

	if (inet_aton(str, &gasin->sin_addr) == 0) {
		if ((hp = gethostbyname(str)) != NULL)
			(void) memcpy(&gasin->sin_addr, hp->h_addr, hp->h_length);
		else if ((np = getnetbyname(str)) != NULL)
			gasin->sin_addr = inet_makeaddr(np->n_net, INADDR_ANY);
		else
			errx(EXIT_FAILURE, "%s: bad value", str);
	}
}

void
in_getprefix(const char *plen, int which)
{
	register struct sockaddr_in *igsin = sintab[which];
	register u_char *cp;
	int len = strtol(plen, (char **)NULL, 10);

	if ((len < 0) || (len > 32))
		errx(EXIT_FAILURE, "%s: bad value", plen);
	igsin->sin_len = sizeof(*igsin);
	if (which != MASK)
		igsin->sin_family = AF_INET;
	if ((len == 0) || (len == 32)) {
		memset(&igsin->sin_addr, 0xff, sizeof(struct in_addr));
		return;
	}
	memset((void *)&igsin->sin_addr, 0x00, sizeof(igsin->sin_addr));
	for (cp = (u_char *)&igsin->sin_addr; len > 7; len -= 8)
		*cp++ = 0xff;
	if (len)
		*cp = 0xff << (8 - len);
}

#ifdef INET6
#define SIN6(x) ((struct sockaddr_in6 *) &(x))
struct sockaddr_in6 *sin6tab[] = {
    SIN6(in6_ridreq.ifr_addr), SIN6(in6_addreq.ifra_addr),
    SIN6(in6_addreq.ifra_prefixmask), SIN6(in6_addreq.ifra_dstaddr)};

void
in6_getaddr(const char *str, int which)
{
#if defined(__KAME__) && defined(KAME_SCOPEID)
	struct sockaddr_in6 *sin6 = sin6tab[which];
	struct addrinfo hints, *res;
	int error;
	char *slash = NULL;

	if (which == ADDR) {
		if ((slash = strrchr(str, '/')) != NULL)
			*slash = '\0';
	}

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET6;
	hints.ai_socktype = SOCK_DGRAM;
#if 0 /* in_getaddr() allows FQDN */
	hints.ai_flags = AI_NUMERICHOST;
#endif
	error = getaddrinfo(str, "0", &hints, &res);
	if (error && slash) {
		/* try again treating the '/' as part of the name */
		*slash = '/';
		slash = NULL;
		error = getaddrinfo(str, "0", &hints, &res);
	}
	if (error)
		errx(EXIT_FAILURE, "%s: %s", str, gai_strerror(error));
	if (res->ai_next)
		errx(EXIT_FAILURE, "%s: resolved to multiple addresses", str);
	if (res->ai_addrlen != sizeof(struct sockaddr_in6))
		errx(EXIT_FAILURE, "%s: bad value", str);
	memcpy(sin6, res->ai_addr, res->ai_addrlen);
	freeaddrinfo(res);
	if (IN6_IS_ADDR_LINKLOCAL(&sin6->sin6_addr) && sin6->sin6_scope_id) {
		*(u_int16_t *)&sin6->sin6_addr.s6_addr[2] =
			htons(sin6->sin6_scope_id);
		sin6->sin6_scope_id = 0;
	}
	if (slash) {
		in6_getprefix(slash + 1, MASK);
		explicit_prefix = 1;
	}
#else
	struct sockaddr_in6 *gasin = sin6tab[which];

	gasin->sin6_len = sizeof(*gasin);
	if (which != MASK)
		gasin->sin6_family = AF_INET6;

	if (which == ADDR) {
		char *p = NULL;
		if((p = strrchr(str, '/')) != NULL) {
			*p = '\0';
			in6_getprefix(p + 1, MASK);
			explicit_prefix = 1;
		}
	}

	if (inet_pton(AF_INET6, str, &gasin->sin6_addr) != 1)
		errx(EXIT_FAILURE, "%s: bad value", str);
#endif
}

void
in6_getprefix(const char *plen, int which)
{
	register struct sockaddr_in6 *gpsin = sin6tab[which];
	register u_char *cp;
	int len = strtol(plen, (char **)NULL, 10);

	if ((len < 0) || (len > 128))
		errx(EXIT_FAILURE, "%s: bad value", plen);
	gpsin->sin6_len = sizeof(*gpsin);
	if (which != MASK)
		gpsin->sin6_family = AF_INET6;
	if ((len == 0) || (len == 128)) {
		memset(&gpsin->sin6_addr, 0xff, sizeof(struct in6_addr));
		return;
	}
	memset((void *)&gpsin->sin6_addr, 0x00, sizeof(gpsin->sin6_addr));
	for (cp = (u_char *)&gpsin->sin6_addr; len > 7; len -= 8)
		*cp++ = 0xff;
	if (len)
		*cp = 0xff << (8 - len);
}

int
prefix(void *val, int size)
{
	register u_char *pname = (u_char *)val;
	register int byte, bit, plen = 0;

	for (byte = 0; byte < size; byte++, plen += 8)
		if (pname[byte] != 0xff)
			break;
	if (byte == size)
		return (plen);
	for (bit = 7; bit != 0; bit--, plen++)
		if (!(pname[byte] & (1 << bit)))
			break;
	for (; bit != 0; bit--)
		if (pname[byte] & (1 << bit))
			return(0);
	byte++;
	for (; byte < size; byte++)
		if (pname[byte])
			return(0);
	return (plen);
}
#endif /*INET6*/

#ifndef INET_ONLY
void
at_getaddr(const char *addr, int which)
{
	struct sockaddr_at *sat = (struct sockaddr_at *) &addreq.ifra_addr;
	u_int net, node;

	sat->sat_family = AF_APPLETALK;
	sat->sat_len = sizeof(*sat);
	if (which == MASK)
		errx(EXIT_FAILURE, "AppleTalk does not use netmasks");
	if (sscanf(addr, "%u.%u", &net, &node) != 2
	    || net == 0 || net > 0xffff || node == 0 || node > 0xfe)
		errx(EXIT_FAILURE, "%s: illegal address", addr);
	sat->sat_addr.s_net = htons(net);
	sat->sat_addr.s_node = node;
}

void
setatrange(const char *range, int d)
{
	u_short	first = 123, last = 123;

	if (sscanf(range, "%hu-%hu", &first, &last) != 2
	    || first == 0 /* || first > 0xffff */
	    || last == 0 /* || last > 0xffff */ || first > last)
		errx(EXIT_FAILURE, "%s: illegal net range: %u-%u", range,
		    first, last);
	at_nr.nr_firstnet = htons(first);
	at_nr.nr_lastnet = htons(last);
}

void
setatphase(const char *phase, int d)
{
	if (!strcmp(phase, "1"))
		at_nr.nr_phase = 1;
	else if (!strcmp(phase, "2"))
		at_nr.nr_phase = 2;
	else
		errx(EXIT_FAILURE, "%s: illegal phase", phase);
}

void
checkatrange(struct sockaddr_at *sat)
{
	if (at_nr.nr_phase == 0)
		at_nr.nr_phase = 2;	/* Default phase 2 */
	if (at_nr.nr_firstnet == 0)
		at_nr.nr_firstnet =	/* Default range of one */
		at_nr.nr_lastnet = sat->sat_addr.s_net;
	printf("\tatalk %d.%d range %d-%d phase %d\n",
	ntohs(sat->sat_addr.s_net), sat->sat_addr.s_node,
	ntohs(at_nr.nr_firstnet), ntohs(at_nr.nr_lastnet), at_nr.nr_phase);
	if ((u_short) ntohs(at_nr.nr_firstnet) >
			(u_short) ntohs(sat->sat_addr.s_net)
		    || (u_short) ntohs(at_nr.nr_lastnet) <
			(u_short) ntohs(sat->sat_addr.s_net))
		errx(EXIT_FAILURE, "AppleTalk address is not in range");
	*((struct netrange *) &sat->sat_zero) = at_nr;
}

#define SNS(x) ((struct sockaddr_ns *) &(x))
struct sockaddr_ns *snstab[] = {
    SNS(ridreq.ifr_addr), SNS(addreq.ifra_addr),
    SNS(addreq.ifra_mask), SNS(addreq.ifra_broadaddr)};

void
xns_getaddr(const char *addr, int which)
{
	struct sockaddr_ns *sns = snstab[which];

	sns->sns_family = AF_NS;
	sns->sns_len = sizeof(*sns);
	sns->sns_addr = ns_addr(addr);
	if (which == MASK)
		puts("Attempt to set XNS netmask will be ineffectual");
}

#define SISO(x) ((struct sockaddr_iso *) &(x))
struct sockaddr_iso *sisotab[] = {
    SISO(iso_ridreq.ifr_Addr), SISO(iso_addreq.ifra_addr),
    SISO(iso_addreq.ifra_mask), SISO(iso_addreq.ifra_dstaddr)};

void
iso_getaddr(const char *addr, int which)
{
	struct sockaddr_iso *siso = sisotab[which];
	siso->siso_addr = *iso_addr(addr);

	if (which == MASK) {
		siso->siso_len = TSEL(siso) - (char *)(siso);
		siso->siso_nlen = 0;
	} else {
		siso->siso_len = sizeof(*siso);
		siso->siso_family = AF_ISO;
	}
}

void
setsnpaoffset(const char *val, int d)
{
	iso_addreq.ifra_snpaoffset = atoi(val);
}

void
setnsellength(const char *val, int d)
{
	nsellength = atoi(val);
	if (nsellength < 0)
		errx(EXIT_FAILURE, "Negative NSEL length is absurd");
	if (afp == 0 || afp->af_af != AF_ISO)
		errx(EXIT_FAILURE, "Setting NSEL length valid only for iso");
}

void
fixnsel(struct sockaddr_iso *siso)
{
	if (siso->siso_family == 0)
		return;
	siso->siso_tlen = nsellength;
}

void
adjust_nsellength(void)
{
	fixnsel(sisotab[RIDADDR]);
	fixnsel(sisotab[ADDR]);
	fixnsel(sisotab[DSTADDR]);
}

#endif	/* INET_ONLY */

void
usage(void)
{
	const char *progname = getprogname();

	fprintf(stderr,
	    "usage: %s [-m] [-v] [-z] "
#ifdef INET6
		"[-L] "
#endif
		"interface\n"
		"\t[ af [ address [ dest_addr ] ] [ netmask mask ] [ prefixlen n ]\n"
		"\t\t[ alias | -alias ] ]\n"
		"\t[ up ] [ down ] [ metric n ] [ mtu n ]\n"
		"\t[ nwid network_id ] [ nwkey network_key | -nwkey ]\n"
		"\t[ powersave | -powersave ] [ powersavesleep duration ]\n"
		"\t[ [ af ] tunnel src_addr dest_addr ] [ deletetunnel ]\n"
		"\t[ arp | -arp ]\n"
		"\t[ media type ] [ mediaopt opts ] [ -mediaopt opts ] "
		"[ instance minst ]\n"
		"\t[ vlan n vlanif i ]\n"
		"\t[ anycast | -anycast ] [ deprecated | -deprecated ]\n"
		"\t[ tentative | -tentative ] [ pltime n ] [ vltime n ] [ eui64 ]\n"
		"\t[ link0 | -link0 ] [ link1 | -link1 ] [ link2 | -link2 ]\n"
		"       %s -a [-b] [-m] [-d] [-u] [-v] [-z] [ af ]\n"
		"       %s -l [-b] [-d] [-u] [-s]\n"
		"       %s -C\n"
		"       %s interface create\n"
		"       %s interface destroy\n",
		progname, progname, progname, progname, progname, progname);
	exit(1);
}

#ifdef INET6
char *
sec2str(total)
	time_t total;
{
	static char result[256];
	int days, hours, mins, secs;
	int first = 1;
	char *p = result;
	char *end = &result[sizeof(result)];
	int n;

	if (0) {	/*XXX*/
		days = total / 3600 / 24;
		hours = (total / 3600) % 24;
		mins = (total / 60) % 60;
		secs = total % 60;

		if (days) {
			first = 0;
			n = snprintf(p, end - p, "%dd", days);
			if (n < 0 || n >= end - p)
				return(result);
			p += n;
		}
		if (!first || hours) {
			first = 0;
			n = snprintf(p, end - p, "%dh", hours);
			if (n < 0 || n >= end - p)
				return(result);
			p += n;
		}
		if (!first || mins) {
			first = 0;
			n = snprintf(p, end - p, "%dm", mins);
			if (n < 0 || n >= end - p)
				return(result);
			p += n;
		}
		snprintf(p, end - p, "%ds", secs);
	} else
		snprintf(p, end - p, "%lu", (u_long)total);

	return(result);
}
#endif
