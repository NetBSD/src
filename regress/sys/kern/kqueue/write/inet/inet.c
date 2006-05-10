/*	$NetBSD: inet.c,v 1.6 2006/05/10 19:10:09 mrg Exp $	*/

/*-
 * Copyright (c) 2002 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Luke Mewburn.
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
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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

#include <sys/param.h>
#include <sys/event.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/wait.h>

#include <netinet/in.h>

#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <inttypes.h>

void	dumpkevent(struct kevent *);

void
dumpkevent(struct kevent *event)
{
	printf(
"kevent: ident 0x%02lx filter %d flags: 0x%02x, fflags: 0x%02x, data %" PRId64 "\n",
	    (long) event->ident, event->filter, event->flags, event->fflags,
	    event->data);
}

static int port;

static void
inetwrite(void)
{
	struct timespec		timeout;
	struct timeval		then, now, diff;
	struct kevent		event[5];
	struct sockaddr_in	sa;
	int			kq, fd, n, i, match, count;
	char			buffer[128];
	int	nevents = 0;

        kq = kqueue();
        if (kq == -1)
                err(1, "iw: kqueue");
	
	fd = socket(PF_INET, SOCK_STREAM, 0);
	if (fd == -1)
		err(1, "iw: socket");
	memset((void *)&sa, 0, sizeof(sa));

	sa.sin_family = AF_INET;
	sa.sin_port = port;
	sa.sin_addr.s_addr = htonl(0x7f000001);
	if (connect(fd, (struct sockaddr *)&sa, sizeof(sa)) == -1)
		err(1, "iw: connect");

	n = sizeof(sa);

	EV_SET(&event[0], fd, EVFILT_WRITE, EV_ADD | EV_ENABLE, 0, 0, 0);
	n = kevent(kq, event, 1, NULL, 0, NULL);
	if (n == -1)
		err(1, "iw: kevent(1)");
printf("iw: registered fd %d\n", fd);
	
sleep(5);
		/*
		 * start writing
		 */
	timeout.tv_sec = 3;
	timeout.tv_nsec = 0;
	count = 0;
	for (; nevents < 10;) {
		if (gettimeofday(&then, NULL) == -1)
			err(1, "iw: gettimeofday then");
		n = kevent(kq, NULL, 0, event, 5, &timeout);
		if (gettimeofday(&now, NULL) == -1)
			err(1, "gettimeofday now");
		timersub(&now, &then, &diff);

		printf("iw: kevent returned %d in %ld.%06ld\n", n,
		    diff.tv_sec, diff.tv_usec);
		if (n == -1)
			err(1, "kevent");
		else if (n == 0)
			continue;

		nevents += n;

		match = -1;
		for (i = 0; i < n; i++) {
			printf("iw: event[%d]: ", i);
			dumpkevent(&event[i]);
			if (event[i].ident == fd)
				match = i;
		}
printf("iw: match is %d\n", match); usleep(10000);
		if (match == -1)
			continue;

		i = snprintf(buffer, sizeof(buffer),
		    "iw: |----> count %d <------------------------------------------------------------------------------------------------------------------------------------------|\n", count++);
		n = write(fd, buffer, i);
		if (event[match].flags & EV_ERROR) {
			printf("iw: error received: %d %s\n",
			    (int)event[match].data,
			    strerror((int)event[match].data));
			break;
		} else if (event[match].flags & EV_EOF) {
			printf("iw: EOF received\n");
			break;
		}
	}
	printf("iw: exiting with %s\n",
		(n >= 10) ? "success" : "failure");
	_exit((n >= 10) ? EXIT_SUCCESS : EXIT_FAILURE);
}

