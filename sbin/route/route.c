/*	$NetBSD: route.c,v 1.69 2003/09/16 09:34:48 cube Exp $	*/

/*
 * Copyright (c) 1983, 1989, 1991, 1993
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
__COPYRIGHT("@(#) Copyright (c) 1983, 1989, 1991, 1993\n\
	The Regents of the University of California.  All rights reserved.\n");
#endif /* not lint */

#ifndef lint
#if 0
static char sccsid[] = "@(#)route.c	8.6 (Berkeley) 4/28/95";
#else
__RCSID("$NetBSD: route.c,v 1.69 2003/09/16 09:34:48 cube Exp $");
#endif
#endif /* not lint */

#include <sys/param.h>
#include <sys/file.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/mbuf.h>
#include <sys/sysctl.h>

#include <net/if.h>
#include <net/route.h>
#include <net/if_dl.h>
#include <netinet/in.h>
#include <netatalk/at.h>
#include <netns/ns.h>
#include <netiso/iso.h>
#include <netccitt/x25.h>
#include <arpa/inet.h>
#include <netdb.h>

#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <paths.h>
#include <err.h>

#include "keywords.h"
#include "extern.h"

typedef union sockunion *sup;

int main __P((int, char **));
static void usage __P((char *)) __attribute__((__noreturn__));
static char *any_ntoa __P((const struct sockaddr *));
static void set_metric __P((char *, int));
static int newroute __P((int, char **));
static void inet_makenetandmask __P((u_int32_t, struct sockaddr_in *));
#ifdef INET6
static int inet6_makenetandmask __P((struct sockaddr_in6 *));
#endif
static int getaddr __P((int, char *, struct hostent **));
static int flushroutes __P((int, char *[], int));
#ifndef SMALL
static int prefixlen __P((char *));
static int x25_makemask __P((void));
static void interfaces __P((void));
static void monitor __P((void));
static void print_getmsg __P((struct rt_msghdr *, int));
static const char *linkstate __P((struct if_msghdr *));
#endif /* SMALL */
static int rtmsg __P((int, int ));
static void mask_addr __P((void));
static void print_rtmsg __P((struct rt_msghdr *, int));
static void pmsg_common __P((struct rt_msghdr *));
static void pmsg_addrs __P((char *, int));
static void bprintf __P((FILE *, int, u_char *));
static int keyword __P((char *));
static void sodump __P((sup, char *));
static void sockaddr __P((char *, struct sockaddr *));

union	sockunion {
	struct	sockaddr sa;
	struct	sockaddr_in sin;
#ifdef INET6
	struct	sockaddr_in6 sin6;
#endif
	struct	sockaddr_at sat;
	struct	sockaddr_dl sdl;
#ifndef SMALL
	struct	sockaddr_ns sns;
	struct	sockaddr_iso siso;
	struct	sockaddr_x25 sx25;
#endif /* SMALL */
} so_dst, so_gate, so_mask, so_genmask, so_ifa, so_ifp;

int	pid, rtm_addrs;
int	sock;
int	forcehost, forcenet, doflush, nflag, af, qflag, tflag;
int	iflag, verbose, aflen = sizeof (struct sockaddr_in);
int	locking, lockrest, debugonly, shortoutput, rv;
struct	rt_metrics rt_metrics;
u_int32_t  rtm_inits;
short ns_nullh[] = {0,0,0};
short ns_bh[] = {-1,-1,-1};


static void
usage(cp)
	char *cp;
{

	if (cp)
		warnx("botched keyword: %s", cp);
	(void) fprintf(stderr,
	    "Usage: %s [ -fnqvs ] cmd [[ -<qualifers> ] args ]\n",
	    getprogname());
	exit(1);
	/* NOTREACHED */
}

#define ROUNDUP(a) \
	((a) > 0 ? (1 + (((a) - 1) | (sizeof(long) - 1))) : sizeof(long))
#define ADVANCE(x, n) (x += ROUNDUP((n)->sa_len))

int
main(argc, argv)
	int argc;
	char **argv;
{
	int ch;

	if (argc < 2)
		usage(NULL);

	while ((ch = getopt(argc, argv, "fnqvdts")) != -1)
		switch(ch) {
		case 'f':
			doflush = 1;
			break;
		case 'n':
			nflag = 1;
			break;
		case 'q':
			qflag = 1;
			break;
		case 'v':
			verbose = 1;
			break;
		case 't':
			tflag = 1;
			break;
		case 'd':
			debugonly = 1;
			break;
		case 's':
			shortoutput = 1;
			break;
		case '?':
		default:
			usage(NULL);
			/*NOTREACHED*/
		}
	argc -= optind;
	argv += optind;

	pid = getpid();
	if (tflag)
		sock = open("/dev/null", O_WRONLY, 0);
	else
		sock = socket(PF_ROUTE, SOCK_RAW, 0);
	if (sock < 0)
		err(1, "socket");

	if (*argv == NULL) {
		if (doflush)
			ch = K_FLUSH;
		else
			goto no_cmd;
	} else
		ch = keyword(*argv);

	switch (ch) {
#ifndef SMALL
	case K_GET:
#endif /* SMALL */
	case K_CHANGE:
	case K_ADD:
	case K_DELETE:
		if (doflush)
			(void)flushroutes(1, argv, 0);
		return newroute(argc, argv);

	case K_SHOW:
		show(argc, argv);
		return 0;

#ifndef SMALL
	case K_MONITOR:
		monitor();
		return 0;

#endif /* SMALL */
	case K_FLUSH:
		return flushroutes(argc, argv, 0);

	case K_FLUSHALL:
		return flushroutes(argc, argv, 1);
	no_cmd:
	default:
		usage(*argv);
		/*NOTREACHED*/
	}
}

/*
 * Purge all entries in the routing tables not
 * associated with network interfaces.
 */
static int
flushroutes(argc, argv, doall)
	int argc;
	char *argv[];
	int doall;
{
	size_t needed;
	int mib[6], rlen, seqno;
	char *buf, *next, *lim;
	struct rt_msghdr *rtm;

	af = 0;
	shutdown(sock, SHUT_RD); /* Don't want to read back our messages */
	if (argc > 1) {
		argv++;
		if (argc == 2 && **argv == '-')
		    switch (keyword(*argv + 1)) {
			case K_INET:
				af = AF_INET;
				break;
#ifdef INET6
			case K_INET6:
				af = AF_INET6;
				break;
#endif
#ifndef SMALL
			case K_ATALK:
				af = AF_APPLETALK;
				break;
			case K_XNS:
				af = AF_NS;
				break;
#endif /* SMALL */
			case K_LINK:
				af = AF_LINK;
				break;
#ifndef SMALL
			case K_ISO:
			case K_OSI:
				af = AF_ISO;
				break;
			case K_X25:
				af = AF_CCITT;
#endif /* SMALL */
			default:
				goto bad;
		} else
bad:			usage(*argv);
	}
	mib[0] = CTL_NET;
	mib[1] = PF_ROUTE;
	mib[2] = 0;		/* protocol */
	mib[3] = 0;		/* wildcard address family */
	mib[4] = NET_RT_DUMP;
	mib[5] = 0;		/* no flags */
	if (sysctl(mib, 6, NULL, &needed, NULL, 0) < 0)
		err(1, "route-sysctl-estimate");
	if (needed == 0)
		return 0;
	if ((buf = malloc(needed)) == NULL)
		err(1, "malloc");
	if (sysctl(mib, 6, buf, &needed, NULL, 0) < 0)
		err(1, "actual retrieval of routing table");
	lim = buf + needed;
	if (verbose) {
		(void) printf("Examining routing table from sysctl\n");
		if (af) printf("(address family %s)\n", (*argv + 1));
	}
	seqno = 0;		/* ??? */
	for (next = buf; next < lim; next += rtm->rtm_msglen) {
		rtm = (struct rt_msghdr *)next;
		if (verbose)
			print_rtmsg(rtm, rtm->rtm_msglen);
		if (!(rtm->rtm_flags & (RTF_GATEWAY | RTF_STATIC |
					RTF_LLINFO)) && !doall)
			continue;
		if (af) {
			struct sockaddr *sa = (struct sockaddr *)(rtm + 1);

			if (sa->sa_family != af)
				continue;
		}
		if (debugonly)
			continue;
		rtm->rtm_type = RTM_DELETE;
		rtm->rtm_seq = seqno;
		rlen = write(sock, next, rtm->rtm_msglen);
		if (rlen < (int)rtm->rtm_msglen) {
			warn("write to routing socket, got %d for rlen", rlen);
			break;
		}
		seqno++;
		if (qflag)
			continue;
		if (verbose)
			print_rtmsg(rtm, rlen);
		else {
			struct sockaddr *sa = (struct sockaddr *)(rtm + 1);
			(void) printf("%-20.20s ",
			    routename(sa, NULL, rtm->rtm_flags));
			sa = (struct sockaddr *)(ROUNDUP(sa->sa_len) + (char *)sa);
			(void) printf("%-20.20s ",
			    routename(sa, NULL, RTF_HOST));
			(void) printf("done\n");
		}
	}
	return 0;
}


