/*	$NetBSD: t_mount.c,v 1.11 2010/11/07 17:51:17 jmmv Exp $	*/

/*
 * Basic tests for mounting
 */

/*
 * 48Kimage:
 * Adapted for rump and atf from a testcase supplied
 * by Hubert Feyrer on netbsd-users@
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

#include "../../h_macros.h"

ATF_TC(48Kimage);
ATF_TC_HEAD(48Kimage, tc)
{
	atf_tc_set_md_var(tc, "descr", "mount small 48K ffs image");
}

ATF_TC_BODY(48Kimage, tc)
{
	void *tmp;

	atf_tc_expect_fail("PR kern/43573");
	FSTEST_CONSTRUCTOR(tc, ffs, tmp);
	atf_tc_expect_pass();

	FSTEST_DESTRUCTOR(tc, ffs, tmp);
}

ATF_TC(fsbsize2big);
ATF_TC_HEAD(fsbsize2big, tc)
{

	atf_tc_set_md_var(tc, "descr", "mounts file system with "
	    "blocksize > MAXPHYS");
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

	/*
	 * We cannot pass newfs parameters via the fstest interface,
	 * so do things the oldfashioned manual way.
	 */
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
		atf_tc_fail("not expecting to be alive");
	}

	/* otherwise we're do-ne */
}

ATF_TP_ADD_TCS(tp)
{

	ATF_TP_ADD_TC(tp, 48Kimage);
	ATF_TP_ADD_TC(tp, fsbsize2big);

	return atf_no_error();
}
