/*	$NetBSD: unfdpass.c,v 1.6 2000/06/05 06:01:42 thorpej Exp $	*/

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
 * Test passing of file descriptors and credentials over Unix domain sockets.
 */

#include <sys/param.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <sys/un.h>
#include <sys/uio.h>

#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#define	SOCK_NAME	"test-sock"

int	main __P((int, char *[]));
void	child __P((void));
void	catch_sigchld __P((int));
void	usage __P((char *progname));

#define	FILE_SIZE	128
#define	MSG_SIZE	-1
#define	NFILES		24

#define	FDCM_DATASIZE	(sizeof(int) * NFILES)
#define	CRCM_DATASIZE	(SOCKCREDSIZE(NGROUPS))

#define	MESSAGE_SIZE	(CMSG_SPACE(FDCM_DATASIZE) +			\
			 CMSG_SPACE(CRCM_DATASIZE))

int chroot_rcvr = 0;
int pass_dir = 0;
int pass_root_dir = 0;
int exit_early = 0;
int exit_later = 0;
int pass_sock = 0;
int make_pretzel = 0;

/* ARGSUSED */
int
main(argc, argv)
	int argc;
	char *argv[];
{
#if MSG_SIZE >= 0
	struct iovec iov;
#endif
	char *progname=argv[0];
	struct msghdr msg;
	int listensock, sock, fd, i;
	char fname[16], buf[FILE_SIZE];
	struct cmsghdr *cmp;
	void *message;
	int *files = NULL;
	struct sockcred *sc = NULL;
	struct sockaddr_un sun, csun;
	int csunlen;
	pid_t pid;
	int ch;

	message = malloc(CMSG_SPACE(MESSAGE_SIZE));
	if (message == NULL)
		err(1, "unable to malloc message buffer");
	memset(message, 0, CMSG_SPACE(MESSAGE_SIZE));

	while ((ch = getopt(argc, argv, "DESdepr")) != -1) {
		switch(ch) {

		case 'e':
			exit_early++; /* test early GC */
			break;

		case 'E':
			exit_later++; /* test later GC */
			break;
			
		case 'd':
			pass_dir++;
			break;
			
		case 'D':
			pass_dir++;
			pass_root_dir++;
			break;

		case 'S':
			pass_sock++;
			break;

		case 'r':
			chroot_rcvr++;
			break;

		case 'p':
			make_pretzel++;
			break;
			
		case '?':
		default:
			usage(progname);
		}
	}


	/*
	 * Create the test files.
	 */
	for (i = 0; i < NFILES; i++) {
		(void) sprintf(fname, "file%d", i + 1);
		if ((fd = open(fname, O_WRONLY|O_CREAT|O_TRUNC, 0666)) == -1)
			err(1, "open %s", fname);
		(void) sprintf(buf, "This is file %d.\n", i + 1);
		if (write(fd, buf, strlen(buf)) != strlen(buf))
			err(1, "write %s", fname);
		(void) close(fd);
	}

	/*
	 * Create the listen socket.
	 */
	if ((listensock = socket(PF_LOCAL, SOCK_STREAM, 0)) == -1)
		err(1, "socket");

	(void) unlink(SOCK_NAME);
	(void) memset(&sun, 0, sizeof(sun));
	sun.sun_family = AF_LOCAL;
	(void) strcpy(sun.sun_path, SOCK_NAME);
	sun.sun_len = SUN_LEN(&sun);

	i = 1;
	if (setsockopt(listensock, 0, LOCAL_CREDS, &i, sizeof(i)) == -1)
		err(1, "setsockopt");

	if (bind(listensock, (struct sockaddr *)&sun, sizeof(sun)) == -1)
		err(1, "bind");

	if (listen(listensock, 1) == -1)
		err(1, "listen");

	/*
	 * Create the sender.
	 */
	(void) signal(SIGCHLD, catch_sigchld);
	pid = fork();
	switch (pid) {
	case -1:
		err(1, "fork");
		/* NOTREACHED */

	case 0:
		child();
		/* NOTREACHED */
	}

	if (exit_early)
		exit(0);
	
	if (chroot_rcvr &&
	    ((chroot(".") < 0)))
		err(1, "chroot");
	
	/*
	 * Wait for the sender to connect.
	 */
	if ((sock = accept(listensock, (struct sockaddr *)&csun,
	    &csunlen)) == -1)
		err(1, "accept");

	/*
	 * Give sender a chance to run.  We will get going again
	 * once the SIGCHLD arrives.
	 */
	(void) sleep(10);

	if (exit_later)
		exit(0);
	
	/*
	 * Grab the descriptors and credentials passed to us.
	 */

	/* Expect 2 messages; descriptors and creds. */
	do {
		(void) memset(&msg, 0, sizeof(msg));
		msg.msg_control = message;
		msg.msg_controllen = MESSAGE_SIZE;
#if MSG_SIZE >= 0
		iov.iov_base = buf;
		iov.iov_len = MSG_SIZE;
		msg.msg_iov = &iov;
		msg.msg_iovlen = 1;
#endif

		if (recvmsg(sock, &msg, 0) == -1)
			err(1, "recvmsg");

		(void) close(sock);
		sock = -1;

		if (msg.msg_controllen == 0)
			errx(1, "no control messages received");

		if (msg.msg_flags & MSG_CTRUNC)
			errx(1, "lost control message data");

		for (cmp = CMSG_FIRSTHDR(&msg); cmp != NULL;
		     cmp = CMSG_NXTHDR(&msg, cmp)) {
			if (cmp->cmsg_level != SOL_SOCKET)
				errx(1, "bad control message level %d",
				    cmp->cmsg_level);

			switch (cmp->cmsg_type) {
			case SCM_RIGHTS:
				if (cmp->cmsg_len != CMSG_LEN(FDCM_DATASIZE))
					errx(1, "bad fd control message "
					    "length %d", cmp->cmsg_len);

				files = (int *)CMSG_DATA(cmp);
				break;

			case SCM_CREDS:
				if (cmp->cmsg_len < CMSG_LEN(SOCKCREDSIZE(1)))
					errx(1, "bad cred control message "
					    "length %d", cmp->cmsg_len);

				sc = (struct sockcred *)CMSG_DATA(cmp);
				break;

			default:
				errx(1, "unexpected control message");
				/* NOTREACHED */
			}
		}

		/*
		 * Read the files and print their contents.
		 */
		if (files == NULL)
			warnx("didn't get fd control message");
		else {
			for (i = 0; i < NFILES; i++) {
				struct stat st;
				(void) memset(buf, 0, sizeof(buf));
				fstat(files[i], &st);
				if (S_ISDIR(st.st_mode)) {
					printf("file %d is a directory\n", i+1);
				} else if (S_ISSOCK(st.st_mode)) {
					printf("file %d is a socket\n", i+1);
					sock = files[i];
				} else {
					int c;
					c = read (files[i], buf, sizeof(buf));
					if (c < 0)
						err(1, "read file %d", i + 1);
					else if (c == 0)
						printf("[eof on %d]\n", i + 1);
					else
						printf("%s", buf);
				}
			}
		}
		/*
		 * Double-check credentials.
		 */
		if (sc == NULL)
			warnx("didn't get cred control message");
		else {
			if (sc->sc_uid == getuid() &&
			    sc->sc_euid == geteuid() &&
			    sc->sc_gid == getgid() &&
			    sc->sc_egid == getegid())
				printf("Credentials match.\n");
			else
				printf("Credentials do NOT match.\n");
		}
	} while (sock != -1);

	/*
	 * All done!
	 */
	exit(0);
}

