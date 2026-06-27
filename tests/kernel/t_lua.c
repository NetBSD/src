/*	$NetBSD: t_lua.c,v 1.1 2026/06/27 20:05:06 riastradh Exp $	*/

/*-
 * Copyright (c) 2026 The NetBSD Foundation, Inc.
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
__RCSID("$NetBSD: t_lua.c,v 1.1 2026/06/27 20:05:06 riastradh Exp $");

/*
 * Test lua(4)
 *
 * Minimal tests to verify access control -- if we want to use lua(4)
 * for anything else, this needs more tests.
 */

#include <sys/lua.h>
#include <sys/ioctl.h>
#include <sys/module.h>
#include <sys/stat.h>
#include <sys/wait.h>

#include <atf-c.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "h_macros.h"

#define	_PATH_DEV_LUA	"/dev/lua"
#define	TEST_STATE	"t_lua"

static void
require_lua(void)
{
	struct iovec iov;
	const modstat_t *mp;
	size_t nbytes = 8192;
	const int *countp;
	unsigned i, count;

	for (;;) {
		memset(&iov, 0, sizeof(iov));
		REQUIRE_LIBC(iov.iov_base = malloc(nbytes), NULL);
		iov.iov_len = nbytes;
		RL(modctl(MODCTL_STAT, &iov));
		if (iov.iov_len <= nbytes)
			break;
		free(iov.iov_base);
		nbytes = iov.iov_len;
	}
	countp = (const int *)iov.iov_base;
	ATF_REQUIRE_MSG(*countp >= 0, "count=%d", *countp);
	count = (unsigned)*countp;
	ATF_REQUIRE_MSG(count <= iov.iov_len/sizeof(*mp),
	    "count=%u iov_len=%zu max=%zu",
	    count, iov.iov_len, iov.iov_len/sizeof(*mp));
	mp = (const void *)(countp + 1);
	for (i = 0; i < count; i++) {
		modstat_t m;

		memcpy(&m, &mp[i], sizeof(m));
		if (strcmp(m.ms_name, "lua") == 0)
			goto out;
	}
	atf_tc_skip("lua.kmod not loaded");

out:	free(iov.iov_base);
}

static int lua_fd = -1;

ATF_TC_WITH_CLEANUP(access);
ATF_TC_HEAD(access, tc)
{
	atf_tc_set_md_var(tc, "descr", "Verifies /dev/lua access control");
	atf_tc_set_md_var(tc, "require.user", "root");
}
ATF_TC_BODY(access, tc)
{
	int fd;
	char cwd[PATH_MAX], path[PATH_MAX];
	struct lua_create cr;
	struct lua_load ld;
	struct lua_require rq;
	struct lua_info info;

	require_lua();

	REQUIRE_LIBC(getcwd(cwd, sizeof(cwd)), NULL);
	if ((size_t)snprintf(path, sizeof(path), "%s/test.lua", cwd) >=
	    sizeof(path))
		atf_tc_fail("overlong working directory");
	RL(fd = open(path, O_WRONLY|O_CREAT));
	RL(close(fd));

	/*
	 * Verify that if we're read-only, we can query info but not
	 * create states.
	 */
	RL(fd = open(_PATH_DEV_LUA, O_RDONLY));
	RL(ioctl(fd, LUAINFO, &info));
	ATF_REQUIRE_ERRNO(EACCES, ioctl(fd, LUACREATE, &cr) == -1);
	ATF_REQUIRE_ERRNO(EACCES, ioctl(fd, LUALOAD, &ld) == -1);
	ATF_REQUIRE_ERRNO(EACCES, ioctl(fd, LUAREQUIRE, &rq) == -1);
	ATF_REQUIRE_ERRNO(EACCES, ioctl(fd, LUADESTROY, &cr) == -1);
	RL(close(fd));

	/*
	 * Verify that if we're write-only, we can create states and
	 * load things but not query them.
	 */
	RL(fd = open(_PATH_DEV_LUA, O_WRONLY));

	ATF_REQUIRE_ERRNO(EACCES, ioctl(fd, LUAINFO, &info) == -1);

	memset(&cr, 0, sizeof(cr));
	strlcpy(cr.name, "_invalid", sizeof(cr.name));
	ATF_REQUIRE_ERRNO(ENXIO, ioctl(fd, LUACREATE, &cr) == -1);

	memset(&cr, 0, sizeof(cr));
	strlcpy(cr.name, TEST_STATE, sizeof(cr.name));
	RL(ioctl(fd, LUACREATE, &cr));

	memset(&cr, 0, sizeof(cr));
	strlcpy(cr.name, TEST_STATE, sizeof(cr.name));
	ATF_REQUIRE_ERRNO(EBUSY, ioctl(fd, LUACREATE, &cr) == -1);

	lua_fd = fd;		/* for cleanup */

	memset(&ld, 0, sizeof(ld));
	strlcpy(ld.state, TEST_STATE, sizeof(ld.state));
	strlcpy(ld.path, path, sizeof(ld.path));
	RL(ioctl(fd, LUALOAD, &ld));

	/* XXX LUAREQUIRE */

	RL(ioctl(fd, LUADESTROY, &cr));

	RL(close(fd));
}
ATF_TC_CLEANUP(access, tc)
{
	struct lua_create cr;

	memset(&cr, 0, sizeof(cr));
	strlcpy(cr.name, TEST_STATE, sizeof(cr.name));
	(void)ioctl(lua_fd, LUADESTROY, &cr);
}

ATF_TC(perms);
ATF_TC_HEAD(perms, tc)
{
	atf_tc_set_md_var(tc, "descr", "Verifies permissions on /dev/lua");
	atf_tc_set_md_var(tc, "require.user", "root");
}
ATF_TC_BODY(perms, tc)
{
	struct stat st;
	struct passwd *pw;
	pid_t child;
	int status;

	RL(stat(_PATH_DEV_LUA, &st));
	ATF_REQUIRE_MSG(st.st_uid == 0,
	    "/dev/lua should be root-owned: uid=%d", (int)st.st_uid);
	ATF_REQUIRE_MSG((st.st_mode & S_IWOTH) == 0,
	    "/dev/lua should not be world-writable: mode=%04o", st.st_mode);

	REQUIRE_LIBC(pw = getpwnam("nobody"), NULL);

	RL(child = fork());
	if (child == 0) {
		if (setgroups(0, NULL) == -1)
			err(EXIT_FAILURE, "setgroups");
		if (setregid(pw->pw_gid, pw->pw_gid))
			err(EXIT_FAILURE, "setregid");
		if (setreuid(pw->pw_uid, pw->pw_uid))
			err(EXIT_FAILURE, "setreuid");
		if (open(_PATH_DEV_LUA, O_WRONLY) != -1) {
			errx(EXIT_FAILURE,
			    "open /dev/lua unexpectedly succeeded");
		}
		if (errno != EACCES) {
			err(EXIT_FAILURE,
			    "open /dev/lua didn't fail with EACCES");
		}
		_exit(0);
	}
	RL(waitpid(child, &status, 0));
	ATF_REQUIRE_MSG(!WIFSIGNALED(status),
	    "child terminated on signal %d", WTERMSIG(status));
	ATF_REQUIRE_MSG(WIFEXITED(status),
	    "child exited unexpectedly, status=0x%x", status);
	ATF_REQUIRE_MSG(WEXITSTATUS(status) == 0,
	    "child exited with code %d", WEXITSTATUS(status));
}

ATF_TP_ADD_TCS(tp)
{

	ATF_TP_ADD_TC(tp, access);
	ATF_TP_ADD_TC(tp, perms);

	return atf_no_error();
}
