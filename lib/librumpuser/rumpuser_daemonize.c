/*	$NetBSD: rumpuser_daemonize.c,v 1.2.4.2 2013/01/16 05:32:28 yamt Exp $	*/

/*
 * Copyright (c) 2010 Antti Kantee.  All Rights Reserved.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include "rumpuser_port.h"

#if !defined(lint)
__RCSID("$NetBSD: rumpuser_daemonize.c,v 1.2.4.2 2013/01/16 05:32:28 yamt Exp $");
#endif /* !lint */

#include <sys/types.h>
#include <sys/socket.h>

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>

#ifdef __sun__
#define _PATH_DEVNULL "/dev/null"
#else
#include <paths.h>
#endif

static int isdaemonizing;
static int daemonpipe[2];

#include <rump/rumpuser.h>

int
rumpuser_daemonize_begin(void)
{
	ssize_t n;
	int error;

	if (isdaemonizing)
		return EINPROGRESS;
	isdaemonizing = 1;

	/*
	 * For daemons we need to fork.  However, since we can't fork
	 * after rump_init (which creates threads), do it now.  Add
	 * a little pipe trickery to make sure we don't exit until the
	 * service is fully inited (i.e. interlocked daemonization).
	 * Actually, use sucketpair since that allows to easily steer
	 * clear of the dreaded sigpipe.
	 *
	 * Note: We do *NOT* host chdir("/").  It's up to the caller to
	 * take care of that or not.
	 */
	if (socketpair(PF_LOCAL, SOCK_STREAM, 0, daemonpipe) == -1) {
		return errno;
	}

	switch (fork()) {
	case 0:
		if (setsid() == -1) {
			rumpuser_daemonize_done(errno);
		}
		return 0;
	case -1:
		return errno;
	default:
		close(daemonpipe[1]);
		n = recv(daemonpipe[0], &error, sizeof(error), MSG_NOSIGNAL);
		if (n == -1)
			error = errno;
		else if (n != sizeof(error))
			error = ESRCH;
		_exit(error);
		/*NOTREACHED*/
	}
}

int
rumpuser_daemonize_done(int error)
{
	ssize_t n;
	int fd;

	if (!isdaemonizing)
		return ENOENT;

	if (error == 0) {
		fd = open(_PATH_DEVNULL, O_RDWR);
		if (fd == -1) {
			error = errno;
			goto out;
		}
		dup2(fd, STDIN_FILENO);
		dup2(fd, STDOUT_FILENO);
		dup2(fd, STDERR_FILENO);
		if (fd > STDERR_FILENO)
			close(fd);
	}

 out:
	n = send(daemonpipe[1], &error, sizeof(error), MSG_NOSIGNAL);
	if (n != sizeof(error))
		return EPIPE;
	else if (n == -1)
		return errno;
	close(daemonpipe[0]);
	close(daemonpipe[1]);

	return 0;
}
