/*	$NetBSD: arp.c,v 1.32 2001/10/06 19:09:44 bjh21 Exp $ */

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

#include <sys/cdefs.h>
#ifndef lint
__COPYRIGHT("@(#) Copyright (c) 1984, 1993\n\
	The Regents of the University of California.  All rights reserved.\n");
#endif /* not lint */

#ifndef lint
#if 0
static char sccsid[] = "@(#)arp.c	8.3 (Berkeley) 4/28/95";
#else
__RCSID("$NetBSD: arp.c,v 1.32 2001/10/06 19:09:44 bjh21 Exp $");
#endif
#endif /* not lint */

/*
 * arp - display, set, and delete arp table entries
 */

/* Roundup the same way rt_xaddrs does */
#define ROUNDUP(a) \
	((a) > 0 ? (1 + (((a) - 1) | (sizeof(long) - 1))) : sizeof(long))

#include <sys/param.h>
#include <sys/file.h>
#include <sys/socket.h>
#include <sys/sysctl.h>
#include <sys/ioctl.h>

#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_ether.h>
#include <net/if_types.h>
#include <net/route.h>
#include <netinet/in.h>
#include <netinet/if_inarp.h>
#include <arpa/inet.h>

#include <err.h>
#include <errno.h>
#include <netdb.h>
#include <nlist.h>
#include <paths.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int	delete __P((const char *, const char *));
void	dump __P((u_long));
void	delete_all __P((void));
void	sdl_print __P((const struct sockaddr_dl *));
int	getifname __P((u_int16_t, char *));
int	atosdl __P((const char *s, struct sockaddr_dl *sdl));
int	file __P((char *));
void	get __P((const char *));
int	getinetaddr __P((const char *, struct in_addr *));
void	getsocket __P((void));
int	main __P((int, char **));
int	rtmsg __P((int));
int	set __P((int, char **));
void	usage __P((void));

static int pid;
static int aflag, nflag, vflag;
static int s = -1;
static int is = -1;

static struct ifconf ifc;
static char ifconfbuf[8192];
static char *progname;

int
main(argc, argv)
	int argc;
	char **argv;
{
	int ch;
	int op = 0;

	pid = getpid();
	progname = ((progname = strrchr(argv[0], '/')) ? progname + 1 : argv[0]);

	while ((ch = getopt(argc, argv, "andsfv")) != -1)
		switch((char)ch) {
		case 'a':
			aflag = 1;
			break;
		case 'd':
		case 's':
		case 'f':
			if (op)
				usage();
			op = ch;
			break;
		case 'n':
			nflag = 1;
			break;
		case 'v':
			vflag = 1;
			break;
		default:
			usage();
		}
	argc -= optind;
	argv += optind;

	if (!op && aflag)
		op = 'a';

	switch((char)op) {
	case 'a':
		dump(0);
		break;
	case 'd':
		if (aflag && argc == 0)
			delete_all();
		else {
			if (aflag || argc < 1 || argc > 2)
				usage();
			(void)delete(argv[0], argv[1]);
		}
		break;
	case 's':
		if (argc < 2 || argc > 5)
			usage();
		return (set(argc, argv) ? 1 : 0);
	case 'f':
		if (argc != 1)
			usage();
		return (file(argv[0]));
	default:
		if (argc != 1)
			usage();
		get(argv[0]);
		break;
	}
	return (0);
}

/*
 * Process a file to set standard arp entries
 */
int
file(name)
	char *name;
{
	char line[100], arg[5][50], *args[5];
	int i, retval;
	FILE *fp;

	if ((fp = fopen(name, "r")) == NULL)
		err(1, "cannot open %s", name);
	args[0] = &arg[0][0];
	args[1] = &arg[1][0];
	args[2] = &arg[2][0];
	args[3] = &arg[3][0];
	args[4] = &arg[4][0];
	retval = 0;
	while (fgets(line, 100, fp) != NULL) {
		i = sscanf(line, "%s %s %s %s %s", arg[0], arg[1], arg[2],
		    arg[3], arg[4]);
		if (i < 2) {
			warnx("bad line: %s", line);
			retval = 1;
			continue;
		}
		if (set(i, args))
			retval = 1;
	}
	fclose(fp);
	return (retval);
}

