/*	$NetBSD: t_union.c,v 1.2 2011/01/12 21:45:39 pooka Exp $	*/

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

#include <miscfs/union/union.h>

#include "../../h_macros.h"
#include "../common/h_fsmacros.h"

#define MSTR "magic bus"

static void
xput_tfile(const char *mp, const char *path)
{
	char pathb[MAXPATHLEN];
	int fd;

	strcpy(pathb, mp);
	strcat(pathb, "/");
	strcat(pathb, path);

	RL(fd = rump_sys_open(pathb, O_CREAT | O_RDWR, 0777));
	if (rump_sys_write(fd, MSTR, sizeof(MSTR)) != sizeof(MSTR))
		atf_tc_fail_errno("write to testfile");
	RL(rump_sys_close(fd));
}

static int
xread_tfile(const char *mp, const char *path)
{
	char pathb[MAXPATHLEN];
	char buf[128];
	int fd;

	strcpy(pathb, mp);
	strcat(pathb, "/");
	strcat(pathb, path);

	fd = rump_sys_open(pathb, O_RDONLY);
	if (fd == -1)
		return errno;
	if (rump_sys_read(fd, buf, sizeof(buf)) == -1)
		atf_tc_fail_errno("read tfile");
	RL(rump_sys_close(fd));
	if (strcmp(buf, MSTR) == 0)
		return 0;
	return EPROGMISMATCH;
}

#define TFILE "tensti"
static void
basic(const atf_tc_t *tc, const char *mp)
{
	char lowerpath[MAXPATHLEN];
	char dbuf[8192];
	struct union_args unionargs;
	struct stat sb;
	struct dirent *dp;
	int error, fd, dsize;

	snprintf(lowerpath, sizeof(lowerpath), "%s.lower", mp);

	RL(rump_sys_mkdir(lowerpath, 0777));

	/* create a file in the lower layer */
	xput_tfile(lowerpath, TFILE);

	/* mount the union with our testfs as the upper layer */
	memset(&unionargs, 0, sizeof(unionargs));
	unionargs.target = lowerpath;
	unionargs.mntflags = UNMNT_BELOW;

	if (rump_sys_mount(MOUNT_UNION, mp, 0,
	    &unionargs, sizeof(unionargs)) == -1) {
		if (errno == EOPNOTSUPP) {
			atf_tc_skip("fs does not support VOP_WHITEOUT");
		} else {
			atf_tc_fail_errno("union mount");
		}
	}

	/* first, test we can read the old file from the new namespace */
	error = xread_tfile(mp, TFILE);
	if (error != 0)
		atf_tc_fail("union compare failed: %d (%s)",
		    error, strerror(error));

	/* then, test upper layer writes don't affect the lower layer */
	xput_tfile(mp, "kiekko");
	if ((error = xread_tfile(lowerpath, "kiekko")) != ENOENT)
		atf_tc_fail("invisibility failed: %d (%s)",
		    error, strerror(error));
	
	/* check that we can whiteout stuff in the upper layer */
	FSTEST_ENTER();
	RL(rump_sys_unlink(TFILE));
	ATF_REQUIRE_ERRNO(ENOENT, rump_sys_stat(TFILE, &sb) == -1);
	FSTEST_EXIT();

	/* check that the removed node is not in the directory listing */
	RL(fd = rump_sys_open(mp, O_RDONLY));
	RL(dsize = rump_sys_getdents(fd, dbuf, sizeof(dbuf)));
	for (dp = (struct dirent *)dbuf;
	    (char *)dp < dbuf + dsize;
	    dp = _DIRENT_NEXT(dp)) {
		if (strcmp(dp->d_name, TFILE) == 0 && dp->d_type != DT_WHT)
			atf_tc_fail("removed file non-white-outed");
	}
	RL(rump_sys_close(fd));

	RL(rump_sys_unmount(mp, 0));
}

ATF_TC_FSAPPLY(basic, "check basic union functionality");

ATF_TP_ADD_TCS(tp)
{

	ATF_TP_FSAPPLY(basic);
	return atf_no_error();
}
