/*	$NetBSD: t_mount.c,v 1.1.2.1 2024/06/29 19:43:26 perseant Exp $	*/

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

#include <fs/exfatfs/exfatfs.h>
#include <fs/exfatfs/exfatfs_mount.h>

#include <rump/rump.h>
#include <rump/rump_syscalls.h>

#include "h_macros.h"

ATF_TC(basic_mount);
ATF_TC_HEAD(basic_mount, tc)
{

	atf_tc_set_md_var(tc, "descr", "create and mount exFAT file system");
}

ATF_TC_BODY(basic_mount, tc)
{
	char cmd[1024];
	struct exfatfs_args args;
	struct statvfs svb;
	FILE *fp;
	int i, fd;

	/*
	 * We cannot pass newfs parameters via the fstest interface,
	 * so do things the oldfashioned manual way.
	 */
	fp = fopen("exfatfs.img", "wb");
	for (i = 0; i < 1024; i++)
		fwrite(cmd, sizeof(cmd), 1, fp); /* arbitrary data */
	fclose(fp);
	system("ls -l `pwd`/exfatfs.img");

	snprintf(cmd, sizeof(cmd), "newfs_exfatfs -F -s 10000 "
	    "./exfatfs.img > /dev/null");
	if (system(cmd))
		atf_tc_fail("cannot create file system");

	rump_init();
	if (rump_pub_etfs_register("/devdisk", "exfatfs.img", RUMP_ETFS_BLK))
		atf_tc_fail("cannot register rump fake device");

	memset(&args, 0, sizeof(args));
	args.fspec = __UNCONST("/devdisk");
	args.version = EXFATFSMNT_VERSION;

	if (rump_sys_mkdir("/mp", 0777) == -1)
		atf_tc_fail_errno("create mountpoint");

	/* If mount failed, bomb out. */
	if (rump_sys_mount(MOUNT_EXFATFS, "/mp", 0, &args, sizeof(args)) < 0)
		atf_tc_fail_errno("mount filesystem");

	if (rump_sys_statvfs1("/mp", &svb, ST_WAIT))
		atf_tc_fail_errno("stat filesystem");

	if (rump_sys_mkdir("/mp/dir", 0777) == -1)
		atf_tc_fail_errno("create directory");

	if ((fd = rump_sys_open("/mp/file", O_CREAT|O_WRONLY)) < 0)
		atf_tc_fail_errno("create file in directory");
	for (i = 0; i < 128; i++)
		if (write(fd, cmd, sizeof(cmd)) < 0)
			atf_tc_fail_errno("write to file");
	if (rump_sys_close(fd) < 0)
		atf_tc_fail_errno("write to file");

	if (rump_sys_unmount("/mp", 0) < 0)
		atf_tc_fail_errno("unmount filesystem");

	snprintf(cmd, sizeof(cmd), "fsck_exfatfs -n "
	    "exfatfs.img > /dev/null");
	if (system(cmd))
		atf_tc_fail("fsck reports errors after unmount");
}

ATF_TP_ADD_TCS(tp)
{

	ATF_TP_ADD_TC(tp, basic_mount);

	return atf_no_error();
}
