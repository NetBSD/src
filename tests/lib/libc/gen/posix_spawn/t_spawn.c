/* $NetBSD: t_spawn.c,v 1.8 2022/05/31 11:22:34 andvar Exp $ */

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
__RCSID("$NetBSD: t_spawn.c,v 1.8 2022/05/31 11:22:34 andvar Exp $");

#include <atf-c.h>

#include <sys/fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>

#include <spawn.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <stdarg.h>
#include <fcntl.h>
#include <unistd.h>

#include "fa_spawn_utils.h"


static void check_success(const char *, int, ...);

ATF_TC(t_spawn_ls);

ATF_TC_HEAD(t_spawn_ls, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Tests a simple posix_spawn executing /bin/ls");
}

ATF_TC_BODY(t_spawn_ls, tc)
{
	char * const args[] = { __UNCONST("ls"), __UNCONST("-la"), NULL };
	int err;

	err = posix_spawn(NULL, "/bin/ls", NULL, NULL, args, NULL);
	ATF_REQUIRE(err == 0);
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
	int err;

	err = posix_spawnp(NULL, "ls", NULL, NULL, args, NULL);
	ATF_REQUIRE(err == 0);
}

static void
spawn_error(const atf_tc_t *tc, const char *name, int error)
{
	char buf[FILENAME_MAX];
	char * const args[] = { __UNCONST(name), NULL };
	int err;

	snprintf(buf, sizeof buf, "%s/%s",
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
	    "posix_spawn a non existant binary");
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
	char buf[FILENAME_MAX];
	char rv[2] = { '0', '\0' };
	char * const args0[] = { __UNCONST("h_spawn"), rv, NULL };
	int rets[] = { 0, 1, 7 };
	int err, status;
	pid_t pid;

	snprintf(buf, sizeof buf, "%s/h_spawn",
	    atf_tc_get_config_var(tc, "srcdir"));

	for (size_t i = 0; i < __arraycount(rets); i++) {
		rv[0] = rets[i] + '0';
		err = posix_spawn(&pid, buf, NULL, NULL, args0, NULL);
		ATF_REQUIRE(err == 0);
		ATF_REQUIRE(pid > 0);
		waitpid(pid, &status, 0);
		ATF_REQUIRE(WIFEXITED(status) &&
		    WEXITSTATUS(status) == rets[i]);
	}
}

#define CHDIRPATH "/tmp"
#define FILENAME "output"
#define FILEPATH "/tmp/output"

#define CHDIR 1
#define FCHDIR 2

static void
check_success(const char *file, int argc, ...)
{
	va_list ap;
	ssize_t bytesRead;
	int fd;
	size_t sizeOfFile = (size_t)filesize(file);
	size_t sizeOfStr;
	char *contents;
	const char *dir;

	contents = malloc(sizeOfFile);
	ATF_REQUIRE(contents != NULL);

	/*
	 * for now only 1 variadic argument expected
	 * only from t_spawn_[f]chdir_rel
	 */
	if (argc != 0) {
		va_start(ap, argc);
		dir = va_arg(ap, char *);
		ATF_REQUIRE(dir != NULL);
		va_end(ap);
	} else
		dir = CHDIRPATH;

	fd = open(file, O_RDONLY);
	ATF_REQUIRE_MSG(fd != -1, "Can't open `%s' (%s)", file,
	    strerror(errno));

	/*
	 * file contains form feed i.e ASCII - 10 at the end.
	 * Therefore sizeOfFile - 1
	 */
	sizeOfStr = strlen(dir);
	ATF_CHECK_MSG(sizeOfStr == sizeOfFile - 1, "%zu (%s) != %zu (%s)",
	    sizeOfStr, dir, sizeOfFile - 1, file);

	bytesRead = read(fd, contents, sizeOfFile - 1);
	contents[sizeOfFile - 1] = '\0';
	ATF_REQUIRE_MSG(strcmp(dir, contents) == 0,
	    "[%s] != [%s] Directories dont match", dir, contents);

	fd = close(fd);
	ATF_REQUIRE(fd == 0);

	unlink(file);
	free(contents);

	/* XXX not really required */
	ATF_REQUIRE((size_t)bytesRead == sizeOfStr);
}

