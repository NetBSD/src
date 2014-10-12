/*	$NetBSD: t_mcast.c,v 1.2 2014/10/12 13:48:25 christos Exp $	*/

/*-
 * Copyright (c) 2014 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Christos Zoulas.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
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
#include <sys/cdefs.h>
__RCSID("$NetBSD: t_mcast.c,v 1.2 2014/10/12 13:48:25 christos Exp $");

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include <assert.h>
#include <netdb.h>
#include <time.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <err.h>
#include <errno.h>
#include <poll.h>
#include <stdbool.h>

#ifndef TEST
#include <atf-c.h>

#define ERRX(ev, msg, ...)	ATF_REQUIRE_MSG(0, msg, __VA_ARGS__)

#define SKIPX(ev, msg, ...)	do {			\
	atf_tc_skip(msg, __VA_ARGS__);			\
	return;						\
} while(/*CONSTCOND*/0)

#else
#define ERRX(ev, msg, ...)	errx(ev, msg, __VA_ARGS__)
#define SKIPX(ev, msg, ...)	errx(ev, msg, __VA_ARGS__)
#endif

static int debug;

#define TOTAL 10
#define PORT_V4MAPPED "6666"
#define HOST_V4MAPPED "::FFFF:239.1.1.1"
#define PORT_V4 "6666"
#define HOST_V4 "239.1.1.1"
#define PORT_V6 "6666"
#define HOST_V6 "FF05:0:0:0:0:0:0:1"

static int
addmc(int s, struct addrinfo *ai)
{
	struct ip_mreq 	m4;
	struct ipv6_mreq m6;
	struct sockaddr_in *s4;
	struct sockaddr_in6 *s6;
	
	switch (ai->ai_family) {
	case AF_INET:
		s4 = (void *)ai->ai_addr;
		assert(sizeof(*s4) == ai->ai_addrlen);
		m4.imr_multiaddr = s4->sin_addr;
		m4.imr_interface.s_addr = htonl(INADDR_ANY);
		return setsockopt(s, IPPROTO_IP, IP_ADD_MEMBERSHIP,
		    &m4, sizeof(m4));
	case AF_INET6:
		s6 = (void *)ai->ai_addr;
		// XXX: Both linux and we do this thing wrong...
		if (IN6_IS_ADDR_V4MAPPED(&s6->sin6_addr)) {
			memcpy(&m4.imr_multiaddr, &s6->sin6_addr.s6_addr[12],
			    sizeof(m4.imr_multiaddr));
			m4.imr_interface.s_addr = htonl(INADDR_ANY);
			return setsockopt(s, IPPROTO_IP, IP_ADD_MEMBERSHIP,
			    &m4, sizeof(m4));
		}
		assert(sizeof(*s6) == ai->ai_addrlen);
		memset(&m6, 0, sizeof(m6));
		m6.ipv6mr_interface = 0;
	        m6.ipv6mr_multiaddr = s6->sin6_addr;
		return setsockopt(s, IPPROTO_IPV6, IPV6_JOIN_GROUP,
		    &m6, sizeof(m6));
	default:
		errno = EOPNOTSUPP;
		return -1;
	}
}

static int
allowv4mapped(int s, struct addrinfo *ai)
{
	struct sockaddr_in6 *s6;
	int zero = 0;

	if (ai->ai_family != AF_INET6)
		return 0;

	s6 = (void *)ai->ai_addr;

	if (!IN6_IS_ADDR_V4MAPPED(&s6->sin6_addr))
		return 0;
	return setsockopt(s, IPPROTO_IPV6, IPV6_V6ONLY, &zero, sizeof(zero));
}

static struct sockaddr_storage ss;
static int
connector(int fd, const struct sockaddr *sa, socklen_t slen)
{
	assert(sizeof(ss) > slen);
	memcpy(&ss, sa, slen);
	return 0;
}

static int
getsocket(const char *host, const char *port,
    int (*f)(int, const struct sockaddr *, socklen_t))
{
	int e, s;
	struct addrinfo hints, *ai0, *ai;
	const char *cause = "?";

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_DGRAM;
	e = getaddrinfo(host, port, &hints, &ai0);
	if (e)
		ERRX(EXIT_FAILURE, "Can't resolve %s:%s (%s)", host, port,
		    gai_strerror(e));

	s = -1;
	for (ai = ai0; ai; ai = ai->ai_next) {
		s = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
		if (s == -1) {
			cause = "socket";
			continue;
		}
		if (allowv4mapped(s, ai) == -1) {
			cause = "allow v4 mapped";
			goto out;
		}
		if ((*f)(s, ai->ai_addr, ai->ai_addrlen) == -1) {
			cause = f == bind ? "bind" : "connect";
			goto out;
		}
		if ((f == bind || f == connector) && addmc(s, ai) == -1) {
			cause = "join group";
			goto out;
		}
		break;
out:
		close(s);
		s = -1;
		continue;
	}
	freeaddrinfo(ai0);
	if (s == -1)
		ERRX(1, "%s (%s)", cause, strerror(errno));
	return s;
}

