/*	$NetBSD: t_inetd.c,v 1.2 2021/09/01 06:12:50 christos Exp $	*/

/*-
 * Copyright (c) 2021 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by James Browning, Gabe Coffland, Alex Gavin, and Solomon Ritzow.
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
__RCSID("$NetBSD: t_inetd.c,v 1.2 2021/09/01 06:12:50 christos Exp $");

#include <atf-c.h>
#include <spawn.h>
#include <errno.h>
#include <stdlib.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <err.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netdb.h>

#define CHECK_ERROR(expr) ATF_REQUIRE_MSG((expr) != -1,\
    "%s", strerror(errno))

#define TCP 6
#define UDP 17

static pid_t	run(const char *, char *const *);
static char	*concat(const char *restrict, const char *restrict);
static void	waitfor(pid_t, const char *);
static bool	run_udp_client(const char *);
static int	create_socket(const char *, const char *, int, int, time_t, struct sockaddr_storage *);
static bool	run_tcp_client(const char *);

/* This test should take around 5 to 7 seconds to complete. */
ATF_TC(test_ratelimit);

ATF_TC_HEAD(test_ratelimit, tc)
{
	atf_tc_set_md_var(tc, "descr", "Test inetd rate limiting values, "
	"uses UDP/TCP ports 5432-5439 with localhost.");
	/* Need to run as root so inetd can set uid */
	atf_tc_set_md_var(tc, "require.user", "root");
	atf_tc_set_md_var(tc, "require.progs", "inetd");
	/* Time out after 10 seconds, just in case */
	atf_tc_set_md_var(tc, "timeout", "10");
}

ATF_TC_BODY(test_ratelimit, tc)
{
	pid_t proc;

	/* Copy test server to relative path used in inetd_ratelimit.conf */
	atf_utils_copy_file(
	    concat(atf_tc_get_config_var(tc, "srcdir"), "/test_server"),
	    "test_server"
	);
	
	/* Run inetd in debug mode using specified config file */
	proc = run("inetd", (char* const []) {
	    __UNCONST("inetd"), __UNCONST("-d"),
	    concat(atf_tc_get_config_var(tc, "srcdir"),
	    "/inetd_ratelimit.conf"),
	    NULL
	});

	/* Wait for inetd to load services */
	sleep(1);

	/*
	 * TODO test dgram/nowait? Specified in manpage but doesn't seem to
	 * work
	 */

	/* dgram/wait ip_max of 3, should receive these 3 responses */
	for (int i = 0; i < 3; i++) {
		ATF_REQUIRE(run_udp_client("5432"));
	}
	
	/* Rate limiting should prevent a response to this request */
	ATF_REQUIRE(!run_udp_client("5432"));	

	/* dgram/wait ip_max of 0 */
	ATF_REQUIRE(!run_udp_client("5433"));

	/* dgram/wait service_max of 2 */
	ATF_REQUIRE(run_udp_client("5434"));
	ATF_REQUIRE(run_udp_client("5434"));
	ATF_REQUIRE(!run_udp_client("5434"));

	/* dgram/wait service_max of 0 */
	ATF_REQUIRE(!run_udp_client("5435"));

	/* stream/wait service_max of 2 */
	ATF_REQUIRE(run_tcp_client("5434"));
	ATF_REQUIRE(run_tcp_client("5434"));
	ATF_REQUIRE(!run_tcp_client("5434"));

	/* stream/wait service_max of 0 */
	ATF_REQUIRE(!run_tcp_client("5435"));

	/* stream/nowait ip_max of 3 */
	for (int i = 0; i < 3; i++) {
		ATF_REQUIRE(run_tcp_client("5436"));
	}
	ATF_REQUIRE(!run_tcp_client("5436"));

	/* stream/nowait ip_max of 0 */
	ATF_REQUIRE(!run_tcp_client("5437"));

	/* dgram/wait service_max of 2 */
	ATF_REQUIRE(run_tcp_client("5438"));
	ATF_REQUIRE(run_tcp_client("5438"));
	ATF_REQUIRE(!run_tcp_client("5438"));

	/* dgram/wait service_max of 0 */
	ATF_REQUIRE(!run_tcp_client("5439"));

	/* Exit inetd */
	CHECK_ERROR(kill(proc, SIGTERM));

	waitfor(proc, "inetd");
}

ATF_TP_ADD_TCS(tp)
{
	ATF_TP_ADD_TC(tp, test_ratelimit);

	return atf_no_error();
}

