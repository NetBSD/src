/*	$NetBSD: dislodgefd.c,v 1.1 2023/10/13 18:46:22 ad Exp $	*/

/*-
 * Copyright (c) 2023 The NetBSD Foundation, Inc.
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

#include <sys/socket.h>
#include <sys/un.h>

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <err.h>

pthread_barrier_t	barrier;
int			fds[2];

static void *
reader(void *cookie)
{
	char buf[1];

	(void)pthread_barrier_wait(&barrier);
	printf("reader(): commencing read, this should error out... after 1s\n");
	if (read(fds[0], buf, sizeof(buf)) == -1)
		err(1, "read");

	printf("reader(): read terminated without error??\n");
	return NULL;
}

int
main(int argc, char *argv[])
{
	pthread_t pt;

	if (argc > 1 && strcmp(argv[1], "pipe") == 0) {
		if (pipe(fds))
			err(1, "pipe");
	} else {
		if (socketpair(PF_LOCAL, SOCK_STREAM, 0, fds) < 0)
			err(1, "socketpair");
	}
	pthread_barrier_init(&barrier, NULL, 2);
	if (pthread_create(&pt, NULL, reader, NULL)) {
		errx(1, "pthread_create failed");
	}
	(void)pthread_barrier_wait(&barrier);
	sleep(1);
	printf("main(): closing the reader side fd..\n");
	close(fds[0]);
	printf("main(): sleeping again for a bit..\n");
	sleep(1);
	printf("main(): exiting.\n");
	return 0;
}
