/*	$NetBSD: ndp.c,v 1.59 2021/11/27 22:30:25 rillig Exp $	*/
/*	$KAME: ndp.c,v 1.121 2005/07/13 11:30:13 keiichi Exp $	*/

/*
 * Copyright (C) 1995, 1996, 1997, 1998, and 1999 WIDE Project.
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
 * 3. Neither the name of the project nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE PROJECT AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE PROJECT OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
/*
 * Copyright (c) 1984, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Sun Microsystems, Inc.
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

/*
 * Based on:
 * "@(#) Copyright (c) 1984, 1993\n\
 *	The Regents of the University of California.  All rights reserved.\n";
 *
 * "@(#)arp.c	8.2 (Berkeley) 1/2/94";
 */

/*
 * ndp - display, set, delete and flush neighbor cache
 */


#include <sys/param.h>
#include <sys/file.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/sysctl.h>
#include <sys/time.h>

#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_types.h>
#include <net/route.h>

#include <netinet/in.h>

#include <netinet/icmp6.h>
#include <netinet6/in6_var.h>
#include <netinet6/nd6.h>

#include <arpa/inet.h>

#include <netdb.h>
#include <errno.h>
#include <nlist.h>
#include <stdio.h>
#include <string.h>
#include <paths.h>
#include <err.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

#include "gmt2local.h"
#include "prog_ops.h"

static pid_t pid;
static int nflag;
static int tflag;
static int32_t thiszone;	/* time difference with gmt */
static int my_s = -1;
static unsigned int repeat = 0;


static char host_buf[NI_MAXHOST];		/* getnameinfo() */
static char ifix_buf[IFNAMSIZ];		/* if_indextoname() */

static void getsocket(void);
static int set(int, char **);
static void get(char *);
static int delete(struct rt_msghdr *, char *);
static void delete_one(char *);
static void do_foreach(struct in6_addr *, char *, int);
static struct in6_nbrinfo *getnbrinfo(struct in6_addr *, unsigned int, int);
static char *ether_str(struct sockaddr_dl *);
static int ndp_ether_aton(char *, u_char *);
__dead static void usage(void);
static int rtmsg(int, struct rt_msghdr *);
static void ifinfo(char *, int, char **);
static const char *sec2str(time_t);
static char *ether_str(struct sockaddr_dl *);
static void ts_print(const struct timeval *);

#define NDP_F_CLEAR	1
#define NDP_F_DELETE	2

static int mode = 0;
static char *arg = NULL;

int
main(int argc, char **argv)
{
	int ch;

	while ((ch = getopt(argc, argv, "acd:f:i:nstA:")) != -1)
		switch (ch) {
		case 'a':
		case 'c':
		case 's':
		case 'd':
		case 'f':
		case 'i' :
			if (mode) {
				usage();
				/*NOTREACHED*/
			}
			mode = ch;
			arg = optarg;
			break;
		case 'n':
			nflag = 1;
			break;
		case 't':
			tflag = 1;
			break;
		case 'A':
			if (mode) {
				usage();
				/*NOTREACHED*/
			}
			mode = 'a';
			repeat = atoi(optarg);
			break;
		default:
			usage();
		}

	argc -= optind;
	argv += optind;

	if (prog_init && prog_init() == -1)
		err(1, "init failed");

	pid = prog_getpid();
	thiszone = gmt2local(0L);

	switch (mode) {
	case 'a':
	case 'c':
		if (argc != 0) {
			usage();
			/*NOTREACHED*/
		}
		do_foreach(0, NULL, mode == 'c' ? NDP_F_CLEAR : 0);
		break;
	case 'd':
		if (argc != 0) {
			usage();
			/*NOTREACHED*/
		}
		delete_one(arg);
		break;
	case 'i':
		ifinfo(arg, argc, argv);
		break;
	case 's':
		if (argc < 2 || argc > 4)
			usage();
		return(set(argc, argv) ? 1 : 0);
	case 0:
		if (argc != 1) {
			usage();
			/*NOTREACHED*/
		}
		get(argv[0]);
		break;
	}
	return(0);
}

static void
makeaddr(struct sockaddr_in6 *mysin, const void *resp)
{
	const struct sockaddr_in6 *res = resp;
	mysin->sin6_addr = res->sin6_addr;
	mysin->sin6_scope_id = res->sin6_scope_id;
	inet6_putscopeid(mysin, INET6_IS_ADDR_LINKLOCAL);
}

