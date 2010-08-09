/*	$NetBSD: t_mount.c,v 1.8 2010/08/09 17:42:26 pooka Exp $	*/

/*
 * Basic tests for mounting
 */

/*
 * 48Kimage:
 * Adapted for rump and atf from a testcase supplied
 * by Hubert Feyrer on netbsd-users@
 */

#include <atf-c.h>

#define IMGNAME "image.ffs"
#define IMGSIZE (96 * 512)

#define MNTDIR  "/mnt"

#include "../common/h_fsmacros.h"

#include <sys/types.h>
#include <sys/mount.h>

#include <stdlib.h>

#include <ufs/ufs/ufsmount.h>

#include <rump/rump.h>
#include <rump/rump_syscalls.h>

#include "../../h_macros.h"

ATF_TC(48Kimage);
ATF_TC_HEAD(48Kimage, tc)
{
	atf_tc_set_md_var(tc, "descr", "mount small 48K ffs image");
	atf_tc_set_md_var(tc, "use.fs", "true");
}

ATF_TC_BODY(48Kimage, tc)
{
	void *tmp;

	if (ffs_fstest_newfs(tc, &tmp, IMGNAME, IMGSIZE, NULL) != 0)
		atf_tc_fail("newfs failed");

	atf_tc_expect_fail("PR kern/43573");
	if (ffs_fstest_mount(tc, tmp, MNTDIR, 0) != 0) {
		atf_tc_fail("mount failed");
	}
	atf_tc_expect_pass();

	if (ffs_fstest_unmount(tc, MNTDIR, 0) != 0)
		atf_tc_fail("unmount failed");

	if (ffs_fstest_delfs(tc, tmp) != 0)
		atf_tc_fail("delfs failed");
}

ATF_TC(fsbsize2big);
ATF_TC_HEAD(fsbsize2big, tc)
{

	atf_tc_set_md_var(tc, "descr", "mounts file system with "
	    "blocksize > MAXPHYS");
	atf_tc_set_md_var(tc, "use.fs", "true");
	/* PR kern/43727 */
}

#define MYBLOCKSIZE 131072
#if MAXPHYS >= MYBLOCKSIZE
#error MAXPHYS too large for test to work
#endif
ATF_TC_BODY(fsbsize2big, tc)
{
	char cmd[1024];
	struct ufs_args args;
	struct statvfs svb;

	snprintf(cmd, sizeof(cmd), "newfs -G -b %d -F -s 10000 "
	    "ffs.img > /dev/null", MYBLOCKSIZE);
	if (system(cmd))
		atf_tc_fail("cannot create file system");

	rump_init();
	if (rump_pub_etfs_register("/devdisk", "ffs.img", RUMP_ETFS_BLK))
		atf_tc_fail("cannot register rump fake device");

	args.fspec = __UNCONST("/devdisk");

	if (rump_sys_mkdir("/mp", 0777) == -1)
		atf_tc_fail_errno("create mountpoint");

	/* mount succeeded?  bad omen.  confirm we're in trouble.  */
	if (rump_sys_mount(MOUNT_FFS, "/mp", 0, &args, sizeof(args)) != -1) {
		rump_sys_statvfs1("/mp", &svb, ST_WAIT);
	}

	/* otherwise we're do-ne */
}

ATF_TP_ADD_TCS(tp)
{

	ATF_TP_ADD_TC(tp, 48Kimage);
	ATF_TP_ADD_TC(tp, fsbsize2big);

	return atf_no_error();
}
