/*	$Id: loan1.c,v 1.3 2004/02/15 12:50:38 yamt Exp $	*/

/*-
 * Copyright (c)2004 YAMAMOTO Takashi,
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/mman.h>
#include <sys/socket.h>

#include <err.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define	BUFSIZE	(32 * 1024)	/* enough size to trigger sosend_loan */

void testloan(void *, char, int);
int main(int, char *[]);

void
testloan(void *vp, char pat, int docheck)
{
	char buf[BUFSIZE];
	char backup[BUFSIZE];

	int fds[2];
	int val;
	ssize_t nwritten;
	ssize_t nread;

	if (docheck)
		memcpy(backup, vp, BUFSIZE);

	if (socketpair(AF_LOCAL, SOCK_STREAM, PF_UNSPEC, fds))
		err(EXIT_FAILURE, "socketpair");

	val = BUFSIZE;
	if (setsockopt(fds[1], SOL_SOCKET, SO_RCVBUF, &val, sizeof(val)))
		err(EXIT_FAILURE, "SO_RCVBUF");

	val = BUFSIZE;
	if (setsockopt(fds[0], SOL_SOCKET, SO_SNDBUF, &val, sizeof(val)))
		err(EXIT_FAILURE, "SO_SNDBUF");

	if (fcntl(fds[0], F_SETFL, O_NONBLOCK))
		err(EXIT_FAILURE, "fcntl");

	nwritten = write(fds[0], vp, BUFSIZE);
	if (nwritten == (ssize_t)-1)
		err(EXIT_FAILURE, "write");

	/* break loan */
	memset(vp, pat, BUFSIZE);

	nread = read(fds[1], buf, BUFSIZE);
	if (nread == (ssize_t)-1)
		err(EXIT_FAILURE, "read");

	if (nread != nwritten)
		errx(EXIT_FAILURE, "short read");

	if (docheck && memcmp(backup, buf, nread))
		errx(EXIT_FAILURE, "data mismatch");

	if (close(fds[0]) || close(fds[1]))
		err(EXIT_FAILURE, "close");
}

int
main(int argc, char *argv[])
{
	void *vp;
	int fd;

	fd = open(argv[1], O_RDWR);
	if (fd == -1)
		err(EXIT_FAILURE, "open");

	vp = mmap(NULL, BUFSIZE, PROT_READ|PROT_WRITE, MAP_FILE|MAP_PRIVATE,
	    fd, 0);
	if (vp == MAP_FAILED)
		err(EXIT_FAILURE, "mmap");

	testloan(vp, 'A', 0);
	testloan(vp, 'B', 1);

	if (munmap(vp, BUFSIZE))
		err(EXIT_FAILURE, "munmap");

	vp = mmap(NULL, BUFSIZE, PROT_READ|PROT_WRITE, MAP_FILE|MAP_SHARED,
	    fd, 0);
	if (vp == MAP_FAILED)
		err(EXIT_FAILURE, "mmap");

	testloan(vp, 'C', 0);
	testloan(vp, 'D', 1);

	if (munmap(vp, BUFSIZE))
		err(EXIT_FAILURE, "munmap");

	exit(EXIT_SUCCESS);
}
