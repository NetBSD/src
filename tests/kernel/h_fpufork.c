/*	$NetBSD: h_fpufork.c,v 1.1 2020/02/11 03:15:10 riastradh Exp $	*/

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
__RCSID("$NetBSD: h_fpufork.c,v 1.1 2020/02/11 03:15:10 riastradh Exp $");

#include <sys/wait.h>

#include <err.h>
#include <sched.h>
#include <stdio.h>
#include <unistd.h>

int
main(void)
{
	pid_t child, pid;
	volatile double x = 1;
	register double y = 2*x;
	int status;

	pid = fork();
	switch (pid) {
	case -1:		/* error */
		err(1, "fork");
	case 0:			/* child */
		_exit(y == 2 ? 0 : 1);
	default:		/* parent */
		break;
	}

	if ((child = wait(&status)) == -1)
		err(1, "wait");
	if (WIFSIGNALED(status))
		errx(1, "child exited on signal %d", WTERMSIG(status));
	if (!WIFEXITED(status))
		errx(1, "child didn't exit");
	return WEXITSTATUS(status);
}
