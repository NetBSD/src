/*	$NetBSD: cmd_pollpal.c,v 1.1 2020/04/30 00:48:10 christos Exp $	*/

/*-
 * Copyright (c) 2020 The NetBSD Foundation, Inc.
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
__RCSID("$NetBSD: cmd_pollpal.c,v 1.1 2020/04/30 00:48:10 christos Exp $");

#include <sys/mman.h>

#include <errno.h>
#include <err.h>
#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>

#define BUFFLEN 100
#define TIMEOUT 10000

static int fd; 

static __dead __printflike(2, 3) void 
done(int e, const char *msg, ...)
{
	if (msg) {
		va_list ap;

		va_start(ap, msg);
		verr(e, msg, ap);
		/*NOTREACHED*/
		va_end(ap);
	}
	exit(e);
}
	
static __dead void 
handler(int sig)
{
	done(EXIT_SUCCESS, NULL);
}

static const char dev[] = "/dev/pollpal";
static const char wbuf[] = "abcdefghijklmnopqrstuvwxyz";

int 
main(int argc, char **argv)
{
	struct pollfd fds;
	ssize_t ret;
	size_t i;
	char rbuf[BUFFLEN];

	setprogname(argv[0]);

	fd = open(dev, O_RDWR, 0);
	if (fd == -1)
		err(EXIT_FAILURE, "open `%s'", dev);

	fds.fd = fd;
	fds.events = POLLOUT|POLLIN;

	signal(SIGINT, handler);
	for (i = 1; i <= 100; i++) {
		ret = poll(&fds, 1, TIMEOUT);
		if (ret == -1) {
			if (errno == EINTR)
				continue;
			done(EXIT_FAILURE, "poll");
		}
		printf("Poll_executing %zu times\n", i);
		if (ret == 0) {
			printf("%d seconds elapsed \n.TIMEOUT.\n",
			    TIMEOUT / 1000);
			done(EXIT_SUCCESS, NULL);
		}
		
		if (fds.revents & (POLLERR | POLLHUP | POLLNVAL)) 
			done(EXIT_FAILURE, "ERR|HUP|NVAL");
		
		if (fds.revents & POLLOUT) {
			do {
				ret = write(fd, wbuf, sizeof(wbuf) - 1);
			} while (ret == -1 && errno == EINTR);
			
			if (ret == -1) {
				done(EXIT_FAILURE, "write");
			}
			printf("Wrote %zd byte\n", ret);
		}
		if (fds.revents & POLLIN) {
			memset(rbuf, 0, BUFFLEN);
			do {
				ret = read(fd, rbuf, BUFFLEN);
			} while (ret == -1 && errno == EINTR);
			if (ret == -1) {
				done(EXIT_FAILURE, "read");
			}
			printf("Read %zd bytes\n", ret);
		}
	}
	done(EXIT_SUCCESS, NULL);
}
