/* $NetBSD: t_spawn.c,v 1.12 2025/03/16 15:35:36 riastradh Exp $ */

/*-
 * Copyright (c) 2012, 2021 The NetBSD Foundation, Inc.
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
__RCSID("$NetBSD: t_spawn.c,v 1.12 2025/03/16 15:35:36 riastradh Exp $");

#include <sys/fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <atf-c.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <signal.h>
#include <spawn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "fa_spawn_utils.h"
#include "h_macros.h"

static void check_success(const char *, const char *);

ATF_TC(t_spawn_ls);
ATF_TC_HEAD(t_spawn_ls, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Tests a simple posix_spawn executing /bin/ls");
}
ATF_TC_BODY(t_spawn_ls, tc)
{
	char * const args[] = { __UNCONST("ls"), __UNCONST("-la"), NULL };

	RZ(posix_spawn(NULL, "/bin/ls", NULL, NULL, args, NULL));
}

ATF_TC(t_spawnp_ls);
ATF_TC_HEAD(t_spawnp_ls, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Tests a simple posix_spawnp executing ls via $PATH");
}
ATF_TC_BODY(t_spawnp_ls, tc)
{
	char * const args[] = { __UNCONST("ls"), __UNCONST("-la"), NULL };

	RZ(posix_spawnp(NULL, "ls", NULL, NULL, args, NULL));
}

static void
spawn_error(const atf_tc_t *tc, const char *name, int error)
{
	char buf[PATH_MAX];
	char * const args[] = { __UNCONST(name), NULL };
	int err;

	snprintf(buf, sizeof(buf), "%s/%s",
	    atf_tc_get_config_var(tc, "srcdir"), name);
	err = posix_spawn(NULL, buf, NULL, NULL, args, NULL);
	ATF_REQUIRE_MSG(err == error, "expected error %d, "
	    "got %d when spawning %s", error, err, buf);
}

ATF_TC(t_spawn_zero);
ATF_TC_HEAD(t_spawn_zero, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "posix_spawn an invalid binary");
}
ATF_TC_BODY(t_spawn_zero, tc)
{
	spawn_error(tc, "h_zero", ENOEXEC);
}

ATF_TC(t_spawn_missing);
ATF_TC_HEAD(t_spawn_missing, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "posix_spawn a nonexistent binary");
}
ATF_TC_BODY(t_spawn_missing, tc)
{
	spawn_error(tc, "h_nonexist", ENOENT);
}

ATF_TC(t_spawn_nonexec);
ATF_TC_HEAD(t_spawn_nonexec, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "posix_spawn a script with non existing interpreter");
}
ATF_TC_BODY(t_spawn_nonexec, tc)
{
	spawn_error(tc, "h_nonexec", ENOENT);
}

ATF_TC(t_spawn_child);
ATF_TC_HEAD(t_spawn_child, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "posix_spawn a child and get its return code");
}

ATF_TC_BODY(t_spawn_child, tc)
{
	char buf[PATH_MAX];
	char rv[2] = { '0', '\0' };
	char * const args0[] = { __UNCONST("h_spawn"), rv, NULL };
	int rets[] = { 0, 1, 7 };
	int status;
	pid_t pid;

	snprintf(buf, sizeof(buf), "%s/h_spawn",
	    atf_tc_get_config_var(tc, "srcdir"));

	for (size_t i = 0; i < __arraycount(rets); i++) {
		rv[0] = rets[i] + '0';
		RZ(posix_spawn(&pid, buf, NULL, NULL, args0, NULL));
		ATF_REQUIRE_MSG(pid > 0, "pid=%lld", (long long)pid);
		RL(waitpid(pid, &status, 0));
		ATF_REQUIRE_MSG((WIFEXITED(status) &&
			WEXITSTATUS(status) == rets[i]),
		    "status=0x%x", status);
	}
}

#define FILENAME "output"

enum chdirop {
	OP_CHDIR = 1,
	OP_FCHDIR = 2,
};

static void
check_success(const char *dir, const char *file)
{
	ssize_t bytes_read;
	int fd;
	size_t sizeof_file = (size_t)filesize(file);
	size_t sizeof_str;
	char *contents;

	REQUIRE_LIBC(contents = malloc(sizeof_file), NULL);

	RL(fd = open(file, O_RDONLY));

	/*
	 * file contains form feed i.e ASCII - 10 at the end.
	 * Therefore sizeof_file - 1
	 */
	sizeof_str = strlen(dir);
	ATF_CHECK_MSG(sizeof_str == sizeof_file - 1, "%zu (%s) != %zu (%s)",
	    sizeof_str, dir, sizeof_file - 1, file);

	bytes_read = read(fd, contents, sizeof_file - 1);
	contents[sizeof_file - 1] = '\0';
	ATF_REQUIRE_MSG(strcmp(dir, contents) == 0,
	    "[%s] != [%s] Directories don't match", dir, contents);

	RL(close(fd));

	RL(unlink(file));
	free(contents);

	/* XXX not really required */
	ATF_REQUIRE_MSG((size_t)bytes_read == sizeof_str,
	    "bytes_read=%zu sizeof_str=%zu", bytes_read, sizeof_str);
}