static void
spawn_chdir(const char *dirpath, const char *filepath, int operation,
    int expected_error)
{
	int error, fd=-1, status;
	char * const args[2] = { __UNCONST("pwd"), NULL };
	pid_t pid;
	posix_spawnattr_t attr, *attr_p;
	posix_spawn_file_actions_t fa;

	if (filepath)
		empty_outfile(filepath);

	error = posix_spawn_file_actions_init(&fa);
	ATF_REQUIRE(error == 0);

	switch(operation) {
	case CHDIR:
		error = posix_spawn_file_actions_addchdir(&fa, dirpath);
		break;

	case FCHDIR:
		fd = open(dirpath, O_RDONLY);
		ATF_REQUIRE(fd != -1);

		error = posix_spawn_file_actions_addfchdir(&fa, fd);
		break;
	}
	ATF_REQUIRE(error == 0);

	/*
	 * if POSIX_SPAWN_RETURNERROR is expected, then no need to open the
	 * file
	 */
	if (expected_error == 0) {
		error = posix_spawn_file_actions_addopen(&fa, STDOUT_FILENO,
		    FILENAME, O_WRONLY, 0);
		ATF_REQUIRE(error == 0);
		attr_p = NULL;
	} else {
		error = posix_spawnattr_init(&attr);
		ATF_REQUIRE(error == 0);

		/*
		 * POSIX_SPAWN_RETURNERROR is a NetBSD specific flag that
		 * will cause a "proper" return value from posix_spawn(2)
		 * instead of a (potential) success there and a 127 exit
		 * status from the child process (c.f. the non-diag variant
		 * of this test).
		 */
		error = posix_spawnattr_setflags(&attr,
		    POSIX_SPAWN_RETURNERROR);
		ATF_REQUIRE(error == 0);
		attr_p = &attr;
	}

	error = posix_spawn(&pid, "/bin/pwd", &fa, attr_p, args, NULL);
	ATF_REQUIRE(error == expected_error);

	/* wait for the child to finish only when no spawnattr */
	if (attr_p) {
		posix_spawnattr_destroy(&attr);
	} else {
		waitpid(pid, &status, 0);
		ATF_REQUIRE_MSG(WIFEXITED(status) &&
		    WEXITSTATUS(status) == EXIT_SUCCESS,
		    "%s", "[f]chdir failed");
	}

	posix_spawn_file_actions_destroy(&fa);

	/*
	 * The file incase of fchdir(),
	 * should be closed before reopening in 'check_success'
	 */
	if (fd != -1) {
		error = close(fd);
		ATF_REQUIRE(error == 0);
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
	spawn_chdir(CHDIRPATH, FILEPATH, 1, 0);

	/* finally cross check the output of "pwd" directory */
	check_success(FILEPATH, 0);
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
	int error;
	const char *relative_dir = "ch-dir";
	const char *testdir = getcwd(NULL, 0);
	char *chdirwd, *filepath;

	/* cleanup previous */
	error = asprintf(&filepath, "%s/%s", relative_dir, FILENAME);
	ATF_CHECK(error != -1);
	unlink(filepath);
	free(filepath);

	rmdir(relative_dir);
	error = mkdir(relative_dir, 0777);
	ATF_REQUIRE_MSG(error == 0, "mkdir `%s' (%s)", relative_dir,
	    strerror(errno));

	error = asprintf(&chdirwd, "%s/%s", testdir, relative_dir);
	ATF_CHECK(error != -1);

	error = asprintf(&filepath, "%s/%s", chdirwd, FILENAME);
	ATF_CHECK(error != -1);

#ifdef DEBUG
	printf("cwd: %s\n", testdir);
	printf("chdirwd: %s\n", chdirwd);
	printf("filepath: %s\n", filepath);
#endif

	spawn_chdir(relative_dir, filepath, 1, 0);

	/* finally cross check the directory */
	check_success(filepath, 1, chdirwd);
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
	spawn_chdir(FILEPATH, FILEPATH, 1, ENOTDIR);

	unlink(FILEPATH);
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
	int error;
	const char *restrRelDir = "prohibited";

	rmdir(restrRelDir);
	error = mkdir(restrRelDir, 0055);
	ATF_REQUIRE(error == 0);

	spawn_chdir(restrRelDir, NULL, 1, EACCES);
}


ATF_TC(t_spawn_fchdir_abs);

ATF_TC_HEAD(t_spawn_fchdir_abs, tc)
{
	atf_tc_set_md_var(tc, "descr", "Test posix_spawn_fa_fchdir");
	atf_tc_set_md_var(tc, "require.progs", "/bin/pwd");
}

ATF_TC_BODY(t_spawn_fchdir_abs, tc)
{
	spawn_chdir(CHDIRPATH, FILEPATH, 2, 0);

	/* finally cross check the directory */
	check_success(FILEPATH, 0);
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
	int error;
	const char *relative_dir = "ch-dir";
	const char *testdir = getcwd(NULL, 0);
	char *chdirwd, *filepath;

	rmdir(relative_dir);
	error = mkdir(relative_dir, 0755);
	ATF_REQUIRE(error == 0);

	/*
	 * This is done in parts purposely.
	 * It enables the abs path of the relative dir
	 * to be passed to 'check_success()' for comparing
	 */
	error = asprintf(&chdirwd, "%s/%s", testdir, relative_dir);
	ATF_CHECK(error != -1);

	error = asprintf(&filepath, "%s/%s", chdirwd, FILENAME);
	ATF_CHECK(error != -1);

	spawn_chdir(relative_dir, filepath, 2, 0);

	/* finally cross check the directory */
	check_success(filepath, 1, chdirwd);
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
	int error, fd;

	fd = open(FILEPATH, O_WRONLY | O_CREAT | O_TRUNC, 0644);
	ATF_REQUIRE_MSG(fd != -1, "Can't open `%s' (%s)", FILEPATH,
	    strerror(errno));

	error = close(fd);
	ATF_REQUIRE(error == 0);

	spawn_chdir(FILEPATH, NULL, 2, ENOTDIR);

	unlink(FILEPATH);

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

	error = posix_spawn_file_actions_init(&fa);
	ATF_REQUIRE(error == 0);

	error = posix_spawn_file_actions_addfchdir(&fa, fd);
	ATF_REQUIRE(error == EBADF);

	posix_spawn_file_actions_destroy(&fa);
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
	error = posix_spawnattr_init(&attr);
	ATF_CHECK(error == 0);
	/*
	 * POSIX_SPAWN_RETURNERROR is a NetBSD specific flag that
	 * will cause a "proper" return value from posix_spawn(2)
	 * instead of a (potential) success there and a 127 exit
	 * status from the child process (c.f. the non-diag variant
	 * of this test).
	 */
	error = posix_spawnattr_setflags(&attr, POSIX_SPAWN_RETURNERROR);
	ATF_REQUIRE(error == 0);

	error = posix_spawn_file_actions_init(&fa);
	ATF_REQUIRE(error == 0);

	error = posix_spawn_file_actions_addfchdir(&fa, fd);
	ATF_REQUIRE(error == 0);

	error = posix_spawn(&pid, "/bin/pwd", &fa, &attr, args, NULL);
	ATF_REQUIRE(error == EBADF);

	posix_spawn_file_actions_destroy(&fa);
	posix_spawnattr_destroy(&attr);
}

#undef CHDIR
#undef FCHDIR
#undef CHDIRPATH
#undef FILENAME
#undef FILEPATH

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

	return atf_no_error();
}
