/*	$NetBSD: t_basic.c,v 1.5 2010/11/07 17:51:18 jmmv Exp $	*/

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
#include <ufs/ufs/ufsmount.h>

#include "../../h_macros.h"

ATF_TC(basic);
ATF_TC_HEAD(basic, tc)
{
	atf_tc_set_md_var(tc, "descr", "basic union functionality: two views");
}

#define MSTR "magic bus"

static void
xput_tfile(const char *path)
{
	int fd;

	fd = rump_sys_open(path, O_CREAT | O_RDWR, 0777);
	if (fd == -1)
		atf_tc_fail_errno("create %s", path);
	if (rump_sys_write(fd, MSTR, sizeof(MSTR)) != sizeof(MSTR))
		atf_tc_fail_errno("write to testfile");
	rump_sys_close(fd);
}

static int
xread_tfile(const char *path)
{
	char buf[128];
	int fd;

	fd = rump_sys_open(path, O_RDONLY);
	if (fd == -1)
		return errno;
	if (rump_sys_read(fd, buf, sizeof(buf)) == -1)
		atf_tc_fail_errno("read tfile");
	rump_sys_close(fd);
	if (strcmp(buf, MSTR) == 0)
		return 0;
	return EPROGMISMATCH;
}

#define IMG1 "atf1.img"
#define IMG2 "atf2.img"
#define DEV1 "/dev/fs1"
#define DEV2 "/dev/fs2"
#define newfs_base "newfs -F -s 10000 "

ATF_TC_BODY(basic, tc)
{
	struct ufs_args args;
	struct union_args unionargs;
	int error;

	if (system(newfs_base IMG1) == -1)
		atf_tc_fail_errno("create img1");
	if (system(newfs_base IMG2) == -1)
		atf_tc_fail_errno("create img2");

	rump_init();
        rump_pub_etfs_register(DEV1, IMG1, RUMP_ETFS_BLK);
        rump_pub_etfs_register(DEV2, IMG2, RUMP_ETFS_BLK);

	if (rump_sys_mkdir("/mp1", 0777) == -1)
		atf_tc_fail_errno("mp1");
	if (rump_sys_mkdir("/mp2", 0777) == -1)
		atf_tc_fail_errno("mp1");

	memset(&args, 0, sizeof(args));
	args.fspec = __UNCONST(DEV1);
	if (rump_sys_mount(MOUNT_FFS, "/mp1", 0, &args, sizeof(args)) == -1)
		atf_tc_fail_errno("could not mount ffs1");

	memset(&args, 0, sizeof(args));
	args.fspec = __UNCONST(DEV2);
	if (rump_sys_mount(MOUNT_FFS, "/mp2", 0, &args, sizeof(args)) == -1)
		atf_tc_fail_errno("could not mount tmpfs2");

	xput_tfile("/mp1/tensti");
	memset(&unionargs, 0, sizeof(unionargs));

	unionargs.target = __UNCONST("/mp1");
	unionargs.mntflags = UNMNT_BELOW;

	if (rump_sys_mount(MOUNT_UNION, "/mp2", 0,
	    &unionargs, sizeof(unionargs)) == -1)
		atf_tc_fail_errno("union mount");

	/* first, test we can read the old file from the new namespace */
	error = xread_tfile("/mp2/tensti");
	if (error != 0)
		atf_tc_fail("union compare failed: %d (%s)",
		    error, strerror(error));

	/* then, test upper layer writes don't affect the lower layer */
	xput_tfile("/mp2/kiekko");
	if ((error = xread_tfile("/mp1/kiekko")) != ENOENT)
		atf_tc_fail("invisibility failed: %d (%s)",
		    error, strerror(error));

	/* unmount etc. yaddayadda if non-lazy */
}

ATF_TP_ADD_TCS(tp)
{
	ATF_TP_ADD_TC(tp, basic);
	return 0; /*XXX?*/
}
