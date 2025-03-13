/*	$NetBSD: t_execve.c,v 1.3 2025/03/13 01:27:27 riastradh Exp $	*/

/*-
 * Copyright (c) 2014 The NetBSD Foundation, Inc.
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
__RCSID("$NetBSD: t_execve.c,v 1.3 2025/03/13 01:27:27 riastradh Exp $");

#include <sys/wait.h>

#include <atf-c.h>

#include <errno.h>
#include <limits.h>
#include <signal.h>
#include <stddef.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>

#include "h_macros.h"

ATF_TC(t_execve_null);
ATF_TC_HEAD(t_execve_null, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Tests an empty execve(2) executing");
}
ATF_TC_BODY(t_execve_null, tc)
{
	int err;

	err = execve(NULL, NULL, NULL);
	ATF_REQUIRE(err == -1);
	ATF_REQUIRE_MSG(errno == EFAULT,
	    "wrong error returned %d instead of %d", errno, EFAULT);
}

ATF_TC(t_execve_sig);
ATF_TC_HEAD(t_execve_sig, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Checks that execve does not drop pending signals");
}
ATF_TC_BODY(t_execve_sig, tc)
{
	const char *srcdir = atf_tc_get_config_var(tc, "srcdir");
	char h_execsig[PATH_MAX];
	time_t start;

	snprintf(h_execsig, sizeof(h_execsig), "%s/../h_execsig", srcdir);
	REQUIRE_LIBC(signal(SIGPIPE, SIG_IGN), SIG_ERR);

	atf_tc_expect_fail("PR kern/580911: after fork/execve or posix_spawn,"
	    " parent kill(child, SIGTERM) has race condition"
	    " making it unreliable");

	for (start = time(NULL); time(NULL) - start <= 10;) {
		int fd[2];
		char *const argv[] = {h_execsig, NULL};
		pid_t pid;
		int status;

		RL(pipe(fd));
		RL(pid = vfork());
		if (pid == 0) {
			if (dup2(fd[0], STDIN_FILENO) == -1)
				_exit(1);
			(void)execve(argv[0], argv, NULL);
			_exit(2);
		}
		RL(close(fd[0]));
		RL(kill(pid, SIGTERM));
		if (write(fd[1], (char[]){0}, 1) == -1 && errno != EPIPE)
			atf_tc_fail("write failed: %s", strerror(errno));
		RL(waitpid(pid, &status, 0));
		ATF_REQUIRE_MSG(WIFSIGNALED(status),
		    "child exited with status 0x%x", status);
		ATF_REQUIRE_EQ_MSG(WTERMSIG(status), SIGTERM,
		    "child exited on signal %d (%s)",
		    WTERMSIG(status), strsignal(WTERMSIG(status)));
		RL(close(fd[1]));
	}
}

ATF_TP_ADD_TCS(tp)
{

	ATF_TP_ADD_TC(tp, t_execve_null);
	ATF_TP_ADD_TC(tp, t_execve_sig);

	return atf_no_error();
}
