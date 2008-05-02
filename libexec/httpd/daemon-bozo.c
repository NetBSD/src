/*	$NetBSD: daemon-bozo.c,v 1.4 2008/05/02 19:14:03 degroote Exp $	*/

/*	$eterna: daemon-bozo.c,v 1.9 2008/03/03 03:36:11 mrg Exp $	*/

/*
 * Copyright (c) 1997-2008 Matthew R. Green
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer and
 *    dedication in the documentation and/or other materials provided
 *    with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

/* this code implements daemon mode for bozohttpd */

#ifndef NO_DAEMON_MODE

#include <sys/param.h>
#include <sys/socket.h>
#include <sys/wait.h>

#include <netinet/in.h>

#include <errno.h>
#include <netdb.h>
#include <poll.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "bozohttpd.h"

	char	*iflag;		/* bind address; default INADDR_ANY */
static	int	*sock;		/* bound sockets */
static	int	nsock;		/* number of above */

static	void	sigchild(int);	/* SIGCHLD handler */

/* ARGSUSED */
static void
sigchild(signo)
	int signo;
{
	while (waitpid(-1, NULL, WNOHANG) > 0)
		;
}

void
daemon_init()
{
	struct addrinfo h, *r, *r0;
	int e, i, on = 1;

	if (!bflag) 
		return;

	if (fflag == 0)
		daemon(1, 0);

	warning("started in daemon mode as `%s' port `%s' root `%s'",
	    myname, Iflag, slashdir);
	
	memset(&h, 0, sizeof h);
	h.ai_family = PF_UNSPEC;
	h.ai_socktype = SOCK_STREAM;
	h.ai_flags = AI_PASSIVE;
	e = getaddrinfo(iflag, Iflag, &h, &r0);
	if (e)
		error(1, "getaddrinfo([%s]:%s): %s",
		    iflag ? iflag : "*", Iflag, gai_strerror(e));
	for (r = r0; r != NULL; r = r->ai_next)
		nsock++;
	sock = bozomalloc(nsock * sizeof *sock);
	for (i = 0, r = r0; r != NULL; r = r->ai_next) {
		sock[i] = socket(r->ai_family, SOCK_STREAM, 0);
		if (sock[i] == -1)
			continue;
		if (setsockopt(sock[i], SOL_SOCKET, SO_REUSEADDR, &on,
		    sizeof(on)) == -1)
			warning("setsockopt SO_REUSEADDR: %s",
			    strerror(errno));
		if (bind(sock[i], r->ai_addr, r->ai_addrlen) == -1)
			continue;
		if (listen(sock[i], SOMAXCONN) == -1)
			continue;
		i++;
	}
	if (i == 0)
		error(1, "could not find any addresses to bind");
	nsock = i;
	freeaddrinfo(r0);

	signal(SIGCHLD, sigchild);
}

/*
 * the parent never returns from this function, only children that
 * are ready to run...
 */
void
daemon_fork()
{
	struct pollfd *fds = NULL;

	while (bflag) {
		struct	sockaddr_storage ss;
		socklen_t slen;
		int fd;
		int i;

#ifndef POLLRDNORM
#define POLLRDNORM 0
#endif
#ifndef POLLRDBAND
#define POLLRDBAND 0
#endif
#ifndef INFTIM
#define INFTIM -1
#endif
		if (fds == NULL) {
			fds = bozomalloc(nsock * sizeof *fds);
			for (i = 0; i < nsock; i++) {
				fds[i].events = POLLIN | POLLPRI | POLLRDNORM |
						POLLRDBAND | POLLERR;
				fds[i].fd = sock[i];
			}
		}

		/*
		 * wait for a connection, then fork() and return NULL in
		 * the parent, who will come back here waiting for another
		 * connection.  read the request in in the child, and return
		 * it, for processing.
		 */
again:
		if (poll(fds, nsock, INFTIM) == -1) {
			if (errno != EINTR)
				error(1, "poll: %s", strerror(errno));
			goto again;
		}

		for (i = 0; i < nsock; i++) {
			if (fds[i].revents & (POLLNVAL|POLLERR|POLLHUP)) {
				warning("poll on fd %d: %s", fds[i].fd,
				    strerror(errno));
				continue;
			}
			if (fds[i].revents == 0)
				continue;

			slen = sizeof(ss);
			fd = accept(sock[i], (struct sockaddr *)&ss, &slen);
			if (fd == -1) {
				if (errno != EAGAIN)
					error(1, "accept: %s", strerror(errno));
				continue;
			}
			switch (fork()) {
			case -1: /* eep, failure */
				warning("fork() failed, sleeping for 10 seconds: %s",
				    strerror(errno));
				close(fd);
				sleep(10);
				continue;

			case 0: /* child */
				/* setup stdin/stdout/stderr */
				dup2(fd, 0);
				dup2(fd, 1);
				/*dup2(fd, 2);*/
				close(fd);
				return;

			default: /* parent */
				close(fd);
				continue;
			}
		}
	}
}

#endif /* NO_DAEMON_MODE */