void
getsocket()
{
	if (s >= 0)
		return;
	s = socket(PF_ROUTE, SOCK_RAW, 0);
	if (s < 0)
		err(1, "socket");
}

struct	sockaddr_in so_mask = {8, 0, 0, { 0xffffffff}};
struct	sockaddr_inarp blank_sin = {sizeof(blank_sin), AF_INET }, sin_m;
struct	sockaddr_dl blank_sdl = {sizeof(blank_sdl), AF_LINK }, sdl_m;
int	expire_time, flags, export_only, doing_proxy, found_entry;
struct	{
	struct	rt_msghdr m_rtm;
	char	m_space[512];
}	m_rtmsg;

/*
 * Set an individual arp entry 
 */
int
set(argc, argv)
	int argc;
	char **argv;
{
	struct sockaddr_inarp *sin;
	struct sockaddr_dl *sdl;
	struct rt_msghdr *rtm;
	char *host = argv[0], *eaddr;
	int rval;

	sin = &sin_m;
	rtm = &(m_rtmsg.m_rtm);
	eaddr = argv[1];

	getsocket();
	argc -= 2;
	argv += 2;
	sdl_m = blank_sdl;		/* struct copy */
	sin_m = blank_sin;		/* struct copy */
	if (getinetaddr(host, &sin->sin_addr) == -1)
		return (1);
	if (atosdl(eaddr, &sdl_m))
		warnx("invalid link-level address '%s'", eaddr);
	doing_proxy = flags = export_only = expire_time = 0;
	while (argc-- > 0) {
		if (strncmp(argv[0], "temp", 4) == 0) {
			struct timeval time;
			(void)gettimeofday(&time, 0);
			expire_time = time.tv_sec + 20 * 60;
		}
		else if (strncmp(argv[0], "pub", 3) == 0) {
			flags |= RTF_ANNOUNCE;
			doing_proxy = SIN_PROXY;
		} else if (strncmp(argv[0], "trail", 5) == 0) {
			(void)printf(
			    "%s: Sending trailers is no longer supported\n",
			     host);
		}
		argv++;
	}
tryagain:
	if (rtmsg(RTM_GET) < 0) {
		warn("%s", host);
		return (1);
	}
	sin = (struct sockaddr_inarp *)(rtm + 1);
	sdl = (struct sockaddr_dl *)(ROUNDUP(sin->sin_len) + (char *)sin);
	if (sin->sin_addr.s_addr == sin_m.sin_addr.s_addr) {
		if (sdl->sdl_family == AF_LINK &&
		    (rtm->rtm_flags & RTF_LLINFO) &&
		    !(rtm->rtm_flags & RTF_GATEWAY)) switch (sdl->sdl_type) {
		case IFT_ETHER: case IFT_FDDI: case IFT_ISO88023:
		case IFT_ISO88024: case IFT_ISO88025: case IFT_ARCNET:
			goto overwrite;
		}
		if (doing_proxy == 0) {
			(void)printf("set: can only proxy for %s\n", host);
			return (1);
		}
		if (sin_m.sin_other & SIN_PROXY) {
			(void)printf(
			    "set: proxy entry exists for non 802 device\n");
			return (1);
		}
		sin_m.sin_other = SIN_PROXY;
		export_only = 1;
		goto tryagain;
	}
overwrite:
	if (sdl->sdl_family != AF_LINK) {
		(void)printf("cannot intuit interface index and type for %s\n",
		    host);
		return (1);
	}
	sdl_m.sdl_type = sdl->sdl_type;
	sdl_m.sdl_index = sdl->sdl_index;
	rval = rtmsg(RTM_ADD);
	if (vflag)
		(void)printf("%s (%s) added\n", host, eaddr);
	return (rval);
}

/*
 * Display an individual arp entry
 */
void
get(host)
	const char *host;
{
	struct sockaddr_inarp *sin;

	sin = &sin_m;
	sin_m = blank_sin;		/* struct copy */
	if (getinetaddr(host, &sin->sin_addr) == -1)
		exit(1);
	dump(sin->sin_addr.s_addr);
	if (found_entry == 0) {
		(void)printf("%s (%s) -- no entry\n", host,
		    inet_ntoa(sin->sin_addr));
		exit(1);
	}
}

