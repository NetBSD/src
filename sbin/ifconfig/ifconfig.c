/*	$NetBSD: ifconfig.c,v 1.32 1997/04/03 02:07:59 christos Exp $	*/

/*
 * Copyright (c) 1997 Jason R. Thorpe.
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed for the NetBSD Project
 *	by Jason R. Thorpe.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
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

#ifndef lint
static char copyright[] =
"@(#) Copyright (c) 1983, 1993\n\
	The Regents of the University of California.  All rights reserved.\n";
#endif /* not lint */

#ifndef lint
#if 0
static char sccsid[] = "@(#)ifconfig.c	8.2 (Berkeley) 2/16/94";
#else
static char rcsid[] = "$NetBSD: ifconfig.c,v 1.32 1997/04/03 02:07:59 christos Exp $";
#endif
#endif /* not lint */

#include <sys/param.h>
#include <sys/socket.h>
#include <sys/ioctl.h>

#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_media.h>
#include <netinet/in.h>
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

struct	ifreq		ifr, ridreq;
struct	ifaliasreq	addreq;
struct	iso_aliasreq	iso_addreq;
struct	sockaddr_in	netmask;
struct	netrange	at_nr;		/* AppleTalk net range */

char	name[30];
int	flags, metric, setaddr, setipdst, doalias;
int	clearaddr, s;
int	newaddr = 1;
int	nsellength = 1;
int	af;
int	mflag, lflag;

void 	notealias __P((char *, int));
void 	notrailers __P((char *, int));
void 	setifaddr __P((char *, int));
void 	setifdstaddr __P((char *, int));
void 	setifflags __P((char *, int));
void 	setifbroadaddr __P((char *, int));
void 	setifipdst __P((char *, int));
void 	setifmetric __P((char *, int));
void 	setifnetmask __P((char *, int));
void 	setnsellength __P((char *, int));
void 	setsnpaoffset __P((char *, int));
void	setatrange __P((char *, int));
void	setatphase __P((char *, int));
void	checkatrange __P ((struct sockaddr_at *));
void	setmedia __P((char *, int));
void	setmediaopt __P((char *, int));
void	unsetmediaopt __P((char *, int));
void	fixnsel __P((struct sockaddr_iso *));
int	main __P((int, char *[]));

#define	NEXTARG		0xffffff

struct	cmd {
	char	*c_name;
	int	c_parameter;		/* NEXTARG means next argv */
	void	(*c_func) __P((char *, int));
} cmds[] = {
	{ "up",		IFF_UP,		setifflags } ,
	{ "down",	-IFF_UP,	setifflags },
	{ "trailers",	-1,		notrailers },
	{ "-trailers",	1,		notrailers },
	{ "arp",	-IFF_NOARP,	setifflags },
	{ "-arp",	IFF_NOARP,	setifflags },
	{ "debug",	IFF_DEBUG,	setifflags },
	{ "-debug",	-IFF_DEBUG,	setifflags },
	{ "alias",	IFF_UP,		notealias },
	{ "-alias",	-IFF_UP,	notealias },
	{ "delete",	-IFF_UP,	notealias },
#ifdef notdef
#define	EN_SWABIPS	0x1000
	{ "swabips",	EN_SWABIPS,	setifflags },
	{ "-swabips",	-EN_SWABIPS,	setifflags },
#endif
	{ "netmask",	NEXTARG,	setifnetmask },
	{ "metric",	NEXTARG,	setifmetric },
	{ "broadcast",	NEXTARG,	setifbroadaddr },
	{ "ipdst",	NEXTARG,	setifipdst },
#ifndef INET_ONLY
	{ "range",	NEXTARG,	setatrange },
	{ "phase",	NEXTARG,	setatphase },
	{ "snpaoffset",	NEXTARG,	setsnpaoffset },
	{ "nsellength",	NEXTARG,	setnsellength },
#endif	/* INET_ONLY */
	{ "link0",	IFF_LINK0,	setifflags } ,
	{ "-link0",	-IFF_LINK0,	setifflags } ,
	{ "link1",	IFF_LINK1,	setifflags } ,
	{ "-link1",	-IFF_LINK1,	setifflags } ,
	{ "link2",	IFF_LINK2,	setifflags } ,
	{ "-link2",	-IFF_LINK2,	setifflags } ,
	{ "media",	NEXTARG,	setmedia },
	{ "mediaopt",	NEXTARG,	setmediaopt },
	{ "-mediaopt",	NEXTARG,	unsetmediaopt },
	{ 0,		0,		setifaddr },
	{ 0,		0,		setifdstaddr },
};

void 	adjust_nsellength __P((void));
int	getinfo __P((struct ifreq *));
void	getsock __P((int));
void	printall __P((void));
void 	printb __P((char *, unsigned short, char *));
void 	status __P((const u_int8_t *, int));
void 	usage __P((void));

void	domediaopt __P((char *, int));
int	get_media_subtype __P((int, char *));
int	get_media_options __P((int, char *));
int	lookup_media_word __P((struct ifmedia_description *, char *));
void	print_media_word __P((int));

/*
 * XNS support liberally adapted from code written at the University of
 * Maryland principally by James O'Toole and Chris Torek.
 */