static void
getsocket(void)
{
	if (my_s < 0) {
		my_s = prog_socket(PF_ROUTE, SOCK_RAW, 0);
		if (my_s < 0)
			err(1, "socket");
	}
}

#ifdef notdef
static struct sockaddr_in6 so_mask = {
	.sin6_len = sizeof(so_mask),
	.sin6_family = AF_INET6
};
#endif
static struct sockaddr_in6 blank_sin = {
	.sin6_len = sizeof(blank_sin),
	.sin6_family = AF_INET6
};
static struct sockaddr_in6 sin_m;
static struct sockaddr_dl blank_sdl = {
	.sdl_len = sizeof(blank_sdl),
	.sdl_family = AF_LINK,
};
static struct sockaddr_dl sdl_m;
static int expire_time, flags, found_entry;
static struct {
	struct	rt_msghdr m_rtm;
	char	m_space[512];
} m_rtmsg;

/*
 * Set an individual neighbor cache entry
 */
static int
set(int argc, char **argv)
{
	register struct sockaddr_in6 *mysin = &sin_m;
	register struct sockaddr_dl *sdl;
	register struct rt_msghdr *rtm = &(m_rtmsg.m_rtm);
	struct addrinfo hints, *res;
	int gai_error;
	u_char *ea;
	char *host = argv[0], *eaddr = argv[1];

	getsocket();
	argc -= 2;
	argv += 2;
	sdl_m = blank_sdl;
	sin_m = blank_sin;

	(void)memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET6;
	gai_error = getaddrinfo(host, NULL, &hints, &res);
	if (gai_error) {
		warnx("%s: %s", host, gai_strerror(gai_error));
		return 1;
	}
	makeaddr(mysin, res->ai_addr);
	freeaddrinfo(res);
	ea = (u_char *)LLADDR(&sdl_m);
	if (ndp_ether_aton(eaddr, ea) == 0)
		sdl_m.sdl_alen = 6;
	flags = expire_time = 0;
	while (argc-- > 0) {
		if (strncmp(argv[0], "temp", 4) == 0) {
			struct timeval tim;

			(void)gettimeofday(&tim, 0);
			expire_time = tim.tv_sec + 20 * 60;
		} else if (strncmp(argv[0], "proxy", 5) == 0)
			flags |= RTF_ANNOUNCE;
		argv++;
	}
	if (rtmsg(RTM_GET, NULL) < 0) {
		errx(1, "RTM_GET(%s) failed", host);
		/* NOTREACHED */
	}
	mysin = (struct sockaddr_in6 *)(void *)(rtm + 1);
	sdl = (struct sockaddr_dl *)(void *)(RT_ROUNDUP(mysin->sin6_len) + (char *)(void *)mysin);
	if (IN6_ARE_ADDR_EQUAL(&mysin->sin6_addr, &sin_m.sin6_addr)) {
		if (sdl->sdl_family == AF_LINK &&
		    !(rtm->rtm_flags & RTF_GATEWAY)) {
			switch (sdl->sdl_type) {
			case IFT_ETHER: case IFT_FDDI: case IFT_ISO88023:
			case IFT_ISO88024: case IFT_ISO88025:
				goto overwrite;
			}
		}
		/*
		 * IPv4 arp command retries with sin_other = SIN_PROXY here.
		 */
		(void)fprintf(stderr, "set: cannot configure a new entry\n");
		return 1;
	}

overwrite:
	if (sdl->sdl_family != AF_LINK) {
		warnx("cannot intuit interface index and type for %s", host);
		return (1);
	}
	sdl_m.sdl_type = sdl->sdl_type;
	sdl_m.sdl_index = sdl->sdl_index;
	return (rtmsg(RTM_ADD, NULL));
}

/*
 * Display an individual neighbor cache entry
 */
static void
get(char *host)
{
	struct sockaddr_in6 *mysin = &sin_m;
	struct addrinfo hints, *res;
	int gai_error;

	sin_m = blank_sin;
	(void)memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET6;
	gai_error = getaddrinfo(host, NULL, &hints, &res);
	if (gai_error) {
		warnx("%s: %s", host, gai_strerror(gai_error));
		return;
	}
	makeaddr(mysin, res->ai_addr);
	freeaddrinfo(res);
	do_foreach(&mysin->sin6_addr, host, 0);
	if (found_entry == 0) {
		(void)getnameinfo((struct sockaddr *)(void *)mysin,
		    (socklen_t)mysin->sin6_len,
		    host_buf, sizeof(host_buf), NULL ,0,
		    (nflag ? NI_NUMERICHOST : 0));
		errx(1, "%s (%s) -- no entry", host, host_buf);
	}
}