static char hexlist[] = "0123456789abcdef";

static char *
any_ntoa(sa)
	const struct sockaddr *sa;
{
	static char obuf[64];
	const char *in;
	char *out;
	int len;

	len = sa->sa_len;
	in  = sa->sa_data;
	out = obuf;

	do {
		*out++ = hexlist[(*in >> 4) & 15];
		*out++ = hexlist[(*in++)    & 15];
		*out++ = '.';
	} while (--len > 0);
	out[-1] = '\0';
	return obuf;
}


int
netmask_length(nm, family)
	struct sockaddr *nm;
	int family;
{
	static int
	    /* number of bits in a nibble */
	    _t[] = { 0,1,1,2,1,2,2,3,1,2,2,3,2,3,3,4 },
	    /* good nibbles are 1111, 1110, 1100, 1000, 0000 */
	    _g[] = { 1,0,0,0,0,0,0,0,1,0,0,0,1,0,1,1 };
	int mask, good, zeroes, maskbytes, bit, i;
	unsigned char *maskdata;

	if (nm == NULL)
		return 0;

	mask = 0;
	good = 1;
	zeroes = 0;

	switch (family) {
	case AF_INET: {
		struct sockaddr_in *nsin = (struct sockaddr_in *)nm;
		maskdata = (unsigned char *) &nsin->sin_addr;
		maskbytes = nsin->sin_len -
		    ((caddr_t)&nsin->sin_addr - (caddr_t)nsin);
		break;
	}
	case AF_INET6: {
		struct sockaddr_in6 *sin6 = (struct sockaddr_in6 *)nm;
		maskdata = (unsigned char *) &sin6->sin6_addr;
		maskbytes = sin6->sin6_len -
		    ((caddr_t)&sin6->sin6_addr - (caddr_t)sin6);
		break;
	}
	default:
		return 0;
	}

	/*
	 * Count the bits in the nibbles of the mask, and marking the
	 * netmask as not good (or at best, non-standard and very
	 * discouraged, in the case of AF_INET) if we find either of
	 * a nibble with non-contiguous bits, or a non-zero nibble
	 * after we've found a zero nibble.
	 */
	for (i = 0; i < maskbytes; i++) {
		/* high nibble */
		mask += bit = _t[maskdata[i] >> 4];
		good &= _g[maskdata[i] >> 4];
		if (zeroes && bit)
			good = 0;
		if (bit == 0)
			zeroes = 1;
		/* low nibble */
		mask += bit = _t[maskdata[i] & 0xf];
		good &= _g[maskdata[i] & 0xf];
		if (zeroes && bit)
			good = 0;
		if (bit == 0)
			zeroes = 1;
	}

	/*
	 * Always return the number of bits found, but as a negative
	 * if the mask wasn't one we like.
	 */
	return good ? mask : -mask;
}

char *
netmask_string(mask, len)
	struct sockaddr *mask;
	int len;
{
	static char smask[16];

	if (len >= 0)
		snprintf(smask, sizeof(smask), "%d", len);
	else {
		/* XXX AF_INET only?! */
		struct sockaddr_in nsin;
	
		memset(&nsin, 0, sizeof(nsin));
		memcpy(&nsin, mask, mask->sa_len);
		snprintf(smask, sizeof(smask), "%s", inet_ntoa(nsin.sin_addr));
	}

	return smask;
}


char *
routename(sa, nm, flags)
	struct sockaddr *sa, *nm;
	int flags;
{
	char *cp;
	static char line[50];
	struct hostent *hp;
	static char domain[MAXHOSTNAMELEN + 1];
	static int first = 1;
	struct in_addr in;
	int nml;

	if ((flags & RTF_HOST) == 0)
		return netname(sa, nm);

	if (first) {
		first = 0;
		if (gethostname(domain, MAXHOSTNAMELEN) == 0 &&
		    (cp = strchr(domain, '.')))
			(void)strlcpy(domain, cp + 1, sizeof(domain));
		else
			domain[0] = 0;
	}

	if (sa->sa_len == 0)
		strlcpy(line, "default", sizeof(line));
	else switch (sa->sa_family) {

	case AF_INET:
		in = ((struct sockaddr_in *)sa)->sin_addr;
		nml = netmask_length(nm, AF_INET);

		cp = 0;
		if (in.s_addr == INADDR_ANY || sa->sa_len < 4) {
			if (nml == 0)
				cp = "default";
			else {
				static char notdefault[sizeof(NOTDEFSTRING)];

				snprintf(notdefault, sizeof(notdefault),
				    "0.0.0.0/%s", netmask_string(nm, nml));
				cp = notdefault;
			}
		}
		if (cp == 0 && !nflag) {
			hp = gethostbyaddr((char *)&in, sizeof (struct in_addr),
				AF_INET);
			if (hp) {
				if ((cp = strchr(hp->h_name, '.')) &&
				    !strcmp(cp + 1, domain))
					*cp = 0;
				cp = hp->h_name;
			}
		}
		if (cp)
			(void)strlcpy(line, cp, sizeof(line));
		else
			(void)strlcpy(line, inet_ntoa(in), sizeof(line));
		break;

	case AF_LINK:
		return (link_ntoa((struct sockaddr_dl *)sa));

#ifndef SMALL
#ifdef INET6
	case AF_INET6:
	    {
		struct sockaddr_in6 sin6;
		int niflags;

#ifdef NI_WITHSCOPEID
		niflags = NI_WITHSCOPEID;
#else
		niflags = 0;
#endif
		if (nflag)
			niflags |= NI_NUMERICHOST;
		memset(&sin6, 0, sizeof(sin6));
		memcpy(&sin6, sa, sa->sa_len);
		sin6.sin6_len = sizeof(struct sockaddr_in6);
		sin6.sin6_family = AF_INET6;
#ifdef __KAME__
		if (sa->sa_len == sizeof(struct sockaddr_in6) &&
		    (IN6_IS_ADDR_LINKLOCAL(&sin6.sin6_addr) ||
		     IN6_IS_ADDR_MC_LINKLOCAL(&sin6.sin6_addr)) &&
		    sin6.sin6_scope_id == 0) {
			sin6.sin6_scope_id =
			    ntohs(*(u_int16_t *)&sin6.sin6_addr.s6_addr[2]);
			sin6.sin6_addr.s6_addr[2] = 0;
			sin6.sin6_addr.s6_addr[3] = 0;
		}
#endif
		nml = netmask_length(nm, AF_INET6);
		if (IN6_IS_ADDR_UNSPECIFIED(&sin6.sin6_addr)) {
			if (nml == 0)
				strlcpy(line, "::", sizeof(line));
			else
				/* noncontiguous never happens in ipv6 */
				snprintf(line, sizeof(line), "::/%d", nml);
		}

		else if (getnameinfo((struct sockaddr *)&sin6, sin6.sin6_len,
		    line, sizeof(line), NULL, 0, niflags) != 0)
			strlcpy(line, "invalid", sizeof(line));
		break;
	    }
#endif

	case AF_NS:
		return (ns_print((struct sockaddr_ns *)sa));

	case AF_ISO:
		(void)snprintf(line, sizeof line, "iso %s",
		    iso_ntoa(&((struct sockaddr_iso *)sa)->siso_addr));
		break;

	case AF_APPLETALK:
		(void) snprintf(line, sizeof(line), "atalk %d.%d",
		    ((struct sockaddr_at *)sa)->sat_addr.s_net,
		    ((struct sockaddr_at *)sa)->sat_addr.s_node);
		break;
#endif /* SMALL */

	default:
		(void)snprintf(line, sizeof line, "(%d) %s",
			sa->sa_family, any_ntoa(sa));
		break;

	}
	return (line);
}