void	in_status __P((int));
void 	in_getaddr __P((char *, int));
void	at_status __P((int));
void	at_getaddr __P((char *, int));
void 	xns_status __P((int));
void 	xns_getaddr __P((char *, int));
void 	iso_status __P((int));
void 	iso_getaddr __P((char *, int));

/* Known address families */
struct afswtch {
	char *af_name;
	short af_af;
	void (*af_status) __P((int));
	void (*af_getaddr) __P((char *, int));
	u_long af_difaddr;
	u_long af_aifaddr;
	caddr_t af_ridreq;
	caddr_t af_addreq;
} afs[] = {
#define C(x) ((caddr_t) &x)
	{ "inet", AF_INET, in_status, in_getaddr,
	     SIOCDIFADDR, SIOCAIFADDR, C(ridreq), C(addreq) },
#ifndef INET_ONLY	/* small version, for boot media */
	{ "atalk", AF_APPLETALK, at_status, at_getaddr,
	     SIOCDIFADDR, SIOCAIFADDR, C(addreq), C(addreq) },
	{ "ns", AF_NS, xns_status, xns_getaddr,
	     SIOCDIFADDR, SIOCAIFADDR, C(ridreq), C(addreq) },
	{ "iso", AF_ISO, iso_status, iso_getaddr,
	     SIOCDIFADDR, SIOCAIFADDR, C(ridreq), C(iso_addreq) },
#endif	/* INET_ONLY */
	{ 0,	0,	    0,		0 }
};

struct afswtch *afp;	/*the address family being set or asked about*/

struct afswtch *lookup_af __P((const char *));