static void
delete_one(char *host)
{
	struct sockaddr_in6 *mysin = &sin_m;
	struct addrinfo hints, *res;
	int gai_error;

	sin_m = blank_sin;
	(void)memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET6;
	gai_error = getaddrinfo(host, NULL, &hints, &res);
	if (gai_error) {
		warnx("%s: %s", host, gai_strerror(gai_error));
		return;
	}
	makeaddr(mysin, res->ai_addr);
	freeaddrinfo(res);
	do_foreach(&mysin->sin6_addr, host, NDP_F_DELETE);
}

/*
 * Delete a neighbor cache entry
 */
static int
delete(struct rt_msghdr *rtm, char *host)
{
	char delete_host_buf[NI_MAXHOST];
	struct sockaddr_in6 *mysin = &sin_m;
	struct sockaddr_dl *sdl;

	getsocket();
	mysin = (struct sockaddr_in6 *)(void *)(rtm + 1);
	sdl = (struct sockaddr_dl *)(void *)(RT_ROUNDUP(mysin->sin6_len) +
	    (char *)(void *)mysin);

	if (sdl->sdl_family != AF_LINK) {
		(void)printf("cannot locate %s\n", host);
		return (1);
	}
	if (rtmsg(RTM_DELETE, rtm) == 0) {
		struct sockaddr_in6 s6 = *mysin; /* XXX: for safety */

		s6.sin6_scope_id = 0;
		inet6_putscopeid(&s6, INET6_IS_ADDR_LINKLOCAL);
		(void)getnameinfo((struct sockaddr *)(void *)&s6,
		    (socklen_t)s6.sin6_len, delete_host_buf,
		    sizeof(delete_host_buf), NULL, 0,
		    (nflag ? NI_NUMERICHOST : 0));
		(void)printf("%s (%s) deleted\n", host, delete_host_buf);
	}

	return 0;
}

#define W_ADDR	(8 * 4 + 7)
#define W_LL	17
#define W_IF	6

/*
 * Iterate on neighbor caches and do
 * - dump all caches,
 * - clear all caches (NDP_F_CLEAR) or
 * - remove matched caches (NDP_F_DELETE)
 */
