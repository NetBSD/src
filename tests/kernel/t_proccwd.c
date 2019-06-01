/*	$NetBSD: t_proccwd.c,v 1.2 2019/06/01 22:18:23 kamil Exp $	*/
/*-
 * Copyright (c) 2019 The NetBSD Foundation, Inc.
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
__COPYRIGHT("@(#) Copyright (c) 2019\
 The NetBSD Foundation, inc. All rights reserved.");
__RCSID("$NetBSD: t_proccwd.c,v 1.2 2019/06/01 22:18:23 kamil Exp $");

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/sysctl.h>
#include <sys/wait.h>

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <err.h>

#include <atf-c.h>

static int
getproccwd(char *path, size_t *len, pid_t pid)
{
	const int name[] = {
	    CTL_KERN, KERN_PROC_ARGS, pid, KERN_PROC_CWD,
	};

     	return sysctl(name, __arraycount(name), path, len, NULL, 0);
}

ATF_TC(prompt_pid);
ATF_TC_HEAD(prompt_pid, tc)
{

        atf_tc_set_md_var(tc, "descr",
            "Prompt length of the current dir and assert that it is sane");
}

ATF_TC_BODY(prompt_pid, tc)
{
	char buf[MAXPATHLEN];
	char cwdbuf[MAXPATHLEN];
	size_t len, prompted_len;
	size_t i;

	pid_t t[] = {
		-1,
		getpid(),
		1 /* /sbin/init */
	};

	for (i = 0; i < __arraycount(t); i++) {
		len = 0;
		ATF_REQUIRE_EQ(getproccwd(NULL, &len, t[i]), 0);

		prompted_len = len;
		ATF_REQUIRE_EQ(getproccwd(buf, &len, t[i]), 0);

		ATF_REQUIRE_EQ(strlen(buf) + 1, prompted_len);
		ATF_REQUIRE(strlen(buf) > 0);

		if (t[i] == -1 || t[i] == getpid()) {
			getcwd(cwdbuf, MAXPATHLEN);
			ATF_REQUIRE_EQ(strcmp(buf, cwdbuf), 0);
			ATF_REQUIRE(strlen(buf) > strlen("/"));
		} else if (t[i] == 1) {
			ATF_REQUIRE_EQ(strcmp(buf, "/"), 0);
		}
	}
}

ATF_TC(chroot);
ATF_TC_HEAD(chroot, tc)
{

        atf_tc_set_md_var(tc, "descr",
            "prompt length of the current dir and assert that it is right");

	atf_tc_set_md_var(tc, "require.user", "root");
}

#define ASSERT(x) do { if (!(x)) _exit(EXIT_FAILURE); } while (0)

ATF_TC_BODY(chroot, tc)
{
	char buf[MAXPATHLEN];
	struct stat root_dir;
	struct stat cur_dir;
	size_t len;
	pid_t child, wpid;
	int status;
	int rv;

	const pid_t pid_one = 1; /* /sbin/init */

	ATF_REQUIRE(getcwd(buf, sizeof(buf)) != NULL);

	ATF_REQUIRE_EQ(stat(buf, &cur_dir), 0);
	ATF_REQUIRE_EQ(stat("/", &root_dir), 0);

	if (cur_dir.st_ino == root_dir.st_ino)
		atf_tc_skip("This test does not work in /");

	child = atf_utils_fork();
	if (child == 0) {
		len = 0;

		ASSERT(chroot(buf) == 0);

		errno = 0;
		rv = getproccwd(buf, &len, pid_one);
		ASSERT(rv == -1);
		ASSERT(errno == ENOENT);

		_exit(EXIT_SUCCESS);
	}
	wpid = waitpid(child, &status, WEXITED);
	ATF_REQUIRE_EQ(wpid, child);
	ATF_REQUIRE_EQ(WEXITSTATUS(status), EXIT_SUCCESS);
}

ATF_TP_ADD_TCS(tp)
{

	ATF_TP_ADD_TC(tp, prompt_pid);
	ATF_TP_ADD_TC(tp, chroot);

	return atf_no_error();
}
