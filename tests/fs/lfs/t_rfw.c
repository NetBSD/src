/*	$NetBSD: t_rfw.c,v 1.1 2020/08/18 03:02:50 perseant Exp $	*/

#include <sys/types.h>
#include <sys/mount.h>
#include <sys/sysctl.h>
#include <sys/wait.h>

#include <atf-c.h>
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

/* Debugging conditions */
/* #define FORCE_SUCCESS */ /* Don't actually revert, everything worked */
/* #define FORCE_SYSCTL */ /* Don't check - necessary for FORCE_SUCCESS */

/* Write a well-known byte pattern into a file, appending if it exists */
int write_file(const char *, int);

/* Check the byte pattern and size of the file */
int check_file(const char *, int);

ATF_TC(rfw);
ATF_TC_HEAD(rfw, tc)
{
	atf_tc_set_md_var(tc, "descr",
		"LFS roll-forward creates an inconsistent filesystem");
	atf_tc_set_md_var(tc, "timeout", "20");
}

#define IMGNAME "disk.img"
#define FAKEBLK "/dev/blk"
#define LOGFILE "newfs.log"
#define SBLOCK0_COPY "sb0.dd"
#define SBLOCK1_COPY "sb1.dd"
#define MAXLINE 132
#define CHUNKSIZE 300
ATF_TC_BODY(rfw, tc)
{
	struct ufs_args args;
	FILE *pipe;
#if (!defined FORCE_SUCCESS && !defined FORCE_SYSCTL)
	int o_sysctl_rfw;
	int n_sysctl_rfw;
	int mib[3];
	size_t len;
#endif
	char buf[MAXLINE];
	long long sb0addr = -1, sb1addr = -1;

	/*
	 * Initialize.
	 */
#if (!defined FORCE_SUCCESS && !defined FORCE_SYSCTL)
	/* Set sysctl to allow roll-forward */
	fprintf(stderr, "* Set sysctl\n");
	mib[0] = CTL_VFS;
	mib[1] = 5; /* LFS */
	mib[2] = LFS_DO_RFW;
	len = sizeof(o_sysctl_rfw);
	if (sysctl(mib, 3, &o_sysctl_rfw, &len,
		&n_sysctl_rfw, sizeof(n_sysctl_rfw)) < 0)
		atf_tc_skip("roll-forward not enabled in kernel");
#endif /* !FORCE_SUCCESS && !FORCE_SYSCTL */

	/* Create filesystem */
	fprintf(stderr, "* Create file system\n");
	if (system("newfs_lfs -D -F -s 10000 ./" IMGNAME " > " LOGFILE) == -1)
		atf_tc_fail_errno("newfs failed");
	pipe = fopen(LOGFILE, "r");
	if (pipe == NULL)
		atf_tc_fail_errno("newfs failed to execute");
	while (fgets(buf, MAXLINE, pipe) != NULL) {
		if (sscanf(buf, "%lld,%lld", &sb0addr, &sb1addr) == 2)
			break;
	}
	while (fgets(buf, MAXLINE, pipe) != NULL)
		;
	fclose(pipe);
	if (sb0addr < 0 || sb1addr < 0)
		atf_tc_fail("superblock not found");
	fprintf(stderr, "* Superblocks at %lld and %lld\n",
		sb0addr, sb1addr);

	/* Set up rump */
	rump_init();
	if (rump_sys_mkdir("/mp", 0777) == -1)
		atf_tc_fail_errno("cannot create mountpoint");
	rump_pub_etfs_register(FAKEBLK, IMGNAME, RUMP_ETFS_BLK);

	/*
	 * Create initial filesystem state, from which
	 * we will attempt to roll forward.
	 */

	/* Mount filesystem */
	fprintf(stderr, "* Mount fs [1]\n");
	memset(&args, 0, sizeof(args));
	args.fspec = __UNCONST(FAKEBLK);
	if (rump_sys_mount(MOUNT_LFS, "/mp", 0, &args, sizeof(args)) == -1)
		atf_tc_fail_errno("rump_sys_mount failed");

	/* Payload */
	fprintf(stderr, "* Initial payload\n");
	write_file("/mp/to_be_deleted", CHUNKSIZE);
	write_file("/mp/to_be_appended", CHUNKSIZE);

	/* Unmount */
	rump_sys_unmount("/mp", 0);

	/* Make copies of superblocks */
	sprintf(buf, "dd if=%s of=%s bs=512 iseek=%lld count=16 conv=sync",
		IMGNAME, SBLOCK0_COPY, sb0addr);
	system(buf);
	sprintf(buf, "dd if=%s of=%s bs=512 iseek=%lld count=16 conv=sync",
		IMGNAME, SBLOCK1_COPY, sb1addr);
	system(buf);

	/*
	 * Make changes which we will attempt to roll forward.
	 */

	/* Reconfigure and mount filesystem again */
	fprintf(stderr, "* Mount fs [2]\n");
	if (rump_sys_mount(MOUNT_LFS, "/mp", 0, &args, sizeof(args)) == -1)
		atf_tc_fail_errno("rump_sys_mount failed [2]");

	/* Add new file */
	write_file("/mp/newly_created", CHUNKSIZE);

	/* Append to existing file */
	write_file("/mp/to_be_appended", CHUNKSIZE);

	/* Delete file */
	rump_sys_unlink("/mp/to_be_deleted");

	/* Done with payload, unmount fs */
	rump_sys_unmount("/mp", 0);

#ifndef FORCE_SUCCESS
	/*
	 * Copy back old superblocks, reverting FS to old state
	 */
	sprintf(buf, "dd of=%s if=%s bs=512 oseek=%lld count=16 conv=sync,notrunc",
		IMGNAME, SBLOCK0_COPY, sb0addr);
	system(buf);
	sprintf(buf, "dd of=%s if=%s bs=512 oseek=%lld count=16 conv=sync,notrunc",
		IMGNAME, SBLOCK1_COPY, sb1addr);
	system(buf);

	if (system("fsck_lfs -n -f " IMGNAME))
		atf_tc_fail_errno("fsck found errors with old superblocks");
#endif

	/*
	 * Roll forward.
	 */

	/* Mount filesystem; this will roll forward. */
	fprintf(stderr, "* Mount fs [3]\n");
	if (rump_sys_mount(MOUNT_LFS, "/mp", 0, &args, sizeof(args)) == -1)
		atf_tc_fail_errno("rump_sys_mount failed [3]");

	/* Unmount filesystem */
	rump_sys_unmount("/mp", 0);

	/*
	 * Use fsck_lfs to look for consistency errors
	 */

	if (system("fsck_lfs -n -f " IMGNAME))
		atf_tc_fail_errno("fsck found errors");

	/*
	 * Check file system contents
	 */

	/* Mount filesystem one last time */
	fprintf(stderr, "* Mount fs [4]\n");
	if (rump_sys_mount(MOUNT_LFS, "/mp", 0, &args, sizeof(args)) == -1)
		atf_tc_fail_errno("rump_sys_mount failed [4]");

	if (check_file("/mp/newly_created", CHUNKSIZE) != 0)
		atf_tc_fail("Newly added file differs");

	if (check_file("/mp/to_be_appended", 2 * CHUNKSIZE) != 0)
		atf_tc_fail("Appended file differs");

	if (rump_sys_access("/mp/to_be_deleted", F_OK) == 0)
		atf_tc_fail("Removed file still present");

	/* Umount filesystem, test completed */
	rump_sys_unmount("/mp", 0);
}