/*
 * Return the name of the network whose address is given.
 * The address is assumed to be that of a net or subnet, not a host.
 */
char *
netname(sa, nm)
	struct sockaddr *sa, *nm;
{
	char *cp = 0;
	static char line[50];
	struct netent *np = 0;
	u_int32_t net, mask;
	u_int32_t i;
	int subnetshift, nml;
	struct in_addr in;

	switch (sa->sa_family) {

	case AF_INET:
		in = ((struct sockaddr_in *)sa)->sin_addr;
		i = ntohl(in.s_addr);
		nml = netmask_length(nm, AF_INET);
		if (i == 0) {
			if (nml == 0)
				cp = "default";
			else {
				static char notdefault[sizeof(NOTDEFSTRING)];

				snprintf(notdefault, sizeof(notdefault),
				    "0.0.0.0/%s", netmask_string(nm, nml));
				cp = notdefault;
			}
		}
		else if (!nflag) {
			if (IN_CLASSA(i)) {
				mask = IN_CLASSA_NET;
				subnetshift = 8;
			} else if (IN_CLASSB(i)) {
				mask = IN_CLASSB_NET;
				subnetshift = 8;
			} else {
				mask = IN_CLASSC_NET;
				subnetshift = 4;
			}
			/*
			 * If there are more bits than the standard mask
			 * would suggest, subnets must be in use.
			 * Guess at the subnet mask, assuming reasonable
			 * width subnet fields.
			 */
			while (i &~ mask)
				mask = (int32_t)mask >> subnetshift;
			net = i & mask;
			while ((mask & 1) == 0)
				mask >>= 1, net >>= 1;
			np = getnetbyaddr(net, AF_INET);
			if (np)
				cp = np->n_name;
		}
		if (cp)
			(void)strlcpy(line, cp, sizeof(line));
		else {
#if 0	/* XXX - This is silly... */
#define C(x)	((x) & 0xff)
			if ((i & 0xffffff) == 0)
				(void)snprintf(line, sizeof line, "%u",
				    C(i >> 24));
			else if ((i & 0xffff) == 0)
				(void)snprintf(line, sizeof line, "%u.%u",
				    C(i >> 24), C(i >> 16));
			else if ((i & 0xff) == 0)
				(void)snprintf(line, sizeof line, "%u.%u.%u",
				    C(i >> 24), C(i >> 16), C(i >> 8));
			else
				(void)snprintf(line, sizeof line, "%u.%u.%u.%u",
				    C(i >> 24), C(i >> 16), C(i >> 8), C(i));
#undef C
#else /* XXX */
			(void)strlcpy(line, inet_ntoa(in), sizeof(line));
#endif /* XXX */
		}
		break;

	case AF_LINK:
		return (link_ntoa((struct sockaddr_dl *)sa));

#ifndef SMALL
#ifdef INET6
	case AF_INET6:
	    {
		struct sockaddr_in6 sin6;
		int niflags;

#ifdef NI_WITHSCOPEID
		niflags = NI_WITHSCOPEID;
#else
		niflags = 0;
#endif
		if (nflag)
			niflags |= NI_NUMERICHOST;
		memset(&sin6, 0, sizeof(sin6));
		memcpy(&sin6, sa, sa->sa_len);
		sin6.sin6_len = sizeof(struct sockaddr_in6);
		sin6.sin6_family = AF_INET6;
#ifdef __KAME__
		if (sa->sa_len == sizeof(struct sockaddr_in6) &&
		    (IN6_IS_ADDR_LINKLOCAL(&sin6.sin6_addr) ||
		     IN6_IS_ADDR_MC_LINKLOCAL(&sin6.sin6_addr)) &&
		    sin6.sin6_scope_id == 0) {
			sin6.sin6_scope_id =
			    ntohs(*(u_int16_t *)&sin6.sin6_addr.s6_addr[2]);
			sin6.sin6_addr.s6_addr[2] = 0;
			sin6.sin6_addr.s6_addr[3] = 0;
		}
#endif
		nml = netmask_length(nm, AF_INET6);
		if (IN6_IS_ADDR_UNSPECIFIED(&sin6.sin6_addr)) {
			if (nml == 0)
				strlcpy(line, "::", sizeof(line));
			else
				/* noncontiguous never happens in ipv6 */
				snprintf(line, sizeof(line), "::/%d", nml);
		}

		else if (getnameinfo((struct sockaddr *)&sin6, sin6.sin6_len,
		    line, sizeof(line), NULL, 0, niflags) != 0)
			strlcpy(line, "invalid", sizeof(line));
		break;
	    }
#endif

	case AF_NS:
		return (ns_print((struct sockaddr_ns *)sa));

	case AF_ISO:
		(void)snprintf(line, sizeof line, "iso %s",
		    iso_ntoa(&((struct sockaddr_iso *)sa)->siso_addr));
		break;

	case AF_APPLETALK:
		(void) snprintf(line, sizeof(line), "atalk %d.%d",
		    ((struct sockaddr_at *)sa)->sat_addr.s_net,
		    ((struct sockaddr_at *)sa)->sat_addr.s_node);
		break;
#endif /* SMALL */

	default:
		(void)snprintf(line, sizeof line, "af %d: %s",
			sa->sa_family, any_ntoa(sa));
		break;
	}
	return (line);
}

static void
set_metric(value, key)
	char *value;
	int key;
{
	int flag = 0;
	u_long noval, *valp = &noval;

	switch (key) {
#define caseof(x, y, z)	case x: valp = &rt_metrics.z; flag = y; break
	caseof(K_MTU, RTV_MTU, rmx_mtu);
	caseof(K_HOPCOUNT, RTV_HOPCOUNT, rmx_hopcount);
	caseof(K_EXPIRE, RTV_EXPIRE, rmx_expire);
	caseof(K_RECVPIPE, RTV_RPIPE, rmx_recvpipe);
	caseof(K_SENDPIPE, RTV_SPIPE, rmx_sendpipe);
	caseof(K_SSTHRESH, RTV_SSTHRESH, rmx_ssthresh);
	caseof(K_RTT, RTV_RTT, rmx_rtt);
	caseof(K_RTTVAR, RTV_RTTVAR, rmx_rttvar);
	}
	rtm_inits |= flag;
	if (lockrest || locking)
		rt_metrics.rmx_locks |= flag;
	if (locking)
		locking = 0;
	*valp = atoi(value);
}