void
usage(progname)
	char *progname;
{
	fprintf(stderr, "usage: %s [-derDES]\n", progname);
	exit(1);
}

void
catch_sigchld(sig)
	int sig;
{
	int status;

	(void) wait(&status);
}

void
child()
{
#if MSG_SIZE >= 0
	struct iovec iov;
#endif
	struct msghdr msg;
	char fname[16];
	struct cmsghdr *cmp;
	void *fdcm;
	int i, fd, sock, nfd, *files;
	struct sockaddr_un sun;
	int spair[2];

	fdcm = malloc(CMSG_SPACE(FDCM_DATASIZE));
	if (fdcm == NULL)
		err(1, "unable to malloc fd control message");
	memset(fdcm, 0, CMSG_SPACE(FDCM_DATASIZE));

	cmp = fdcm;
	files = (int *)CMSG_DATA(fdcm);

	/*
	 * Create socket and connect to the receiver.
	 */
	if ((sock = socket(PF_LOCAL, SOCK_STREAM, 0)) == -1)
		errx(1, "child socket");

	(void) memset(&sun, 0, sizeof(sun));
	sun.sun_family = AF_LOCAL;
	(void) strcpy(sun.sun_path, SOCK_NAME);
	sun.sun_len = SUN_LEN(&sun);

	if (connect(sock, (struct sockaddr *)&sun, sizeof(sun)) == -1)
		err(1, "child connect");

	nfd = NFILES;
	i = 0;

	if (pass_sock) {
		files[i++] = sock;
	}

	if (pass_dir)
		nfd--;

	/*
	 * Open the files again, and pass them to the child
	 * over the socket.
	 */

	for (; i < nfd; i++) {
		(void) sprintf(fname, "file%d", i + 1);
		if ((fd = open(fname, O_RDONLY, 0666)) == -1)
			err(1, "child open %s", fname);
		files[i] = fd;
	}
	
	if (pass_dir) {
		char *dirname = pass_root_dir ? "/" : ".";

		
		if ((fd = open(dirname, O_RDONLY, 0)) == -1) {
			err(1, "child open directory %s", dirname);
		}
		files[i] = fd;
	}
	
	(void) memset(&msg, 0, sizeof(msg));
	msg.msg_control = fdcm;
	msg.msg_controllen = CMSG_LEN(FDCM_DATASIZE);
#if MSG_SIZE >= 0
	iov.iov_base = buf;
	iov.iov_len = MSG_SIZE;
	msg.msg_iov = &iov;
	msg.msg_iovlen = 1;
#endif

	cmp = CMSG_FIRSTHDR(&msg);
	cmp->cmsg_len = CMSG_LEN(FDCM_DATASIZE);
	cmp->cmsg_level = SOL_SOCKET;
	cmp->cmsg_type = SCM_RIGHTS;

	while (make_pretzel > 0) {
		if (socketpair(PF_LOCAL, SOCK_STREAM, 0, spair) < 0)
			err(1, "socketpair");

		printf("send pretzel\n");
		if (sendmsg(spair[0], &msg, 0) < 0)
			err(1, "child prezel sendmsg");

		close(files[0]);
		close(files[1]);		
		files[0] = spair[0];
		files[1] = spair[1];		
		make_pretzel--;
	}

	if (sendmsg(sock, &msg, 0) == -1)
		err(1, "child sendmsg");

	/*
	 * All done!
	 */
	exit(0);
}