static void
spawn_chdir(const char *dirpath, const char *filepath, enum chdirop operation,
    int expected_error)
{
	int error, fd = -1, status;
	char * const args[2] = { __UNCONST("pwd"), NULL };
	pid_t pid;
	posix_spawnattr_t attr, *attr_p;
	posix_spawn_file_actions_t fa;

	if (filepath)
		empty_outfile(filepath);

	RZ(posix_spawn_file_actions_init(&fa));

	switch (operation) {
	case OP_CHDIR:
		RZ(posix_spawn_file_actions_addchdir(&fa, dirpath));
		break;

	case OP_FCHDIR:
		RL(fd = open(dirpath, O_RDONLY));
		RZ(posix_spawn_file_actions_addfchdir(&fa, fd));
		break;
	}

	/*
	 * if POSIX_SPAWN_RETURNERROR is expected, then no need to open the
	 * file
	 */
	if (expected_error == 0) {
		RZ(posix_spawn_file_actions_addopen(&fa, STDOUT_FILENO,
		    FILENAME, O_WRONLY, 0));
		attr_p = NULL;
	} else {
		RZ(posix_spawnattr_init(&attr));

		/*
		 * POSIX_SPAWN_RETURNERROR is a NetBSD specific flag that
		 * will cause a "proper" return value from posix_spawn(2)
		 * instead of a (potential) success there and a 127 exit
		 * status from the child process (c.f. the non-diag variant
		 * of this test).
		 */
		RZ(posix_spawnattr_setflags(&attr, POSIX_SPAWN_RETURNERROR));
		attr_p = &attr;
	}

	error = posix_spawn(&pid, "/bin/pwd", &fa, attr_p, args, NULL);
	ATF_REQUIRE_MSG(error == expected_error, "error=%d expected_error=%d",
	    error, expected_error);

	/* wait for the child to finish only when no spawnattr */
	if (attr_p) {
		RZ(posix_spawnattr_destroy(&attr));
	} else {
		RL(waitpid(pid, &status, 0));
		ATF_REQUIRE_MSG((WIFEXITED(status) &&
			WEXITSTATUS(status) == EXIT_SUCCESS),
		    "[f]chdir failed");
	}

	RZ(posix_spawn_file_actions_destroy(&fa));

	/*
	 * The file incase of fchdir(),
	 * should be closed before reopening in 'check_success'
	 */
	if (fd != -1) {
		RL(close(fd));
	}
}

ATF_TC(t_spawn_chdir_abs);
ATF_TC_HEAD(t_spawn_chdir_abs, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Test posix_spawn_fa_addchdir for absolute path");
	atf_tc_set_md_var(tc, "require.progs", "/bin/pwd");
}
ATF_TC_BODY(t_spawn_chdir_abs, tc)
{
	char chdirpath[PATH_MAX], filepath[PATH_MAX];

	REQUIRE_LIBC(getcwd(chdirpath, sizeof(chdirpath)), NULL);
	RL(chdir("/"));
	if (snprintf(filepath, sizeof(filepath), "%s/%s", chdirpath, FILENAME)
	    >= (int)sizeof(filepath))
		atf_tc_fail("too deep: %s", chdirpath);

	spawn_chdir(chdirpath, filepath, OP_CHDIR, 0);

	/* finally cross check the output of "pwd" directory */
	check_success(chdirpath, filepath);
}

ATF_TC(t_spawn_chdir_rel);

ATF_TC_HEAD(t_spawn_chdir_rel, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Test posix_spawn_fa_addchdir for relative path");
	atf_tc_set_md_var(tc, "require.progs", "/bin/pwd");
}

ATF_TC_BODY(t_spawn_chdir_rel, tc)
{
	const char *relative_dir = "ch-dir";
	const char *testdir = getcwd(NULL, 0);
	char *chdirwd, *filepath;

	RL(mkdir(relative_dir, 0777));
	RL(asprintf(&chdirwd, "%s/%s", testdir, relative_dir));
	RL(asprintf(&filepath, "%s/%s", chdirwd, FILENAME));

#ifdef DEBUG
	printf("cwd: %s\n", testdir);
	printf("chdirwd: %s\n", chdirwd);
	printf("filepath: %s\n", filepath);
#endif

	spawn_chdir(relative_dir, filepath, 1, 0);

	/* finally cross check the directory */
	check_success(chdirwd, filepath);
	free(chdirwd);
	free(filepath);
}