static void
do_foreach(struct in6_addr *addr, char *host, int _flags)
{
	int mib[6];
	size_t needed;
	char *lim, *buf, *next;
	struct rt_msghdr *rtm;
	struct sockaddr_in6 *mysin;
	struct sockaddr_dl *sdl;
	struct in6_nbrinfo *nbi;
	struct timeval tim;
	int addrwidth;
	int llwidth;
	int ifwidth;
	char flgbuf[8], *fl;
	const char *ifname;
	int cflag = _flags == NDP_F_CLEAR;
	int dflag = _flags == NDP_F_DELETE;

	/* Print header */
	if (!tflag && !cflag)
		(void)printf("%-*.*s %-*.*s %*.*s %-9.9s %1s %2s\n",
		    W_ADDR, W_ADDR, "Neighbor", W_LL, W_LL, "Linklayer Address",
		    W_IF, W_IF, "Netif", "Expire", "S", "Fl");

again:;
	mib[0] = CTL_NET;
	mib[1] = PF_ROUTE;
	mib[2] = 0;
	mib[3] = AF_INET6;
	mib[4] = NET_RT_FLAGS;
	mib[5] = RTF_LLDATA;
	if (prog_sysctl(mib, 6, NULL, &needed, NULL, 0) < 0)
		err(1, "sysctl(PF_ROUTE estimate)");
	if (needed > 0) {
		if ((buf = malloc(needed)) == NULL)
			err(1, "malloc");
		if (prog_sysctl(mib, 6, buf, &needed, NULL, 0) < 0) {
			free(buf);
			if (errno == ENOBUFS)
				goto again;
			err(1, "sysctl(PF_ROUTE, NET_RT_FLAGS)");
		}
		lim = buf + needed;
	} else
		buf = lim = NULL;

	for (next = buf; next && next < lim; next += rtm->rtm_msglen) {
		int isrouter = 0, prbs = 0;

		rtm = (struct rt_msghdr *)(void *)next;
		mysin = (struct sockaddr_in6 *)(void *)(rtm + 1);
		sdl = (struct sockaddr_dl *)(void *)((char *)(void *)mysin + RT_ROUNDUP(mysin->sin6_len));

		/*
		 * Some OSes can produce a route that has the LINK flag but
		 * has a non-AF_LINK gateway (e.g. fe80::xx%lo0 on FreeBSD
		 * and BSD/OS, where xx is not the interface identifier on
		 * lo0).  Such routes entry would annoy getnbrinfo() below,
		 * so we skip them.
		 * XXX: such routes should have the GATEWAY flag, not the
		 * LINK flag.  However, there is rotten routing software
		 * that advertises all routes that have the GATEWAY flag.
		 * Thus, KAME kernel intentionally does not set the LINK flag.
		 * What is to be fixed is not ndp, but such routing software
		 * (and the kernel workaround)...
		 */
		if (sdl->sdl_family != AF_LINK)
			continue;

		if (!(rtm->rtm_flags & RTF_HOST))
			continue;

		if (addr) {
			if (!IN6_ARE_ADDR_EQUAL(addr, &mysin->sin6_addr))
				continue;
			found_entry = 1;
		} else if (IN6_IS_ADDR_MULTICAST(&mysin->sin6_addr))
			continue;
		if (dflag) {
			(void)delete(rtm, host_buf);
			continue;
		}
		if (IN6_IS_ADDR_LINKLOCAL(&mysin->sin6_addr) ||
		    IN6_IS_ADDR_MC_LINKLOCAL(&mysin->sin6_addr)) {
			uint16_t scopeid = mysin->sin6_scope_id;
			inet6_getscopeid(mysin, INET6_IS_ADDR_LINKLOCAL|
			    INET6_IS_ADDR_MC_LINKLOCAL);
			if (scopeid == 0)
				mysin->sin6_scope_id = sdl->sdl_index;
		}
		(void)getnameinfo((struct sockaddr *)(void *)mysin,
		    (socklen_t)mysin->sin6_len,
		    host_buf, sizeof(host_buf), NULL, 0,
		    (nflag ? NI_NUMERICHOST : 0));
		if (cflag) {
			/* Restore scopeid */
			if (IN6_IS_ADDR_LINKLOCAL(&mysin->sin6_addr) ||
			    IN6_IS_ADDR_MC_LINKLOCAL(&mysin->sin6_addr))
				inet6_putscopeid(mysin, INET6_IS_ADDR_LINKLOCAL|
				    INET6_IS_ADDR_MC_LINKLOCAL);
			if ((rtm->rtm_flags & RTF_STATIC) == 0)
				(void)delete(rtm, host_buf);
			continue;
		}
		(void)gettimeofday(&tim, 0);
		if (tflag)
			ts_print(&tim);

		addrwidth = strlen(host_buf);
		if (addrwidth < W_ADDR)
			addrwidth = W_ADDR;
		llwidth = strlen(ether_str(sdl));
		if (W_ADDR + W_LL - addrwidth > llwidth)
			llwidth = W_ADDR + W_LL - addrwidth;
		ifname = if_indextoname((unsigned int)sdl->sdl_index, ifix_buf);
		if (!ifname)
			ifname = "?";
		ifwidth = strlen(ifname);
		if (W_ADDR + W_LL + W_IF - addrwidth - llwidth > ifwidth)
			ifwidth = W_ADDR + W_LL + W_IF - addrwidth - llwidth;

		(void)printf("%-*.*s %-*.*s %*.*s", addrwidth, addrwidth,
		    host_buf, llwidth, llwidth, ether_str(sdl), ifwidth,
		    ifwidth, ifname);

		/* Print neighbor discovery specific informations */
		nbi = getnbrinfo(&mysin->sin6_addr,
		    (unsigned int)sdl->sdl_index, 1);
		if (nbi) {
			if (nbi->expire > tim.tv_sec) {
				(void)printf(" %-9.9s",
				    sec2str(nbi->expire - tim.tv_sec));
			} else if (nbi->expire == 0)
				(void)printf(" %-9.9s", "permanent");
			else
				(void)printf(" %-9.9s", "expired");

			switch (nbi->state) {
			case ND_LLINFO_NOSTATE:
				 (void)printf(" N");
				 break;
			case ND_LLINFO_WAITDELETE:
				 (void)printf(" W");
				 break;
			case ND_LLINFO_INCOMPLETE:
				 (void)printf(" I");
				 break;
			case ND_LLINFO_REACHABLE:
				 (void)printf(" R");
				 break;
			case ND_LLINFO_STALE:
				 (void)printf(" S");
				 break;
			case ND_LLINFO_DELAY:
				 (void)printf(" D");
				 break;
			case ND_LLINFO_PROBE:
				 (void)printf(" P");
				 break;
			case ND_LLINFO_UNREACHABLE:
				 (void)printf(" U");
				 break;
			default:
				 (void)printf(" ?");
				 break;
			}

			isrouter = nbi->isrouter;
			prbs = nbi->asked;
		} else {
			warnx("failed to get neighbor information");
			(void)printf("  ");
		}

		/*
		 * other flags. R: router, P: proxy, W: ??
		 */
		fl = flgbuf;
		if (isrouter)
			*fl++ = 'R';
		if (rtm->rtm_flags & RTF_ANNOUNCE)
			*fl++ = 'p';
		*fl++ = '\0';
		(void)printf(" %s", flgbuf);

		if (prbs)
			(void)printf(" %d", prbs);

		(void)printf("\n");
	}
	if (buf != NULL)
		free(buf);

	if (repeat) {
		(void)printf("\n");
		(void)fflush(stdout);
		(void)sleep(repeat);
		goto again;
	}
}

