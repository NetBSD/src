/* $NetBSD: t_spawn.c,v 1.5 2021/11/15 13:59:16 christos Exp $ */

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
__RCSID("$NetBSD: t_spawn.c,v 1.5 2021/11/15 13:59:16 christos Exp $");

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

ATF_TC(t_spawn_zero);

ATF_TC_HEAD(t_spawn_zero, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "posix_spawn an invalid binary");
}

ATF_TC_BODY(t_spawn_zero, tc)
{
	char buf[FILENAME_MAX];
	char * const args[] = { __UNCONST("h_zero"), NULL };
	int err;

	snprintf(buf, sizeof buf, "%s/h_zero", atf_tc_get_config_var(tc, "srcdir"));
	err = posix_spawn(NULL, buf, NULL, NULL, args, NULL);
	ATF_REQUIRE_MSG(err == ENOEXEC, "expected error %d, got %d when spawning %s", ENOEXEC, err, buf);
}

ATF_TC(t_spawn_missing);

ATF_TC_HEAD(t_spawn_missing, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "posix_spawn a non existant binary");
}

ATF_TC_BODY(t_spawn_missing, tc)
{
	char buf[FILENAME_MAX];
	char * const args[] = { __UNCONST("h_nonexist"), NULL };
	int err;

	snprintf(buf, sizeof buf, "%s/h_nonexist",
	    atf_tc_get_config_var(tc, "srcdir"));
	err = posix_spawn(NULL, buf, NULL, NULL, args, NULL);
	ATF_REQUIRE_MSG(err == ENOENT, "expected error %d, got %d when spawning %s", ENOENT, err, buf);
}

ATF_TC(t_spawn_nonexec);

ATF_TC_HEAD(t_spawn_nonexec, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "posix_spawn a script with non existing interpreter");
}

ATF_TC_BODY(t_spawn_nonexec, tc)
{
	char buf[FILENAME_MAX];
	char * const args[] = { __UNCONST("h_nonexec"), NULL };
	int err;

	snprintf(buf, sizeof buf, "%s/h_nonexec",
	    atf_tc_get_config_var(tc, "srcdir"));
	err = posix_spawn(NULL, buf, NULL, NULL, args, NULL);
	ATF_REQUIRE_MSG(err == ENOENT, "expected error %d, got %d when spawning %s", ENOENT, err, buf);
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
	char * const args0[] = { __UNCONST("h_spawn"), __UNCONST("0"), NULL };
	char * const args1[] = { __UNCONST("h_spawn"), __UNCONST("1"), NULL };
	char * const args7[] = { __UNCONST("h_spawn"), __UNCONST("7"), NULL };
	int err, status;
	pid_t pid;

	snprintf(buf, sizeof buf, "%s/h_spawn",
	    atf_tc_get_config_var(tc, "srcdir"));

	err = posix_spawn(&pid, buf, NULL, NULL, args0, NULL);
	ATF_REQUIRE(err == 0);
	ATF_REQUIRE(pid > 0);
	waitpid(pid, &status, 0);
	ATF_REQUIRE(WIFEXITED(status) && WEXITSTATUS(status) == 0);

	err = posix_spawn(&pid, buf, NULL, NULL, args1, NULL);
	ATF_REQUIRE(err == 0);
	ATF_REQUIRE(pid > 0);
	waitpid(pid, &status, 0);
	ATF_REQUIRE(WIFEXITED(status) && WEXITSTATUS(status) == 1);

	err = posix_spawn(&pid, buf, NULL, NULL, args7, NULL);
	ATF_REQUIRE(err == 0);
	ATF_REQUIRE(pid > 0);
	waitpid(pid, &status, 0);
	ATF_REQUIRE(WIFEXITED(status) && WEXITSTATUS(status) == 7);
}

#define CHDIRPATH "/tmp"
#define FILENAME "output"
#define FILEPATH "/tmp/output"

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
	ATF_REQUIRE_MSG(fd != -1, "Can't open `%s' (%s)", file, strerror(errno));

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

ATF_TC(t_spawn_chdir_abs);

ATF_TC_HEAD(t_spawn_chdir_abs, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Test posix_spawn_fa_addchdir for absolute path");
	atf_tc_set_md_var(tc, "require.progs", "/bin/pwd");
}

