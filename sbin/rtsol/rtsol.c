/*	$NetBSD: rtsol.c,v 1.2 1999/07/04 02:43:39 itojun Exp $	*/

/*
 * Copyright (C) 1995, 1996, 1997, and 1998 WIDE Project.
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

#include <sys/param.h>
#include <sys/socket.h>
#ifdef ADVAPI
#include <sys/uio.h>
#endif
#include <sys/ioctl.h>

#include <net/if.h>
#if defined(__FreeBSD__) && __FreeBSD__ >= 3
#include <net/if_var.h>
#endif /* __FreeBSD__ >= 3 */
#include <netinet/in.h>
#include <netinet/ip6.h>
#include <netinet/icmp6.h>
#include <unistd.h>
#include <stdio.h>
#include <err.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/sysctl.h>
#include <netdb.h>
#include <netinet6/in6.h>
#include <netinet6/in6_var.h>
#include <netinet6/nd6.h>

#define ALLROUTER	"ff02::2"

int main __P((int, char **));
void usage __P((void));
int sendit __P((int, char *, int, struct sockaddr_in6 *, int));
int interface_down __P((char *));
int interface_up __P((char *));
static int getifflag6 __P((char *, int *));
static int getdadcount __P((void));

static int trick = 1;
static int verbose = 0;
#define vmsg(x)		{ if (verbose) fprintf x; }

extern char *optarg;
extern int optind;

int
main(argc, argv)
	int argc;
	char *argv[];
{
	struct sockaddr_in6 to;
	struct icmp6_hdr *icp;
	u_int hlim = 255;
	u_char outpack[sizeof(struct icmp6_hdr)];
	u_short index;
	int s, i, cc = sizeof(struct icmp6_hdr);
	int opt;
	fd_set fdset;
	struct timeval timeout;
	int rtsol_retry = MAX_RTR_SOLICITATIONS;
	
	while ((opt = getopt(argc, argv, "nv")) != EOF) {
		switch (opt) {
		case 'n':
			trick = 0;
			break;
		case 'v':
			verbose++;
			break;
		default:
			usage();
			/*NOTREACHED*/
		}
	}
	argc -= optind;
	argv += optind;
	if (argc != 1) {
		usage();
		/*NOTREACHED*/
	}

	index = (u_short)if_nametoindex(argv[0]);
	if (index == 0) {
		errx(1, "invalid interface %s", argv[0]);
		/*NOTREACHED*/
	}

	if (trick) {
		/*
		 * Notebook hack:
		 * It looks that, most of the pccard drivers do not reset
		 * multicast packet filter properly on suspend/resume session.
		 * bring interface down, then up, to initialize it properly.
		 */
		if (interface_down(argv[0]) < 0)
			err(1, "interface state could not be changed");
		vmsg((stderr, "a bit of delay...\n"));
		usleep(500 * 1000);	/* 0.5s */
	}

	if (interface_up(argv[0]) < 0)
		err(1, "interface not ready");

    {
	struct addrinfo hints, *res;
	int error;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET6;
	error = getaddrinfo(ALLROUTER, NULL, &hints, &res);
	if (error) {
		errx(1, "getaddrinfo: %s", gai_strerror(error));
		/*NOTREACHED*/
	}
	memcpy(&to, res->ai_addr, res->ai_addrlen);
    }
	
	if ((s = socket(AF_INET6, SOCK_RAW, IPPROTO_ICMPV6)) < 0)
		err(1, "socket");

	if (setsockopt(s, IPPROTO_IPV6, IPV6_MULTICAST_HOPS, &hlim,
			sizeof(hlim)) == -1)
		err(1, "setsockopt(IPV6_MULTICAST_HOPS)");
#ifdef ICMP6_FILTER
    {
	struct icmp6_filter filt;
	ICMP6_FILTER_SETBLOCKALL(&filt);
	ICMP6_FILTER_SETPASS(ND_ROUTER_ADVERT, &filt);
	if (setsockopt(s, IPPROTO_ICMPV6, ICMP6_FILTER, &filt,
			sizeof(filt)) == -1)
		err(1, "setsockopt(ICMP6_FILTER)");
    }
#endif
	
	/* fill ICMP header */
	icp = (struct icmp6_hdr *)outpack;
	icp->icmp6_type = ND_ROUTER_SOLICIT;
	icp->icmp6_code = 0;
	icp->icmp6_cksum = 0;
	icp->icmp6_data32[0] = 0; /* RS reserved field */

	vmsg((stderr, "sending RS for %d time%s:\n",
		rtsol_retry, (1 < rtsol_retry) ? "s" : ""));

retry:
	vmsg((stderr, "%d \r", rtsol_retry));

	i = sendit(s, (char *)outpack, cc, &to, index);
	if (i < 0) {
		err(1, "rtsol: solicitation");
		/*NOTREACHED*/
	} else if (i != cc) {
		fprintf(stderr, "rtsol: short write (wrote %d of be %d)\n",
			i, cc);
	}
	FD_ZERO(&fdset);
	FD_SET(s, &fdset);
	timeout.tv_sec = 1;
	timeout.tv_usec = 0;
	if (select(s + 1, &fdset, NULL, NULL, &timeout) < 1) {
		if (0 < --rtsol_retry)
			goto retry;
	} else {
		vmsg((stderr, "got a RA packet\n"));
	}

	close(s);
	exit(0);
	/*NOTREACHED*/
}