static struct in6_nbrinfo *
getnbrinfo(struct in6_addr *addr, unsigned int ifindex, int warning)
{
	static struct in6_nbrinfo nbi;
	int s;

	if ((s = prog_socket(AF_INET6, SOCK_DGRAM, 0)) < 0)
		err(1, "socket");

	(void)memset(&nbi, 0, sizeof(nbi));
	(void)if_indextoname(ifindex, nbi.ifname);
	nbi.addr = *addr;
	if (prog_ioctl(s, SIOCGNBRINFO_IN6, &nbi) < 0) {
		if (warning)
			warn("ioctl(SIOCGNBRINFO_IN6)");
		(void)prog_close(s);
		return(NULL);
	}

	(void)prog_close(s);
	return(&nbi);
}

static char *
ether_str(struct sockaddr_dl *sdl)
{
	static char hbuf[NI_MAXHOST];

	if (sdl->sdl_alen) {
		if (getnameinfo((struct sockaddr *)(void *)sdl,
		    (socklen_t)sdl->sdl_len,
		    hbuf, sizeof(hbuf), NULL, 0, NI_NUMERICHOST) != 0)
			(void)snprintf(hbuf, sizeof(hbuf), "<invalid>");
	} else
		(void)snprintf(hbuf, sizeof(hbuf), "(incomplete)");

	return(hbuf);
}

static int
ndp_ether_aton(char *a, u_char *n)
{
	int i, o[6];

	i = sscanf(a, "%x:%x:%x:%x:%x:%x", &o[0], &o[1], &o[2],
	    &o[3], &o[4], &o[5]);
	if (i != 6) {
		warnx("invalid Ethernet address '%s'", a);
		return (1);
	}
	for (i = 0; i < 6; i++)
		n[i] = o[i];
	return (0);
}

static void
usage(void)
{
	const char *pn = getprogname();

	(void)fprintf(stderr, "Usage: %s [-nt] hostname\n", pn);
	(void)fprintf(stderr,
	    "       %s [-nt] -a | -c\n", pn);
	(void)fprintf(stderr, "       %s [-nt] -A wait\n", pn);
	(void)fprintf(stderr, "       %s [-nt] -d hostname\n", pn);
	(void)fprintf(stderr, "       %s [-nt] -f filename\n", pn);
	(void)fprintf(stderr, "       %s [-nt] -i interface [flags...]\n", pn);
	(void)fprintf(stderr,
	    "       %s [-nt] -s nodename etheraddr [temp] [proxy]\n", pn);
	exit(1);
}