int
main(int argc, char **argv)
{
	struct timespec		timeout;
	struct timeval		then, now, diff;
	struct kevent		event[5];
	struct sockaddr_in	sa;
	socklen_t		salen;
	int			sock, fd, kq, i, n, match;
	char			buffer[128];
	pid_t child;
	int status;

	if (argc != 1)
		errx(1, "Usage: %s", argv[0]);

        kq = kqueue();
        if (kq == -1)
                err(1, "ir: kqueue");
	
	sock = socket(PF_INET, SOCK_STREAM, 0);
	if (sock == -1)
		err(1, "ir: socket");
	memset((void *)&sa, 0, sizeof(sa));
	sa.sin_family = AF_INET;
	if (bind(sock, (struct sockaddr *)&sa, sizeof(sa)) == -1)
		err(1, "ir: bind");

		/*
		 * start listening
		 */

	if (listen(sock, 5) == -1)
		err(1, "ir: listen");
	salen = sizeof(sa);
	if (getsockname(sock, (struct sockaddr *)&sa, &salen) == -1)
		err(1, "ir: getsockname");
	port = sa.sin_port;
	printf("ir: pid %d bound to port %d\n", getpid(), ntohs(sa.sin_port));

	if ((child = fork()) == 0) {
		inetwrite();
		_exit(EXIT_FAILURE);
	}

printf("ir: waiting 5 seconds before accept()\n"); sleep(5);
	
	timeout.tv_sec = 5;
	timeout.tv_nsec = 0;

		/* set a kevent on the listen socket */
	EV_SET(&event[0], sock, EVFILT_READ, EV_ADD | EV_ENABLE | EV_ONESHOT,
	    0, 0, 0);
	n = kevent(kq, event, 1, NULL, 0, NULL);
	if (n == -1)
		err(1, "ir: kevent(1)");
printf("ir: registered sock %d\n", sock);

	for (;;) {
		if (gettimeofday(&then, NULL) == -1)
			err(1, "ir: gettimeofday then");
		n = kevent(kq, NULL, 0, event, 1, &timeout);
		if (gettimeofday(&now, NULL) == -1)
			err(1, "ir: gettimeofday now");
		timersub(&now, &then, &diff);

		printf("ir: kevent returned %d in %ld.%06ld\n", n,
		    diff.tv_sec, diff.tv_usec);
		if (n == -1)
			err(1, "ir: kevent");
		else if (n == 0)
			continue;
		dumpkevent(event);
		break;
	}


		/*
		 * accept connection, start reading
		 */
	salen = sizeof(sa);
	fd = accept(sock, (struct sockaddr *)&sa, &salen);
	if (fd == -1)
		err(1, "ir: accept");
	printf("ir: got connection from 0x%x port %d\n",
	    ntohl(sa.sin_addr.s_addr), ntohs(sa.sin_port));

	EV_SET(&event[0], fd, EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, 0);
	n = kevent(kq, event, 1, NULL, 0, NULL);
	if (n == -1)
		err(1, "ir: kevent(1)");
printf("ir: registered fd %d\n", fd);
	
	for (;;) {
		if (gettimeofday(&then, NULL) == -1)
			err(1, "ir: gettimeofday then");
		n = kevent(kq, NULL, 0, event, 5, &timeout);
		if (gettimeofday(&now, NULL) == -1)
			err(1, "ir: gettimeofday now");
		timersub(&now, &then, &diff);

		printf("ir: kevent returned %d in %ld.%06ld\n", n,
		    diff.tv_sec, diff.tv_usec);
		if (n == -1)
			err(1, "ir: kevent");
		else if (n == 0)
			continue;

		match = -1;
		for (i = 0; i < n; i++) {
			printf("ir: event[%d]: ", i);
			dumpkevent(&event[i]);
			if (event[i].ident == fd)
				match = i;
		}
printf("ir: match is %d\n", match); /* sleep(1); */
		if (match == -1)
			continue;

		n = read(fd, buffer, 128);
		buffer[n] = '\0';
buffer[0] = '\0';
		printf("[%d] %s", n, buffer);
		if (event[match].flags & EV_ERROR) {
			printf("ir: error received: %" PRId64 " %s\n",
			    event[match].data,
			    strerror((int)event[match].data));
			break;
		} else if (event[match].flags & EV_EOF) {
			printf("ir: EOF received\n");
			break;
		}
	}
	if (waitpid(child, &status, 0) != child)
		err(1, "ir: waipid returned bad pid");
	return (WEXITSTATUS(status));
}
