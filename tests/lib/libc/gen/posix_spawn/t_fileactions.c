/* $NetBSD: t_fileactions.c,v 1.9 2025/07/09 11:40:43 martin Exp $ */

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

#include <sys/cdefs.h>
__RCSID("$NetBSD: t_fileactions.c,v 1.9 2025/07/09 11:40:43 martin Exp $");

#include <sys/stat.h>
#include <sys/wait.h>

#include <atf-c.h>
#include <errno.h>
#include <fcntl.h>
#include <spawn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "fa_spawn_utils.h"
#include "h_macros.h"

#define TESTFILE	"./the_input_data"
#define CHECKFILE	"./the_output_data"
#define TESTCONTENT	"marry has a little lamb"

static void
make_testfile(const char *restrict file)
{
	FILE *f;
	ssize_t written;

	REQUIRE_LIBC(f = fopen(file, "w"), NULL);
	RL(written = fwrite(TESTCONTENT, 1, strlen(TESTCONTENT), f));
	REQUIRE_LIBC(fclose(f), EOF);
	ATF_REQUIRE((size_t)written == strlen(TESTCONTENT));
}

ATF_TC(t_spawn_openmode);
ATF_TC_HEAD(t_spawn_openmode, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Test the proper handling of 'mode' for 'open' fileactions");
	atf_tc_set_md_var(tc, "require.progs", "/bin/cat");
}
ATF_TC_BODY(t_spawn_openmode, tc)
{
	int status;
	pid_t pid;
	size_t insize, outsize;
	char * const args[2] = { __UNCONST("cat"), NULL };
	posix_spawn_file_actions_t fa;

	/*
	 * try a "cat < testfile > checkfile"
	 */
	make_testfile(TESTFILE);

	RZ(posix_spawn_file_actions_init(&fa));
	RZ(posix_spawn_file_actions_addopen(&fa, fileno(stdin),
		TESTFILE, O_RDONLY, 0));
	RZ(posix_spawn_file_actions_addopen(&fa, fileno(stdout),
		CHECKFILE, O_WRONLY|O_CREAT, 0600));
	RZ(posix_spawn(&pid, "/bin/cat", &fa, NULL, args, NULL));
	RZ(posix_spawn_file_actions_destroy(&fa));

	/* ok, wait for the child to finish */
	RL(waitpid(pid, &status, 0));
	ATF_REQUIRE_MSG((WIFEXITED(status) &&
		WEXITSTATUS(status) == EXIT_SUCCESS),
	    "status=0x%x", status);

	/* now check that input and output have the same size */
	insize = filesize(TESTFILE);
	outsize = filesize(CHECKFILE);
	ATF_CHECK_MSG(insize == strlen(TESTCONTENT),
	    "insize=%zu strlen(TESTCONTENT)=%zu", insize, strlen(TESTCONTENT));
	ATF_CHECK_MSG(insize == outsize,
	    "insize=%zu outsize=%zu", insize, outsize);

	/*
	 * try a "cat < testfile >> checkfile"
	 */
	make_testfile(TESTFILE);
	make_testfile(CHECKFILE);

	RZ(posix_spawn_file_actions_init(&fa));
	RZ(posix_spawn_file_actions_addopen(&fa, fileno(stdin),
		TESTFILE, O_RDONLY, 0));
	RZ(posix_spawn_file_actions_addopen(&fa, fileno(stdout),
		CHECKFILE, O_WRONLY|O_APPEND, 0));
	RZ(posix_spawn(&pid, "/bin/cat", &fa, NULL, args, NULL));
	RZ(posix_spawn_file_actions_destroy(&fa));

	/* ok, wait for the child to finish */
	RL(waitpid(pid, &status, 0));
	ATF_REQUIRE_MSG((WIFEXITED(status) &&
		WEXITSTATUS(status) == EXIT_SUCCESS),
	    "status=0x%x", status);

	/* now check that output is twice as long as input */
	insize = filesize(TESTFILE);
	outsize = filesize(CHECKFILE);
	ATF_CHECK_MSG(insize == strlen(TESTCONTENT),
	    "insize=%zu strlen(TESTCONTENT)=%zu", insize, strlen(TESTCONTENT));
	ATF_CHECK_MSG(insize*2 == outsize,
	    "insize*2=%zu outsize=%zu", insize*2, outsize);

	/*
	 * try a "cat < testfile  > checkfile" with input and output swapped
	 */
	make_testfile(TESTFILE);
	empty_outfile(CHECKFILE);

	RZ(posix_spawn_file_actions_init(&fa));
	RZ(posix_spawn_file_actions_addopen(&fa, fileno(stdout),
		TESTFILE, O_RDONLY, 0));
	RZ(posix_spawn_file_actions_addopen(&fa, fileno(stdin),
		CHECKFILE, O_WRONLY, 0));
	RZ(posix_spawn(&pid, "/bin/cat", &fa, NULL, args, NULL));
	RZ(posix_spawn_file_actions_destroy(&fa));

	/* ok, wait for the child to finish */
	RL(waitpid(pid, &status, 0));
	ATF_REQUIRE_MSG((WIFEXITED(status) &&
		WEXITSTATUS(status) == EXIT_FAILURE),
	    "status=0x%x", status);

	/* now check that input and output are still the same size */
	insize = filesize(TESTFILE);
	outsize = filesize(CHECKFILE);
	ATF_CHECK_MSG(insize == strlen(TESTCONTENT),
	    "insize=%zu strlen(TESTCONTENT)=%zu", insize, strlen(TESTCONTENT));
	ATF_CHECK_MSG(outsize == 0,
	    "outsize=%zu", outsize);
}

