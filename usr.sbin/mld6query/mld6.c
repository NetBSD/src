/*	$NetBSD: mld6.c,v 1.12.28.1 2009/05/13 19:20:29 jym Exp $	*/
/*	$KAME: mld6.c,v 1.9 2000/12/04 06:29:37 itojun Exp $	*/

/*
 * Copyright (C) 1998 WIDE Project.
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
#include <sys/uio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/time.h>
#include <poll.h>
#include <unistd.h>
#include <signal.h>

#include <net/if.h>
#if defined(__FreeBSD__) && __FreeBSD__ >= 3
#include <net/if_var.h>
#endif /* __FreeBSD__ >= 3 */

#include <netinet/in.h>
#include <netinet/ip6.h>
#include <netinet/icmp6.h>

#include  <arpa/inet.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <err.h>

struct msghdr m;
struct sockaddr_in6 dst;
struct mld6_hdr mldh;
struct in6_addr maddr = IN6ADDR_ANY_INIT, any = IN6ADDR_ANY_INIT;
struct ipv6_mreq mreq;
u_short ifindex;
int sock;

#define QUERY_RESPONSE_INTERVAL 10000

void make_msg(int index, struct in6_addr *addr, u_int type);
void usage(void);
void dump(int);
void quit(int);

int
main(int argc, char *argv[])
{
	int i;
	struct icmp6_filter filt;
	u_int hlim = 1;
	struct pollfd set[1];
	struct itimerval itimer;
	u_int type;
	int ch;

	type = MLD_LISTENER_QUERY;
	while ((ch = getopt(argc, argv, "d")) != -1) {
		switch (ch) {
		case 'd':
			type = MLD_LISTENER_DONE;
			break;
		case 'r':
			type = MLD_LISTENER_REPORT;
			break;
		default:
			usage();
			/*NOTREACHED*/
		}
	}

	argv += optind;
	argc -= optind;
	
	if (argc != 1 && argc != 2)
		usage();

	ifindex = (u_short)if_nametoindex(argv[0]);
	if (ifindex == 0)
		usage();
	if (argc == 3 && inet_pton(AF_INET6, argv[1], &maddr) != 1)
		usage();

	if ((sock = socket(AF_INET6, SOCK_RAW, IPPROTO_ICMPV6)) < 0)
		err(1, "socket");

	if (setsockopt(sock, IPPROTO_IPV6, IPV6_MULTICAST_HOPS, &hlim,
		       sizeof(hlim)) == -1)
		err(1, "setsockopt(IPV6_MULTICAST_HOPS)");

	mreq.ipv6mr_multiaddr = any;
	mreq.ipv6mr_interface = ifindex;
	if (setsockopt(sock, IPPROTO_IPV6, IPV6_JOIN_GROUP, &mreq,
		       sizeof(mreq)) == -1)
		err(1, "setsockopt(IPV6_JOIN_GROUP)");

	ICMP6_FILTER_SETBLOCKALL(&filt);
	ICMP6_FILTER_SETPASS(ICMP6_MEMBERSHIP_QUERY, &filt);
	ICMP6_FILTER_SETPASS(ICMP6_MEMBERSHIP_REPORT, &filt);
	ICMP6_FILTER_SETPASS(ICMP6_MEMBERSHIP_REDUCTION, &filt);
	if (setsockopt(sock, IPPROTO_ICMPV6, ICMP6_FILTER, &filt,
			sizeof(filt)) < 0)
		err(1, "setsockopt(ICMP6_FILTER)");

	make_msg(ifindex, &maddr, type);

	if (sendmsg(sock, &m, 0) < 0)
		err(1, "sendmsg");

	itimer.it_value.tv_sec =  QUERY_RESPONSE_INTERVAL / 1000;
	itimer.it_interval.tv_sec = 0;
	itimer.it_interval.tv_usec = 0;
	itimer.it_value.tv_usec = 0;

	(void)signal(SIGALRM, quit);
	(void)setitimer(ITIMER_REAL, &itimer, NULL);

	set[0].fd = sock;
	set[0].events = POLLIN;
	for (;;) {
		if ((i = poll(set, 1, INFTIM)) < 0)
			perror("poll");
		if (i == 0)
			continue;
		else
			dump(sock);
	}
}

