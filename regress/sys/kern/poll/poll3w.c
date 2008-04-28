/* $NetBSD: poll3w.c,v 1.3 2008/04/28 20:23:07 martin Exp $ */

/*-
 * Copyright (c) 2002 The NetBSD Foundation, Inc.
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
 
/*
 * Check for 3-way collision for descriptor. First child comes
 * and polls on descriptor, second child comes and polls, first
 * child times out and exits, third child comes and polls.
 * When the wakeup event happens, the two remaining children
 * should both be awaken.
 */

#include <sys/types.h>
#include <sys/wait.h>
#include <poll.h>
#include <stdio.h>
#include <unistd.h>

int desc;

static void
child1(void)
{
	struct pollfd pfd;

	pfd.fd = desc;
	pfd.events = POLLIN | POLLHUP | POLLOUT;

	(void) poll(&pfd, 1, 2000);
	printf("child1 exit\n");
}

static void
child2(void)
{
	struct pollfd pfd;

	pfd.fd = desc;
	pfd.events = POLLIN | POLLHUP | POLLOUT;

	sleep(1);
	(void) poll(&pfd, 1, INFTIM);
	printf("child2 exit\n");
}

static void
child3(void)
{
	struct pollfd pfd;

	sleep(5);
	pfd.fd = desc;
	pfd.events = POLLIN | POLLHUP | POLLOUT;

	(void) poll(&pfd, 1, INFTIM);
	printf("child3 exit\n");
}

int main(void);

int
main(void)
{
	int pf[2];
	int status, i;

	pipe(pf);
	desc = pf[0];

	if (fork() == 0) {
		close(pf[1]);
		child1();
		_exit(0);
	}

	if (fork() == 0) {
		close(pf[1]);
		child2();
		_exit(0);
	}

	if (fork() == 0) {
		close(pf[1]);
		child3();
		_exit(0);
	}

	sleep(10);

	printf("parent write\n");
	write(pf[1], "konec\n", 6);

	for(i=0; i < 3; i++)
		wait(&status);

	printf("parent terminated\n");

	return (0);
}