ATF_TC_BODY(t_spawn_chdir_abs, tc)
{
	char * const args[2] = { __UNCONST("pwd"), NULL };
	int error, status;
	pid_t pid;
	posix_spawn_file_actions_t fa;

	empty_outfile(FILEPATH);

	error = posix_spawn_file_actions_init(&fa);
	ATF_REQUIRE(error == 0);

	error = posix_spawn_file_actions_addchdir(&fa, CHDIRPATH);
	ATF_REQUIRE(error == 0);

	error = posix_spawn_file_actions_addopen(&fa, STDOUT_FILENO, FILENAME,
			O_WRONLY, 0);
	ATF_REQUIRE(error == 0);

	error = posix_spawn(&pid, "/bin/pwd", &fa, NULL, args, NULL);
	ATF_REQUIRE(error == 0);

	/* wait for the child to finish */
	waitpid(pid, &status, 0);
	ATF_REQUIRE_MSG(WIFEXITED(status) && WEXITSTATUS(status) == EXIT_SUCCESS,
	    "%s", "chdir failed");
	posix_spawn_file_actions_destroy(&fa);

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
	const char *relative_dir = "ch-dir";
	const char *testdir = getcwd(NULL, 0);
	char * const args[2] = { __UNCONST("pwd"), NULL };
	char *chdirwd, *filepath;
	int error, status;
	pid_t pid;
	posix_spawn_file_actions_t fa;

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

	empty_outfile(filepath);

	error = posix_spawn_file_actions_init(&fa);
	ATF_REQUIRE(error == 0);

	error = posix_spawn_file_actions_addchdir(&fa, relative_dir);
	ATF_REQUIRE(error == 0);

	error = posix_spawn_file_actions_addopen(&fa, STDOUT_FILENO, FILENAME,
			O_WRONLY, 0);
	ATF_REQUIRE(error == 0);

	error = posix_spawn(&pid, "/bin/pwd", &fa, NULL, args, NULL);
	ATF_REQUIRE(error == 0);

	/* wait for the child to finish */
	waitpid(pid, &status, 0);
	ATF_REQUIRE_MSG(WIFEXITED(status) && WEXITSTATUS(status) == EXIT_SUCCESS,
	    "%s", "chdir failed");
	posix_spawn_file_actions_destroy(&fa);

	/* finally cross check the directory */
	check_success(filepath, 1, chdirwd);
	free(chdirwd);
	free(filepath);

	rmdir(relative_dir);
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
	int error;
	pid_t pid;
	char * const args[2] = { __UNCONST("pwd"), NULL };
	posix_spawnattr_t attr;
	posix_spawn_file_actions_t fa;

	empty_outfile(FILEPATH);

	error = posix_spawnattr_init(&attr);
	ATF_REQUIRE(error == 0);
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

	error = posix_spawn_file_actions_addchdir(&fa, FILEPATH);
	ATF_REQUIRE(error == 0);
	
	error = posix_spawn(&pid, "/bin/pwd", &fa, &attr, args, NULL);
	ATF_REQUIRE(error == ENOTDIR);

	posix_spawn_file_actions_destroy(&fa);
	posix_spawnattr_destroy(&attr);

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
	int error;
	pid_t pid;
	const char *dirpath = "/not/a/valid/dir";
	char * const args[2] = { __UNCONST("pwd"), NULL };
	posix_spawnattr_t attr;
	posix_spawn_file_actions_t fa;

	error = posix_spawnattr_init(&attr);
	ATF_REQUIRE(error == 0);
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

	error = posix_spawn_file_actions_addchdir(&fa, dirpath);
	ATF_REQUIRE(error == 0);
	
	error = posix_spawn(&pid, "/bin/pwd", &fa, &attr, args, NULL);
	ATF_REQUIRE(error == ENOENT);

	posix_spawn_file_actions_destroy(&fa);
	posix_spawnattr_destroy(&attr);
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
	pid_t pid;
	const char *restrRelDir = "prohibited";
	char * const args[2] = { __UNCONST("pwd"), NULL };
	posix_spawnattr_t attr;
	posix_spawn_file_actions_t fa;

	error = mkdir(restrRelDir, 0055);
	ATF_REQUIRE(error == 0);

	posix_spawnattr_init(&attr);
	ATF_REQUIRE(error == 0);
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

	error = posix_spawn_file_actions_addchdir(&fa, restrRelDir);
	ATF_REQUIRE(error == 0);
	
	error = posix_spawn(&pid, "/bin/pwd", &fa, &attr, args, NULL);
	ATF_CHECK(error == EACCES);

	posix_spawn_file_actions_destroy(&fa);
	posix_spawnattr_destroy(&attr);

	rmdir(restrRelDir);
}

	
ATF_TC(t_spawn_fchdir_abs);