ATF_TC(t_spawn_chdir_file);
ATF_TC_HEAD(t_spawn_chdir_file, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Test posix_spawn_fa_addchdir on plain file (not a directory)");
	atf_tc_set_md_var(tc, "require.progs", "/bin/pwd");
}
ATF_TC_BODY(t_spawn_chdir_file, tc)
{
	char cwd[PATH_MAX], filepath[PATH_MAX];

	REQUIRE_LIBC(getcwd(cwd, sizeof(cwd)), NULL);
	if (snprintf(filepath, sizeof(filepath), "%s/%s", cwd, FILENAME)
	    >= (int)sizeof(filepath))
		atf_tc_fail("too deep: %s", cwd);

	spawn_chdir(filepath, filepath, 1, ENOTDIR);
}

ATF_TC(t_spawn_chdir_invalid);
ATF_TC_HEAD(t_spawn_chdir_invalid, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Test posix_spawn_fa_addchdir for an invalid dir");
	atf_tc_set_md_var(tc, "require.progs", "/bin/pwd");
}
ATF_TC_BODY(t_spawn_chdir_invalid, tc)
{
	spawn_chdir("/not/a/valid/dir", NULL, 1, ENOENT);
}

ATF_TC(t_spawn_chdir_permissions);
ATF_TC_HEAD(t_spawn_chdir_permissions, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Test posix_spawn_file_actions_addchdir for prohibited directory");
	atf_tc_set_md_var(tc, "require.progs", "/bin/pwd");
	atf_tc_set_md_var(tc, "require.user", "unprivileged");
}
ATF_TC_BODY(t_spawn_chdir_permissions, tc)
{
	const char *restricted_dir = "prohibited";

	RL(mkdir(restricted_dir, 0055));

	spawn_chdir(restricted_dir, NULL, 1, EACCES);
}

ATF_TC(t_spawn_fchdir_abs);
ATF_TC_HEAD(t_spawn_fchdir_abs, tc)
{
	atf_tc_set_md_var(tc, "descr", "Test posix_spawn_fa_fchdir");
	atf_tc_set_md_var(tc, "require.progs", "/bin/pwd");
}
ATF_TC_BODY(t_spawn_fchdir_abs, tc)
{
	char chdirpath[PATH_MAX], filepath[PATH_MAX];

	REQUIRE_LIBC(getcwd(chdirpath, sizeof(chdirpath)), NULL);
	RL(chdir("/"));
	if (snprintf(filepath, sizeof(filepath), "%s/%s", chdirpath, FILENAME)
	    >= (int)sizeof(filepath))
		atf_tc_fail("too deep: %s", chdirpath);

	spawn_chdir(chdirpath, filepath, OP_FCHDIR, 0);

	/* finally cross check the directory */
	check_success(chdirpath, filepath);
}

ATF_TC(t_spawn_fchdir_rel);
ATF_TC_HEAD(t_spawn_fchdir_rel, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Testing posix_spawn_file_actions_addfchdir on a relative "
	    "directory");
	atf_tc_set_md_var(tc, "require.progs", "/bin/pwd");
}
ATF_TC_BODY(t_spawn_fchdir_rel, tc)
{
	const char *relative_dir = "ch-dir";
	const char *testdir = getcwd(NULL, 0);
	char *chdirwd, *filepath;

	RL(mkdir(relative_dir, 0755));

	/*
	 * This is done in parts purposely.
	 * It enables the abs path of the relative dir
	 * to be passed to 'check_success()' for comparing
	 */
	RL(asprintf(&chdirwd, "%s/%s", testdir, relative_dir));
	RL(asprintf(&filepath, "%s/%s", chdirwd, FILENAME));

	spawn_chdir(relative_dir, filepath, 2, 0);

	/* finally cross check the directory */
	check_success(chdirwd, filepath);
	free(chdirwd);
	free(filepath);
}

ATF_TC(t_spawn_fchdir_file);
ATF_TC_HEAD(t_spawn_fchdir_file, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Testing posix_spawn_file_actions_addfchdir on a "
	    "regular file (not a directory)");
	atf_tc_set_md_var(tc, "require.progs", "/bin/pwd");
}
ATF_TC_BODY(t_spawn_fchdir_file, tc)
{
	char cwd[PATH_MAX], filepath[PATH_MAX];
	int fd;

	REQUIRE_LIBC(getcwd(cwd, sizeof(cwd)), NULL);
	if (snprintf(filepath, sizeof(filepath), "%s/%s", cwd, FILENAME)
	    >= (int)sizeof(filepath))
		atf_tc_fail("too deep: %s", cwd);
	RL(fd = open(FILENAME, O_WRONLY | O_CREAT | O_TRUNC, 0644));
	RL(close(fd));

	spawn_chdir(filepath, NULL, 2, ENOTDIR);
}