ATF_TC(t_spawn_reopen);
ATF_TC_HEAD(t_spawn_reopen, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "an open filehandle can be replaced by a 'open' fileaction");
	atf_tc_set_md_var(tc, "require.progs", "/bin/cat");
}
ATF_TC_BODY(t_spawn_reopen, tc)
{
	int status;
	pid_t pid;
	char * const args[2] = { __UNCONST("cat"), NULL };
	posix_spawn_file_actions_t fa;

	/*
	 * make sure stdin is open in the parent
	 */
	REQUIRE_LIBC(freopen("/dev/zero", "r", stdin), NULL);
	/*
	 * now request an open for this fd again in the child
	 */
	RZ(posix_spawn_file_actions_init(&fa));
	RZ(posix_spawn_file_actions_addopen(&fa, fileno(stdin),
		"/dev/null", O_RDONLY, 0));
	RZ(posix_spawn(&pid, "/bin/cat", &fa, NULL, args, NULL));
	RZ(posix_spawn_file_actions_destroy(&fa));

	RL(waitpid(pid, &status, 0));
	ATF_REQUIRE_MSG((WIFEXITED(status) &&
		WEXITSTATUS(status) == EXIT_SUCCESS),
	    "status=0x%x", status);
}

ATF_TC(t_spawn_open_nonexistent);
ATF_TC_HEAD(t_spawn_open_nonexistent, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "posix_spawn fails when a file to open does not exist");
	atf_tc_set_md_var(tc, "require.progs", "/bin/cat");
}
ATF_TC_BODY(t_spawn_open_nonexistent, tc)
{
	int err, status;
	pid_t pid;
	char * const args[2] = { __UNCONST("cat"), NULL };
	posix_spawn_file_actions_t fa;

	RZ(posix_spawn_file_actions_init(&fa));
	RZ(posix_spawn_file_actions_addopen(&fa, STDIN_FILENO,
		"./non/ex/ist/ent", O_RDONLY, 0));
	err = posix_spawn(&pid, "/bin/cat", &fa, NULL, args, NULL);
	if (err == 0) {
		/*
		 * The child has been created - it should fail and
		 * return exit code 127
		 */
		waitpid(pid, &status, 0);
		ATF_REQUIRE(WIFEXITED(status) && WEXITSTATUS(status) == 127);
	} else {
		/*
		 * The error has been noticed early enough, no child has
		 * been run
		 */
		ATF_REQUIRE_MSG(err == ENOENT, "err=%d (%s)",
		    err, strerror(err));
	}
	RZ(posix_spawn_file_actions_destroy(&fa));
}