static void
sender(const char *host, const char *port, size_t n, bool conn)
{
	int s;
	ssize_t l;
	size_t seq;
	char buf[64];

	s = getsocket(host, port, conn ? connect : connector);
	for (seq = 0; seq < n; seq++) {
		time_t t = time(&t);
		snprintf(buf, sizeof(buf), "%zu: %-24.24s", seq, ctime(&t));
		if (debug)
			printf("sending: %s\n", buf);
		l = conn ? send(s, buf, sizeof(buf), 0) :
		    sendto(s, buf, sizeof(buf), 0, (void *)&ss, ss.ss_len);
		if (l == -1)
			ERRX(EXIT_FAILURE, "send (%s)", strerror(errno));
		usleep(100);
	}
}

static void
receiver(const char *host, const char *port, size_t n, bool conn)
{
	int s;
	ssize_t l;
	size_t seq;
	char buf[64];
	struct pollfd pfd;
	socklen_t slen;

	s = getsocket(host, port, conn ? bind : connector);
	pfd.fd = s;
	pfd.events = POLLIN;
	for (seq = 0; seq < n; seq++) {
		if (poll(&pfd, 1, 1000) == -1)
			ERRX(EXIT_FAILURE, "poll (%s)", strerror(errno));
		slen = ss.ss_len;
		l = conn ? recv(s, buf, sizeof(buf), 0) :
		    recvfrom(s, buf, sizeof(buf), 0, (void *)&ss, &slen);
		if (l == -1)
			ERRX(EXIT_FAILURE, "recv (%s)", strerror(errno));
		if (debug)
			printf("got: %s\n", buf);
	}
}

static void
run(const char *host, const char *port, size_t n, bool conn)
{
	switch (fork()) {
	case 0:
		receiver(host, port, n, conn);
		return;
	case -1:
		ERRX(EXIT_FAILURE, "fork (%s)", strerror(errno));
	default:
		usleep(100);
		sender(host, port, n, conn);
		return;
	}
}

#ifdef TEST
int
main(int argc, char *argv[])
{
	const char *host, *port;
	int c;
	size_t n;
	bool conn;

	host = HOST_V4;
	port = PORT_V4;
	n = TOTAL;
	conn = false;

	while ((c = getopt(argc, argv, "46cdmn:")) != -1)
		switch (c) {
		case '4':
			host = HOST_V4;
			port = PORT_V4;
			break;
		case '6':
			host = HOST_V6;
			port = PORT_V6;
			break;
		case 'c':
			conn = true;
			break;
		case 'd':
			debug++;
			break;
		case 'm':
			host = HOST_V4MAPPED;
			port = PORT_V4MAPPED;
			break;
		case 'n':
			n = atoi(optarg);
			break;
		default:
			fprintf(stderr, "Usage: %s [-cdm46] [-n <tot>]",
			    getprogname());
			return 1;
		}

	run(host, port, n, conn);
	return 0;
}
#else

ATF_TC(conninet4);
ATF_TC_HEAD(conninet4, tc)
{
	atf_tc_set_md_var(tc, "descr", "Checks connected multicast for ipv4");
}

ATF_TC_BODY(conninet4, tc)
{
	run(HOST_V4, PORT_V4, TOTAL, true);
}

ATF_TC(connmappedinet4);
ATF_TC_HEAD(connmappedinet4, tc)
{
	atf_tc_set_md_var(tc, "descr", "Checks connected multicast for mapped ipv4");
}

ATF_TC_BODY(connmappedinet4, tc)
{
	run(HOST_V4MAPPED, PORT_V4MAPPED, TOTAL, true);
}

ATF_TC(conninet6);
ATF_TC_HEAD(conninet6, tc)
{
	atf_tc_set_md_var(tc, "descr", "Checks connected multicast for ipv6");
}

ATF_TC_BODY(conninet6, tc)
{
	run(HOST_V6, PORT_V6, TOTAL, true);
}

ATF_TC(unconninet4);
ATF_TC_HEAD(unconninet4, tc)
{
	atf_tc_set_md_var(tc, "descr", "Checks unconnected multicast for ipv4");
}

ATF_TC_BODY(unconninet4, tc)
{
	run(HOST_V4, PORT_V4, TOTAL, false);
}

ATF_TC(unconnmappedinet4);
ATF_TC_HEAD(unconnmappedinet4, tc)
{
	atf_tc_set_md_var(tc, "descr", "Checks unconnected multicast for mapped ipv4");
}

ATF_TC_BODY(unconnmappedinet4, tc)
{
	run(HOST_V4MAPPED, PORT_V4MAPPED, TOTAL, false);
}

ATF_TC(unconninet6);
ATF_TC_HEAD(unconninet6, tc)
{
	atf_tc_set_md_var(tc, "descr", "Checks unconnected multicast for ipv6");
}

ATF_TC_BODY(unconninet6, tc)
{
	run(HOST_V6, PORT_V6, TOTAL, false);
}

ATF_TP_ADD_TCS(tp)
{
        ATF_TP_ADD_TC(tp, conninet4);
        ATF_TP_ADD_TC(tp, connmappedinet4);
        ATF_TP_ADD_TC(tp, conninet6);
        ATF_TP_ADD_TC(tp, unconninet4);
        ATF_TP_ADD_TC(tp, unconnmappedinet4);
        ATF_TP_ADD_TC(tp, unconninet6);

	return atf_no_error();
}
#endif
