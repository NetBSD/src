/*	$NetBSD: t_update_log.c,v 1.1 2017/03/22 21:33:53 jdolecek Exp $	*/

/*
 * Check log behaviour on mount updates
 */

#include <atf-c.h>

#define FSTEST_IMGSIZE (96 * 512)
#include "../common/h_fsmacros.h"

#include <sys/types.h>
#include <sys/mount.h>

#include <stdlib.h>

#include <ufs/ufs/ufsmount.h>

#include <rump/rump.h>
#include <rump/rump_syscalls.h>

#include "h_macros.h"

ATF_TC(updaterwtolog);
ATF_TC_HEAD(updaterwtolog, tc)
{

	atf_tc_set_md_var(tc, "descr", "mounts file system with "
	    "rw, then rw+log");
}

/*
 * PR kern/52056
 * This doesn't trigger panic with old, despite same operations triggering
 * it with live kernel.
 * Steps:
 * 1. boot singleuser
 * 2. mount -u /
 * 3. echo abc > abc
 * 4. mount -u -o log /
 * 5. echo abc >> abc
 *
 * Keeping around just in case.
 */
ATF_TC_BODY(updaterwtolog, tc)
{
	char buf[1024];
	struct ufs_args args;
	int n = 10, fd;
	const char *newfs_args = "-O2";
	const char teststring[] = "6c894efc21094525ca1f974c0189b3b0c4e1f28d";

        snprintf(buf, sizeof(buf), "newfs -q user -q group -F -s 4000 -n %d "
            "%s %s", (n + 3), newfs_args, FSTEST_IMGNAME);
        if (system(buf) == -1)
                atf_tc_fail_errno("cannot create file system");

        rump_init();
        if (rump_sys_mkdir(FSTEST_MNTNAME, 0777) == -1)
                atf_tc_fail_errno("mount point create");

        rump_pub_etfs_register("/diskdev", FSTEST_IMGNAME, RUMP_ETFS_BLK);

        args.fspec = __UNCONST("/diskdev");

        if (rump_sys_mount(MOUNT_FFS, FSTEST_MNTNAME, MNT_RDONLY,
            &args, sizeof(args)) == -1)
                atf_tc_fail_errno("mount ffs rw %s", FSTEST_MNTNAME);

	if (rump_sys_chdir(FSTEST_MNTNAME) == 1)
		atf_tc_fail_errno("chdir");

        if (rump_sys_mount(MOUNT_FFS, FSTEST_MNTNAME, MNT_UPDATE,
            &args, sizeof(args)) == -1)
                atf_tc_fail_errno("mount ffs rw %s", FSTEST_MNTNAME);

	RL(fd = rump_sys_open("dummy", O_CREAT | O_RDWR, 0755));
	RL(rump_sys_write(fd, teststring, strlen(teststring)));
	rump_sys_close(fd);

        if (rump_sys_mount(MOUNT_FFS, FSTEST_MNTNAME, MNT_LOG|MNT_UPDATE,
            &args, sizeof(args)) == -1)
                atf_tc_fail_errno("mount ffs rw log update %s", FSTEST_MNTNAME);

	RL(fd = rump_sys_open("dummy", O_APPEND | O_RDWR, 0755));
	RL(rump_sys_write(fd, teststring, strlen(teststring)));
	rump_sys_close(fd);

	/* otherwise we're do-ne */
}

ATF_TC(updaterwtolog_async);
ATF_TC_HEAD(updaterwtolog_async, tc)
{

	atf_tc_set_md_var(tc, "descr", "mounts file system with "
	    "rw+async, then rw+async+log");
}

/*
 * PR kern/52056
 */
ATF_TC_BODY(updaterwtolog_async, tc)
{
	char buf[1024];
	struct ufs_args args;
	int n = 10, fd;
	const char *newfs_args = "-O2";

        snprintf(buf, sizeof(buf), "newfs -q user -q group -F -s 4000 -n %d "
            "%s %s", (n + 3), newfs_args, FSTEST_IMGNAME);
        if (system(buf) == -1)
                atf_tc_fail_errno("cannot create file system");

        rump_init();
        if (rump_sys_mkdir(FSTEST_MNTNAME, 0777) == -1)
                atf_tc_fail_errno("mount point create");

        rump_pub_etfs_register("/diskdev", FSTEST_IMGNAME, RUMP_ETFS_BLK);

        args.fspec = __UNCONST("/diskdev");

        if (rump_sys_mount(MOUNT_FFS, FSTEST_MNTNAME, 0,
            &args, sizeof(args)) == -1)
                atf_tc_fail_errno("mount ffs rw %s", FSTEST_MNTNAME);

	if (rump_sys_chdir(FSTEST_MNTNAME) == 1)
		atf_tc_fail_errno("chdir");

	RL(fd = rump_sys_open("dummy", O_CREAT | O_RDWR, 0755));
	sprintf(buf, "test file");
	RL(rump_sys_write(fd, buf, strlen(buf)));
	rump_sys_close(fd);

        if (rump_sys_mount(MOUNT_FFS, FSTEST_MNTNAME, MNT_LOG|MNT_ASYNC|MNT_UPDATE,
            &args, sizeof(args)) == -1)
                atf_tc_fail_errno("mount ffs rw log update %s", FSTEST_MNTNAME);

	/* otherwise we're do-ne */
}

ATF_TP_ADD_TCS(tp)
{
	ATF_TP_ADD_TC(tp, updaterwtolog);
	ATF_TP_ADD_TC(tp, updaterwtolog_async);

	return atf_no_error();
}
