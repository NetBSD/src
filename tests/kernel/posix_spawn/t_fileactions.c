/* $NetBSD: t_fileactions.c,v 1.1 2012/02/11 23:31:25 martin Exp $ */

/*-
 * Copyright (c) 2012 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Charles Zhang <charles@NetBSD.org> and
 * Martin Husemann <martin@NetBSD.org>.
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


#include <atf-c.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <spawn.h>
#include <unistd.h>
#include <sys/wait.h>

ATF_TC(t_spawn_fileactions);

ATF_TC_HEAD(t_spawn_fileactions, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Tests posix_spawn with fileactions");
}

ATF_TC_BODY(t_spawn_fileactions, tc)
{
	int fd1, fd2, fd3, status, err;
	pid_t pid;
	char * const args[2] = { __UNCONST("h_fileactions"), NULL };
	char helper[FILENAME_MAX];
	posix_spawn_file_actions_t fa;

	posix_spawn_file_actions_init(&fa);

	closefrom(fileno(stderr)+1);

	fd1 = open("/dev/null", O_RDONLY);
	ATF_REQUIRE(fd1 == 3);

	fd2 = open("/dev/null", O_WRONLY, O_CLOEXEC);
	ATF_REQUIRE(fd2 == 4);

	fd3 = open("/dev/null", O_WRONLY);
	ATF_REQUIRE(fd3 == 5);

	posix_spawn_file_actions_addclose(&fa, fd1);
	posix_spawn_file_actions_addopen(&fa, 6, "/dev/null", O_RDWR, 0);
	posix_spawn_file_actions_adddup2(&fa, 1, 7); 

	snprintf(helper, sizeof helper, "%s/h_fileactions",
	    atf_tc_get_config_var(tc, "srcdir"));
	err = posix_spawn(&pid, helper, &fa, NULL, args, NULL);
	ATF_REQUIRE(err == 0);

	waitpid(pid, &status, 0);
	ATF_REQUIRE(WIFEXITED(status) && WEXITSTATUS(status) == EXIT_SUCCESS);

	posix_spawn_file_actions_destroy(&fa);
}

ATF_TP_ADD_TCS(tp)
{
	ATF_TP_ADD_TC(tp, t_spawn_fileactions);

	return atf_no_error();
}