static int
rtmsg(int cmd, struct rt_msghdr *_rtm)
{
	static int seq;
	register struct rt_msghdr *rtm = _rtm;
	register char *cp = m_rtmsg.m_space;
	register int l;

	errno = 0;
	if (rtm != NULL) {
		memcpy(&m_rtmsg, rtm, rtm->rtm_msglen);
		rtm = &m_rtmsg.m_rtm;
		goto doit;
	}
	(void)memset(&m_rtmsg, 0, sizeof(m_rtmsg));
	rtm = &m_rtmsg.m_rtm;
	rtm->rtm_flags = flags;
	rtm->rtm_version = RTM_VERSION;

	switch (cmd) {
	default:
		errx(1, "internal wrong cmd");
		/*NOTREACHED*/
	case RTM_ADD:
		rtm->rtm_addrs |= RTA_GATEWAY;
		if (expire_time) {
			rtm->rtm_rmx.rmx_expire = expire_time;
			rtm->rtm_inits = RTV_EXPIRE;
		}
		rtm->rtm_flags |= (RTF_HOST | RTF_STATIC | RTF_LLDATA);
#ifdef notdef	/* we don't support ipv6addr/128 type proxying. */
		if (rtm->rtm_flags & RTF_ANNOUNCE) {
			rtm->rtm_flags &= ~RTF_HOST;
			rtm->rtm_addrs |= RTA_NETMASK;
		}
#endif
		rtm->rtm_addrs |= RTA_DST;
		break;
	case RTM_GET:
		rtm->rtm_flags |= RTF_LLDATA;
		rtm->rtm_addrs |= RTA_DST | RTA_GATEWAY;
	}
#define NEXTADDR(w, s) \
	if (rtm->rtm_addrs & (w)) { \
		(void)memcpy(cp, &s, sizeof(s)); \
		RT_ADVANCE(cp, (struct sockaddr *)(void *)&s); \
	}

	NEXTADDR(RTA_DST, sin_m);
	NEXTADDR(RTA_GATEWAY, sdl_m);
#ifdef notdef	/* we don't support ipv6addr/128 type proxying. */
	(void)memset(&so_mask.sin6_addr, 0xff, sizeof(so_mask.sin6_addr));
	NEXTADDR(RTA_NETMASK, so_mask);
#endif

	rtm->rtm_msglen = cp - (char *)(void *)&m_rtmsg;
doit:
	l = rtm->rtm_msglen;
	rtm->rtm_seq = ++seq;
	rtm->rtm_type = cmd;
	if (prog_write(my_s, &m_rtmsg, (size_t)l) == -1) {
		if (errno != ESRCH || cmd != RTM_DELETE)
			err(1, "writing to routing socket");
	}
	do {
		l = prog_read(my_s, &m_rtmsg, sizeof(m_rtmsg));
	} while (l > 0 && (rtm->rtm_seq != seq || rtm->rtm_pid != pid));
	if (l < 0)
		warn("read from routing socket");
	return (0);
}