/*
 * Delete an arp entry 
 */
int
delete(host, info)
	const char *host;
	const char *info;
{
	struct sockaddr_inarp *sin;
	struct rt_msghdr *rtm;
	struct sockaddr_dl *sdl;

	sin = &sin_m;
	rtm = &m_rtmsg.m_rtm;

	if (info && strncmp(info, "pro", 3) )
		export_only = 1;
	getsocket();
	sin_m = blank_sin;		/* struct copy */
	if (getinetaddr(host, &sin->sin_addr) == -1)
		return (1);
tryagain:
	if (rtmsg(RTM_GET) < 0) {
		warn("%s", host);
		return (1);
	}
	sin = (struct sockaddr_inarp *)(rtm + 1);
	sdl = (struct sockaddr_dl *)(ROUNDUP(sin->sin_len) + (char *)sin);
	if (sin->sin_addr.s_addr == sin_m.sin_addr.s_addr) {
		if (sdl->sdl_family == AF_LINK &&
		    (rtm->rtm_flags & RTF_LLINFO) &&
		    !(rtm->rtm_flags & RTF_GATEWAY)) switch (sdl->sdl_type) {
		case IFT_ETHER: case IFT_FDDI: case IFT_ISO88023:
		case IFT_ISO88024: case IFT_ISO88025: case IFT_ARCNET:
			goto delete;
		}
	}
	if (sin_m.sin_other & SIN_PROXY) {
		warnx("delete: can't locate %s", host);
		return (1);
	} else {
		sin_m.sin_other = SIN_PROXY;
		goto tryagain;
	}
delete:
	if (sdl->sdl_family != AF_LINK) {
		(void)printf("cannot locate %s\n", host);
		return (1);
	}
	if (rtmsg(RTM_DELETE)) 
		return (1);
	if (vflag)
		(void)printf("%s (%s) deleted\n", host,
		    inet_ntoa(sin->sin_addr));
	return (0);
}

/*
 * Dump the entire arp table
 */
void
dump(addr)
	u_long addr;
{
	int mib[6];
	size_t needed;
	char ifname[IFNAMSIZ];
	char *host, *lim, *buf, *next;
	struct rt_msghdr *rtm;
	struct sockaddr_inarp *sin;
	struct sockaddr_dl *sdl;
	struct hostent *hp;

	mib[0] = CTL_NET;
	mib[1] = PF_ROUTE;
	mib[2] = 0;
	mib[3] = AF_INET;
	mib[4] = NET_RT_FLAGS;
	mib[5] = RTF_LLINFO;
	if (sysctl(mib, 6, NULL, &needed, NULL, 0) < 0)
		err(1, "route-sysctl-estimate");
	if (needed == 0)
		return;
	if ((buf = malloc(needed)) == NULL)
		err(1, "malloc");
	if (sysctl(mib, 6, buf, &needed, NULL, 0) < 0)
		err(1, "actual retrieval of routing table");
	lim = buf + needed;
	for (next = buf; next < lim; next += rtm->rtm_msglen) {
		rtm = (struct rt_msghdr *)next;
		sin = (struct sockaddr_inarp *)(rtm + 1);
		sdl = (struct sockaddr_dl *)
		    (ROUNDUP(sin->sin_len) + (char *)sin);
		if (addr) {
			if (addr != sin->sin_addr.s_addr)
				continue;
			found_entry = 1;
		}
		if (nflag == 0)
			hp = gethostbyaddr((caddr_t)&(sin->sin_addr),
			    sizeof sin->sin_addr, AF_INET);
		else
			hp = NULL;

		host = hp ? hp->h_name : "?";

		(void)printf("%s (%s) at ", host, inet_ntoa(sin->sin_addr));
		if (sdl->sdl_alen)
			sdl_print(sdl);
		else
			(void)printf("(incomplete)");

		if (sdl->sdl_index) {
			if (getifname(sdl->sdl_index, ifname) == 0)
				printf(" on %s", ifname);
		}

		if (rtm->rtm_rmx.rmx_expire == 0)
			(void)printf(" permanent");
		if (sin->sin_other & SIN_PROXY)
			(void)printf(" published (proxy only)");
		if (rtm->rtm_addrs & RTA_NETMASK) {
			sin = (struct sockaddr_inarp *)
				(ROUNDUP(sdl->sdl_len) + (char *)sdl);
			if (sin->sin_addr.s_addr == 0xffffffff)
				(void)printf(" published");
			if (sin->sin_len != 8)
				(void)printf("(weird)");
		}
		(void)printf("\n");
	}
}