ATF_TC(t_spawn_open_nonexistent_diag);
ATF_TC_HEAD(t_spawn_open_nonexistent_diag, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "posix_spawn fails when a file to open does not exist "
	    "and delivers proper diagnostic");
	atf_tc_set_md_var(tc, "require.progs", "/bin/cat");
}
ATF_TC_BODY(t_spawn_open_nonexistent_diag, tc)
{
	int err;
	pid_t pid;
	char * const args[2] = { __UNCONST("cat"), NULL };
	posix_spawnattr_t attr;
	posix_spawn_file_actions_t fa;

	RZ(posix_spawnattr_init(&attr));
	/*
	 * POSIX_SPAWN_RETURNERROR is a NetBSD specific flag that
	 * will cause a "proper" return value from posix_spawn(2)
	 * instead of a (potential) success there and a 127 exit
	 * status from the child process (c.f. the non-diag variant
	 * of this test).
	 */
	RZ(posix_spawnattr_setflags(&attr, POSIX_SPAWN_RETURNERROR));
	RZ(posix_spawn_file_actions_init(&fa));
	RZ(posix_spawn_file_actions_addopen(&fa, STDIN_FILENO,
		"./non/ex/ist/ent", O_RDONLY, 0));
	err = posix_spawn(&pid, "/bin/cat", &fa, &attr, args, NULL);
	ATF_REQUIRE_MSG(err == ENOENT, "err=%d (%s)", err, strerror(err));
	RZ(posix_spawn_file_actions_destroy(&fa));
	RZ(posix_spawnattr_destroy(&attr));
}

ATF_TC(t_spawn_fileactions);
ATF_TC_HEAD(t_spawn_fileactions, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Tests various complex fileactions");
}
ATF_TC_BODY(t_spawn_fileactions, tc)
{
	int fd1, fd2, fd3, status;
	pid_t pid;
	char * const args[2] = { __UNCONST("h_fileactions"), NULL };
	char helper[FILENAME_MAX];
	posix_spawn_file_actions_t fa;

	RZ(posix_spawn_file_actions_init(&fa));

	RL(closefrom(fileno(stderr) + 1));

	RL(fd1 = open("/dev/null", O_RDONLY));
	ATF_REQUIRE(fd1 == 3);

	RL(fd2 = open("/dev/null", O_WRONLY, O_CLOEXEC));
	ATF_REQUIRE(fd2 == 4);

	RL(fd3 = open("/dev/null", O_WRONLY));
	ATF_REQUIRE(fd3 == 5);

	RZ(posix_spawn_file_actions_addclose(&fa, fd1));
	RZ(posix_spawn_file_actions_addopen(&fa, 6, "/dev/null", O_RDWR, 0));
	RZ(posix_spawn_file_actions_adddup2(&fa, 1, 7));

	snprintf(helper, sizeof helper, "%s/h_fileactions",
	    atf_tc_get_config_var(tc, "srcdir"));
	RZ(posix_spawn(&pid, helper, &fa, NULL, args, NULL));
	RZ(posix_spawn_file_actions_destroy(&fa));

	RL(waitpid(pid, &status, 0));
	ATF_REQUIRE_MSG((WIFEXITED(status) &&
		WEXITSTATUS(status) == EXIT_SUCCESS),
	    "status=0x%x", status);
}

ATF_TC(t_spawn_close_already_closed);
ATF_TC_HEAD(t_spawn_close_already_closed, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "file actions closing closed descriptors are allowed (PR 59523");
	atf_tc_set_md_var(tc, "require.progs", "/bin/ls");
}

ATF_TC_BODY(t_spawn_close_already_closed, tc)
{
	int status, fd;
	pid_t pid;
	char * const args[2] = { __UNCONST("ls"), NULL };
	posix_spawn_file_actions_t fa;

	/* get a free file descriptor number */
	fd = open("/dev/null", O_RDONLY);
	ATF_REQUIRE(fd >= 0);
	close(fd);

	RZ(posix_spawn_file_actions_init(&fa));
	// redirect output to /dev/null to not garble atf test results
	RZ(posix_spawn_file_actions_addopen(&fa, STDOUT_FILENO, "/dev/null",
	    O_WRONLY, 0));
	// known closed fd
	RZ(posix_spawn_file_actions_addclose(&fa, fd));
	// a random fd we know nothing about (cross fingers!
	RZ(posix_spawn_file_actions_addclose(&fa, fd+1));
	// high fd probably not ever been allocated, likely to trigger
	// a fd_getfile() failure in the kernel, which is another
	// path that originaly caused the fallout in PR 59523
	RZ(posix_spawn_file_actions_addclose(&fa, 560));
	RZ(posix_spawn(&pid, "/bin/ls", &fa, NULL, args, NULL));
	RZ(posix_spawn_file_actions_destroy(&fa));

	/* ok, wait for the child to finish */
	RL(waitpid(pid, &status, 0));
	ATF_REQUIRE_MSG((WIFEXITED(status) &&
		WEXITSTATUS(status) == EXIT_SUCCESS),
	    "status=0x%x", status);
}