static int
newroute(argc, argv)
	int argc;
	char **argv;
{
	char *cmd, *dest = "", *gateway = "";
	const char *error;
	int ishost = 0, ret, attempts, oerrno, flags = RTF_STATIC;
	int key;
	struct hostent *hp = 0;

	cmd = argv[0];
	af = 0;
	if (*cmd != 'g')
		shutdown(sock, SHUT_RD); /* Don't want to read back our messages */
	while (--argc > 0) {
		if (**(++argv)== '-') {
			switch (key = keyword(1 + *argv)) {

			case K_SA:
				af = PF_ROUTE;
				aflen = sizeof(union sockunion);
				break;

#ifndef SMALL
			case K_ATALK:
				af = AF_APPLETALK;
				aflen = sizeof(struct sockaddr_at);
				break;
#endif

			case K_INET:
				af = AF_INET;
				aflen = sizeof(struct sockaddr_in);
				break;

#ifdef INET6
			case K_INET6:
				af = AF_INET6;
				aflen = sizeof(struct sockaddr_in6);
				break;
#endif

			case K_LINK:
				af = AF_LINK;
				aflen = sizeof(struct sockaddr_dl);
				break;

#ifndef SMALL
			case K_OSI:
			case K_ISO:
				af = AF_ISO;
				aflen = sizeof(struct sockaddr_iso);
				break;

			case K_X25:
				af = AF_CCITT;
				aflen = sizeof(struct sockaddr_x25);
				break;

			case K_XNS:
				af = AF_NS;
				aflen = sizeof(struct sockaddr_ns);
				break;
#endif /* SMALL */

			case K_IFACE:
			case K_INTERFACE:
				iflag++;
				break;
			case K_NOSTATIC:
				flags &= ~RTF_STATIC;
				break;
			case K_LLINFO:
				flags |= RTF_LLINFO;
				break;
			case K_LOCK:
				locking = 1;
				break;
			case K_LOCKREST:
				lockrest = 1;
				break;
			case K_HOST:
				forcehost++;
				break;
			case K_REJECT:
				flags |= RTF_REJECT;
				break;
			case K_BLACKHOLE:
				flags |= RTF_BLACKHOLE;
				break;
			case K_CLONED:
				flags |= RTF_CLONED;
				break;
			case K_PROTO1:
				flags |= RTF_PROTO1;
				break;
			case K_PROTO2:
				flags |= RTF_PROTO2;
				break;
			case K_CLONING:
				flags |= RTF_CLONING;
				break;
			case K_XRESOLVE:
				flags |= RTF_XRESOLVE;
				break;
			case K_STATIC:
				flags |= RTF_STATIC;
				break;
			case K_IFA:
				if (!--argc)
					usage(1+*argv);
				(void) getaddr(RTA_IFA, *++argv, 0);
				break;
			case K_IFP:
				if (!--argc)
					usage(1+*argv);
				(void) getaddr(RTA_IFP, *++argv, 0);
				break;
			case K_GENMASK:
				if (!--argc)
					usage(1+*argv);
				(void) getaddr(RTA_GENMASK, *++argv, 0);
				break;
			case K_GATEWAY:
				if (!--argc)
					usage(1+*argv);
				(void) getaddr(RTA_GATEWAY, *++argv, 0);
				break;
			case K_DST:
				if (!--argc)
					usage(1+*argv);
				ishost = getaddr(RTA_DST, *++argv, &hp);
				dest = *argv;
				break;
			case K_NETMASK:
				if (!--argc)
					usage(1+*argv);
				(void) getaddr(RTA_NETMASK, *++argv, 0);
				/* FALLTHROUGH */
			case K_NET:
				forcenet++;
				break;
			case K_PREFIXLEN:
				if (!--argc)
					usage(1+*argv);
				ishost = prefixlen(*++argv);
				break;
			case K_MTU:
			case K_HOPCOUNT:
			case K_EXPIRE:
			case K_RECVPIPE:
			case K_SENDPIPE:
			case K_SSTHRESH:
			case K_RTT:
			case K_RTTVAR:
				if (!--argc)
					usage(1+*argv);
				set_metric(*++argv, key);
				break;
			default:
				usage(1+*argv);
			}
		} else {
			if ((rtm_addrs & RTA_DST) == 0) {
				dest = *argv;
				ishost = getaddr(RTA_DST, *argv, &hp);
			} else if ((rtm_addrs & RTA_GATEWAY) == 0) {
				gateway = *argv;
				(void) getaddr(RTA_GATEWAY, *argv, &hp);
			} else {
				ret = atoi(*argv);

				if (ret == 0) {
				    if (strcmp(*argv, "0") == 0) {
					if (!qflag)  {
					    warnx("%s, %s",
						"old usage of trailing 0",
						"assuming route to if");
					}
				    } else
					usage((char *)NULL);
				    iflag = 1;
				    continue;
				} else if (ret > 0 && ret < 10) {
				    if (!qflag) {
					warnx("%s, %s",
					    "old usage of trailing digit",
					    "assuming route via gateway");
				    }
				    iflag = 0;
				    continue;
				}
				(void) getaddr(RTA_NETMASK, *argv, 0);
			}
		}
	}
	if (forcehost && forcenet)
		errx(1, "-host and -net conflict");
	else if (forcehost)
		ishost = 1;
	else if (forcenet)
		ishost = 0;
	flags |= RTF_UP;
	if (ishost)
		flags |= RTF_HOST;
	if (iflag == 0)
		flags |= RTF_GATEWAY;
	for (attempts = 1; ; attempts++) {
		errno = 0;
		if ((ret = rtmsg(*cmd, flags)) == 0)
			break;
		if (errno != ENETUNREACH && errno != ESRCH)
			break;
		if (af == AF_INET && *gateway && hp && hp->h_addr_list[1]) {
			hp->h_addr_list++;
			memmove(&so_gate.sin.sin_addr, hp->h_addr_list[0],
			    hp->h_length);
		} else
			break;
	}
	if (*cmd == 'g')
		return rv;
	oerrno = errno;
	if (!qflag) {
		(void) printf("%s %s %s", cmd, ishost? "host" : "net", dest);
		if (*gateway) {
			(void) printf(": gateway %s", gateway);
			if (attempts > 1 && ret == 0 && af == AF_INET)
			    (void) printf(" (%s)",
			        inet_ntoa(so_gate.sin.sin_addr));
		}
		if (ret == 0)
			(void) printf("\n");
	}
	if (ret != 0) {
		switch (oerrno) {
		case ESRCH:
			error = "not in table";
			break;
		case EBUSY:
			error = "entry in use";
			break;
		case ENOBUFS:
			error = "routing table overflow";
			break;
		default:
			error = strerror(oerrno);
			break;
		}
		(void) printf(": %s\n", error);
		return 1;
	}
	return 0;
}

static void
inet_makenetandmask(u_int32_t net, struct sockaddr_in *isin)
{
	u_int32_t addr, mask = 0;
	char *cp;

	rtm_addrs |= RTA_NETMASK;
	if (net == 0)
		mask = addr = 0;
	else if (net < 128) {
		addr = net << IN_CLASSA_NSHIFT;
		mask = IN_CLASSA_NET;
	} else if (net < 192) {
		addr = net << IN_CLASSA_NSHIFT;
		mask = IN_CLASSB_NET;
	} else if (net < 224) {
		addr = net << IN_CLASSA_NSHIFT;
		mask = IN_CLASSC_NET;
	} else if (net < 256) {
		addr = net << IN_CLASSA_NSHIFT;
		mask = IN_CLASSD_NET;
	} else if (net < 49152) { /* 192 * 256 */
		addr = net << IN_CLASSB_NSHIFT;
		mask = IN_CLASSB_NET;
	} else if (net < 57344) { /* 224 * 256 */
		addr = net << IN_CLASSB_NSHIFT;
		mask = IN_CLASSC_NET;
	} else if (net < 65536) {
		addr = net << IN_CLASSB_NSHIFT;
		mask = IN_CLASSB_NET;
	} else if (net < 14680064L) { /* 224 * 65536 */
		addr = net << IN_CLASSC_NSHIFT;
		mask = IN_CLASSC_NET;
	} else if (net < 16777216L) { 
		addr = net << IN_CLASSC_NSHIFT;
		mask = IN_CLASSD_NET;
	} else {
		addr = net;
		if ((addr & IN_CLASSA_HOST) == 0)
			mask =  IN_CLASSA_NET;
		else if ((addr & IN_CLASSB_HOST) == 0)
			mask =  IN_CLASSB_NET;
		else if ((addr & IN_CLASSC_HOST) == 0)
			mask =  IN_CLASSC_NET;
		else
			mask = -1;
	}
	isin->sin_addr.s_addr = htonl(addr);
	isin = &so_mask.sin;
	isin->sin_addr.s_addr = htonl(mask);
	isin->sin_len = 0;
	isin->sin_family = 0;
	cp = (char *)(&isin->sin_addr + 1);
	while (*--cp == 0 && cp > (char *)isin)
		;
	isin->sin_len = 1 + cp - (char *)isin;
}