/*
 * Delete the entire arp table
 */
void
delete_all(void)
{
	int mib[6];
	size_t needed;
	char addr[sizeof("000.000.000.000\0")];
	char *lim, *buf, *next;
	struct rt_msghdr *rtm;
	struct sockaddr_inarp *sin;
	struct sockaddr_dl *sdl;

	mib[0] = CTL_NET;
	mib[1] = PF_ROUTE;
	mib[2] = 0;
	mib[3] = AF_INET;
	mib[4] = NET_RT_FLAGS;
	mib[5] = RTF_LLINFO;
	if (sysctl(mib, 6, NULL, &needed, NULL, 0) < 0)
		err(1, "route-sysctl-estimate");
	if (needed == 0)
		return;
	if ((buf = malloc(needed)) == NULL)
		err(1, "malloc");
	if (sysctl(mib, 6, buf, &needed, NULL, 0) < 0)
		err(1, "actual retrieval of routing table");
	lim = buf + needed;
	for (next = buf; next < lim; next += rtm->rtm_msglen) {
		rtm = (struct rt_msghdr *)next;
		sin = (struct sockaddr_inarp *)(rtm + 1);
		sdl = (struct sockaddr_dl *)
		    (ROUNDUP(sin->sin_len) + (char *)sin);
		snprintf(addr, sizeof(addr), "%s", inet_ntoa(sin->sin_addr));
		delete(addr, NULL);
	}
}

void
sdl_print(sdl)
	const struct sockaddr_dl *sdl;
{
	char hbuf[NI_MAXHOST];

	if (getnameinfo((struct sockaddr *)sdl, sdl->sdl_len,
	    hbuf, sizeof(hbuf), NULL, 0, NI_NUMERICHOST) != 0)
		printf("<invalid>");
	else
		printf("%s", hbuf);
}

int
atosdl(s, sdl)
	const char *s;
	struct sockaddr_dl *sdl;
{
	int i;
	long b;
	caddr_t endp;
	caddr_t p; 
	char *t, *r;

	p = LLADDR(sdl);
	endp = ((caddr_t)sdl) + sdl->sdl_len;
	i = 0;
	
	b = strtol(s, &t, 16);
	if (t == s)
		return 1;

	*p++ = b;
	++i;
	while ((p < endp) && (*t++ == ':')) {
		b = strtol(t, &r, 16);
		if (r == t)
			break;
		*p++ = b;
		++i;
		t = r;
	}
	sdl->sdl_alen = i;

	return 0;
}

void
usage()
{

	(void)fprintf(stderr, "usage: %s [-n] hostname\n", progname);
	(void)fprintf(stderr, "usage: %s [-n] -a\n", progname);
	(void)fprintf(stderr, "usage: %s -d [-a|hostname]\n", progname);
	(void)fprintf(stderr,
	    "usage: %s -s hostname ether_addr [temp] [pub]\n", progname);
	(void)fprintf(stderr, "usage: %s -f filename\n", progname);
	exit(1);
}