static void
ifinfo(char *ifname, int argc, char **argv)
{
	struct in6_ndireq nd;
	int i, s;
	u_int32_t newflags;
	bool valset = false, flagset = false;

	if ((s = prog_socket(AF_INET6, SOCK_DGRAM, 0)) < 0)
		err(1, "socket");
	(void)memset(&nd, 0, sizeof(nd));
	(void)strlcpy(nd.ifname, ifname, sizeof(nd.ifname));
	if (prog_ioctl(s, SIOCGIFINFO_IN6, &nd) < 0)
		err(1, "ioctl(SIOCGIFINFO_IN6)");
#define ND nd.ndi
	newflags = ND.flags;
	for (i = 0; i < argc; i++) {
		int clear = 0;
		char *cp = argv[i];

		if (*cp == '-') {
			clear = 1;
			cp++;
		}

#define SETFLAG(s, f) \
	do {\
		if (strcmp(cp, (s)) == 0) {\
			if (clear)\
				newflags &= ~(f);\
			else\
				newflags |= (f);\
			flagset = true; \
		}\
	} while (0)
/*
 * XXX: this macro is not 100% correct, in that it matches "nud" against
 *      "nudbogus".  But we just let it go since this is minor.
 */
#define SETVALUE(f, v) \
	do { \
		char *valptr; \
		unsigned long newval; \
		v = 0; /* unspecified */ \
		if (strncmp(cp, f, strlen(f)) == 0) { \
			valptr = strchr(cp, '='); \
			if (valptr == NULL) \
				err(1, "syntax error in %s field", (f)); \
			errno = 0; \
			newval = strtoul(++valptr, NULL, 0); \
			if (errno) \
				err(1, "syntax error in %s's value", (f)); \
			v = newval; \
			valset = true; \
		} \
	} while (0)

#ifdef ND6_IFF_IFDISABLED
		SETFLAG("disabled", ND6_IFF_IFDISABLED);
#endif
		SETFLAG("nud", ND6_IFF_PERFORMNUD);
#ifdef ND6_IFF_ACCEPT_RTADV
		SETFLAG("accept_rtadv", ND6_IFF_ACCEPT_RTADV);
#endif
#ifdef ND6_IFF_OVERRIDE_RTADV
		SETFLAG("override_rtadv", ND6_IFF_OVERRIDE_RTADV);
#endif
#ifdef ND6_IFF_AUTO_LINKLOCAL
		SETFLAG("auto_linklocal", ND6_IFF_AUTO_LINKLOCAL);
#endif
#ifdef ND6_IFF_PREFER_SOURCE
		SETFLAG("prefer_source", ND6_IFF_PREFER_SOURCE);
#endif
#ifdef ND6_IFF_DONT_SET_IFROUTE
		SETFLAG("dont_set_ifroute", ND6_IFF_DONT_SET_IFROUTE);
#endif
		SETVALUE("basereachable", ND.basereachable);
		SETVALUE("retrans", ND.retrans);
		SETVALUE("curhlim", ND.chlim);

		ND.flags = newflags;
#ifdef SIOCSIFINFO_IN6
		if (valset && prog_ioctl(s, SIOCSIFINFO_IN6, &nd) < 0)
			err(1, "ioctl(SIOCSIFINFO_IN6)");
#endif
		if (flagset && prog_ioctl(s, SIOCSIFINFO_FLAGS, &nd) < 0)
			err(1, "ioctl(SIOCSIFINFO_FLAGS)");
#undef SETFLAG
#undef SETVALUE
	}

	if (prog_ioctl(s, SIOCGIFINFO_IN6, &nd) < 0)
		err(1, "ioctl(SIOCGIFINFO_IN6)");
	(void)printf("curhlim=%d", ND.chlim);
	(void)printf(", basereachable=%ds%dms",
	    ND.basereachable / 1000, ND.basereachable % 1000);
	(void)printf(", retrans=%ds%dms", ND.retrans / 1000, ND.retrans % 1000);
	if (ND.flags) {
		(void)printf("\nFlags: ");
		if ((ND.flags & ND6_IFF_PERFORMNUD))
			(void)printf("nud ");
#ifdef ND6_IFF_IFDISABLED
		if ((ND.flags & ND6_IFF_IFDISABLED))
			(void)printf("disabled ");
#endif
#ifdef ND6_IFF_AUTO_LINKLOCAL
		if ((ND.flags & ND6_IFF_AUTO_LINKLOCAL))
			(void)printf("auto_linklocal ");
#endif
#ifdef ND6_IFF_PREFER_SOURCE
		if ((ND.flags & ND6_IFF_PREFER_SOURCE))
			(void)printf("prefer_source ");
#endif
	}
	(void)putc('\n', stdout);
#undef ND

	(void)prog_close(s);
}

static const char *
sec2str(time_t total)
{
	static char result[256];
	int days, hours, mins, secs;
	int first = 1;
	char *p = result;
	char *ep = &result[sizeof(result)];
	int n;

	days = total / 3600 / 24;
	hours = (total / 3600) % 24;
	mins = (total / 60) % 60;
	secs = total % 60;

	if (days) {
		first = 0;
		n = snprintf(p, (size_t)(ep - p), "%dd", days);
		if (n < 0 || n >= ep - p)
			return "?";
		p += n;
	}
	if (!first || hours) {
		first = 0;
		n = snprintf(p, (size_t)(ep - p), "%dh", hours);
		if (n < 0 || n >= ep - p)
			return "?";
		p += n;
	}
	if (!first || mins) {
		first = 0;
		n = snprintf(p, (size_t)(ep - p), "%dm", mins);
		if (n < 0 || n >= ep - p)
			return "?";
		p += n;
	}
	(void)snprintf(p, (size_t)(ep - p), "%ds", secs);

	return(result);
}

/*
 * Print the timestamp
 * from tcpdump/util.c
 */
static void
ts_print(const struct timeval *tvp)
{
	int s;

	/* Default */
	s = (tvp->tv_sec + thiszone) % 86400;
	(void)printf("%02d:%02d:%02d.%06u ",
	    s / 3600, (s % 3600) / 60, s % 60, (u_int32_t)tvp->tv_usec);
}