int
main(argc, argv)
	int argc;
	char *argv[];
{
	extern int optind;
	int ch, aflag;

	/* Parse command-line options */
	aflag = mflag = 0;
	while ((ch = getopt(argc, argv, "alm")) != -1) {
		switch (ch) {
		case 'a':
			aflag = 1;
			break;

		case 'l':
			lflag = 1;
			break;

		case 'm':
			mflag = 1;
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
	 * -a means "print status of all interfaces".
	 */
	if (lflag && (aflag || mflag || argc))
		usage();
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
		printall();
		exit(0);
	}

	/* Make sure there's an interface name. */
	if (argc < 1)
		usage();
	(void) strncpy(name, argv[0], sizeof(name));
	argc--; argv++;

	/* Check for address family. */
	afp = NULL;
	if (argc > 0) {
		afp = lookup_af(argv[0]);
		if (afp != NULL) {
			argv++;
			argc--;
		}
	}

	if (afp == NULL)
		afp = afs;
	af = ifr.ifr_addr.sa_family = afp->af_af;

	/* Get information about the interface. */
	(void) strncpy(ifr.ifr_name, name, sizeof(ifr.ifr_name));
	if (getinfo(&ifr) < 0)
		exit(1);

	/* No more arguments means interface status. */
	if (argc == 0) {
		status(NULL, 0);
		exit(0);
	}

	/* Process commands. */
	while (argc > 0) {
		register struct cmd *p;

		for (p = cmds; p->c_name; p++)
			if (strcmp(argv[0], p->c_name) == 0)
				break;
		if (p->c_name == 0 && setaddr)
			p++;	/* got src, do dst */
		if (p->c_func) {
			if (p->c_parameter == NEXTARG) {
				if (argc < 2)
					errx(1, "'%s' requires argument",
					    p->c_name);
				(*p->c_func)(argv[1], 0);
				argc--, argv++;
			} else
				(*p->c_func)(argv[0], p->c_parameter);
		}
		argc--, argv++;
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
		int ret;
		(void) strncpy(afp->af_ridreq, name, sizeof ifr.ifr_name);
		if ((ret = ioctl(s, afp->af_difaddr, afp->af_ridreq)) < 0) {
			if (errno == EADDRNOTAVAIL && (doalias >= 0)) {
				/* means no previous address for interface */
			} else
				warn("SIOCDIFADDR");
		}
	}
	if (newaddr) {
		(void) strncpy(afp->af_addreq, name, sizeof ifr.ifr_name);
		if (ioctl(s, afp->af_aifaddr, afp->af_addreq) < 0)
			warn("SIOCAIFADDR");
	}
	exit(0);
}

struct afswtch *
lookup_af(cp)
	const char *cp;
{
	struct afswtch *a;

	for (a = afs; a->af_name != NULL; a++)
		if (strcmp(a->af_name, cp) == 0)
			return (a);
	return (NULL);
}

void
getsock(naf)
	int naf;
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
getinfo(ifr)
	struct ifreq *ifr;
{

	getsock(af);
	if (s < 0)
		err(1, "socket");
	if (ioctl(s, SIOCGIFFLAGS, (caddr_t)ifr) < 0) {
		warn("SIOCGIFFLAGS %s", ifr->ifr_name);
		return (-1);
	}
	flags = ifr->ifr_flags;
	if (ioctl(s, SIOCGIFMETRIC, (caddr_t)ifr) < 0) {
		warn("SIOCGIFMETRIC %s", ifr->ifr_name);
		metric = 0;
	} else
		metric = ifr->ifr_metric;
	return (0);
}

void
printall()
{
	char inbuf[8192];
	const struct sockaddr_dl *sdl = NULL;
	struct ifconf ifc;
	struct ifreq ifreq, *ifr;
	int i, idx;

	ifc.ifc_len = sizeof(inbuf);
	ifc.ifc_buf = inbuf;
	getsock(af);
	if (s < 0)
		err(1, "socket");
	if (ioctl(s, SIOCGIFCONF, &ifc) < 0)
		err(1, "SIOCGIFCONF");
	ifr = ifc.ifc_req;
	ifreq.ifr_name[0] = '\0';
	for (i = 0, idx = 0; i < ifc.ifc_len; idx++) {
		ifr = (struct ifreq *)((caddr_t)ifc.ifc_req + i);
		i += sizeof(ifr->ifr_name) +
			(ifr->ifr_addr.sa_len > sizeof(struct sockaddr)
				? ifr->ifr_addr.sa_len
				: sizeof(struct sockaddr));
		if (ifr->ifr_addr.sa_family == AF_LINK)
			sdl = (const struct sockaddr_dl *) &ifr->ifr_addr;
		if (!strncmp(ifreq.ifr_name, ifr->ifr_name,
			     sizeof(ifr->ifr_name)))
			continue;
		(void) strncpy(name, ifr->ifr_name, sizeof(ifr->ifr_name));
		ifreq = *ifr;

		/*
		 * Are we just listing the interfaces?
		 */
		if (lflag) {
			if (idx)
				putchar(' ');
			printf(name);
			continue;
		}

		if (getinfo(&ifreq) < 0)
			continue;
		if (sdl == NULL) {
			status(NULL, 0);
		} else {
			status(LLADDR(sdl), sdl->sdl_alen);
			sdl = NULL;
		}
	}
	if (lflag)
		putchar('\n');
}

#define RIDADDR 0
#define ADDR	1
#define MASK	2
#define DSTADDR	3

/*ARGSUSED*/
void
setifaddr(addr, param)
	char *addr;
	int param;
{
	/*
	 * Delay the ioctl to set the interface addr until flags are all set.
	 * The address interpretation may depend on the flags,
	 * and the flags may change when the address is set.
	 */
	setaddr++;
	if (doalias == 0)
		clearaddr = 1;
	(*afp->af_getaddr)(addr, (doalias >= 0 ? ADDR : RIDADDR));
}

void
setifnetmask(addr, d)
	char *addr;
	int d;
{
	(*afp->af_getaddr)(addr, MASK);
}

void
setifbroadaddr(addr, d)
	char *addr;
	int d;
{
	(*afp->af_getaddr)(addr, DSTADDR);
}

void
setifipdst(addr, d)
	char *addr;
	int d;
{
	in_getaddr(addr, DSTADDR);
	setipdst++;
	clearaddr = 0;
	newaddr = 0;
}

#define rqtosa(x) (&(((struct ifreq *)(afp->x))->ifr_addr))
/*ARGSUSED*/
void
notealias(addr, param)
	char *addr;
	int param;
{
	if (setaddr && doalias == 0 && param < 0)
		(void) memcpy(rqtosa(af_ridreq), rqtosa(af_addreq),
		    rqtosa(af_addreq)->sa_len);
	doalias = param;
	if (param < 0) {
		clearaddr = 1;
		newaddr = 0;
	} else
		clearaddr = 0;
}

/*ARGSUSED*/
void
notrailers(vname, value)
	char *vname;
	int value;
{
	printf("Note: trailers are no longer sent, but always received\n");
}

/*ARGSUSED*/
void
setifdstaddr(addr, param)
	char *addr;
	int param;
{
	(*afp->af_getaddr)(addr, DSTADDR);
}

void
setifflags(vname, value)
	char *vname;
	int value;
{
 	if (ioctl(s, SIOCGIFFLAGS, (caddr_t)&ifr) < 0)
		err(1, "SIOCGIFFLAGS");
	(void) strncpy(ifr.ifr_name, name, sizeof (ifr.ifr_name));
 	flags = ifr.ifr_flags;

	if (value < 0) {
		value = -value;
		flags &= ~value;
	} else
		flags |= value;
	ifr.ifr_flags = flags;
	if (ioctl(s, SIOCSIFFLAGS, (caddr_t)&ifr) < 0)
		err(1, "SIOCSIFFLAGS");
}

void
setifmetric(val, d)
	char *val;
	int d;
{
	(void) strncpy(ifr.ifr_name, name, sizeof (ifr.ifr_name));
	ifr.ifr_metric = atoi(val);
	if (ioctl(s, SIOCSIFMETRIC, (caddr_t)&ifr) < 0)
		warn("SIOCSIFMETRIC");
}

void
setmedia(val, d)
	char *val;
	int d;
{
	struct ifmediareq ifmr;
	int first_type, subtype;

	(void) memset(&ifmr, 0, sizeof(ifmr));
	(void) strncpy(ifmr.ifm_name, name, sizeof(ifmr.ifm_name));

	ifmr.ifm_count = 1;
	ifmr.ifm_ulist = &first_type;
	if (ioctl(s, SIOCGIFMEDIA, (caddr_t)&ifmr) < 0) {
		/*
		 * If we get E2BIG, the kernel is telling us
		 * that there are more, so we can ignore it.
		 */
		if (errno != E2BIG)
			err(1, "SIOCGIFMEDIA");
	}

	if (ifmr.ifm_count == 0)
		errx(1, "%s: no media types?", name);

	/*
	 * We are primarily concerned with the top-level type.
	 * However, "current" may be only IFM_NONE, so we just look
	 * for the top-level type in the first "supported type"
	 * entry.
	 *
	 * (I'm assuming that all supported media types for a given
	 * interface will be the same top-level type..)
	 */
	subtype = get_media_subtype(IFM_TYPE(first_type), val);

	strncpy(ifr.ifr_name, name, sizeof(ifr.ifr_name));
	ifr.ifr_media = (ifmr.ifm_current & ~(IFM_NMASK|IFM_TMASK)) |
	    IFM_TYPE(first_type) | subtype;

	if (ioctl(s, SIOCSIFMEDIA, (caddr_t)&ifr) < 0)
		err(1, "SIOCSIFMEDIA");
}

void
setmediaopt(val, d)
	char *val;
	int d;
{

	domediaopt(val, 0);
}

void
unsetmediaopt(val, d)
	int d;
	char *val;
{

	domediaopt(val, 1);
}

void
domediaopt(val, clear)
	char *val;
	int clear;
{
	struct ifmediareq ifmr;
	int *mwords, options;

	(void) memset(&ifmr, 0, sizeof(ifmr));
	(void) strncpy(ifmr.ifm_name, name, sizeof(ifmr.ifm_name));

	/*
	 * We must go through the motions of reading all
	 * supported media because we need to know both
	 * the current media type and the top-level type.
	 */

	if (ioctl(s, SIOCGIFMEDIA, (caddr_t)&ifmr) < 0)
		err(1, "SIOCGIFMEDIA");

	if (ifmr.ifm_count == 0)
		errx(1, "%s: no media types?", name);

	mwords = (int *)malloc(ifmr.ifm_count * sizeof(int));
	if (mwords == NULL)
		err(1, "malloc");

	ifmr.ifm_ulist = mwords;
	if (ioctl(s, SIOCGIFMEDIA, (caddr_t)&ifmr) < 0)
		err(1, "SIOCGIFMEDIA");

	options = get_media_options(IFM_TYPE(mwords[0]), val);

	free(mwords);

	strncpy(ifr.ifr_name, name, sizeof(ifr.ifr_name));
	ifr.ifr_media = ifmr.ifm_current;
	if (clear)
		ifr.ifr_media &= ~options;
	else
		ifr.ifr_media |= options;

	if (ioctl(s, SIOCSIFMEDIA, (caddr_t)&ifr) < 0)
		err(1, "SIOCSIFMEDIA");
}

/**********************************************************************
 * A good chunk of this is duplicated from sys/net/ifmedia.c
 **********************************************************************/

struct ifmedia_description ifm_type_descriptions[] =
    IFM_TYPE_DESCRIPTIONS;

struct ifmedia_description ifm_subtype_ethernet_descriptions[] =
    IFM_SUBTYPE_ETHERNET_DESCRIPTIONS;

struct ifmedia_description ifm_subtype_ethernet_aliases[] =
    IFM_SUBTYPE_ETHERNET_ALIASES;

struct ifmedia_description ifm_subtype_ethernet_option_descriptions[] =
    IFM_SUBTYPE_ETHERNET_OPTION_DESCRIPTIONS;

struct ifmedia_description ifm_subtype_tokenring_descriptions[] =
    IFM_SUBTYPE_TOKENRING_DESCRIPTIONS;

struct ifmedia_description ifm_subtype_tokenring_aliases[] =
    IFM_SUBTYPE_TOKENRING_ALIASES;

struct ifmedia_description ifm_subtype_tokenring_option_descriptions[] =
    IFM_SUBTYPE_TOKENRING_OPTION_DESCRIPTIONS;

struct ifmedia_description ifm_subtype_fddi_descriptions[] =
    IFM_SUBTYPE_FDDI_DESCRIPTIONS;

struct ifmedia_description ifm_subtype_fddi_aliases[] =
    IFM_SUBTYPE_FDDI_ALIASES;

struct ifmedia_description ifm_subtype_fddi_option_descriptions[] =
    IFM_SUBTYPE_FDDI_OPTION_DESCRIPTIONS;

struct ifmedia_description ifm_subtype_shared_descriptions[] =
    IFM_SUBTYPE_SHARED_DESCRIPTIONS;

struct ifmedia_description ifm_subtype_shared_aliases[] =
    IFM_SUBTYPE_SHARED_ALIASES;

struct ifmedia_description ifm_shared_option_descriptions[] =
    IFM_SHARED_OPTION_DESCRIPTIONS;

struct ifmedia_type_to_subtype {
	struct {
		struct ifmedia_description *desc;
		int alias;
	} subtypes[5];
	struct {
		struct ifmedia_description *desc;
		int alias;
	} options[3];
};

/* must be in the same order as IFM_TYPE_DESCRIPTIONS */
struct ifmedia_type_to_subtype ifmedia_types_to_subtypes[] = {
	{
		{
			{ &ifm_subtype_shared_descriptions[0], 0 },
			{ &ifm_subtype_shared_aliases[0], 1 },
			{ &ifm_subtype_ethernet_descriptions[0], 0 },
			{ &ifm_subtype_ethernet_aliases[0], 1 },
			{ NULL, 0 },
		},
		{
			{ &ifm_shared_option_descriptions[0], 0 },
			{ &ifm_subtype_ethernet_option_descriptions[0], 1 },
			{ NULL, 0 },
		},
	},
	{
		{
			{ &ifm_subtype_shared_descriptions[0], 0 },
			{ &ifm_subtype_shared_aliases[0], 1 },
			{ &ifm_subtype_tokenring_descriptions[0], 0 },
			{ &ifm_subtype_tokenring_aliases[0], 1 },
			{ NULL, 0 },
		},
		{
			{ &ifm_shared_option_descriptions[0], 0 },
			{ &ifm_subtype_tokenring_option_descriptions[0], 1 },
			{ NULL, 0 },
		},
	},
	{
		{
			{ &ifm_subtype_shared_descriptions[0], 0 },
			{ &ifm_subtype_shared_aliases[0], 1 },
			{ &ifm_subtype_fddi_descriptions[0], 0 },
			{ &ifm_subtype_fddi_aliases[0], 1 },
			{ NULL, 0 },
		},
		{
			{ &ifm_shared_option_descriptions[0], 0 },
			{ &ifm_subtype_fddi_option_descriptions[0], 1 },
			{ NULL, 0 },
		},
	},
};

int
get_media_subtype(type, val)
	int type;
	char *val;
{
	struct ifmedia_description *desc;
	struct ifmedia_type_to_subtype *ttos;
	int rval, i;

	/* Find the top-level interface type. */
	for (desc = ifm_type_descriptions, ttos = ifmedia_types_to_subtypes;
	    desc->ifmt_string != NULL; desc++, ttos++)
		if (type == desc->ifmt_word)
			break;
	if (desc->ifmt_string == NULL)
		errx(1, "unknown media type 0x%x", type);

	for (i = 0; ttos->subtypes[i].desc != NULL; i++) {
		rval = lookup_media_word(ttos->subtypes[i].desc, val);
		if (rval != -1)
			return (rval);
	}
	errx(1, "unknown media subtype: %s", val);
	/* NOTREACHED */
}

int
get_media_options(type, val)
	int type;
	char *val;
{
	struct ifmedia_description *desc;
	struct ifmedia_type_to_subtype *ttos;
	char *optlist;
	int option = 0, i, rval = 0;

	/* We muck with the string, so copy it. */
	optlist = strdup(val);
	if (optlist == NULL)
		err(1, "strdup");
	val = optlist;

	/* Find the top-level interface type. */
	for (desc = ifm_type_descriptions, ttos = ifmedia_types_to_subtypes;
	    desc->ifmt_string != NULL; desc++, ttos++)
		if (type == desc->ifmt_word)
			break;
	if (desc->ifmt_string == NULL)
		errx(1, "unknown media type 0x%x", type);

	/*
	 * Look up the options in the user-provided comma-separated
	 * list.
	 */
	for (; (val = strtok(val, ",")) != NULL; val = NULL) {
		for (i = 0; ttos->options[i].desc != NULL; i++) {
			option = lookup_media_word(ttos->options[i].desc, val);
			if (option != -1)
				break;
		}
		if (option == 0)
			errx(1, "unknown option: %s", val);
		rval |= option;
	}

	free(optlist);
	return (rval);
}

int
lookup_media_word(desc, val)
	struct ifmedia_description *desc;
	char *val;
{

	for (; desc->ifmt_string != NULL; desc++)
		if (strcasecmp(desc->ifmt_string, val) == 0)
			return (desc->ifmt_word);

	return (-1);
}

void
print_media_word(ifmw)
	int ifmw;
{
	struct ifmedia_description *desc;
	struct ifmedia_type_to_subtype *ttos;
	int seen_option = 0, i;

	/* Find the top-level interface type. */
	for (desc = ifm_type_descriptions, ttos = ifmedia_types_to_subtypes;
	    desc->ifmt_string != NULL; desc++, ttos++)
		if (IFM_TYPE(ifmw) == desc->ifmt_word)
			break;
	if (desc->ifmt_string == NULL) {
		printf("<unknown type>");
		return;
	}

	/*
	 * Don't print the top-level type; it's not like we can
	 * change it, or anything.
	 */

	/* Find subtype. */
	for (i = 0; ttos->subtypes[i].desc != NULL; i++) {
		if (ttos->subtypes[i].alias)
			continue;
		for (desc = ttos->subtypes[i].desc;
		    desc->ifmt_string != NULL; desc++) {
			if (IFM_SUBTYPE(ifmw) == desc->ifmt_word)
				goto got_subtype;
		}
	}

	/* Falling to here means unknown subtype. */
	printf("<unknown subtype>");
	return;

 got_subtype:
	printf("%s", desc->ifmt_string);

	/* Find options. */
	for (i = 0; ttos->options[i].desc != NULL; i++) {
		if (ttos->options[i].alias)
			continue;
		for (desc = ttos->options[i].desc;
		    desc->ifmt_string != NULL; desc++) {
			if (ifmw & desc->ifmt_word) {
				if (seen_option == 0)
					printf(" <");
				printf("%s%s", seen_option++ ? "," : "",
				    desc->ifmt_string);
			}
		}
	}
	printf("%s", seen_option ? ">" : "");
}

/**********************************************************************
 * ...until here.
 **********************************************************************/

#define	IFFBITS \
"\020\1UP\2BROADCAST\3DEBUG\4LOOPBACK\5POINTOPOINT\6NOTRAILERS\7RUNNING\10NOARP\
\11PROMISC\12ALLMULTI\13OACTIVE\14SIMPLEX\15LINK0\16LINK1\17LINK2\20MULTICAST"

/*
 * Print the status of the interface.  If an address family was
 * specified, show it and it only; otherwise, show them all.
 */
void
status(ap, alen)
	const u_int8_t *ap;
	int alen;
{
	register struct afswtch *p = afp;
	struct ifmediareq ifmr;
	int *media_list, i;

	printf("%s: ", name);
	printb("flags", flags, IFFBITS);
	if (metric)
		printf(" metric %d", metric);
	putchar('\n');
	if (ap && alen > 0) {
		printf("\taddress:");
		for (i = 0; i < alen; i++, ap++)
			printf("%c%02x", i > 0 ? ':' : ' ', *ap);
		putchar('\n');
	}

	(void) memset(&ifmr, 0, sizeof(ifmr));
	(void) strncpy(ifmr.ifm_name, name, sizeof(ifmr.ifm_name));

	if (ioctl(s, SIOCGIFMEDIA, (caddr_t)&ifmr) < 0) {
		/*
		 * Interface doesn't support SIOC{G,S}IFMEDIA.
		 */
		goto proto_status;
	}

	if (ifmr.ifm_count == 0) {
		warnx("%s: no media types?", name);
		goto proto_status;
	}

	media_list = (int *)malloc(ifmr.ifm_count * sizeof(int));
	if (media_list == NULL)
		err(1, "malloc");
	ifmr.ifm_ulist = media_list;

	if (ioctl(s, SIOCGIFMEDIA, (caddr_t)&ifmr) < 0)
		err(1, "SIOCGIFMEDIA");

	printf("\tmedia: ");
	print_media_word(ifmr.ifm_current);
	if (ifmr.ifm_active != ifmr.ifm_current) {
		putchar(' ');
		putchar('(');
		print_media_word(ifmr.ifm_active);
		putchar(')');
	}

	if (ifmr.ifm_status & IFM_AVALID) {
		printf(" status: ");
		switch (IFM_TYPE(ifmr.ifm_active)) {
		case IFM_ETHER:
			if (ifmr.ifm_status & IFM_ACTIVE)
				printf("active");
			else
				printf("no carrier");
			break;

		case IFM_FDDI:
		case IFM_TOKEN:
			if (ifmr.ifm_status & IFM_ACTIVE)
				printf("inserted");
			else
				printf("no ring");
			break;
		}
	}

	putchar('\n');

	if (mflag) {
		printf("\tsupported media:");
		for (i = 0; i < ifmr.ifm_count; i++) {
			putchar(' ');
			print_media_word(media_list[i]);
		}
		putchar('\n');
	}

	free(media_list);

 proto_status:
	if ((p = afp) != NULL) {
		(*p->af_status)(1);
	} else for (p = afs; p->af_name; p++) {
		ifr.ifr_addr.sa_family = p->af_af;
		(*p->af_status)(0);
	}
}

void
in_status(force)
	int force;
{
	struct sockaddr_in *sin;

	getsock(AF_INET);
	if (s < 0) {
		if (errno == EPROTONOSUPPORT)
			return;
		err(1, "socket");
	}
	(void) memset(&ifr, 0, sizeof(ifr));
	(void) strncpy(ifr.ifr_name, name, sizeof(ifr.ifr_name));
	if (ioctl(s, SIOCGIFADDR, (caddr_t)&ifr) < 0) {
		if (errno == EADDRNOTAVAIL || errno == EAFNOSUPPORT) {
			if (!force)
				return;
			(void) memset(&ifr.ifr_addr, 0, sizeof(ifr.ifr_addr));
		} else
			warn("SIOCGIFADDR");
	}
	(void) strncpy(ifr.ifr_name, name, sizeof (ifr.ifr_name));
	sin = (struct sockaddr_in *)&ifr.ifr_addr;
	printf("\tinet %s ", inet_ntoa(sin->sin_addr));
	(void) strncpy(ifr.ifr_name, name, sizeof (ifr.ifr_name));
	if (ioctl(s, SIOCGIFNETMASK, (caddr_t)&ifr) < 0) {
		if (errno != EADDRNOTAVAIL)
			warn("SIOCGIFNETMASK");
		(void) memset(&ifr.ifr_addr, 0, sizeof(ifr.ifr_addr));
	} else
		netmask.sin_addr =
		    ((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr;
	if (flags & IFF_POINTOPOINT) {
		if (ioctl(s, SIOCGIFDSTADDR, (caddr_t)&ifr) < 0) {
			if (errno == EADDRNOTAVAIL)
			    (void) memset(&ifr.ifr_addr, 0,
				sizeof(ifr.ifr_addr));
			else
			    warn("SIOCGIFDSTADDR");
		}
		(void) strncpy(ifr.ifr_name, name, sizeof (ifr.ifr_name));
		sin = (struct sockaddr_in *)&ifr.ifr_dstaddr;
		printf("--> %s ", inet_ntoa(sin->sin_addr));
	}
	printf("netmask 0x%x ", ntohl(netmask.sin_addr.s_addr));
	if (flags & IFF_BROADCAST) {
		if (ioctl(s, SIOCGIFBRDADDR, (caddr_t)&ifr) < 0) {
			if (errno == EADDRNOTAVAIL)
				(void) memset(&ifr.ifr_addr, 0,
				    sizeof(ifr.ifr_addr));
			else
			    warn("SIOCGIFBRDADDR");
		}
		(void) strncpy(ifr.ifr_name, name, sizeof (ifr.ifr_name));
		sin = (struct sockaddr_in *)&ifr.ifr_addr;
		if (sin->sin_addr.s_addr != 0)
			printf("broadcast %s", inet_ntoa(sin->sin_addr));
	}
	putchar('\n');
}

#ifndef INET_ONLY

void
at_status(force)
	int force;
{
	struct sockaddr_at *sat, null_sat;
	struct netrange *nr;

	getsock(AF_APPLETALK);
	if (s < 0) {
		if (errno == EPROTONOSUPPORT)
			return;
		err(1, "socket");
	}
	(void) memset(&ifr, 0, sizeof(ifr));
	(void) strncpy(ifr.ifr_name, name, sizeof(ifr.ifr_name));
	if (ioctl(s, SIOCGIFADDR, (caddr_t)&ifr) < 0) {
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
		if (ioctl(s, SIOCGIFDSTADDR, (caddr_t)&ifr) < 0) {
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
	putchar('\n');
}

void
xns_status(force)
	int force;
{
	struct sockaddr_ns *sns;

	getsock(AF_NS);
	if (s < 0) {
		if (errno == EPROTONOSUPPORT)
			return;
		err(1, "socket");
	}
	(void) memset(&ifr, 0, sizeof(ifr));
	(void) strncpy(ifr.ifr_name, name, sizeof(ifr.ifr_name));
	if (ioctl(s, SIOCGIFADDR, (caddr_t)&ifr) < 0) {
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
		if (ioctl(s, SIOCGIFDSTADDR, (caddr_t)&ifr) < 0) {
			if (errno == EADDRNOTAVAIL)
			    memset(&ifr.ifr_addr, 0, sizeof(ifr.ifr_addr));
			else
			    warn("SIOCGIFDSTADDR");
		}
		(void) strncpy(ifr.ifr_name, name, sizeof (ifr.ifr_name));
		sns = (struct sockaddr_ns *)&ifr.ifr_dstaddr;
		printf("--> %s ", ns_ntoa(sns->sns_addr));
	}
	putchar('\n');
}

void
iso_status(force)
	int force;
{
	struct sockaddr_iso *siso;

	getsock(AF_ISO);
	if (s < 0) {
		if (errno == EPROTONOSUPPORT)
			return;
		err(1, "socket");
	}
	(void) memset(&ifr, 0, sizeof(ifr));
	(void) strncpy(ifr.ifr_name, name, sizeof(ifr.ifr_name));
	if (ioctl(s, SIOCGIFADDR, (caddr_t)&ifr) < 0) {
		if (errno == EADDRNOTAVAIL || errno == EAFNOSUPPORT) {
			if (!force)
				return;
			(void) memset(&ifr.ifr_addr, 0, sizeof(ifr.ifr_addr));
		} else
			warn("SIOCGIFADDR");
	}
	(void) strncpy(ifr.ifr_name, name, sizeof ifr.ifr_name);
	siso = (struct sockaddr_iso *)&ifr.ifr_addr;
	printf("\tiso %s ", iso_ntoa(&siso->siso_addr));
	if (ioctl(s, SIOCGIFNETMASK, (caddr_t)&ifr) < 0) {
		if (errno == EADDRNOTAVAIL)
			memset(&ifr.ifr_addr, 0, sizeof(ifr.ifr_addr));
		else
			warn("SIOCGIFNETMASK");
	} else {
		printf(" netmask %s ", iso_ntoa(&siso->siso_addr));
	}
	if (flags & IFF_POINTOPOINT) {
		if (ioctl(s, SIOCGIFDSTADDR, (caddr_t)&ifr) < 0) {
			if (errno == EADDRNOTAVAIL)
			    memset(&ifr.ifr_addr, 0, sizeof(ifr.ifr_addr));
			else
			    warn("SIOCGIFDSTADDR");
		}
		(void) strncpy(ifr.ifr_name, name, sizeof (ifr.ifr_name));
		siso = (struct sockaddr_iso *)&ifr.ifr_addr;
		printf("--> %s ", iso_ntoa(&siso->siso_addr));
	}
	putchar('\n');
}

#endif	/* INET_ONLY */

#define SIN(x) ((struct sockaddr_in *) &(x))
struct sockaddr_in *sintab[] = {
SIN(ridreq.ifr_addr), SIN(addreq.ifra_addr),
SIN(addreq.ifra_mask), SIN(addreq.ifra_broadaddr)};

void
in_getaddr(s, which)
	char *s;
	int which;
{
	register struct sockaddr_in *sin = sintab[which];
	struct hostent *hp;
	struct netent *np;

	sin->sin_len = sizeof(*sin);
	if (which != MASK)
		sin->sin_family = AF_INET;

	if (inet_aton(s, &sin->sin_addr) == 0) {
		if ((hp = gethostbyname(s)) != NULL)
			(void) memcpy(&sin->sin_addr, hp->h_addr, hp->h_length);
		else if ((np = getnetbyname(s)) != NULL)
			sin->sin_addr = inet_makeaddr(np->n_net, INADDR_ANY);
		else
			errx(1, "%s: bad value", s);
	}
}

/*
 * Print a value a la the %b format of the kernel's printf
 */
void
printb(s, v, bits)
	char *s;
	register char *bits;
	register unsigned short v;
{
	register int i, any = 0;
	register char c;

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

#ifndef INET_ONLY

void
at_getaddr(addr, which)
	char *addr;
	int which;
{
	struct sockaddr_at *sat = (struct sockaddr_at *) &addreq.ifra_addr;
	u_int net, node;

	sat->sat_family = AF_APPLETALK;
	sat->sat_len = sizeof(*sat);
	if (which == MASK)
		errx(1, "AppleTalk does not use netmasks\n");
	if (sscanf(addr, "%u.%u", &net, &node) != 2
	    || net == 0 || net > 0xffff || node == 0 || node > 0xfe)
		errx(1, "%s: illegal address", addr);
	sat->sat_addr.s_net = htons(net);
	sat->sat_addr.s_node = node;
}

void
setatrange(range, d)
	char *range;
	int d;
{
	u_short	first = 123, last = 123;

	if (sscanf(range, "%hu-%hu", &first, &last) != 2
	    || first == 0 || first > 0xffff
	    || last == 0 || last > 0xffff || first > last)
		errx(1, "%s: illegal net range: %u-%u", range, first, last);
	at_nr.nr_firstnet = htons(first);
	at_nr.nr_lastnet = htons(last);
}

void
setatphase(phase, d)
	char *phase;
	int d;
{
	if (!strcmp(phase, "1"))
		at_nr.nr_phase = 1;
	else if (!strcmp(phase, "2"))
		at_nr.nr_phase = 2;
	else
		errx(1, "%s: illegal phase", phase);
}

void
checkatrange(sat)
	struct sockaddr_at *sat;
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
		errx(1, "AppleTalk address is not in range");
	*((struct netrange *) &sat->sat_zero) = at_nr;
}

#define SNS(x) ((struct sockaddr_ns *) &(x))
struct sockaddr_ns *snstab[] = {
SNS(ridreq.ifr_addr), SNS(addreq.ifra_addr),
SNS(addreq.ifra_mask), SNS(addreq.ifra_broadaddr)};

void
xns_getaddr(addr, which)
	char *addr;
	int which;
{
	struct sockaddr_ns *sns = snstab[which];

	sns->sns_family = AF_NS;
	sns->sns_len = sizeof(*sns);
	sns->sns_addr = ns_addr(addr);
	if (which == MASK)
		printf("Attempt to set XNS netmask will be ineffectual\n");
}

#define SISO(x) ((struct sockaddr_iso *) &(x))
struct sockaddr_iso *sisotab[] = {
SISO(ridreq.ifr_addr), SISO(iso_addreq.ifra_addr),
SISO(iso_addreq.ifra_mask), SISO(iso_addreq.ifra_dstaddr)};

void
iso_getaddr(addr, which)
	char *addr;
	int which;
{
	register struct sockaddr_iso *siso = sisotab[which];
	siso->siso_addr = *iso_addr(addr);

	if (which == MASK) {
		siso->siso_len = TSEL(siso) - (caddr_t)(siso);
		siso->siso_nlen = 0;
	} else {
		siso->siso_len = sizeof(*siso);
		siso->siso_family = AF_ISO;
	}
}

void
setsnpaoffset(val, d)
	char *val;
	int d;
{
	iso_addreq.ifra_snpaoffset = atoi(val);
}

void
setnsellength(val, d)
	char *val;
	int d;
{
	nsellength = atoi(val);
	if (nsellength < 0)
		errx(1, "Negative NSEL length is absurd");
	if (afp == 0 || afp->af_af != AF_ISO)
		errx(1, "Setting NSEL length valid only for iso");
}

void
fixnsel(s)
	register struct sockaddr_iso *s;
{
	if (s->siso_family == 0)
		return;
	s->siso_tlen = nsellength;
}

void
adjust_nsellength()
{
	fixnsel(sisotab[RIDADDR]);
	fixnsel(sisotab[ADDR]);
	fixnsel(sisotab[DSTADDR]);
}

#endif	/* INET_ONLY */

void
usage()
{
	fprintf(stderr,
	    "usage: ifconfig [ -m ] interface\n%s%s%s%s%s%s%s%s%s%s",
		"\t[ af [ address [ dest_addr ] ] [ up ] [ down ] ",
		"[ netmask mask ] ]\n",
		"\t[ metric n ]\n",
		"\t[ arp | -arp ]\n",
		"\t[ media mtype ]\n",
		"\t[ mediaopt mopts ]\n",
		"\t[ -mediaopt mopts ]\n",
		"\t[ link0 | -link0 ] [ link1 | -link1 ] [ link2 | -link2 ]\n",
		"       ifconfig -a [ -m ] [ af ]\n",
		"       ifconfig -l\n");
	exit(1);
}
