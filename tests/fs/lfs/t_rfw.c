/*	$NetBSD: t_rfw.c,v 1.3 2020/08/23 22:34:29 riastradh Exp $	*/

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

/* Debugging conditions */
/* #define FORCE_SUCCESS */ /* Don't actually revert, everything worked */

/* Write a well-known byte pattern into a file, appending if it exists */
int write_file(const char *, int);

/* Check the byte pattern and size of the file */
int check_file(const char *, int);

/* Check the file system for consistency */
int fsck(void);

/* Actually run the test, differentiating the orphaned delete problem */
void test(int);

ATF_TC(rfw);
ATF_TC_HEAD(rfw, tc)
{
	atf_tc_set_md_var(tc, "descr",
		"LFS roll-forward creates an inconsistent filesystem");
	atf_tc_set_md_var(tc, "timeout", "20");
}

#define MAXLINE 132
#define CHUNKSIZE 300

#define IMGNAME "disk.img"
#define FAKEBLK "/dev/blk"
#define LOGFILE "newfs.log"
#define SBLOCK0_COPY "sb0.dd"
#define SBLOCK1_COPY "sb1.dd"

#define MP "/mp"
#define UNCHANGED_CONTROL MP "/3-unchanged-control"
#define TO_BE_DELETED     MP "/4-to-be-deleted"
#define TO_BE_APPENDED    MP "/5-to-be-appended"
#define NEWLY_CREATED     MP "/6-newly-created"

long long sbaddr[2] = { -1, -1 };
const char *sblock[2] = { SBLOCK0_COPY, SBLOCK1_COPY };

ATF_TC_BODY(rfw, tc)
{
	struct ufs_args args;
	FILE *pipe;
	char buf[MAXLINE];
	int i;

	setvbuf(stdout, NULL, _IONBF, 0);

	/*
	 * Initialize.
	 */
	atf_tc_expect_fail("roll-forward not yet implemented");

	/* Create filesystem, note superblock locations */
	fprintf(stderr, "* Create file system\n");
	if (system("newfs_lfs -D -F -s 10000 ./" IMGNAME " > " LOGFILE) == -1)
		atf_tc_fail_errno("newfs failed");
	pipe = fopen(LOGFILE, "r");
	if (pipe == NULL)
		atf_tc_fail_errno("newfs failed to execute");
	while (fgets(buf, MAXLINE, pipe) != NULL) {
		if (sscanf(buf, "%lld,%lld", sbaddr, sbaddr + 1) == 2)
			break;
	}
	while (fgets(buf, MAXLINE, pipe) != NULL)
		;
	fclose(pipe);
	if (sbaddr[0] < 0 || sbaddr[1] < 0)
		atf_tc_fail("superblock not found");
	fprintf(stderr, "* Superblocks at %lld and %lld\n",
		sbaddr[0], sbaddr[1]);

	/* Set up rump */
	rump_init();
	if (rump_sys_mkdir(MP, 0777) == -1)
		atf_tc_fail_errno("cannot create mountpoint");
	rump_pub_etfs_register(FAKEBLK, IMGNAME, RUMP_ETFS_BLK);

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
	write_file(UNCHANGED_CONTROL, CHUNKSIZE);
	write_file(TO_BE_DELETED, CHUNKSIZE);
	write_file(TO_BE_APPENDED, CHUNKSIZE);
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

	/*
	 * Make changes which we will attempt to roll forward.
	 */

	/* Reconfigure and mount filesystem again */
	fprintf(stderr, "* Mount fs [2, after changes]\n");
	if (rump_sys_mount(MOUNT_LFS, MP, 0, &args, sizeof(args)) == -1)
		atf_tc_fail_errno("rump_sys_mount failed [2]");

	/* Add new file */
	write_file(NEWLY_CREATED, CHUNKSIZE);

	/* Append to existing file */
	write_file(TO_BE_APPENDED, CHUNKSIZE);

	/* Delete file */
	rump_sys_unlink(TO_BE_DELETED);

	/* Done with payload, unmount fs */
	rump_sys_unmount(MP, 0);

#ifndef FORCE_SUCCESS
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
#endif

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

	if (fsck()) {
		fprintf(stderr, "*** FAILED FSCK ***\n");
		atf_tc_fail("fsck found errors after roll forward");
	}

	/*
	 * Check file system contents
	 */

	/* Mount filesystem one last time */
	fprintf(stderr, "* Mount fs [4, after roll forward complete]\n");
	if (rump_sys_mount(MOUNT_LFS, MP, 0, &args, sizeof(args)) == -1)
		atf_tc_fail_errno("rump_sys_mount failed [4]");

	if (check_file(UNCHANGED_CONTROL, CHUNKSIZE) != 0)
		atf_tc_fail("Unchanged control file differs(!)");

	if (check_file(TO_BE_APPENDED, 2 * CHUNKSIZE) != 0)
		atf_tc_fail("Appended file differs");

	if (rump_sys_access(NEWLY_CREATED, F_OK) != 0)
		atf_tc_fail("Newly added file missing");

	if (check_file(NEWLY_CREATED, CHUNKSIZE) != 0)
		atf_tc_fail("Newly added file differs");

	if (rump_sys_access(TO_BE_DELETED, F_OK) == 0)
		atf_tc_fail("Removed file still present");

	/* Umount filesystem */
	rump_sys_unmount(MP, 0);

	/* Final fsck to double check */
	if (fsck()) {
		fprintf(stderr, "*** FAILED FSCK ***\n");
		atf_tc_fail("fsck found errors after final unmount");
	}
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
		b = ((unsigned)(size + i)) & 0xff;
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
		if (b != (((unsigned)i) & 0xff)) {
			fprintf(stderr, "%s: byte %d: expected %x found %x\n",
				filename, i, ((unsigned)(i)) & 0xff, b);
			rump_sys_close(fd);
			return 3;
		}
	}
	rump_sys_close(fd);
	fprintf(stderr, "%s: no problem\n", filename);
	return 0;
}

/* Run a file system consistency check */
int fsck(void)
{
	char s[MAXLINE];
	int i, errors = 0;
	FILE *pipe;
	char cmd[MAXLINE];

	for (i = 0; i < 2; i++) {
		sprintf(cmd, "fsck_lfs -n -b %jd -f " IMGNAME,
			(intmax_t)sbaddr[i]);
		pipe = popen(cmd, "r");
		while (fgets(s, MAXLINE, pipe) != NULL) {
			if (isdigit((int)s[0])) /* "5 files ... " */
				continue;
			if (isspace((int)s[0]) || s[0] == '*')
				continue;
			if (strncmp(s, "Alternate", 9) == 0)
				continue;
			if (strncmp(s, "ROLL ", 5) == 0)
				continue;
			fprintf(stderr, "FSCK[sb@%lld]: %s", sbaddr[i], s);
			++errors;
		}
		pclose(pipe);
		if (errors) {
			break;
		}
	}

	return errors;
}