ATF_TP_ADD_TCS(tp)
{

	ATF_TP_ADD_TC(tp, rfw);
	return atf_no_error();
}

/* Write some data into a file */
int write_file(const char *filename, int add)
{
	int fd, size, i;
	struct stat statbuf;
	unsigned char b;
	int flags = O_CREAT|O_WRONLY;

	if (rump_sys_stat(filename, &statbuf) < 0)
		size = 0;
	else {
		size = statbuf.st_size;
		flags |= O_APPEND;
	}

	fd = rump_sys_open(filename, flags);

	for (i = 0; i < add; i++) {
		b = (size + i) & 0xff;
		rump_sys_write(fd, &b, 1);
	}
	rump_sys_close(fd);

	return 0;
}

/* Check file's existence, size and contents */
int check_file(const char *filename, int size)
{
	int fd, i;
	struct stat statbuf;
	unsigned char b;

	if (rump_sys_stat(filename, &statbuf) < 0) {
		fprintf(stderr, "%s: stat failed\n", filename);
		return 1;
	}
	if (size != statbuf.st_size) {
		fprintf(stderr, "%s: expected %d bytes, found %d\n",
			filename, size, (int)statbuf.st_size);
		return 2;
	}

	fd = rump_sys_open(filename, O_RDONLY);

	for (i = 0; i < size; i++) {
		rump_sys_read(fd, &b, 1);
		if (b != (i & 0xff)) {
			fprintf(stderr, "%s: byte %d: expected %x found %x\n",
				filename, i, (unsigned)(i & 0xff), b);
			return 3;
		}
	}
	rump_sys_close(fd);

	return 0;
}