#ifdef INET6
/*
 * XXX the function may need more improvement...
 */
static int
inet6_makenetandmask(sin6)
	struct sockaddr_in6 *sin6;
{
	char *plen;
	struct in6_addr in6;

	plen = NULL;
	if (IN6_IS_ADDR_UNSPECIFIED(&sin6->sin6_addr) &&
	    sin6->sin6_scope_id == 0) {
		plen = "0";
	} else if ((sin6->sin6_addr.s6_addr[0] & 0xe0) == 0x20) {
		/* aggregatable global unicast - RFC2374 */
		memset(&in6, 0, sizeof(in6));
		if (!memcmp(&sin6->sin6_addr.s6_addr[8], &in6.s6_addr[8], 8))
			plen = "64";
	}

	if (!plen || strcmp(plen, "128") == 0)
		return 1;
	else {
		rtm_addrs |= RTA_NETMASK;
		(void)prefixlen(plen);
		return 0;
	}
}
#endif

/*
 * Interpret an argument as a network address of some kind,
 * returning 1 if a host address, 0 if a network address.
 */
static int
getaddr(which, s, hpp)
	int which;
	char *s;
	struct hostent **hpp;
{
	sup su;
	struct hostent *hp;
	struct netent *np;
	u_int32_t val;
	char *t;
	int afamily;  /* local copy of af so we can change it */

	if (af == 0) {
		af = AF_INET;
		aflen = sizeof(struct sockaddr_in);
	}
	afamily = af;
	rtm_addrs |= which;
	switch (which) {
	case RTA_DST:
		su = &so_dst;
		break;
	case RTA_GATEWAY:
		su = &so_gate;
		break;
	case RTA_NETMASK:
		su = &so_mask;
		break;
	case RTA_GENMASK:
		su = &so_genmask;
		break;
	case RTA_IFP:
		su = &so_ifp;
		afamily = AF_LINK;
		break;
	case RTA_IFA:
		su = &so_ifa;
		su->sa.sa_family = af;
		break;
	default:
		su = NULL;
		usage("Internal Error");
		/*NOTREACHED*/
	}
	su->sa.sa_len = aflen;
	su->sa.sa_family = afamily; /* cases that don't want it have left already */
	if (strcmp(s, "default") == 0) {
		switch (which) {
		case RTA_DST:
			forcenet++;
			(void) getaddr(RTA_NETMASK, s, 0);
			break;
		case RTA_NETMASK:
		case RTA_GENMASK:
			su->sa.sa_len = 0;
		}
		return (0);
	}
	switch (afamily) {
#ifndef SMALL
#ifdef INET6
	case AF_INET6:
	    {
		struct addrinfo hints, *res;
		char *slash = 0;

		if (which == RTA_DST && (slash = (strrchr(s, '/'))) != 0)
			*slash = '\0';
		memset(&hints, 0, sizeof(hints));
		hints.ai_family = afamily;	/*AF_INET6*/
		hints.ai_flags = AI_NUMERICHOST;
		hints.ai_socktype = SOCK_DGRAM;		/*dummy*/
		if (getaddrinfo(s, "0", &hints, &res) != 0) {
			hints.ai_flags = 0;
			if (slash) {
				*slash = '/';
				slash = 0;
			}
			if (getaddrinfo(s, "0", &hints, &res) != 0) {
				(void) fprintf(stderr, "%s: bad value\n", s);
				exit(1);
			}
		}
		if (slash)
			*slash = '/';
		if (sizeof(su->sin6) != res->ai_addrlen) {
			(void) fprintf(stderr, "%s: bad value\n", s);
			exit(1);
		}
		if (res->ai_next) {
			(void) fprintf(stderr,
			    "%s: resolved to multiple values\n", s);
			exit(1);
		}
		memcpy(&su->sin6, res->ai_addr, sizeof(su->sin6));
		freeaddrinfo(res);
#ifdef __KAME__
		if ((IN6_IS_ADDR_LINKLOCAL(&su->sin6.sin6_addr) ||
		     IN6_IS_ADDR_MC_LINKLOCAL(&su->sin6.sin6_addr)) &&
		    su->sin6.sin6_scope_id) {
			*(u_int16_t *)&su->sin6.sin6_addr.s6_addr[2] =
				htons(su->sin6.sin6_scope_id);
			su->sin6.sin6_scope_id = 0;
		}
#endif
		if (hints.ai_flags == AI_NUMERICHOST) {
			if (slash)
				return (prefixlen(slash + 1));
			if (which == RTA_DST)
				return (inet6_makenetandmask(&su->sin6));
			return (0);
		} else
			return (1);
	    }
#endif

	case AF_NS:
		if (which == RTA_DST) {
			struct sockaddr_ns *sms = &(so_mask.sns);
			memset(sms, 0, sizeof(*sms));
			sms->sns_family = 0;
			sms->sns_len = 6;
			sms->sns_addr.x_net = *(union ns_net *)ns_bh;
			rtm_addrs |= RTA_NETMASK;
		}
		su->sns.sns_addr = ns_addr(s);
		return (!ns_nullhost(su->sns.sns_addr));

	case AF_OSI:
		su->siso.siso_addr = *iso_addr(s);
		if (which == RTA_NETMASK || which == RTA_GENMASK) {
			char *cp = (char *)TSEL(&su->siso);
			su->siso.siso_nlen = 0;
			do {--cp ;} while ((cp > (char *)su) && (*cp == 0));
			su->siso.siso_len = 1 + cp - (char *)su;
		}
		return (1);

	case AF_CCITT:
		ccitt_addr(s, &su->sx25);
		return (which == RTA_DST ? x25_makemask() : 1);
#endif /* SMALL */

	case PF_ROUTE:
		su->sa.sa_len = sizeof(*su);
		sockaddr(s, &su->sa);
		return (1);

#ifndef SMALL
	case AF_APPLETALK:
		t = strchr (s, '.');
		if (!t) {
badataddr:
			errx(1, "bad address: %s", s);
		}
		val = atoi (s);
		if (val > 65535)
			goto badataddr;
		su->sat.sat_addr.s_net = val;
		val = atoi (t);
		if (val > 256)
			goto badataddr;
		su->sat.sat_addr.s_node = val;
		rtm_addrs |= RTA_NETMASK;
		return(forcehost || su->sat.sat_addr.s_node != 0);
#endif

	case AF_LINK:
		link_addr(s, &su->sdl);
		return (1);

	case AF_INET:
	default:
		break;
	}

	if (hpp == NULL)
		hpp = &hp;
	*hpp = NULL;

	if ((t = strchr(s, '/')) != NULL && which == RTA_DST) {
		*t = '\0';
		if ((val = inet_addr(s)) != INADDR_NONE) {
			inet_makenetandmask(htonl(val), &su->sin);
			return prefixlen(&t[1]);
		}
		*t = '/';
	}
	if (((val = inet_addr(s)) != INADDR_NONE) &&
	    (which != RTA_DST || forcenet == 0)) {
		su->sin.sin_addr.s_addr = val;
		if (inet_lnaof(su->sin.sin_addr) != INADDR_ANY)
			return (1);
		else {
			val = ntohl(val);
			goto netdone;
		}
	}
	if ((val = inet_network(s)) != INADDR_NONE ||
	    ((np = getnetbyname(s)) != NULL && (val = np->n_net) != 0)) {
netdone:
		if (which == RTA_DST)
			inet_makenetandmask(val, &su->sin);
		return (0);
	}
	hp = gethostbyname(s);
	if (hp) {
		*hpp = hp;
		su->sin.sin_family = hp->h_addrtype;
		memmove(&su->sin.sin_addr, hp->h_addr, hp->h_length);
		return (1);
	}
	errx(1, "bad value: %s", s);
}