int
rtmsg(cmd)
	int cmd;
{
	static int seq;
	int rlen;
	struct rt_msghdr *rtm;
	char *cp;
	int l;

	rtm = &m_rtmsg.m_rtm;
	cp = m_rtmsg.m_space;
	errno = 0;

	if (cmd == RTM_DELETE)
		goto doit;
	(void)memset(&m_rtmsg, 0, sizeof(m_rtmsg));
	rtm->rtm_flags = flags;
	rtm->rtm_version = RTM_VERSION;

	switch (cmd) {
	default:
		errx(1, "internal wrong cmd");
		/*NOTREACHED*/
	case RTM_ADD:
		rtm->rtm_addrs |= RTA_GATEWAY;
		rtm->rtm_rmx.rmx_expire = expire_time;
		rtm->rtm_inits = RTV_EXPIRE;
		rtm->rtm_flags |= (RTF_HOST | RTF_STATIC);
		sin_m.sin_other = 0;
		if (doing_proxy) {
			if (export_only)
				sin_m.sin_other = SIN_PROXY;
			else {
				rtm->rtm_addrs |= RTA_NETMASK;
				rtm->rtm_flags &= ~RTF_HOST;
			}
		}
		/* FALLTHROUGH */
	case RTM_GET:
		rtm->rtm_addrs |= RTA_DST;
	}

#define NEXTADDR(w, s) \
	if (rtm->rtm_addrs & (w)) { \
		(void)memcpy(cp, &s, ((struct sockaddr *)&s)->sa_len); \
		cp += ROUNDUP(((struct sockaddr *)&s)->sa_len); \
	}

	NEXTADDR(RTA_DST, sin_m);
	NEXTADDR(RTA_GATEWAY, sdl_m);
	NEXTADDR(RTA_NETMASK, so_mask);

	rtm->rtm_msglen = cp - (char *)&m_rtmsg;
doit:
	l = rtm->rtm_msglen;
	rtm->rtm_seq = ++seq;
	rtm->rtm_type = cmd;
	if ((rlen = write(s, (char *)&m_rtmsg, l)) < 0) {
		if (errno != ESRCH || cmd != RTM_DELETE) {
			warn("writing to routing socket");
			return (-1);
		}
	}
	do {
		l = read(s, (char *)&m_rtmsg, sizeof(m_rtmsg));
	} while (l > 0 && (rtm->rtm_seq != seq || rtm->rtm_pid != pid));
	if (l < 0)
		warn("read from routing socket");
	return (0);
}

int
getinetaddr(host, inap)
	const char *host;
	struct in_addr *inap;
{
	struct hostent *hp;

	if (inet_aton(host, inap) == 1)
		return (0);
	if ((hp = gethostbyname(host)) == NULL) {
		warnx("%s: %s", host, hstrerror(h_errno));
		return (-1);
	}
	(void)memcpy(inap, hp->h_addr, sizeof(*inap));
	return (0);
}

int
getifname(ifindex, ifname)
	u_int16_t ifindex;
	char* ifname;
{
	int i, idx, siz;
	char ifrbuf[8192];
	struct ifreq ifreq, *ifr;
	const struct sockaddr_dl *sdl = NULL;

	if (is < 0) {
		is = socket(PF_INET, SOCK_DGRAM, 0);
		if (is < 0)
			err(1, "socket");

		ifc.ifc_len = sizeof(ifconfbuf);
		ifc.ifc_buf = ifconfbuf;

		if (ioctl(is, SIOCGIFCONF, &ifc) < 0) {
			close(is);
			err(1, "SIOCGIFCONF");
			is = -1;
		}
	}

	ifr = ifc.ifc_req;
	ifreq.ifr_name[0] = '\0';
	for (i = 0, idx = 0; i < ifc.ifc_len; ) {
		ifr = (struct ifreq *)((caddr_t)ifc.ifc_req + i);
		siz = sizeof(ifr->ifr_name) +
			(ifr->ifr_addr.sa_len > sizeof(struct sockaddr)
				? ifr->ifr_addr.sa_len
				: sizeof(struct sockaddr));
		i += siz;
		/* avoid alignment issue */
		if (sizeof(ifrbuf) < siz)
			errx(1, "ifr too big");

		memcpy(ifrbuf, ifr, siz);
		ifr = (struct ifreq *)ifrbuf;

		if (ifr->ifr_addr.sa_family != AF_LINK)
			continue;

		sdl = (const struct sockaddr_dl *) &ifr->ifr_addr;
		if (sdl && sdl->sdl_index == ifindex) {
			(void) strncpy(ifname, ifr->ifr_name, IFNAMSIZ);
			return 0;
		}
	}

	return -1;
}