ATF_TC(t_spawn_close_already_closed_wait);
ATF_TC_HEAD(t_spawn_close_already_closed_wait, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "file actions closing closed descriptors are allowed, "
	    "with parent process waiting (PR 59523");
	atf_tc_set_md_var(tc, "require.progs", "/bin/ls");
}

ATF_TC_BODY(t_spawn_close_already_closed_wait, tc)
{
	int status, fd;
	pid_t pid;
	char * const args[2] = { __UNCONST("ls"), NULL };
	posix_spawn_file_actions_t fa;
	posix_spawnattr_t attr;

	/* get a free file descriptor number */
	fd = open("/dev/null", O_RDONLY);
	ATF_REQUIRE(fd >= 0);
	close(fd);
	RZ(posix_spawn_file_actions_init(&fa));
	// redirect output to /dev/null to not garble atf test results
	RZ(posix_spawn_file_actions_addopen(&fa, STDOUT_FILENO, "/dev/null",
	    O_WRONLY, 0));
	// known closed fd
	RZ(posix_spawn_file_actions_addclose(&fa, fd));
	// a random fd we know nothing about (cross fingers!
	RZ(posix_spawn_file_actions_addclose(&fa, fd+1));
	// high fd probably not ever been allocated, likely to trigger
	// a fd_getfile() failure in the kernel, which is another
	// path that originaly caused the fallout in PR 59523
	RZ(posix_spawn_file_actions_addclose(&fa, 560));

	RZ(posix_spawnattr_init(&attr));
	RZ(posix_spawnattr_setflags(&attr, POSIX_SPAWN_SETPGROUP));

	RZ(posix_spawn(&pid, "/bin/ls", &fa, &attr, args, NULL));
	RZ(posix_spawn_file_actions_destroy(&fa));

	/* ok, wait for the child to finish */
	RL(waitpid(pid, &status, 0));
	ATF_REQUIRE_MSG((WIFEXITED(status) &&
		WEXITSTATUS(status) == EXIT_SUCCESS),
	    "status=0x%x", status);
}

ATF_TC(t_spawn_empty_fileactions);
ATF_TC_HEAD(t_spawn_empty_fileactions, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "posix_spawn with empty fileactions (PR kern/46038)");
	atf_tc_set_md_var(tc, "require.progs", "/bin/cat");
}
ATF_TC_BODY(t_spawn_empty_fileactions, tc)
{
	int status;
	pid_t pid;
	char * const args[2] = { __UNCONST("cat"), NULL };
	posix_spawn_file_actions_t fa;
	size_t insize, outsize;

	/*
	 * try a "cat < testfile > checkfile", but set up stdin/stdout
	 * already in the parent and pass empty file actions to the child.
	 */
	make_testfile(TESTFILE);

	REQUIRE_LIBC(freopen(TESTFILE, "r", stdin), NULL);
	REQUIRE_LIBC(freopen(CHECKFILE, "w", stdout), NULL);

	RZ(posix_spawn_file_actions_init(&fa));
	RZ(posix_spawn(&pid, "/bin/cat", &fa, NULL, args, NULL));
	RZ(posix_spawn_file_actions_destroy(&fa));

	/* ok, wait for the child to finish */
	RL(waitpid(pid, &status, 0));
	ATF_REQUIRE_MSG((WIFEXITED(status) &&
		WEXITSTATUS(status) == EXIT_SUCCESS),
	    "status=0x%x", status);

	/* now check that input and output have the same size */
	insize = filesize(TESTFILE);
	outsize = filesize(CHECKFILE);
	ATF_CHECK_MSG(insize == strlen(TESTCONTENT),
	    "insize=%zu strlen(TESTCONTENT)=%zu", insize, strlen(TESTCONTENT));
	ATF_CHECK_MSG(insize == outsize,
	    "insize=%zu outsize=%zu", insize, outsize);
}

ATF_TP_ADD_TCS(tp)
{

	ATF_TP_ADD_TC(tp, t_spawn_fileactions);
	ATF_TP_ADD_TC(tp, t_spawn_open_nonexistent);
	ATF_TP_ADD_TC(tp, t_spawn_open_nonexistent_diag);
	ATF_TP_ADD_TC(tp, t_spawn_reopen);
	ATF_TP_ADD_TC(tp, t_spawn_openmode);
	ATF_TP_ADD_TC(tp, t_spawn_empty_fileactions);
	ATF_TP_ADD_TC(tp, t_spawn_close_already_closed);
	ATF_TP_ADD_TC(tp, t_spawn_close_already_closed_wait);

	return atf_no_error();
}