void
make_msg(int idx, struct in6_addr *addr, u_int type)
{
	static struct iovec iov[2];
	static u_char *cmsgbuf;
	int cmsglen, hbhlen = 0;
#ifdef USE_RFC2292BIS
	void *hbhbuf = NULL, *optp = NULL;
	int currentlen;
#else
	u_int8_t raopt[IP6OPT_RTALERT_LEN];
#endif 
	struct in6_pktinfo *pi;
	struct cmsghdr *cmsgp;
	u_short rtalert_code = htons(IP6OPT_RTALERT_MLD);

	dst.sin6_len = sizeof(dst);
	dst.sin6_family = AF_INET6;
	if (IN6_IS_ADDR_UNSPECIFIED(addr)) {
		if (inet_pton(AF_INET6, "ff02::1", &dst.sin6_addr) != 1)
			errx(1, "inet_pton failed");
	}
	else
		dst.sin6_addr = *addr;
	m.msg_name = (caddr_t)&dst;
	m.msg_namelen = dst.sin6_len;
	iov[0].iov_base = (caddr_t)&mldh;
	iov[0].iov_len = sizeof(mldh);
	m.msg_iov = iov;
	m.msg_iovlen = 1;

	bzero(&mldh, sizeof(mldh));
	mldh.mld6_type = type & 0xff;
	mldh.mld6_maxdelay = htons(QUERY_RESPONSE_INTERVAL);
	mldh.mld6_addr = *addr;

#ifdef USE_RFC2292BIS
	if ((hbhlen = inet6_opt_init(NULL, 0)) == -1)
		errx(1, "inet6_opt_init(0) failed");
	if ((hbhlen = inet6_opt_append(NULL, 0, hbhlen, IP6OPT_ROUTER_ALERT, 2,
				       2, NULL)) == -1)
		errx(1, "inet6_opt_append(0) failed");
	if ((hbhlen = inet6_opt_finish(NULL, 0, hbhlen)) == -1)
		errx(1, "inet6_opt_finish(0) failed");
	cmsglen = CMSG_SPACE(sizeof(struct in6_pktinfo)) + CMSG_SPACE(hbhlen);
#else
	hbhlen = sizeof(raopt); 
	cmsglen = CMSG_SPACE(sizeof(struct in6_pktinfo)) +
	    inet6_option_space(hbhlen);
#endif 

	if ((cmsgbuf = malloc(cmsglen)) == NULL)
		errx(1, "can't allocate enough memory for cmsg");
	cmsgp = (struct cmsghdr *)cmsgbuf;
	m.msg_control = (caddr_t)cmsgbuf;
	m.msg_controllen = cmsglen;
	/* specify the outgoing interface */
	cmsgp->cmsg_len = CMSG_LEN(sizeof(struct in6_pktinfo));
	cmsgp->cmsg_level = IPPROTO_IPV6;
	cmsgp->cmsg_type = IPV6_PKTINFO;
	pi = (struct in6_pktinfo *)CMSG_DATA(cmsgp);
	pi->ipi6_ifindex = idx;
	memset(&pi->ipi6_addr, 0, sizeof(pi->ipi6_addr));
	/* specifiy to insert router alert option in a hop-by-hop opt hdr. */
	cmsgp = CMSG_NXTHDR(&m, cmsgp);
#ifdef USE_RFC2292BIS
	cmsgp->cmsg_len = CMSG_LEN(hbhlen);
	cmsgp->cmsg_level = IPPROTO_IPV6;
	cmsgp->cmsg_type = IPV6_HOPOPTS;
	hbhbuf = CMSG_DATA(cmsgp);
	if ((currentlen = inet6_opt_init(hbhbuf, hbhlen)) == -1)
		errx(1, "inet6_opt_init(len = %d) failed", hbhlen);
	if ((currentlen = inet6_opt_append(hbhbuf, hbhlen, currentlen,
					   IP6OPT_ROUTER_ALERT, 2,
					   2, &optp)) == -1)
		errx(1, "inet6_opt_append(currentlen = %d, hbhlen = %d) failed",
		     currentlen, hbhlen);
	(void)inet6_opt_set_val(optp, 0, &rtalert_code, sizeof(rtalert_code));
	if ((currentlen = inet6_opt_finish(hbhbuf, hbhlen, currentlen)) == -1)
		errx(1, "inet6_opt_finish(buf) failed");
#else  /* old advanced API */
	if (inet6_option_init((void *)cmsgp, &cmsgp, IPV6_HOPOPTS))
		errx(1, "inet6_option_init failed");
	raopt[0] = IP6OPT_RTALERT;
	raopt[1] = IP6OPT_RTALERT_LEN - 2;
	memcpy(&raopt[2], (caddr_t)&rtalert_code, sizeof(u_short));
	if (inet6_option_append(cmsgp, raopt, 4, 0))
		errx(1, "inet6_option_append failed");
#endif 
}

void
dump(int s)
{
	int i;
	struct mld6_hdr *mld;
	u_char buf[1024];
	struct sockaddr_in6 from;
	socklen_t from_len = sizeof(from);
	char ntop_buf[256];

	if ((i = recvfrom(s, buf, sizeof(buf), 0,
			  (struct sockaddr *)&from,
			  &from_len)) < 0)
		return;

	if (i < (int)sizeof(struct mld6_hdr)) {
		printf("too short!\n");
		return;
	}

	mld = (struct mld6_hdr *)buf;

	printf("from %s, ", inet_ntop(AF_INET6, &from.sin6_addr,
				      ntop_buf, sizeof(ntop_buf)));

	switch (mld->mld6_type) {
	case ICMP6_MEMBERSHIP_QUERY:
		printf("type=Multicast Listener Query, ");
		break;
	case ICMP6_MEMBERSHIP_REPORT:
		printf("type=Multicast Listener Report, ");
		break;
	case ICMP6_MEMBERSHIP_REDUCTION:
		printf("type=Multicast Listener Done, ");
		break;
	}
	printf("addr=%s\n", inet_ntop(AF_INET6, &mld->mld6_addr,
				    ntop_buf, sizeof(ntop_buf)));
	
	fflush(stdout);
}

/* ARGSUSED */
void
quit(int signum) {
	mreq.ipv6mr_multiaddr = any;
	mreq.ipv6mr_interface = ifindex;
	if (setsockopt(sock, IPPROTO_IPV6, IPV6_LEAVE_GROUP, &mreq,
		       sizeof(mreq)) == -1)
		err(1, "setsockopt(IPV6_LEAVE_GROUP)");

	exit(0);
}

void
usage()
{
	(void)fprintf(stderr, "usage: %s [-dr] ifname [addr]\n", getprogname());
	exit(1);
}