ATF_TC_HEAD(t_spawn_fchdir_abs, tc)
{
	atf_tc_set_md_var(tc, "descr", "Test posix_spawn_fa_fchdir");
	atf_tc_set_md_var(tc, "require.progs", "/bin/pwd");
}

ATF_TC_BODY(t_spawn_fchdir_abs, tc)
{
	int error, fd, status;
	pid_t pid;
	char * const args[2] = { __UNCONST("pwd"), NULL };
	posix_spawn_file_actions_t fa;

	empty_outfile(FILEPATH);

	fd = open(CHDIRPATH, O_RDONLY);
	ATF_REQUIRE(fd != -1);

	error = posix_spawn_file_actions_init(&fa);
	ATF_REQUIRE(error == 0);

	error = posix_spawn_file_actions_addfchdir(&fa, fd);
	ATF_REQUIRE(error == 0);

	error = posix_spawn_file_actions_addopen(&fa, STDOUT_FILENO, FILENAME,
	    O_WRONLY, 0);
	ATF_REQUIRE(error == 0);

	error = posix_spawn(&pid, "/bin/pwd", &fa, NULL, args, NULL);
	ATF_REQUIRE(error == 0);

	/* wait for the child to finish */
	waitpid(pid, &status, 0);
	ATF_REQUIRE_MSG(WIFEXITED(status) && WEXITSTATUS(status) == EXIT_SUCCESS,
	    "%s", "chdir failed");
	posix_spawn_file_actions_destroy(&fa);

	/* The file should be closed before reopening in 'check_success' */
	error = close(fd);
	ATF_REQUIRE(error == 0);

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
	int error, fd, status;
	pid_t pid;
	const char *relative_dir = "ch-dir";
	const char *testdir = getcwd(NULL, 0);
	char * const args[2] = { __UNCONST("pwd"), NULL };
	char *chdirwd, *filepath;
	posix_spawn_file_actions_t fa;

	error = mkdir(relative_dir, 0755);
	ATF_REQUIRE(error == 0);

	/*
	 * This is done in parts purposely.
	 * It enbales the abs path of the relative dir
	 * to be passed to 'check_success()' for comparing
	 */
	error = asprintf(&chdirwd, "%s/%s", testdir, relative_dir);
	ATF_CHECK(error != -1);

	error = asprintf(&filepath, "%s/%s", chdirwd, FILENAME);
	ATF_CHECK(error != -1);

	empty_outfile(filepath);

	fd = open(relative_dir, O_RDONLY);
	ATF_REQUIRE(fd != -1);

	error = posix_spawn_file_actions_init(&fa);
	ATF_REQUIRE(error == 0);

	error = posix_spawn_file_actions_addfchdir(&fa, fd);
	ATF_REQUIRE(error == 0);

	error = posix_spawn_file_actions_addopen(&fa, STDOUT_FILENO, FILENAME,
	    O_WRONLY, 0);
	ATF_REQUIRE(error == 0);

	error = posix_spawn(&pid, "/bin/pwd", &fa, NULL, args, NULL);
	ATF_REQUIRE(error == 0);

	/* wait for the child to finish */
	waitpid(pid, &status, 0);
	ATF_REQUIRE_MSG(WIFEXITED(status) && WEXITSTATUS(status) == EXIT_SUCCESS,
	    "%s", "chdir failed");

	posix_spawn_file_actions_destroy(&fa);

	error = close(fd);
	ATF_REQUIRE(error == 0);

	/* finally cross check the directory */
	check_success(filepath, 1, chdirwd);
	free(chdirwd);
	free(filepath);
	rmdir(relative_dir);
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
	pid_t pid;
	char * const args[2] = { __UNCONST("pwd"), NULL };
	posix_spawnattr_t attr;
	posix_spawn_file_actions_t fa;

	fd = open(FILEPATH, O_WRONLY | O_CREAT | O_TRUNC, 0644);
	ATF_REQUIRE_MSG(fd != -1, "Can't open `%s' (%s)", FILEPATH,
	    strerror(errno));

	error = posix_spawnattr_init(&attr);
	ATF_REQUIRE(error == 0);
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
	ATF_CHECK(error == ENOTDIR);

	posix_spawn_file_actions_destroy(&fa);
	posix_spawnattr_destroy(&attr);

	error = close(fd);
	ATF_REQUIRE(error == 0);

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