int
prefixlen(s)
	char *s;
{
	int len = atoi(s), q, r;
	int max;

	switch (af) {
	case AF_INET:
		max = sizeof(struct in_addr) * 8;
		break;
#ifdef INET6
	case AF_INET6:
		max = sizeof(struct in6_addr) * 8;
		break;
#endif
	default:
		(void) fprintf(stderr,
		    "prefixlen is not supported with af %d\n", af);
		exit(1);
	}

	rtm_addrs |= RTA_NETMASK;	
	if (len < -1 || len > max) {
		(void) fprintf(stderr, "%s: bad value\n", s);
		exit(1);
	}
	
	q = len >> 3;
	r = len & 7;
	switch (af) {
	case AF_INET:
		memset(&so_mask, 0, sizeof(so_mask));
		so_mask.sin.sin_family = AF_INET;
		so_mask.sin.sin_len = sizeof(struct sockaddr_in);
		so_mask.sin.sin_addr.s_addr = htonl(0xffffffff << (32 - len));
		break;
#ifdef INET6
	case AF_INET6:
		so_mask.sin6.sin6_family = AF_INET6;
		so_mask.sin6.sin6_len = sizeof(struct sockaddr_in6);
		memset((void *)&so_mask.sin6.sin6_addr, 0,
			sizeof(so_mask.sin6.sin6_addr));
		if (q > 0)
			memset((void *)&so_mask.sin6.sin6_addr, 0xff, q);
		if (r > 0)
			*((u_char *)&so_mask.sin6.sin6_addr + q) =
			    (0xff00 >> r) & 0xff;
		break;
#endif
	}
	return (len == max);
}

#ifndef SMALL
int
x25_makemask()
{
	char *cp;

	if ((rtm_addrs & RTA_NETMASK) == 0) {
		rtm_addrs |= RTA_NETMASK;
		for (cp = (char *)&so_mask.sx25.x25_net;
		     cp < &so_mask.sx25.x25_opts.op_flags; cp++)
			*cp = -1;
		so_mask.sx25.x25_len = (u_char)&(((sup)0)->sx25.x25_opts);
	}
	return 0;
}


char *
ns_print(sns)
	struct sockaddr_ns *sns;
{
	struct ns_addr work;
	union { union ns_net net_e; u_int32_t int32_t_e; } net;
	u_short port;
	static char mybuf[50], cport[10], chost[25];
	char *host = "";
	char *p;
	u_char *q;

	work = sns->sns_addr;
	port = ntohs(work.x_port);
	work.x_port = 0;
	net.net_e  = work.x_net;
	if (ns_nullhost(work) && net.int32_t_e == 0) {
		if (!port)
			return ("*.*");
		(void)snprintf(mybuf, sizeof mybuf, "*.%XH", port);
		return (mybuf);
	}

	if (memcmp(ns_bh, work.x_host.c_host, 6) == 0)
		host = "any";
	else if (memcmp(ns_nullh, work.x_host.c_host, 6) == 0)
		host = "*";
	else {
		q = work.x_host.c_host;
		(void)snprintf(chost, sizeof chost, "%02X%02X%02X%02X%02X%02XH",
			q[0], q[1], q[2], q[3], q[4], q[5]);
		for (p = chost; *p == '0' && p < chost + 12; p++)
			/* void */;
		host = p;
	}
	if (port)
		(void)snprintf(cport, sizeof cport, ".%XH", htons(port));
	else
		*cport = 0;

	(void)snprintf(mybuf, sizeof mybuf, "%XH.%s%s",
	    (u_int32_t)ntohl(net.int32_t_e), host, cport);
	return (mybuf);
}

static void
interfaces()
{
	size_t needed;
	int mib[6];
	char *buf, *lim, *next;
	struct rt_msghdr *rtm;

	mib[0] = CTL_NET;
	mib[1] = PF_ROUTE;
	mib[2] = 0;		/* protocol */
	mib[3] = 0;		/* wildcard address family */
	mib[4] = NET_RT_IFLIST;
	mib[5] = 0;		/* no flags */
	if (sysctl(mib, 6, NULL, &needed, NULL, 0) < 0)
		err(1, "route-sysctl-estimate");
	if ((buf = malloc(needed)) == NULL)
		err(1, "malloc");
	if (sysctl(mib, 6, buf, &needed, NULL, 0) < 0)
		err(1, "actual retrieval of interface table");
	lim = buf + needed;
	for (next = buf; next < lim; next += rtm->rtm_msglen) {
		rtm = (struct rt_msghdr *)next;
		print_rtmsg(rtm, rtm->rtm_msglen);
	}
}

static void
monitor()
{
	int n;
	char msg[2048];

	verbose = 1;
	if (debugonly) {
		interfaces();
		exit(0);
	}
	for(;;) {
		time_t now;
		n = read(sock, msg, 2048);
		now = time(NULL);
		(void) printf("got message of size %d on %s", n, ctime(&now));
		print_rtmsg((struct rt_msghdr *)msg, n);
	}
}

#endif /* SMALL */


struct {
	struct	rt_msghdr m_rtm;
	char	m_space[512];
} m_rtmsg;

static int
rtmsg(cmd, flags)
	int cmd, flags;
{
	static int seq;
	int rlen;
	char *cp = m_rtmsg.m_space;
	int l;

#define NEXTADDR(w, u) \
	if (rtm_addrs & (w)) {\
	    l = ROUNDUP(u.sa.sa_len); memmove(cp, &(u), l); cp += l;\
	    if (verbose && ! shortoutput) sodump(&(u),"u");\
	}

	errno = 0;
	memset(&m_rtmsg, 0, sizeof(m_rtmsg));
	if (cmd == 'a')
		cmd = RTM_ADD;
	else if (cmd == 'c')
		cmd = RTM_CHANGE;
	else if (cmd == 'g') {
#ifdef	SMALL
		return (-1);
#else	/* SMALL */
		cmd = RTM_GET;
		if (so_ifp.sa.sa_family == 0) {
			so_ifp.sa.sa_family = AF_LINK;
			so_ifp.sa.sa_len = sizeof(struct sockaddr_dl);
			rtm_addrs |= RTA_IFP;
		}
#endif	/* SMALL */
	} else
		cmd = RTM_DELETE;
#define rtm m_rtmsg.m_rtm
	rtm.rtm_type = cmd;
	rtm.rtm_flags = flags;
	rtm.rtm_version = RTM_VERSION;
	rtm.rtm_seq = ++seq;
	rtm.rtm_addrs = rtm_addrs;
	rtm.rtm_rmx = rt_metrics;
	rtm.rtm_inits = rtm_inits;

	if (rtm_addrs & RTA_NETMASK)
		mask_addr();
	NEXTADDR(RTA_DST, so_dst);
	NEXTADDR(RTA_GATEWAY, so_gate);
	NEXTADDR(RTA_NETMASK, so_mask);
	NEXTADDR(RTA_GENMASK, so_genmask);
	NEXTADDR(RTA_IFP, so_ifp);
	NEXTADDR(RTA_IFA, so_ifa);
	rtm.rtm_msglen = l = cp - (char *)&m_rtmsg;
	if (verbose && ! shortoutput)
		print_rtmsg(&rtm, l);
	if (debugonly)
		return (0);
	if ((rlen = write(sock, (char *)&m_rtmsg, l)) < 0) {
		perror("writing to routing socket");
		return (-1);
	}
#ifndef	SMALL
	if (cmd == RTM_GET) {
		do {
			l = read(sock, (char *)&m_rtmsg, sizeof(m_rtmsg));
		} while (l > 0 && (rtm.rtm_seq != seq || rtm.rtm_pid != pid));
		if (l < 0)
			err(1, "read from routing socket");
		else
			print_getmsg(&rtm, l);
	}
#endif	/* SMALL */
#undef rtm
	return (0);
}