void
usage()
{
	(void)fprintf(stderr, "usage: rtsol [-nv] ifname\n");
	exit(1);
	/*NOTREACHED*/
}

int
sendit(s, p, len, sock, ifindex)
	int s;
	char *p;
	int len;
	struct sockaddr_in6 *sock;
	int ifindex;
{
#ifndef ADVAPI
	sock->sin6_addr.u6_addr.u6_addr16[1] = htons(ifindex);
	return sendto(s, p, len, 0, (struct sockaddr *)sock, sock->sin6_len);
#else
	struct msghdr m;
	struct cmsghdr *cm;
	struct iovec iov[2];
	u_char cmsgbuf[256];
	struct in6_pktinfo *pi;

	m.msg_name = (caddr_t)sock;
	m.msg_namelen = sock->sin6_len;
	iov[0].iov_base = p;
	iov[0].iov_len = len;
	m.msg_iov = iov;
	m.msg_iovlen = 1;
	if (!ifindex) {
		m.msg_control = NULL;
		m.msg_controllen = 0;
	} else {
		memset(cmsgbuf, 0, sizeof(cmsgbuf));
		cm = (struct cmsghdr *)cmsgbuf;
		m.msg_control = (caddr_t)cm;
		m.msg_controllen = CMSG_SPACE(sizeof(struct in6_pktinfo));

		cm->cmsg_len = CMSG_LEN(sizeof(struct in6_pktinfo));
		cm->cmsg_level = IPPROTO_IPV6;
		cm->cmsg_type = IPV6_PKTINFO;
		pi = (struct in6_pktinfo *)CMSG_DATA(cm);
		memset(&pi->ipi6_addr, 0, sizeof(pi->ipi6_addr));	/*XXX*/
		pi->ipi6_ifindex = ifindex;
	}
	return sendmsg(s, &m, 0);
#endif
}

int
interface_down(name)
	char *name;
{
	int s;
	struct ifreq ifr;

	vmsg((stderr, "make interface %s down.\n", name));
	if ((s = socket(AF_INET6, SOCK_DGRAM, 0)) < 0) {
		err(1, "socket");
		/*NOTREACHED*/
	}
	strncpy(ifr.ifr_name, name, sizeof(ifr.ifr_name));

	if (ioctl(s, SIOCGIFFLAGS, (caddr_t)&ifr) < 0)
		perror("ioctl(SIOCGIFFLAGS)");
	ifr.ifr_flags &= ~IFF_UP;
	if (ioctl(s, SIOCSIFFLAGS, (caddr_t)&ifr) < 0)
		perror("ioctl(SIOCSIFFLAGS)");
	close(s);
	return 0;
}

int
interface_up(name)
	char *name;
{
	int s;
	struct ifreq ifr;
	int flag6;
	int retry;
	int wasup;

	vmsg((stderr, "make interface %s up.\n", name));
	if ((s = socket(AF_INET6, SOCK_DGRAM, 0)) < 0) {
		err(1, "socket");
		/*NOTREACHED*/
	}
	strncpy(ifr.ifr_name, name, sizeof(ifr.ifr_name));

	if (ioctl(s, SIOCGIFFLAGS, (caddr_t)&ifr) < 0)
		perror("ioctl(SIOCGIFFLAGS)");
	if (ifr.ifr_flags & IFF_UP)
		wasup = 1;
	else {
		wasup = 0;
		ifr.ifr_flags |= IFF_UP;
		if (ioctl(s, SIOCSIFFLAGS, (caddr_t)&ifr) < 0)
			perror("ioctl(SIOCSIFFLAGS)");
	}
	close(s);

	retry = getdadcount();
	if (retry < 0) {
		retry = 10;
		vmsg((stderr, "invalid dad count; setting retry to %d\n",
			retry));
	} else {
		vmsg((stderr, "dad count is %d\n", retry));
		if (!wasup)
			retry += MAX_RTR_SOLICITATIONS;
		retry += 2;	/* to make sure */
	}
	vmsg((stderr, "checking if %s is ready...\n", name));
	while (0 < retry--) {
		if (getifflag6(name, &flag6) < 0) {
			vmsg((stderr, "getifflag6() failed, anyway I'll try\n"));
			return 0;
		}

		if (verbose) {
			fprintf(stderr, "%s:", name);
			if (!(flag6 & IN6_IFF_NOTREADY))
				fprintf(stderr, " ready\n");
			else {
				if (flag6 & IN6_IFF_TENTATIVE)
					fprintf(stderr, " tentative");
				if (flag6 & IN6_IFF_DUPLICATED)
					fprintf(stderr, " duplicatd");
				fprintf(stderr, "\n");
			}
		}

		if (flag6 & IN6_IFF_DUPLICATED)
			break;
		if (!(flag6 & IN6_IFF_NOTREADY))
			return 0;

		sleep(1);
	}
	fprintf(stderr, "DAD for if %s is in strange state\n", name);
	return -1;
}

