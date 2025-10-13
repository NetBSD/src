/*	$NetBSD: t_basic.c,v 1.1 2025/10/13 00:44:35 perseant Exp $	*/

#include <sys/types.h>
#include <sys/mount.h>
#include <sys/wait.h>

#include <atf-c.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <rump/rump.h>
#include <rump/rump_syscalls.h>

#include <ufs/ufs/ufsmount.h>
#include <ufs/lfs/lfs.h>
#include <ufs/lfs/lfs_extern.h>

#include "h_macros.h"
#include "util.h"

#define FSSIZE 10000

/* Actually run the test */
void test(int);

ATF_TC(newfs_fsck32);
ATF_TC_HEAD(newfs_fsck32, tc)
{
	atf_tc_set_md_var(tc, "descr",
		"LFS32 newfs_lfs produces a filesystem that passes fsck_lfs");
	atf_tc_set_md_var(tc, "timeout", "20");
}

ATF_TC(newfs_fsck64);
ATF_TC_HEAD(newfs_fsck64, tc)
{
	atf_tc_set_md_var(tc, "descr",
		"LFS64 newfs_lfs produces a filesystem that passes fsck_lfs");
	atf_tc_set_md_var(tc, "timeout", "20");
}

ATF_TC_BODY(newfs_fsck32, tc)
{
	test(32);
}

ATF_TC_BODY(newfs_fsck64, tc)
{
	test(64);
}

void test(int width)
{
	setvbuf(stdout, NULL, _IONBF, 0);

	/*
	 * Initialize.
	 */

	/* Create image file larger than filesystem */
	create_lfs(FSSIZE, FSSIZE, width, 1);

	if (fsck())
		atf_tc_fail_errno("fsck found errors after first unmount");
}

ATF_TP_ADD_TCS(tp)
{

	ATF_TP_ADD_TC(tp, newfs_fsck32);
	ATF_TP_ADD_TC(tp, newfs_fsck64);
	return atf_no_error();
}