ATF_TC(t_spawn_fchdir_neg_fd);
ATF_TC_HEAD(t_spawn_fchdir_neg_fd, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Testing posix_spawn_file_actions_addfchdir on a negative file "
	    "descriptor");
	atf_tc_set_md_var(tc, "require.progs", "/bin/pwd");
}
ATF_TC_BODY(t_spawn_fchdir_neg_fd, tc)
{
	int error, fd;
	posix_spawn_file_actions_t fa;

	fd = -1;

	RZ(posix_spawn_file_actions_init(&fa));

	error = posix_spawn_file_actions_addfchdir(&fa, fd);
	ATF_REQUIRE_MSG(error == EBADF, "error=%d", error);

	RZ(posix_spawn_file_actions_destroy(&fa));
}

ATF_TC(t_spawn_fchdir_closed);
ATF_TC_HEAD(t_spawn_fchdir_closed, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Testing posix_spawn_file_actions_addfchdir for a closed fd");
	atf_tc_set_md_var(tc, "require.progs", "/bin/pwd");
}
ATF_TC_BODY(t_spawn_fchdir_closed, tc)
{
	int error, fd;
	pid_t pid;
	char * const args[2] = { __UNCONST("pwd"), NULL };
	posix_spawnattr_t attr;
	posix_spawn_file_actions_t fa;

	fd = 3;
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
	RZ(posix_spawn_file_actions_addfchdir(&fa, fd));

	error = posix_spawn(&pid, "/bin/pwd", &fa, &attr, args, NULL);
	ATF_REQUIRE_MSG(error == EBADF, "error=%d", error);

	RZ(posix_spawn_file_actions_destroy(&fa));
	RZ(posix_spawnattr_destroy(&attr));
}

ATF_TC(t_spawn_sig);
ATF_TC_HEAD(t_spawn_sig, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Checks that posix_spawn does not drop pending signals");
}
ATF_TC_BODY(t_spawn_sig, tc)
{
	const char *srcdir = atf_tc_get_config_var(tc, "srcdir");
	char h_execsig[PATH_MAX];
	time_t start;

	snprintf(h_execsig, sizeof(h_execsig), "%s/../h_execsig", srcdir);
	REQUIRE_LIBC(signal(SIGPIPE, SIG_IGN), SIG_ERR);

	for (start = time(NULL); time(NULL) - start <= 10;) {
		int fd[2];
		char *const argv[] = {h_execsig, NULL};
		posix_spawn_file_actions_t fa;
		pid_t pid;
		int status;

		RL(pipe2(fd, O_CLOEXEC));
		RZ(posix_spawn_file_actions_init(&fa));
		RZ(posix_spawn_file_actions_adddup2(&fa, fd[0], STDIN_FILENO));
		RZ(posix_spawn(&pid, argv[0], &fa, NULL, argv, NULL));
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
		RZ(posix_spawn_file_actions_destroy(&fa));
	}
}

ATF_TP_ADD_TCS(tp)
{
	ATF_TP_ADD_TC(tp, t_spawn_ls);
	ATF_TP_ADD_TC(tp, t_spawnp_ls);
	ATF_TP_ADD_TC(tp, t_spawn_zero);
	ATF_TP_ADD_TC(tp, t_spawn_missing);
	ATF_TP_ADD_TC(tp, t_spawn_nonexec);
	ATF_TP_ADD_TC(tp, t_spawn_child);
	ATF_TP_ADD_TC(tp, t_spawn_chdir_abs);
	ATF_TP_ADD_TC(tp, t_spawn_chdir_rel);
	ATF_TP_ADD_TC(tp, t_spawn_chdir_file);
	ATF_TP_ADD_TC(tp, t_spawn_chdir_invalid);
	ATF_TP_ADD_TC(tp, t_spawn_chdir_permissions);
	ATF_TP_ADD_TC(tp, t_spawn_fchdir_abs);
	ATF_TP_ADD_TC(tp, t_spawn_fchdir_rel);
	ATF_TP_ADD_TC(tp, t_spawn_fchdir_file);
	ATF_TP_ADD_TC(tp, t_spawn_fchdir_neg_fd);
	ATF_TP_ADD_TC(tp, t_spawn_fchdir_closed);
	ATF_TP_ADD_TC(tp, t_spawn_sig);

	return atf_no_error();
}
