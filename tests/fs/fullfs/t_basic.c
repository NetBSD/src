/*	$NetBSD$	*/

#include <sys/types.h>
#include <sys/mount.h>
#include <sys/ioctl.h>
#include <sys/stat.h>

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
#include <miscfs/fullfs/full_ioctl.h>
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
fullfs_setstate(int mode, int opmask, int error, unsigned int rate, unsigned int doom)
{
	struct fullfs_ctl fc;
	int fd;

	memset(&fc, 0, sizeof(fc));
	fc.fc_mode = mode;
	fc.fc_opmask = opmask;
	fc.fc_error = error;
	fc.fc_rate = rate;
	fc.fc_doom = doom;

	RL(fd = rump_sys_open("/td2", O_RDONLY));
	RL(rump_sys_ioctl(fd, FULLFS_SETSTATE, &fc));
	RL(rump_sys_close(fd));
}

static void
fullfs_getstate(struct fullfs_ctl *fc)
{
	int fd;

	RL(fd = rump_sys_open("/td2", O_RDONLY));
	RL(rump_sys_ioctl(fd, FULLFS_GETSTATE, fc));
	RL(rump_sys_close(fd));
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


ATF_TC(mknod_fails);
ATF_TC_HEAD(mknod_fails, tc)
{
	atf_tc_set_md_var(tc, "descr", "mknod through fullfs fails with ENOSPC");
}
ATF_TC_BODY(mknod_fails, tc)
{
	setup_fullfs();

	ATF_REQUIRE_EQ(-1, rump_sys_mknod("/td2/fifo", S_IFIFO | 0600, 0));
	ATF_REQUIRE_EQ(ENOSPC, errno);
}


ATF_TC(link_fails);
ATF_TC_HEAD(link_fails, tc)
{
	atf_tc_set_md_var(tc, "descr", "link through fullfs fails with ENOSPC");
}
ATF_TC_BODY(link_fails, tc)
{
	setup_fullfs();

	xput_tfile("/td1/testfile", MSTR, sizeof(MSTR));

	ATF_REQUIRE_EQ(-1, rump_sys_link("/td2/testfile", "/td2/testlink"));
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


ATF_TC(rename_succeeds);
ATF_TC_HEAD(rename_succeeds, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "rename through fullfs succeeds in fail mode (atomic save)");
}
ATF_TC_BODY(rename_succeeds, tc)
{
	setup_fullfs();

	xput_tfile("/td1/testfile", MSTR, sizeof(MSTR));

	ATF_REQUIRE_EQ(0, rump_sys_rename("/td2/testfile", "/td2/renamed"));
	ATF_REQUIRE_EQ(0, xread_tfile("/td2/renamed", MSTR, sizeof(MSTR)));
}


ATF_TC(rmdir_succeeds);
ATF_TC_HEAD(rmdir_succeeds, tc)
{
	atf_tc_set_md_var(tc, "descr", "rmdir through fullfs succeeds");
}
ATF_TC_BODY(rmdir_succeeds, tc)
{
	setup_fullfs();

	RL(rump_sys_mkdir("/td1/testdir", 0777));
	ATF_REQUIRE_EQ(0, rump_sys_rmdir("/td2/testdir"));
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


ATF_TC(getstate_initial);
ATF_TC_HEAD(getstate_initial, tc)
{
	atf_tc_set_md_var(tc, "descr", "GETSTATE returns default mount state");
}
ATF_TC_BODY(getstate_initial, tc)
{
	struct fullfs_ctl fc;

	setup_fullfs();
	fullfs_getstate(&fc);

	ATF_REQUIRE_EQ(FULLFS_MODE_FAIL, fc.fc_mode);
	ATF_REQUIRE_EQ(FULLFS_OP_ALL, fc.fc_opmask);
	ATF_REQUIRE_EQ(ENOSPC, fc.fc_error);
	ATF_REQUIRE_EQ(0, fc.fc_rate);
	ATF_REQUIRE_EQ(0, fc.fc_doom);
}


ATF_TC(toggle_pass_fail);
ATF_TC_HEAD(toggle_pass_fail, tc)
{
	atf_tc_set_md_var(tc, "descr", "toggling between PASS and FAIL modes");
}
ATF_TC_BODY(toggle_pass_fail, tc)
{
	int fd;

	setup_fullfs();

	xput_tfile("/td1/testfile", MSTR, sizeof(MSTR));

	RL(fd = rump_sys_open("/td2/testfile", O_WRONLY));
	ATF_REQUIRE_EQ(-1, rump_sys_write(fd, MSTR, sizeof(MSTR)));
	ATF_REQUIRE_EQ(ENOSPC, errno);
	RL(rump_sys_close(fd));

	fullfs_setstate(FULLFS_MODE_PASS, FULLFS_OP_ALL, ENOSPC, 0, 0);
	RL(fd = rump_sys_open("/td2/testfile", O_WRONLY));
	ATF_REQUIRE_EQ((ssize_t)sizeof(MSTR), rump_sys_write(fd, MSTR, sizeof(MSTR)));
	RL(rump_sys_close(fd));

	fullfs_setstate(FULLFS_MODE_FAIL, FULLFS_OP_ALL, ENOSPC, 0, 0);
	RL(fd = rump_sys_open("/td2/testfile", O_WRONLY));
	ATF_REQUIRE_EQ(-1, rump_sys_write(fd, MSTR, sizeof(MSTR)));
	ATF_REQUIRE_EQ(ENOSPC, errno);
	RL(rump_sys_close(fd));
}

ATF_TC(error_code);
ATF_TC_HEAD(error_code, tc)
{
	atf_tc_set_md_var(tc, "descr", "configurable error code is honored");
}
ATF_TC_BODY(error_code, tc)
{
	int fd;

	setup_fullfs();
	xput_tfile("/td1/testfile", MSTR, sizeof(MSTR));

	fullfs_setstate(FULLFS_MODE_FAIL, FULLFS_OP_ALL, EIO, 0, 0);
	RL(fd = rump_sys_open("/td2/testfile", O_WRONLY));
	ATF_REQUIRE_EQ(-1, rump_sys_write(fd, MSTR, sizeof(MSTR)));
	ATF_REQUIRE_EQ(EIO, errno);
	RL(rump_sys_close(fd));

	fullfs_setstate(FULLFS_MODE_FAIL, FULLFS_OP_ALL, EDQUOT, 0, 0);
	RL(fd = rump_sys_open("/td2/testfile", O_WRONLY));
	ATF_REQUIRE_EQ(-1, rump_sys_write(fd, MSTR, sizeof(MSTR)));
	ATF_REQUIRE_EQ(EDQUOT, errno);
	RL(rump_sys_close(fd));
}


ATF_TC(error_roundtrip);
ATF_TC_HEAD(error_roundtrip, tc)
{
	atf_tc_set_md_var(tc, "descr", "error code survives SETSTATE/GETSTATE");
}
ATF_TC_BODY(error_roundtrip, tc)
{
	struct fullfs_ctl fc;

	setup_fullfs();

	fullfs_setstate(FULLFS_MODE_FAIL, FULLFS_OP_ALL, EFBIG, 0, 0);
	fullfs_getstate(&fc);
	ATF_REQUIRE_EQ(EFBIG, fc.fc_error);
}


ATF_TC(opmask_write_only);
ATF_TC_HEAD(opmask_write_only, tc)
{
	atf_tc_set_md_var(tc, "descr", "opmask WRITE blocks writes but not creates");
}
ATF_TC_BODY(opmask_write_only, tc)
{
	int fd;

	setup_fullfs();
	xput_tfile("/td1/testfile", MSTR, sizeof(MSTR));

	fullfs_setstate(FULLFS_MODE_FAIL, FULLFS_OP_WRITE, ENOSPC, 0, 0);

	RL(fd = rump_sys_open("/td2/testfile", O_WRONLY));
	ATF_REQUIRE_EQ(-1, rump_sys_write(fd, MSTR, sizeof(MSTR)));
	ATF_REQUIRE_EQ(ENOSPC, errno);
	RL(rump_sys_close(fd));

	RL(fd = rump_sys_open("/td2/newfile", O_CREAT | O_WRONLY, 0600));
	RL(rump_sys_close(fd));
}


ATF_TC(opmask_create_only);
ATF_TC_HEAD(opmask_create_only, tc)
{
	atf_tc_set_md_var(tc, "descr", "opmask CREATE blocks creates but not writes");
}
ATF_TC_BODY(opmask_create_only, tc)
{
	int fd;

	setup_fullfs();
	xput_tfile("/td1/testfile", MSTR, sizeof(MSTR));

	fullfs_setstate(FULLFS_MODE_FAIL, FULLFS_OP_CREATE, ENOSPC, 0, 0);

	ATF_REQUIRE_EQ(-1, rump_sys_open("/td2/newfile", O_CREAT | O_WRONLY, 0600));
	ATF_REQUIRE_EQ(ENOSPC, errno);

	RL(fd = rump_sys_open("/td2/testfile", O_WRONLY));
	ATF_REQUIRE_EQ((ssize_t)sizeof(MSTR), rump_sys_write(fd, MSTR, sizeof(MSTR)));
	RL(rump_sys_close(fd));
}


ATF_TC(opmask_none);
ATF_TC_HEAD(opmask_none, tc)
{
	atf_tc_set_md_var(tc, "descr", "opmask NONE lets all operations through");
}
ATF_TC_BODY(opmask_none, tc)
{
	int fd;

	setup_fullfs();

	fullfs_setstate(FULLFS_MODE_FAIL, FULLFS_OP_NONE, ENOSPC, 0, 0);

	RL(fd = rump_sys_open("/td2/newfile", O_CREAT | O_WRONLY, 0600));
	ATF_REQUIRE_EQ((ssize_t)sizeof(MSTR), rump_sys_write(fd, MSTR, sizeof(MSTR)));
	RL(rump_sys_close(fd));
}


ATF_TC(opmask_multiple);
ATF_TC_HEAD(opmask_multiple, tc)
{
	atf_tc_set_md_var(tc, "descr", "opmask with multiple bits set");
}
ATF_TC_BODY(opmask_multiple, tc)
{
	int fd;

	setup_fullfs();
	xput_tfile("/td1/testfile", MSTR, sizeof(MSTR));

	fullfs_setstate(FULLFS_MODE_FAIL, FULLFS_OP_WRITE | FULLFS_OP_MKDIR, ENOSPC, 0, 0);

	RL(fd = rump_sys_open("/td2/testfile", O_WRONLY));
	ATF_REQUIRE_EQ(-1, rump_sys_write(fd, MSTR, sizeof(MSTR)));
	ATF_REQUIRE_EQ(ENOSPC, errno);
	RL(rump_sys_close(fd));

	ATF_REQUIRE_EQ(-1, rump_sys_mkdir("/td2/subdir", 0777));
	ATF_REQUIRE_EQ(ENOSPC, errno);

	RL(fd = rump_sys_open("/td2/newfile", O_CREAT | O_WRONLY, 0600));
	RL(rump_sys_close(fd));
}


ATF_TC(doom_counter_basic);
ATF_TC_HEAD(doom_counter_basic, tc)
{
	atf_tc_set_md_var(tc, "descr", "COUNT mode allows N ops then fails");
}
ATF_TC_BODY(doom_counter_basic, tc)
{
	struct fullfs_ctl fc;
	int fd, i;

	setup_fullfs();
	xput_tfile("/td1/testfile", MSTR, sizeof(MSTR));

	fullfs_setstate(FULLFS_MODE_COUNT, FULLFS_OP_ALL, ENOSPC, 0, 3);

	for (i = 0; i < 3; i++) {
		RL(fd = rump_sys_open("/td2/testfile", O_WRONLY));
		ATF_REQUIRE_EQ((ssize_t)sizeof(MSTR), rump_sys_write(fd, MSTR, sizeof(MSTR)));
		RL(rump_sys_close(fd));
	}

	RL(fd = rump_sys_open("/td2/testfile", O_WRONLY));
	ATF_REQUIRE_EQ(-1, rump_sys_write(fd, MSTR, sizeof(MSTR)));
	ATF_REQUIRE_EQ(ENOSPC, errno);
	RL(rump_sys_close(fd));

	fullfs_getstate(&fc);
	ATF_REQUIRE_EQ(0, fc.fc_doom);
}


ATF_TC(doom_counter_opmask);
ATF_TC_HEAD(doom_counter_opmask, tc)
{
	atf_tc_set_md_var(tc, "descr", "COUNT mode only decrements for masked ops");
}
ATF_TC_BODY(doom_counter_opmask, tc)
{
	int fd;

	setup_fullfs();
	xput_tfile("/td1/testfile", MSTR, sizeof(MSTR));

	fullfs_setstate(FULLFS_MODE_COUNT, FULLFS_OP_WRITE | FULLFS_OP_CREATE, ENOSPC, 0, 2);

	RL(rump_sys_mkdir("/td2/subdir", 0777));

	RL(fd = rump_sys_open("/td2/testfile", O_WRONLY));
	ATF_REQUIRE_EQ((ssize_t)sizeof(MSTR), rump_sys_write(fd, MSTR, sizeof(MSTR)));
	RL(rump_sys_close(fd));

	RL(fd = rump_sys_open("/td2/newfile", O_CREAT | O_WRONLY, 0600));
	RL(rump_sys_close(fd));

	RL(fd = rump_sys_open("/td2/testfile", O_WRONLY));
	ATF_REQUIRE_EQ(-1, rump_sys_write(fd, MSTR, sizeof(MSTR)));
	ATF_REQUIRE_EQ(ENOSPC, errno);
	RL(rump_sys_close(fd));
}


ATF_TC(doom_counter_midstream);
ATF_TC_HEAD(doom_counter_midstream, tc)
{
	atf_tc_set_md_var(tc, "descr", "COUNT doom=1 simulates disk filling mid-save");
}
ATF_TC_BODY(doom_counter_midstream, tc)
{
	int fd;

	setup_fullfs();
	xput_tfile("/td1/testfile", MSTR, sizeof(MSTR));

	fullfs_setstate(FULLFS_MODE_COUNT, FULLFS_OP_ALL, ENOSPC, 0, 1);

	RL(fd = rump_sys_open("/td2/testfile", O_WRONLY));
	ATF_REQUIRE_EQ((ssize_t)sizeof(MSTR), rump_sys_write(fd, MSTR, sizeof(MSTR)));
	ATF_REQUIRE_EQ(-1, rump_sys_write(fd, MSTR, sizeof(MSTR)));
	ATF_REQUIRE_EQ(ENOSPC, errno);
	RL(rump_sys_close(fd));
}


ATF_TC(byte_counter_basic);
ATF_TC_HEAD(byte_counter_basic, tc)
{
	atf_tc_set_md_var(tc, "descr", "BYTES mode allows N bytes then fails");
}
ATF_TC_BODY(byte_counter_basic, tc)
{
	char buf[10];
	int fd;

	setup_fullfs();
	xput_tfile("/td1/testfile", MSTR, sizeof(MSTR));

	fullfs_setstate(FULLFS_MODE_BYTES, FULLFS_OP_ALL, ENOSPC, 0, 20);

	RL(fd = rump_sys_open("/td2/testfile", O_WRONLY));

	memset(buf, 'A', sizeof(buf));
	ATF_REQUIRE_EQ(10, rump_sys_write(fd, buf, 10));
	ATF_REQUIRE_EQ(10, rump_sys_write(fd, buf, 10));
	ATF_REQUIRE_EQ(-1, rump_sys_write(fd, buf, 1));
	ATF_REQUIRE_EQ(ENOSPC, errno);

	RL(rump_sys_close(fd));
}


ATF_TC(byte_counter_oversize);
ATF_TC_HEAD(byte_counter_oversize, tc)
{
	atf_tc_set_md_var(tc, "descr", "BYTES rejects oversized write without consuming budget");
}
ATF_TC_BODY(byte_counter_oversize, tc)
{
	struct fullfs_ctl fc;
	char buf[10];
	int fd;

	setup_fullfs();
	xput_tfile("/td1/testfile", MSTR, sizeof(MSTR));

	fullfs_setstate(FULLFS_MODE_BYTES, FULLFS_OP_ALL, ENOSPC, 0, 5);

	RL(fd = rump_sys_open("/td2/testfile", O_WRONLY));
	memset(buf, 'A', sizeof(buf));
	ATF_REQUIRE_EQ(-1, rump_sys_write(fd, buf, 10));
	ATF_REQUIRE_EQ(ENOSPC, errno);
	RL(rump_sys_close(fd));

	fullfs_getstate(&fc);
	ATF_REQUIRE_EQ(5, fc.fc_doom);
}


ATF_TC(byte_counter_nonwrite);
ATF_TC_HEAD(byte_counter_nonwrite, tc)
{
	atf_tc_set_md_var(tc, "descr", "BYTES mode does not consume budget for non-write ops");
}
ATF_TC_BODY(byte_counter_nonwrite, tc)
{
	struct fullfs_ctl fc;
	int fd;

	setup_fullfs();

	fullfs_setstate(FULLFS_MODE_BYTES, FULLFS_OP_ALL, ENOSPC, 0, 1);

	RL(fd = rump_sys_open("/td2/newfile", O_CREAT | O_WRONLY, 0600));
	RL(rump_sys_close(fd));

	RL(rump_sys_mkdir("/td2/subdir", 0777));

	fullfs_getstate(&fc);
	ATF_REQUIRE_EQ(1, fc.fc_doom);
}


ATF_TC(random_basic);
ATF_TC_HEAD(random_basic, tc)
{
	atf_tc_set_md_var(tc, "descr", "RANDOM mode fails at approximately the configured rate");
}
ATF_TC_BODY(random_basic, tc)
{
	int fd, i, ok;

	setup_fullfs();
	xput_tfile("/td1/testfile", MSTR, sizeof(MSTR));

	fullfs_setstate(FULLFS_MODE_RANDOM, FULLFS_OP_WRITE, ENOSPC, 50, 0);

	ok = 0;
	for (i = 0; i < 1000; i++) {
		RL(fd = rump_sys_open("/td2/testfile", O_WRONLY));
		if (rump_sys_write(fd, MSTR, sizeof(MSTR)) != -1)
			ok++;
		RL(rump_sys_close(fd));
	}

	ATF_REQUIRE_MSG(ok >= 400 && ok <= 600, "expected 400-600 successes, got %d", ok);
}


ATF_TC(random_error_code);
ATF_TC_HEAD(random_error_code, tc)
{
	atf_tc_set_md_var(tc, "descr", "RANDOM mode uses configured error code");
}
ATF_TC_BODY(random_error_code, tc)
{
	int fd, i;

	setup_fullfs();
	xput_tfile("/td1/testfile", MSTR, sizeof(MSTR));

	fullfs_setstate(FULLFS_MODE_RANDOM, FULLFS_OP_WRITE, EIO, 99, 0);

	for (i = 0; i < 1000; i++) {
		RL(fd = rump_sys_open("/td2/testfile", O_WRONLY));
		if (rump_sys_write(fd, MSTR, sizeof(MSTR)) == -1) {
			ATF_REQUIRE_EQ(EIO, errno);
			RL(rump_sys_close(fd));
			return;
		}
		RL(rump_sys_close(fd));
	}
	atf_tc_fail("no failure in 1000 writes at 99%% rate");
}


ATF_TC(setstate_invalid);
ATF_TC_HEAD(setstate_invalid, tc)
{
	atf_tc_set_md_var(tc, "descr", "SETSTATE rejects invalid parameters");
}
ATF_TC_BODY(setstate_invalid, tc)
{
	struct fullfs_ctl fc;
	int fd;

	setup_fullfs();

	RL(fd = rump_sys_open("/td2", O_RDONLY));

#define	EXPECT_EINVAL() do {						\
	ATF_REQUIRE_EQ(-1, rump_sys_ioctl(fd, FULLFS_SETSTATE, &fc));	\
	ATF_REQUIRE_EQ(EINVAL, errno);					\
} while (0)

	memset(&fc, 0, sizeof(fc));
	fc.fc_mode = FULLFS_MODE_RANDOM + 1;
	fc.fc_error = ENOSPC;
	fc.fc_opmask = FULLFS_OP_ALL;
	EXPECT_EINVAL();

	memset(&fc, 0, sizeof(fc));
	fc.fc_mode = FULLFS_MODE_FAIL;
	fc.fc_error = 0;
	fc.fc_opmask = FULLFS_OP_ALL;
	EXPECT_EINVAL();

	memset(&fc, 0, sizeof(fc));
	fc.fc_mode = FULLFS_MODE_FAIL;
	fc.fc_error = -1;
	fc.fc_opmask = FULLFS_OP_ALL;
	EXPECT_EINVAL();

	memset(&fc, 0, sizeof(fc));
	fc.fc_mode = FULLFS_MODE_FAIL;
	fc.fc_error = ENOSPC;
	fc.fc_opmask = FULLFS_OP_ALL + 1;
	EXPECT_EINVAL();

	memset(&fc, 0, sizeof(fc));
	fc.fc_mode = FULLFS_MODE_COUNT;
	fc.fc_error = ENOSPC;
	fc.fc_opmask = FULLFS_OP_ALL;
	fc.fc_doom = 0;
	EXPECT_EINVAL();

	memset(&fc, 0, sizeof(fc));
	fc.fc_mode = FULLFS_MODE_RANDOM;
	fc.fc_error = ENOSPC;
	fc.fc_opmask = FULLFS_OP_ALL;
	fc.fc_rate = 0;
	EXPECT_EINVAL();

	memset(&fc, 0, sizeof(fc));
	fc.fc_mode = FULLFS_MODE_RANDOM;
	fc.fc_error = ENOSPC;
	fc.fc_opmask = FULLFS_OP_ALL;
	fc.fc_rate = 100;
	EXPECT_EINVAL();

#undef EXPECT_EINVAL

	RL(rump_sys_close(fd));
}


ATF_TC(ioctl_on_regular_file);
ATF_TC_HEAD(ioctl_on_regular_file, tc)
{
	atf_tc_set_md_var(tc, "descr", "ioctl works on regular files, not just mountpoint");
}
ATF_TC_BODY(ioctl_on_regular_file, tc)
{
	struct fullfs_ctl fc;
	int fd;

	setup_fullfs();
	xput_tfile("/td1/testfile", MSTR, sizeof(MSTR));

	RL(fd = rump_sys_open("/td2/testfile", O_RDONLY));

	RL(rump_sys_ioctl(fd, FULLFS_GETSTATE, &fc));
	ATF_REQUIRE_EQ(FULLFS_MODE_FAIL, fc.fc_mode);

	memset(&fc, 0, sizeof(fc));
	fc.fc_mode = FULLFS_MODE_PASS;
	fc.fc_error = ENOSPC;
	fc.fc_opmask = FULLFS_OP_ALL;
	RL(rump_sys_ioctl(fd, FULLFS_SETSTATE, &fc));

	RL(rump_sys_close(fd));
}


ATF_TC(statvfs_follows_mode);
ATF_TC_HEAD(statvfs_follows_mode, tc)
{
	atf_tc_set_md_var(tc, "descr", "statvfs reports free space based on current mode");
}
ATF_TC_BODY(statvfs_follows_mode, tc)
{
	struct statvfs sb;

	setup_fullfs();

	RL(rump_sys_statvfs1("/td2", &sb, ST_WAIT));
	ATF_REQUIRE_EQ(0, sb.f_bfree);

	fullfs_setstate(FULLFS_MODE_PASS, FULLFS_OP_ALL, ENOSPC, 0, 0);
	RL(rump_sys_statvfs1("/td2", &sb, ST_WAIT));
	ATF_REQUIRE_MSG(sb.f_bfree > 0, "expected f_bfree > 0 in PASS mode");

	fullfs_setstate(FULLFS_MODE_FAIL, FULLFS_OP_ALL, ENOSPC, 0, 0);
	RL(rump_sys_statvfs1("/td2", &sb, ST_WAIT));
	ATF_REQUIRE_EQ(0, sb.f_bfree);
}


ATF_TP_ADD_TCS(tp)
{
	ATF_TP_ADD_TC(tp, read_succeeds);
	ATF_TP_ADD_TC(tp, write_fails);
	ATF_TP_ADD_TC(tp, create_fails);
	ATF_TP_ADD_TC(tp, mkdir_fails);
	ATF_TP_ADD_TC(tp, symlink_fails);
	ATF_TP_ADD_TC(tp, mknod_fails);
	ATF_TP_ADD_TC(tp, link_fails);
	ATF_TP_ADD_TC(tp, remove_succeeds);
	ATF_TP_ADD_TC(tp, rename_succeeds);
	ATF_TP_ADD_TC(tp, rmdir_succeeds);
	ATF_TP_ADD_TC(tp, statvfs_shows_full);
	ATF_TP_ADD_TC(tp, getstate_initial);
	ATF_TP_ADD_TC(tp, toggle_pass_fail);
	ATF_TP_ADD_TC(tp, error_code);
	ATF_TP_ADD_TC(tp, error_roundtrip);
	ATF_TP_ADD_TC(tp, opmask_write_only);
	ATF_TP_ADD_TC(tp, opmask_create_only);
	ATF_TP_ADD_TC(tp, opmask_none);
	ATF_TP_ADD_TC(tp, opmask_multiple);
	ATF_TP_ADD_TC(tp, doom_counter_basic);
	ATF_TP_ADD_TC(tp, doom_counter_opmask);
	ATF_TP_ADD_TC(tp, doom_counter_midstream);
	ATF_TP_ADD_TC(tp, byte_counter_basic);
	ATF_TP_ADD_TC(tp, byte_counter_oversize);
	ATF_TP_ADD_TC(tp, byte_counter_nonwrite);
	ATF_TP_ADD_TC(tp, random_basic);
	ATF_TP_ADD_TC(tp, random_error_code);
	ATF_TP_ADD_TC(tp, setstate_invalid);
	ATF_TP_ADD_TC(tp, ioctl_on_regular_file);
	ATF_TP_ADD_TC(tp, statvfs_follows_mode);
	return atf_no_error();
}
