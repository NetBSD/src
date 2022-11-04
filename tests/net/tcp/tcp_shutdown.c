/*	$NetBSD: tcp_shutdown.c,v 1.1 2022/11/04 08:01:42 ozaki-r Exp $	*/

/*-
 * Copyright (c) 2022 Internet Initiative Japan Inc.
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
#ifdef __RCSID
__RCSID("$NetBSD: tcp_shutdown.c,v 1.1 2022/11/04 08:01:42 ozaki-r Exp $");
#else
extern const char *__progname;
#define getprogname() __progname
#endif

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <err.h>
#include <errno.h>
#include <stdbool.h>

static inline bool
match(const char *a, const char *b)
{

	return strncmp(a, b, strlen(b)) == 0;
}

int
main(int argc, char *argv[])
{
	int s, e;
	char *target;

	if (argc != 2)
		errx(EXIT_FAILURE, "invalid argument");
	target = argv[1];

	s = socket(AF_INET, SOCK_STREAM, 0);
	if (s == -1)
		err(EXIT_FAILURE, "socket");
	e = shutdown(s, SHUT_RDWR);
	if (e == -1)
		err(EXIT_FAILURE, "shutdown");

	if (match(target, "connect")) {
		struct sockaddr_in sin;

		memset(&sin, 0, sizeof(sin));
		sin.sin_port = htons(31522);
		sin.sin_addr.s_addr = inet_addr("127.0.0.1");
		sin.sin_family = AF_INET;

		e = connect(s, (struct sockaddr *)&sin, sizeof(sin));
		if (e == 0)
			err(EXIT_FAILURE, "connect didn't fail on a shudown socket");
		if (e == -1 && errno != EINVAL)
			err(EXIT_FAILURE, "connect failed with unexpected error");
	} else if (match(target, "setsockopt")) {
		int opt = 1;
		e = setsockopt(s, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt));
		if (e == 0)
			err(EXIT_FAILURE, "setsockopt didn't fail on a shutdown socket");
		if (e == -1 && errno != ECONNRESET)
			err(EXIT_FAILURE, "setsockopt failed with unexpected error");
	} else if (match(target, "getsockname")) {
		struct sockaddr_storage ss;
		socklen_t len;
		e = getsockname(s, (struct sockaddr *)&ss, &len);
		if (e == 0)
			err(EXIT_FAILURE, "getsockname didn't fail on a shutdown socket");
		if (e == -1 && errno != EINVAL)
			err(EXIT_FAILURE, "getsockname failed with unexpected error");
	} else if (match(target, "listen")) {
		e = listen(s, 5);
		if (e == 0)
			err(EXIT_FAILURE, "listen didn't fail on a shutdown socket");
		if (e == -1 && errno != EINVAL)
			err(EXIT_FAILURE, "listen failed with unexpected error");
	} else if (match(target, "bind")) {
		struct sockaddr_in sin;

		memset(&sin, 0, sizeof(sin));
		sin.sin_port = htons(31522);
		sin.sin_addr.s_addr = inet_addr("127.0.0.1");
		sin.sin_family = AF_INET;

		e = bind(s, (struct sockaddr *)&sin, sizeof(sin));
		if (e == 0)
			err(EXIT_FAILURE, "bind didn't fail on a shutdown socket");
		if (e == -1 && errno != EINVAL)
			err(EXIT_FAILURE, "bind failed with unexpected error");
	} else if (match(target, "shutdown")) {
		e = shutdown(s, SHUT_RDWR);
		if (e == 0)
			err(EXIT_FAILURE, "shutdown didn't fail on a shutdown socket");
		if (e == -1 && errno != EINVAL)
			err(EXIT_FAILURE, "shutdown failed with unexpected error");
	} else {
		errx(EXIT_FAILURE, "unknown target: %s", target);
	}

	e = close(s);
	if (e == -1)
		err(EXIT_FAILURE, "close");
	return 0;
}
