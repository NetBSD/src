/*	$NetBSD: unfdpass.c,v 1.1 1998/01/07 03:53:02 thorpej Exp $	*/

/*-
 * Copyright (c) 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe of the Numerical Aerospace Simulation Facility,
 * NASA Ames Research Center.
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

/*
 * Test passing of file descriptors over Unix domain sockets.
 */

#include <sys/param.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <sys/un.h>
#include <err.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

int	sock[2];

int	main __P((int, char *[]));
void	child __P((void));

struct cmessage {
	struct cmsghdr cm;
	int files[2];
};

/* ARGSUSED */
int
main(argc, argv)
	int argc;
	char *argv[];
{
	struct msghdr msg;
	int fd, i, status;
	char fname[16], buf[64];
	pid_t pid;
	struct cmsghdr *cmp;
	struct cmessage cm;

	/*
	 * Create the test files.
	 */
	for (i = 0; i < 2; i++) {
		(void) sprintf(fname, "file%d", i + 1);
		if ((fd = open(fname, O_WRONLY|O_CREAT|O_TRUNC, 0666)) == -1)
			err(1, "open %s", fname);
		(void) sprintf(buf, "This is file %d.\n", i + 1);
		if (write(fd, buf, strlen(buf)) != strlen(buf))
			err(1, "write %s", fname);
		(void) close(fd);
	}

	/*
	 * Create the socket pair to pass the descriptors over.
	 */
	if (socketpair(AF_LOCAL, SOCK_STREAM, 0, sock) == -1)
		err(1, "socketpair");

	pid = fork();
	switch (pid) {
	case -1:
		err(1, "fork");
		/* NOTREACHED */

	case 0:
		child();
		/* NOTREACHED */
	}

	(void) close(sock[1]);

	/*
	 * Open the files again, and pass them to the child over the socket.
	 */
	for (i = 0; i < 2; i++) {
		(void) sprintf(fname, "file%d", i + 1);
		if ((fd = open(fname, O_RDONLY, 0666)) == -1)
			err(1, "open %s", fname);
		cm.files[i] = fd;
	}

	(void) memset(&msg, 0, sizeof(msg));
	msg.msg_control = (caddr_t) &cm;
	msg.msg_controllen = sizeof(cm);

	cmp = CMSG_FIRSTHDR(&msg);
	cmp->cmsg_len = sizeof(cm);
	cmp->cmsg_level = SOL_SOCKET;
	cmp->cmsg_type = SCM_RIGHTS;

	if (sendmsg(sock[0], &msg, 0)) {
		warn("sendmsg");
		(void) kill(pid, SIGINT);
	}

	/* Wait for the child to exit. */
	if (waitpid(pid, &status, 0) == -1)
		err(1, "waitpid");

	if (WIFEXITED(status) && WEXITSTATUS(status) != 0)
		errx(1, "child exited with status of %d\n",
		    WEXITSTATUS(status));

	exit(0);
}

void
child()
{
	struct msghdr msg;
	char buf[64];
	struct cmsghdr *cmp;
	struct cmessage cm;
	int i;

	/*
	 * Close the socket we're not using.
	 */
	(void) close(sock[0]);

	/*
	 * Grab the descriptors passed to us.
	 */
	(void) memset(&msg, 0, sizeof(msg));
	msg.msg_control = (caddr_t) &cm;
	msg.msg_controllen = sizeof(cm);

	if (recvmsg(sock[1], &msg, 0) < 0)
		err(1, "recvmsg");

	cmp = CMSG_FIRSTHDR(&msg);

	if (cmp->cmsg_len != sizeof(cm))
		err(1, "bad control message length");
	if (cmp->cmsg_level != SOL_SOCKET)
		err(1, "bad control message level");
	if (cmp->cmsg_type != SCM_RIGHTS)
		err(1, "bad control message type");

	/*
	 * Read the files and print their contents.
	 */
	for (i = 0; i < 2; i++) {
		(void) memset(buf, 0, sizeof(buf));
		if (read(cm.files[i], buf, sizeof(buf)) <= 0)
			err(1, "read file %d", i + 1);
		printf("%s", buf);
	}

	/*
	 * All done!
	 */
	exit(0);
}