static void
mask_addr()
{
	int olen = so_mask.sa.sa_len;
	char *cp1 = olen + (char *)&so_mask, *cp2;

	for (so_mask.sa.sa_len = 0; cp1 > (char *)&so_mask; )
		if (*--cp1 != 0) {
			so_mask.sa.sa_len = 1 + cp1 - (char *)&so_mask;
			break;
		}
	if ((rtm_addrs & RTA_DST) == 0)
		return;
	switch (so_dst.sa.sa_family) {
	case AF_INET:
#ifdef INET6
	case AF_INET6:
#endif
#ifndef SMALL
	case AF_APPLETALK:
	case AF_NS:
	case AF_CCITT:
#endif /* SMALL */
	case 0:
		return;
#ifndef SMALL
	case AF_ISO:
		olen = MIN(so_dst.siso.siso_nlen,
			   MAX(so_mask.sa.sa_len - 6, 0));
		break;
#endif /* SMALL */
	}
	cp1 = so_mask.sa.sa_len + 1 + (char *)&so_dst;
	cp2 = so_dst.sa.sa_len + 1 + (char *)&so_dst;
	while (cp2 > cp1)
		*--cp2 = 0;
	cp2 = so_mask.sa.sa_len + 1 + (char *)&so_mask;
	while (cp1 > so_dst.sa.sa_data)
		*--cp1 &= *--cp2;
#ifndef SMALL
	switch (so_dst.sa.sa_family) {
	case AF_ISO:
		so_dst.siso.siso_nlen = olen;
		break;
	}
#endif /* SMALL */
}

char *msgtypes[] = {
	"",
	"RTM_ADD: Add Route",
	"RTM_DELETE: Delete Route",
	"RTM_CHANGE: Change Metrics or flags",
	"RTM_GET: Report Metrics",
	"RTM_LOSING: Kernel Suspects Partitioning",
	"RTM_REDIRECT: Told to use different route",
	"RTM_MISS: Lookup failed on this address",
	"RTM_LOCK: fix specified metrics",
	"RTM_OLDADD: caused by SIOCADDRT",
	"RTM_OLDDEL: caused by SIOCDELRT",
	"RTM_RESOLVE: Route created by cloning",
	"RTM_NEWADDR: address being added to iface",
	"RTM_DELADDR: address being removed from iface",
	"RTM_OIFINFO: iface status change (pre-1.5)",
	"RTM_IFINFO: iface status change",
	"RTM_IFANNOUNCE: iface arrival/departure",
	0,
};

char metricnames[] =
"\011pksent\010rttvar\7rtt\6ssthresh\5sendpipe\4recvpipe\3expire\2hopcount\1mtu";
char routeflags[] =
"\1UP\2GATEWAY\3HOST\4REJECT\5DYNAMIC\6MODIFIED\7DONE\010MASK_PRESENT\011CLONING\012XRESOLVE\013LLINFO\014STATIC\015BLACKHOLE\016CLONED\017PROTO2\020PROTO1";
char ifnetflags[] =
"\1UP\2BROADCAST\3DEBUG\4LOOPBACK\5PTP\6NOTRAILERS\7RUNNING\010NOARP\011PPROMISC\012ALLMULTI\013OACTIVE\014SIMPLEX\015LINK0\016LINK1\017LINK2\020MULTICAST";
char addrnames[] =
"\1DST\2GATEWAY\3NETMASK\4GENMASK\5IFP\6IFA\7AUTHOR\010BRD";


#ifndef SMALL
static const char *
linkstate(ifm)
	struct if_msghdr *ifm;
{
	static char buf[64];

	switch (ifm->ifm_data.ifi_link_state) {
	case LINK_STATE_UNKNOWN:
		return "carrier: unknown";
	case LINK_STATE_DOWN:
		return "carrier: no carrier";
	case LINK_STATE_UP:
		return "carrier: active";
	default:
		(void)snprintf(buf, sizeof(buf), "carrier: 0x%x",
		    ifm->ifm_data.ifi_link_state);
		return buf;
	}
}
#endif /* SMALL */

static void
print_rtmsg(rtm, msglen)
	struct rt_msghdr *rtm;
	int msglen;
{
	struct if_msghdr *ifm;
	struct ifa_msghdr *ifam;
	struct if_announcemsghdr *ifan;

	if (verbose == 0)
		return;
	if (rtm->rtm_version != RTM_VERSION) {
		(void) printf("routing message version %d not understood\n",
		    rtm->rtm_version);
		return;
	}
	if (msgtypes[rtm->rtm_type])
		(void)printf("%s: ", msgtypes[rtm->rtm_type]);
	else
		(void)printf("#%d: ", rtm->rtm_type);
	(void)printf("len %d, ", rtm->rtm_msglen);
	switch (rtm->rtm_type) {
	case RTM_IFINFO:
		ifm = (struct if_msghdr *)rtm;
		(void) printf("if# %d, %s, flags:", ifm->ifm_index,
#ifdef SMALL
		    ""
#else
		    linkstate(ifm)
#endif /* SMALL */
		    );
		bprintf(stdout, ifm->ifm_flags, ifnetflags);
		pmsg_addrs((char *)(ifm + 1), ifm->ifm_addrs);
		break;
	case RTM_NEWADDR:
	case RTM_DELADDR:
		ifam = (struct ifa_msghdr *)rtm;
		(void) printf("metric %d, flags:", ifam->ifam_metric);
		bprintf(stdout, ifam->ifam_flags, routeflags);
		pmsg_addrs((char *)(ifam + 1), ifam->ifam_addrs);
		break;
	case RTM_IFANNOUNCE:
		ifan = (struct if_announcemsghdr *)rtm;
		(void) printf("if# %d, what: ", ifan->ifan_index);
		switch (ifan->ifan_what) {
		case IFAN_ARRIVAL:
			printf("arrival");
			break;
		case IFAN_DEPARTURE:
			printf("departure");
			break;
		default:
			printf("#%d", ifan->ifan_what);
			break;
		}
		printf("\n");
		break;
	default:
		(void) printf("pid: %d, seq %d, errno %d, flags:",
			rtm->rtm_pid, rtm->rtm_seq, rtm->rtm_errno);
		bprintf(stdout, rtm->rtm_flags, routeflags);
		pmsg_common(rtm);
	}
}

