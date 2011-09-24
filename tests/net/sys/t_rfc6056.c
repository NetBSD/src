
#include <sys/cdefs.h>
__RCSID("$NetBSD: t_rfc6056.c,v 1.1 2011/09/24 18:34:18 christos Exp $");

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/udp.h>
#include <arpa/inet.h>
#include <string.h>
#include <strings.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <netdb.h>
#include <err.h>

#include <atf-c.h>

static void
test(const char *hostname, const char *service, int family, int al)
{
	static const char hello[] = "hello\n";
	int s, error;
	struct sockaddr_storage ss;
	struct addrinfo hints, *res;
	
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = family;
	hints.ai_socktype = SOCK_DGRAM;

	error = getaddrinfo(hostname, service, &hints, &res);
	if (error)
		errx(EXIT_FAILURE, "Cannot get address for %s (%s)",
		    hostname, gai_strerror(error));
	
	s = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
	if (s == -1)
		err(EXIT_FAILURE, "socket");
	
	if (setsockopt(s, IPPROTO_UDP, UDP_RFC6056ALGO, &al, sizeof(al)) == -1)
		err(EXIT_FAILURE, "setsockopt");

	memset(&ss, 0, sizeof(ss));
	ss.ss_len = res->ai_addrlen;
	ss.ss_family = res->ai_family;

	if (bind(s, (struct sockaddr *)&ss, ss.ss_len) == -1)
		err(EXIT_FAILURE, "bind");
		
	if (sendto(s, hello, sizeof(hello) - 1, 0,
	    res->ai_addr, res->ai_addrlen) == -1)
		err(EXIT_FAILURE, "sendto");

	if (close(s) == -1)
		err(EXIT_FAILURE, "close");

	s = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
	if (s == -1)
		err(EXIT_FAILURE, "socket");

	if (setsockopt(s, IPPROTO_UDP, UDP_RFC6056ALGO, &al, sizeof(al)) == -1)
		err(EXIT_FAILURE, "setsockopt");

	if (connect(s, res->ai_addr, res->ai_addrlen) == -1)
		err(EXIT_FAILURE, "connect");

	if (send(s, hello, sizeof(hello) - 1, 0) == -1)
		err(EXIT_FAILURE, "send");

	if (close(s) == -1)
		err(EXIT_FAILURE, "close");

	freeaddrinfo(res);
}

ATF_TC(inet4);
ATF_TC_HEAD(inet4, tc)
{
	atf_tc_set_md_var(tc, "descr", "Checks random port allocation "
	    "for ipv4");
}

ATF_TC_BODY(inet4, tc)
{
	for (int i = 0; i < 6; i++)
		test("localhost", "http", AF_INET, i);
}

ATF_TC(inet6);
ATF_TC_HEAD(inet6, tc)
{
	atf_tc_set_md_var(tc, "descr", "Checks random port allocation "
	    "for ipv6");
}

ATF_TC_BODY(inet6, tc)
{
	for (int i = 0; i < 6; i++)
		test("localhost", "http", AF_INET6, i);
}

ATF_TP_ADD_TCS(tp)
{
        ATF_TP_ADD_TC(tp, inet4);
        ATF_TP_ADD_TC(tp, inet6);

	return atf_no_error();
}
