/*	$NetBSD: t_mount.c,v 1.6 2010/07/19 16:22:05 pooka Exp $	*/

/*
 * Adapted for rump and atf from a testcase supplied
 * by Hubert Feyrer on netbsd-users@
 */

#include <atf-c.h>

#define IMGNAME "image.ffs"
#define IMGSIZE (96 * 512)

#define MNTDIR  "/mnt"

#include "../common/h_fsmacros.h"

ATF_TC(48Kimage);
ATF_TC_HEAD(48Kimage, tc)
{
	atf_tc_set_md_var(tc, "descr", "mount small 48K ffs image");
	atf_tc_set_md_var(tc, "use.fs", "true");
}

ATF_TC_BODY(48Kimage, tc)
{
	void *tmp;

	if (ffs_fstest_newfs(tc, &tmp, IMGNAME, IMGSIZE) != 0)
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

ATF_TP_ADD_TCS(tp)
{
	ATF_TP_ADD_TC(tp, 48Kimage);
	return atf_no_error();
}
