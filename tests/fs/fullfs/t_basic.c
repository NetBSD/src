/*	$NetBSD$	*/

#include <sys/types.h>
#include <sys/mount.h>

#include <atf-c.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#include <rump/rump.h>
#include <rump/rump_syscalls.h>

#include <miscfs/fullfs/full.h>
#include <fs/tmpfs/tmpfs_args.h>

#include "h_macros.h"

#define MSTR "magic bus"

static void
xput_tfile(const char *path, const char *mstr, size_t len)
{
	int fd;

	RL(fd = rump_sys_open(path, O_CREAT | O_RDWR, 0777));
	ATF_REQUIRE_EQ_MSG((ssize_t)len, rump_sys_write(fd, mstr, len),
	    "write to testfile: %s", strerror(errno));
	RL(rump_sys_close(fd));
}

static int
xread_tfile(const char *path, const char *mstr, size_t len)
{
	char buf[128];
	int fd;

	fd = rump_sys_open(path, O_RDONLY);
	if (fd == -1)
		return errno;
	RL(rump_sys_read(fd, buf, sizeof(buf)));
	RL(rump_sys_close(fd));
	if (strncmp(buf, mstr, len) == 0)
		return 0;
	return EPROGMISMATCH;
}

static void
mountfull(const char *what, const char *mp, int flags)
{
	struct full_args fargs;

	memset(&fargs, 0, sizeof(fargs));
	fargs.fulla_target = __UNCONST(what);
	RL(rump_sys_mount(MOUNT_FULL, mp, flags, &fargs, sizeof(fargs)));
}

static void
mounttmpfs(const char *mp)
{
	struct tmpfs_args targs;

	memset(&targs, 0, sizeof(targs));
	targs.ta_version = TMPFS_ARGS_VERSION;
	targs.ta_root_mode = 0777;
	RL(rump_sys_mount(MOUNT_TMPFS, mp, 0, &targs, sizeof(targs)));
}

static void
setup_fullfs(void)
{
	rump_init();
	RL(rump_sys_mkdir("/td1", 0777));
	RL(rump_sys_mkdir("/td2", 0777));
	mounttmpfs("/td1");
	mountfull("/td1", "/td2", 0);
}


ATF_TC(read_succeeds);
ATF_TC_HEAD(read_succeeds, tc)
{
	atf_tc_set_md_var(tc, "descr", "read through fullfs succeeds");
}
ATF_TC_BODY(read_succeeds, tc)
{
	setup_fullfs();

	xput_tfile("/td1/testfile", MSTR, sizeof(MSTR));

	ATF_REQUIRE_EQ(0, xread_tfile("/td2/testfile", MSTR, sizeof(MSTR)));
}


ATF_TC(write_fails);
ATF_TC_HEAD(write_fails, tc)
{
	atf_tc_set_md_var(tc, "descr", "write through fullfs fails with ENOSPC");
}
ATF_TC_BODY(write_fails, tc)
{
	int fd;

	setup_fullfs();

	xput_tfile("/td1/testfile", MSTR, sizeof(MSTR));

	RL(fd = rump_sys_open("/td2/testfile", O_WRONLY));
	ATF_REQUIRE_EQ(-1, rump_sys_write(fd, MSTR, sizeof(MSTR)));
	ATF_REQUIRE_EQ(ENOSPC, errno);
}


ATF_TC(create_fails);
ATF_TC_HEAD(create_fails, tc)
{
	atf_tc_set_md_var(tc, "descr", "create through fullfs fails with ENOSPC");
}
ATF_TC_BODY(create_fails, tc)
{
	setup_fullfs();

	ATF_REQUIRE_EQ(-1, rump_sys_open("/td2/testfile", O_CREAT | O_WRONLY, 0600));
	ATF_REQUIRE_EQ(ENOSPC, errno);
}


ATF_TC(mkdir_fails);
ATF_TC_HEAD(mkdir_fails, tc)
{
	atf_tc_set_md_var(tc, "descr", "mkdir through fullfs fails with ENOSPC");
}
ATF_TC_BODY(mkdir_fails, tc)
{
	setup_fullfs();

	ATF_REQUIRE_EQ(-1, rump_sys_mkdir("/td2/testdir", 0777));
	ATF_REQUIRE_EQ(ENOSPC, errno);
}


ATF_TC(symlink_fails);
ATF_TC_HEAD(symlink_fails, tc)
{
	atf_tc_set_md_var(tc, "descr", "symlink through fullfs fails with ENOSPC");
}
ATF_TC_BODY(symlink_fails, tc)
{
	setup_fullfs();

	ATF_REQUIRE_EQ(-1, rump_sys_symlink("anything", "/td2/link"));
	ATF_REQUIRE_EQ(ENOSPC, errno);
}


ATF_TC(remove_succeeds);
ATF_TC_HEAD(remove_succeeds, tc)
{
	atf_tc_set_md_var(tc, "descr", "remove through fullfs succeeds");
}
ATF_TC_BODY(remove_succeeds, tc)
{
	setup_fullfs();

	xput_tfile("/td1/testfile", MSTR, sizeof(MSTR));
	ATF_REQUIRE_EQ(0, rump_sys_unlink("/td2/testfile"));

	ATF_REQUIRE_EQ(ENOENT, xread_tfile("/td1/testfile", NULL, 0));
}


ATF_TC(statvfs_shows_full);
ATF_TC_HEAD(statvfs_shows_full, tc)
{
	atf_tc_set_md_var(tc, "descr", "statvfs through fullfs shows full");
}
ATF_TC_BODY(statvfs_shows_full, tc)
{
	struct statvfs sb;

	setup_fullfs();

	RL(rump_sys_statvfs1("/td2", &sb, ST_WAIT));
	ATF_REQUIRE_EQ(0, sb.f_bfree);
	ATF_REQUIRE_EQ(0, sb.f_bavail);
}


ATF_TP_ADD_TCS(tp)
{
	ATF_TP_ADD_TC(tp, read_succeeds);
	ATF_TP_ADD_TC(tp, write_fails);
	ATF_TP_ADD_TC(tp, create_fails);
	ATF_TP_ADD_TC(tp, mkdir_fails);
	ATF_TP_ADD_TC(tp, symlink_fails);
	ATF_TP_ADD_TC(tp, remove_succeeds);
	ATF_TP_ADD_TC(tp, statvfs_shows_full);
	return atf_no_error();
}
