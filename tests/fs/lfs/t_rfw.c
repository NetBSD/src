/*	$NetBSD: t_rfw.c,v 1.7 2025/10/18 22:18:19 perseant Exp $	*/

#include <sys/types.h>
#include <sys/wait.h>
#include <sys/sysctl.h>

#include <atf-c.h>
#include <errno.h>
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

/* Debugging conditions */
/* #define FORCE_SUCCESS */ /* Don't actually revert, everything worked */
/* #define USE_DUMPLFS */ /* Dump the filesystem at certain steps */

/* Temporary files to store superblocks */
#define SBLOCK0_COPY "sb0.dd"
#define SBLOCK1_COPY "sb1.dd"

/* Size of the image file, in 512-blocks */
#define FSSIZE 10000

/* Actually run the test */
void test(int);

ATF_TC(rfw32);
ATF_TC_HEAD(rfw32, tc)
{
	atf_tc_set_md_var(tc, "descr",
		"LFS32 roll-forward creates an inconsistent filesystem");
	atf_tc_set_md_var(tc, "timeout", "20");
}

ATF_TC(rfw64);
ATF_TC_HEAD(rfw64, tc)
{
	atf_tc_set_md_var(tc, "descr",
		"LFS64 roll-forward creates an inconsistent filesystem");
	atf_tc_set_md_var(tc, "timeout", "20");
}

#define UNCHANGED_CONTROL MP "/3-unchanged-control"
#define TO_BE_DELETED     MP "/4-to-be-deleted"
#define TO_BE_APPENDED    MP "/5-to-be-appended"
#define NEWLY_CREATED     MP "/6-newly-created"

const char *sblock[2] = { SBLOCK0_COPY, SBLOCK1_COPY };

ATF_TC_BODY(rfw32, tc)
{
	test(32);
}

ATF_TC_BODY(rfw64, tc)
{
	test(64);
}

void test(int width)
{
	struct ufs_args args;
	char buf[MAXLINE];
	int i;

	setvbuf(stdout, NULL, _IONBF, 0);

	/*
	 * Initialize.
	 */
	atf_tc_expect_pass();

	/* Create filesystem, note superblock locations */
	create_lfs(FSSIZE, FSSIZE, width, 1);

	/*
	 * Create initial filesystem state, from which
	 * we will attempt to roll forward.
	 */

	/* Mount filesystem */
	fprintf(stderr, "* Mount fs [1, initial]\n");
	memset(&args, 0, sizeof(args));
	args.fspec = __UNCONST(FAKEBLK);
	if (rump_sys_mount(MOUNT_LFS, MP, 0, &args, sizeof(args)) == -1)
		atf_tc_fail_errno("rump_sys_mount failed");

	/* Payload */
	fprintf(stderr, "* Initial payload\n");
	write_file(UNCHANGED_CONTROL, CHUNKSIZE, 1);
	write_file(TO_BE_DELETED, CHUNKSIZE, 1);
	write_file(TO_BE_APPENDED, CHUNKSIZE, 1);
	rump_sys_sync();
	rump_sys_sync();
	sleep(1); /* XXX yuck - but we need the superblocks dirty */

	/* Make copies of superblocks */
	for (i = 0; i < 2; i++) {
		sprintf(buf, "dd if=%s of=%s bs=512 iseek=%lld"
			" count=16 conv=sync", IMGNAME, sblock[i], sbaddr[i]);
		system(buf);
	}

	/* Unmount */
	rump_sys_unmount(MP, 0);
	if (fsck())
		atf_tc_fail_errno("fsck found errors after first unmount");

	/*
	 * Make changes which we will attempt to roll forward.
	 */

	/* Reconfigure and mount filesystem again */
	fprintf(stderr, "* Mount fs [2, after changes]\n");
	if (rump_sys_mount(MOUNT_LFS, MP, 0, &args, sizeof(args)) == -1)
		atf_tc_fail_errno("rump_sys_mount failed [2]");

	fprintf(stderr, "* Update payload\n");

	/* Add new file */
	write_file(NEWLY_CREATED, CHUNKSIZE, 1);

	/* Append to existing file */
	write_file(TO_BE_APPENDED, CHUNKSIZE, 1);

	/* Delete file */
	rump_sys_unlink(TO_BE_DELETED);

	/* Done with payload, unmount fs */
	rump_sys_unmount(MP, 0);
	if (fsck())
		atf_tc_fail_errno("fsck found errors after second unmount");

#ifndef FORCE_SUCCESS
	fprintf(stderr, "* Revert superblocks\n");
	/*
	 * Copy back old superblocks, reverting FS to old state
	 */
	for (i = 0; i < 2; i++) {
		sprintf(buf, "dd of=%s if=%s bs=512 oseek=%lld count=16"
			" conv=sync,notrunc", IMGNAME, sblock[i], sbaddr[i]);
		system(buf);
	}

	if (fsck())
		atf_tc_fail_errno("fsck found errors with old superblocks");
#endif /* FORCE_SUCCESS */
#ifdef USE_DUMPLFS
	dumplfs();
#endif /* USE_DUMPLFS */

	/*
	 * Roll forward.
	 */

	/* Mount filesystem; this will roll forward. */
	fprintf(stderr, "* Mount fs [3, to roll forward]\n");
	if (rump_sys_mount(MOUNT_LFS, MP, 0, &args, sizeof(args)) == -1)
		atf_tc_fail_errno("rump_sys_mount failed [3]");

	/* Unmount filesystem */
	if (rump_sys_unmount(MP, 0) != 0)
		atf_tc_fail_errno("rump_sys_umount failed after roll-forward");

	/*
	 * Use fsck_lfs to look for consistency errors.
	 */

	fprintf(stderr, "* Fsck after roll-forward\n");
	if (fsck()) {
		fprintf(stderr, "*** FAILED FSCK ***\n");
		atf_tc_fail("fsck found errors after roll forward");
	}
#ifdef USE_DUMPLFS
	dumplfs();
#endif /* USE_DUMPLFS */
	
	/*
	 * Check file system contents
	 */

	/* Mount filesystem one last time */
	fprintf(stderr, "* Mount fs [4, after roll forward complete]\n");
	if (rump_sys_mount(MOUNT_LFS, MP, 0, &args, sizeof(args)) == -1)
		atf_tc_fail_errno("rump_sys_mount failed [4]");

	if (check_file(UNCHANGED_CONTROL, CHUNKSIZE) != 0)
		atf_tc_fail("Unchanged control file differs(!)");

	if (rump_sys_access(TO_BE_DELETED, F_OK) == 0)
		atf_tc_fail("Removed file still present");
	else 
		fprintf(stderr, "%s: no problem\n", TO_BE_DELETED);

	if (check_file(TO_BE_APPENDED, 2 * CHUNKSIZE) != 0)
		atf_tc_fail("Appended file differs");

	if (rump_sys_access(NEWLY_CREATED, F_OK) != 0)
		atf_tc_fail("Newly added file missing");

	if (check_file(NEWLY_CREATED, CHUNKSIZE) != 0)
		atf_tc_fail("Newly added file differs");

	/* Umount filesystem */
	rump_sys_unmount(MP, 0);

	/* Final fsck to double check */
	fprintf(stderr, "* Fsck after final unmount\n");
	if (fsck()) {
		fprintf(stderr, "*** FAILED FSCK ***\n");
		atf_tc_fail("fsck found errors after final unmount");
	}
}

ATF_TP_ADD_TCS(tp)
{

	ATF_TP_ADD_TC(tp, rfw32);
	ATF_TP_ADD_TC(tp, rfw64);
	return atf_no_error();
}