#ifndef	SMALL
static void
print_getmsg(rtm, msglen)
	struct rt_msghdr *rtm;
	int msglen;
{
	struct sockaddr *dst = NULL, *gate = NULL, *mask = NULL, *ifa = NULL;
	struct sockaddr_dl *ifp = NULL;
	struct sockaddr *sa;
	char *cp;
	int i;

	if (! shortoutput)
		(void) printf("   route to: %s\n",
		    routename((struct sockaddr *) &so_dst, NULL, RTF_HOST));
	if (rtm->rtm_version != RTM_VERSION) {
		warnx("routing message version %d not understood",
		    rtm->rtm_version);
		return;
	}
	if (rtm->rtm_msglen > msglen) {
		warnx("message length mismatch, in packet %d, returned %d",
		    rtm->rtm_msglen, msglen);
	}
	if (rtm->rtm_errno)  {
		warn("RTM_GET");
		return;
	}
	cp = ((char *)(rtm + 1));
	if (rtm->rtm_addrs)
		for (i = 1; i; i <<= 1)
			if (i & rtm->rtm_addrs) {
				sa = (struct sockaddr *)cp;
				switch (i) {
				case RTA_DST:
					dst = sa;
					break;
				case RTA_GATEWAY:
					gate = sa;
					break;
				case RTA_NETMASK:
					mask = sa;
					break;
				case RTA_IFP:
					if (sa->sa_family == AF_LINK &&
					   ((struct sockaddr_dl *)sa)->sdl_nlen)
						ifp = (struct sockaddr_dl *)sa;
					break;
				case RTA_IFA:
					ifa = sa;
					break;
				}
				ADVANCE(cp, sa);
			}
	if (dst && mask)
		mask->sa_family = dst->sa_family;	/* XXX */
	if (dst && ! shortoutput)
		(void)printf("destination: %s\n",
		    routename(dst, mask, RTF_HOST));
	if (mask && ! shortoutput) {
		int savenflag = nflag;

		nflag = 1;
		(void)printf("       mask: %s\n",
		    routename(mask, NULL, RTF_HOST));
		nflag = savenflag;
	}
	if (gate && rtm->rtm_flags & RTF_GATEWAY && ! shortoutput)
		(void)printf("    gateway: %s\n",
		    routename(gate, NULL, RTF_HOST));
	if (ifa && ! shortoutput)
		(void)printf(" local addr: %s\n",
		    routename(ifa, NULL, RTF_HOST));
	if (ifp && ! shortoutput)
		(void)printf("  interface: %.*s\n",
		    ifp->sdl_nlen, ifp->sdl_data);
	if (! shortoutput) {
		(void)printf("      flags: ");
		bprintf(stdout, rtm->rtm_flags, routeflags);
	}

#define lock(f)	((rtm->rtm_rmx.rmx_locks & __CONCAT(RTV_,f)) ? 'L' : ' ')
#define msec(u)	(((u) + 500) / 1000)		/* usec to msec */

	if (! shortoutput) {
		(void) printf("\n%s\n", "\
 recvpipe  sendpipe  ssthresh  rtt,msec    rttvar  hopcount      mtu     expire");
		printf("%8ld%c ", rtm->rtm_rmx.rmx_recvpipe, lock(RPIPE));
		printf("%8ld%c ", rtm->rtm_rmx.rmx_sendpipe, lock(SPIPE));
		printf("%8ld%c ", rtm->rtm_rmx.rmx_ssthresh, lock(SSTHRESH));
		printf("%8ld%c ", msec(rtm->rtm_rmx.rmx_rtt), lock(RTT));
		printf("%8ld%c ", msec(rtm->rtm_rmx.rmx_rttvar), lock(RTTVAR));
		printf("%8ld%c ", rtm->rtm_rmx.rmx_hopcount, lock(HOPCOUNT));
		printf("%8ld%c ", rtm->rtm_rmx.rmx_mtu, lock(MTU));
		if (rtm->rtm_rmx.rmx_expire)
			rtm->rtm_rmx.rmx_expire -= time(0);
		printf("%8ld%c\n", rtm->rtm_rmx.rmx_expire, lock(EXPIRE));
	}
#undef lock
#undef msec
#define	RTA_IGN	(RTA_DST|RTA_GATEWAY|RTA_NETMASK|RTA_IFP|RTA_IFA|RTA_BRD)

	if ((rtm->rtm_addrs & RTF_GATEWAY) == 0)
		rv = 1;
	else {
		char *name;
		int addrs;

		cp = (char *)(rtm + 1);
		addrs = rtm->rtm_addrs;

		for (i = 1; i; i <<= 1) {
			sa = (struct sockaddr *)cp;
			if (i == RTF_GATEWAY) {
				name = routename(sa, NULL, RTF_HOST);
				if (name[0] == '\0')
					rv = 1;
				else if (shortoutput)
					printf("%s\n", name);
			}
			if (i & addrs)
				ADVANCE(cp, sa);
		}
	}

	if (shortoutput)
		return;
	else if (verbose)
		pmsg_common(rtm);
	else if (rtm->rtm_addrs &~ RTA_IGN) {
		(void) printf("sockaddrs: ");
		bprintf(stdout, rtm->rtm_addrs, addrnames);
		putchar('\n');
	}
#undef	RTA_IGN
}
#endif	/* SMALL */

void
pmsg_common(rtm)
	struct rt_msghdr *rtm;
{
	(void) printf("\nlocks: ");
	bprintf(stdout, rtm->rtm_rmx.rmx_locks, metricnames);
	(void) printf(" inits: ");
	bprintf(stdout, rtm->rtm_inits, metricnames);
	pmsg_addrs(((char *)(rtm + 1)), rtm->rtm_addrs);
}

static void
pmsg_addrs(cp, addrs)
	char	*cp;
	int	addrs;
{
	struct sockaddr *sa;
	int i;

	if (addrs != 0) {
		(void) printf("\nsockaddrs: ");
		bprintf(stdout, addrs, addrnames);
		(void) putchar('\n');
		for (i = 1; i; i <<= 1)
			if (i & addrs) {
				sa = (struct sockaddr *)cp;
				(void) printf(" %s",
				    routename(sa, NULL, RTF_HOST));
				ADVANCE(cp, sa);
			}
	}
	(void) putchar('\n');
	(void) fflush(stdout);
}

static void
bprintf(fp, b, s)
	FILE *fp;
	int b;
	u_char *s;
{
	int i;
	int gotsome = 0;

	if (b == 0)
		return;
	while ((i = *s++) != 0) {
		if (b & (1 << (i-1))) {
			if (gotsome == 0)
				i = '<';
			else
				i = ',';
			(void) putc(i, fp);
			gotsome = 1;
			for (; (i = *s) > 32; s++)
				(void) putc(i, fp);
		} else
			while (*s > 32)
				s++;
	}
	if (gotsome)
		(void) putc('>', fp);
}

static int
keyword(cp)
	char *cp;
{
	struct keytab *kt = keywords;

	while (kt->kt_cp && strcmp(kt->kt_cp, cp))
		kt++;
	return kt->kt_i;
}

static void
sodump(su, which)
	sup su;
	char *which;
{
#ifdef INET6
	char ntop_buf[NI_MAXHOST];
#endif

	switch (su->sa.sa_family) {
	case AF_INET:
		(void) printf("%s: inet %s; ",
		    which, inet_ntoa(su->sin.sin_addr));
		break;
#ifndef SMALL
	case AF_APPLETALK:
		(void) printf("%s: atalk %d.%d; ",
		    which, su->sat.sat_addr.s_net, su->sat.sat_addr.s_node);
		break;
#endif
	case AF_LINK:
		(void) printf("%s: link %s; ",
		    which, link_ntoa(&su->sdl));
		break;
#ifndef SMALL
#ifdef INET6
	case AF_INET6:
		(void) printf("%s: inet6 %s; ",
		    which, inet_ntop(AF_INET6, &su->sin6.sin6_addr,
				     ntop_buf, sizeof(ntop_buf)));
		break;
#endif
	case AF_ISO:
		(void) printf("%s: iso %s; ",
		    which, iso_ntoa(&su->siso.siso_addr));
		break;
	case AF_NS:
		(void) printf("%s: xns %s; ",
		    which, ns_ntoa(su->sns.sns_addr));
		break;
#endif /* SMALL */
	default:
		(void) printf("af %p: %s; ",
			which, any_ntoa(&su->sa));
	}
	(void) fflush(stdout);
}

/* States*/
#define VIRGIN	0
#define GOTONE	1
#define GOTTWO	2
/* Inputs */
#define	DIGIT	(4*0)
#define	END	(4*1)
#define DELIM	(4*2)

static void
sockaddr(addr, sa)
	char *addr;
	struct sockaddr *sa;
{
	char *cp = (char *)sa;
	int size = sa->sa_len;
	char *cplim = cp + size;
	int byte = 0, state = VIRGIN, new = 0;

	(void) memset(cp, 0, size);
	cp++;
	do {
		if ((*addr >= '0') && (*addr <= '9')) {
			new = *addr - '0';
		} else if ((*addr >= 'a') && (*addr <= 'f')) {
			new = *addr - 'a' + 10;
		} else if ((*addr >= 'A') && (*addr <= 'F')) {
			new = *addr - 'A' + 10;
		} else if (*addr == 0)
			state |= END;
		else
			state |= DELIM;
		addr++;
		switch (state /* | INPUT */) {
		case GOTTWO | DIGIT:
			*cp++ = byte; /*FALLTHROUGH*/
		case VIRGIN | DIGIT:
			state = GOTONE; byte = new; continue;
		case GOTONE | DIGIT:
			state = GOTTWO; byte = new + (byte << 4); continue;
		default: /* | DELIM */
			state = VIRGIN; *cp++ = byte; byte = 0; continue;
		case GOTONE | END:
		case GOTTWO | END:
			*cp++ = byte; /* FALLTHROUGH */
		case VIRGIN | END:
			break;
		}
		break;
	} while (cp < cplim);
	sa->sa_len = cp - (char *)sa;
}