/*------------------------------------------------------------*/

static int
getifflag6(name, flag6)
	char *name;
	int *flag6;
{
	int s;
	struct sockaddr_in6 *sin6;
#ifndef SIOCGLIFADDR
	int error;
	char buf[BUFSIZ];
	int i;
	struct ifreq *ifr;
	struct in6_ifreq ifr6;
	struct ifconf ifc;

	if ((s = socket(AF_INET6, SOCK_DGRAM, 0)) < 0) {
		err(1, "socket");
		/*NOTREACHED*/
	}

	ifc.ifc_len = sizeof(buf);
	ifc.ifc_buf = buf;
	if (ioctl(s, SIOCGIFCONF, &ifc) < 0) {
		err(1, "ioctl(SIOCGIFCONF)");
		/*NOTREACHED*/
	}

	error = -1;
	for (i = 0; i < ifc.ifc_len; ) {
		ifr = (struct ifreq *)((caddr_t)ifc.ifc_req + i);
		i += sizeof(ifr->ifr_name) +
			(ifr->ifr_addr.sa_len > sizeof(struct sockaddr)
				? ifr->ifr_addr.sa_len
				: sizeof(struct sockaddr));
		if (strncmp(name, ifr->ifr_name, sizeof(ifr->ifr_name)) != 0)
			continue;
		if (ifr->ifr_addr.sa_family != AF_INET6)
			continue;
		sin6 = (struct sockaddr_in6 *)&ifr->ifr_addr;
		if (!IN6_IS_ADDR_LINKLOCAL(&sin6->sin6_addr))
			continue;

		if (verbose) {
			char addrbuf[BUFSIZ];
			getnameinfo((struct sockaddr *)sin6, sin6->sin6_len,
				addrbuf, sizeof(addrbuf), NULL, 0,
				NI_NUMERICHOST);
			fprintf(stderr, "getting flags6 for if %s addr %s\n",
				ifr->ifr_name, addrbuf);
		}

		strcpy(ifr6.ifr_name, name);
		memcpy(&ifr6.ifr_ifru.ifru_addr, &ifr->ifr_addr,
			ifr->ifr_addr.sa_len);
		if (ioctl(s, SIOCGIFAFLAG_IN6, &ifr6) < 0) {
			err(1, "ioctl(SIOCGIFAFLAG_IN6)");
			/*NOTREACHED*/
		}
		error = 0;
		if (flag6)
			*flag6 = ifr6.ifr_ifru.ifru_flags6;
		break;
	}
	if (error < 0 && verbose)
		fprintf(stderr, "getifflags6: no matches\n");

	close(s);
	return error;
#else
	struct if_laddrreq iflr;

	if ((s = socket(AF_INET6, SOCK_DGRAM, 0)) < 0) {
		err(1, "socket");
		/*NOTREACHED*/
	}

	/* perform prefix match on SIOCGLIFADDR */
	memset(&iflr, 0, sizeof(iflr));
	strcpy(iflr.iflr_name, name);
	iflr.prefixlen = 16;
	iflr.flags = IFLR_PREFIX;
	sin6 = (struct sockaddr_in6 *)&iflr.addr;
	sin6->sin6_family = AF_INET6;
	sin6->sin6_len = sizeof(*sin6);
	sin6->sin6_addr.s6_addr16[0] = htons(0xfe80);	/*XXX hardcode*/
	if (ioctl(s, SIOCGLIFADDR, &iflr) < 0) {
		if (errno == EADDRNOTAVAIL)	/*no match - portability?*/
			return -1;
		err(1, "ioctl(SIOCGLIFADDR)");
		/*NOTREACHED*/
	}
	if (verbose) {
		char addrbuf[BUFSIZ];
		getnameinfo((struct sockaddr *)sin6, sin6->sin6_len,
			addrbuf, sizeof(addrbuf), NULL, 0,
			NI_NUMERICHOST);
		fprintf(stderr, "got flags6 for if %s addr %s=0x%08x\n",
			iflr.iflr_name, addrbuf, iflr.flags);
	}
	if (flag6)
		*flag6 = iflr.flags;
	close(s);
	return 0;
#endif
}

static int
getdadcount()
{
	int mib[] = { CTL_NET, PF_INET6, IPPROTO_IPV6, IPV6CTL_DAD_COUNT };
	int value;
	size_t size;

	size = sizeof(value);
	if (sysctl(mib, sizeof(mib)/sizeof(mib[0]), &value, &size, NULL, 0) < 0)
		return -1;
	else
		return value;
}
