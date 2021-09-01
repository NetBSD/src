/*	$NetBSD: test_server.c,v 1.2 2021/09/01 06:12:50 christos Exp $	*/

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
__RCSID("$NetBSD: test_server.c,v 1.2 2021/09/01 06:12:50 christos Exp $");

#include <sys/socket.h>
#include <unistd.h>
#include <netdb.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <syslog.h>

#define CHECK(expr) do {\
	if ((expr) == -1) {\
		syslog(LOG_ERR, "Error at %s:%d: %s", \
		    __FILE__, __LINE__, \
		    strerror(errno));\
		exit(EXIT_FAILURE);\
	}\
} while(0);

static void stream_nowait_service(void);
static void stream_wait_service(void);
static void dgram_wait_service(void);

int
main(int argc, char **argv)
{

	openlog("inetd_test_server", LOG_PID | LOG_NOWAIT, LOG_DAEMON);

	if (argc < 3) {
		syslog(LOG_ERR, "Invalid arg count");
		exit(EXIT_FAILURE);
	}

	/* Run the correct service according to the args */
	if (strcmp(argv[1], "dgram") == 0) {
		if (strcmp(argv[2], "wait") == 0) {
			dgram_wait_service();
		} else {
			syslog(LOG_ERR, "Invalid arg %s", argv[2]);
			exit(EXIT_FAILURE);
		}
	} else if (strcmp(argv[1], "stream") == 0) {
		if (strcmp(argv[2], "wait") == 0) {
			stream_wait_service();
		} else if (strcmp(argv[2], "nowait") == 0) { 
			stream_nowait_service();
		} else {
			syslog(LOG_ERR, "Invalid arg %s", argv[2]);
			exit(EXIT_FAILURE);
		}
	} else {
		syslog(LOG_ERR, "Invalid args %s %s", argv[1], argv[2]);
		exit(EXIT_FAILURE);
	}
	return 0;
}

static void
stream_nowait_service(void)
{
	ssize_t count;
	char buffer[10];
	CHECK(count = recv(0, buffer, sizeof(buffer), 0));
	syslog(LOG_WARNING, "Received stream/nowait message \"%.*s\"\n",
	    (int)count, buffer);
	CHECK(send(1, buffer, (size_t)count, 0));
}

static void
stream_wait_service(void)
{
	struct sockaddr_storage addr;
	ssize_t count;
	int fd;
	socklen_t addr_len;
	char buffer[10];

	CHECK(fd = accept(0, (struct sockaddr*)&addr, &addr_len));
	CHECK(count = recv(fd, buffer, sizeof(buffer), 0));
	syslog(LOG_WARNING, "Received stream/wait message \"%.*s\"\n",
	    (int)count, buffer);
	CHECK(send(fd, buffer, (size_t)count, 0));
	CHECK(shutdown(fd, SHUT_RDWR));
	CHECK(close(fd));
}

static void
dgram_wait_service(void)
{
	char buffer[256];
	char name[NI_MAXHOST];
	struct sockaddr_storage addr;

	struct iovec store = {
		.iov_base = &buffer,
		.iov_len = sizeof(buffer)
	};
	struct msghdr header = {
		.msg_name = &addr,
		.msg_namelen = sizeof(struct sockaddr_storage),
		.msg_iov = &store,
		.msg_iovlen = 1
		/* scatter/gather and control info is null */
	};
	ssize_t count;

	/* Peek so service can still get the packet */
	CHECK(count = recvmsg(0, &header, 0));

	CHECK(sendto(1, buffer, (size_t)count, 0, 
	    (struct sockaddr*)(&addr), addr.ss_len));
	
	int error = getnameinfo((struct sockaddr*)&addr, 
	    addr.ss_len, name, NI_MAXHOST,
	    NULL, 0, NI_NUMERICHOST);
	
	if (error) {
		syslog(LOG_ERR, "getnameinfo error: %s\n", gai_strerror(error));
		exit(EXIT_FAILURE);
	}
	syslog(LOG_WARNING, "Received dgram/wait message \"%.*s\" from %s\n", 
	    (int)count, buffer, name);
}
