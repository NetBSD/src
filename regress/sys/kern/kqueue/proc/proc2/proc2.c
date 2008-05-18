/*	$NetBSD: proc2.c,v 1.2.30.1 2008/05/18 12:30:48 yamt Exp $	*/

/*-
 * Copyright (c) 2002 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Peter Werner <Peter.Werner@wgsn.com>.
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
 * This used to trigger NOTE_FORK|NOTE_TRACK error path problem
 * fixed in rev. 1.1.1.1.2.17 of sys/kern/kern_event.c
 */
#include <sys/types.h>
#include <sys/time.h>
#include <sys/event.h>
#include <stdio.h>
#include <unistd.h>
#include <pwd.h>
#include <err.h>

static void child_one(void);
static void child_two(void);

int
main(int argc, char **argv)
{
	pid_t pid = 0;
	int kq = -1;
	struct kevent ke;
	struct timespec timeout;

	if (geteuid())
		warnx("dont have uid 0, may not be able to setuid");

	if ((kq = kqueue()) == -1)
		err(1, "kq");

	timeout.tv_sec = 0;
	timeout.tv_nsec = 0;

	pid = fork();

	if (pid == -1)
		err(1, "fork");		
	else if (pid == 0) {
		sleep(1); /* let parent set kevent */
		child_one();
	}

	EV_SET(&ke, pid, EVFILT_PROC, EV_ADD, NOTE_FORK|NOTE_TRACK,
		0, 0);

	if ((kevent(kq, &ke, 1, NULL, 0, &timeout)) == -1)
		err(1, "kevent1"); 

	sleep(2);

	ke.ident = 0;
	ke.fflags = 0;
	ke.flags = EV_ENABLE;
	
	if ((kevent(kq, NULL, 0, &ke, 1, &timeout)) == -1)
		err(1, "kqueue");

	close(kq);

	/* 
	 * we are expecting an error here as we should not have
	 * been able to add a knote to child 2.
	 */

	return(!(ke.fflags & NOTE_TRACKERR));
}

static void
child_one(void) 
{
	pid_t pid;
	struct passwd *pwd;
	char *nam = "nobody";

	pwd = getpwnam(nam);
	if (pwd == NULL) 
		err(1, "%s", nam);
	
	if ((setuid(pwd->pw_uid)) == -1)
		err(1, "setuid %d", pwd->pw_uid);

	pid = fork();
	if (pid == -1)	
		err(1, "fork");
	else if (pid == 0)
		child_two();

	_exit(0);
}	
	
static void
child_two(void)
{
	_exit(0);
}