/* Return true if successfully received message, false if timeout */
static bool 
run_udp_client(const char *port)
{
	char buffer[] = "test";
	struct sockaddr_storage addr;

	int udp = create_socket("127.0.0.1", port, SOCK_DGRAM, UDP, 1, &addr);

	CHECK_ERROR(sendto(udp, buffer, sizeof(buffer), 0, 
	    (struct sockaddr *)&addr, addr.ss_len));

	struct iovec iov = {
		.iov_base = buffer,
		.iov_len = sizeof(buffer)
	};

	struct msghdr msg = {
		.msg_name = &addr,
		.msg_namelen = addr.ss_len, /* is this correct? */
		.msg_iov = &iov,
		.msg_iovlen = 1
	};

	ssize_t count = recvmsg(udp, &msg, 0);
	if (count == -1) {
		if (errno == EAGAIN) {
			/* Timed out, return false */
			CHECK_ERROR(close(udp));
			return false;
		} else {
			/* All other errors fatal */
			CHECK_ERROR(-1);
		}
	}
	CHECK_ERROR(close(udp));
	return true;
}

/* Run localhost tcp echo, return true if successful, false if timeout/disconnect */
static bool
run_tcp_client(const char *port)
{
	struct sockaddr_storage remote;
	ssize_t count;
	int tcp;
	char buffer[] = "test";
	
	tcp = create_socket("127.0.0.1", port, SOCK_STREAM, TCP, 1, &remote);
	CHECK_ERROR(connect(tcp, (const struct sockaddr *)&remote,
	    remote.ss_len));
	CHECK_ERROR(send(tcp, buffer, sizeof(buffer), 0));
	count = recv(tcp, buffer, sizeof(buffer), 0);
	if (count == -1) {
		/* 
		 * Connection reset by peer indicates the connection was
		 * dropped. EAGAIN indicates the timeout expired. Any other
		 * error is unexpected for this client program test.
		 */
		if(errno == ECONNRESET || errno == EAGAIN) {
			return false;
		} else {
			CHECK_ERROR(-1);
			return false;
		}
	}

	if (count == 0) {
		/* socket was shutdown by inetd, no more data available */
		return false;
	}
	return true;
}

/* 
 * Create a socket with the characteristics inferred by the args, return parsed 
 * socket address in dst.
 */
static int
create_socket(const char *address, const char *port, 
    int socktype, int proto, time_t timeout_sec, struct sockaddr_storage *dst)
{
        struct addrinfo hints = {
		.ai_flags = AI_NUMERICHOST,
		.ai_socktype = socktype,
		.ai_protocol = proto
	};
	struct addrinfo * res;
	int error, fd;

	ATF_REQUIRE_EQ_MSG(error = getaddrinfo(address, port, &hints, &res), 0, 
	    "%s", gai_strerror(error));

	/* Make sure there is only one possible bind address */
	ATF_REQUIRE_MSG(res->ai_next == NULL, "Ambiguous create_socket args");
	CHECK_ERROR(fd = socket(res->ai_family, 
	    res->ai_socktype, res->ai_protocol));
	struct timeval timeout = { timeout_sec, 0 };
	CHECK_ERROR(setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, 
	    sizeof(timeout)));
	memcpy(dst, res->ai_addr, res->ai_addrlen);
	freeaddrinfo(res);
	return fd;
}

/* Run program with args */
static pid_t
run(const char *prog, char *const *args)
{
	pid_t proc;
	extern char **environ;
	ATF_REQUIRE_EQ(posix_spawnp(&proc, prog, 
	    NULL, NULL, args, environ), 0);
	return proc;
}

/* Wait for a process to exit, check return value */
static void
waitfor(pid_t pid, const char *taskname)
{
	int status;
	int rpid = waitpid(pid, &status, WALLSIG);
	ATF_REQUIRE_MSG(rpid == pid, "wait %d != %d %s",
	    rpid, pid, strerror(errno));

	ATF_REQUIRE_EQ_MSG(WEXITSTATUS(status), EXIT_SUCCESS, 
	    "%s failed with "
	    "exit status %d", taskname, WEXITSTATUS(status));
}

/* Concatenate two const strings, do not free arguments */
static char *
concat(const char *restrict left, const char *restrict right)
{
	char *res;
	if (asprintf(&res, "%s%s", left, right) == -1)
		return NULL;
	return res;
}
