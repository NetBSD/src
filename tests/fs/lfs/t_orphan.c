/*	$NetBSD: t_orphan.c,v 1.4 2025/12/19 20:58:08 perseant Exp $	*/

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
#include <string.h>
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

#define UNCHANGED_CONTROL MP "/3-a-random-file"
#define FSSIZE 10000     /* In sectors */
#define FILE_SIZE 65000  /* In bytes; a few blocks worth */
#define TMPFILE "filehandle"

/* Actually run the test */
void orphan(int);

ATF_TC(orphan32);
ATF_TC_HEAD(orphan32, tc)
{
	atf_tc_set_md_var(tc, "descr",
		"LFS32 orphan removal");
	atf_tc_set_md_var(tc, "timeout", "20");
}

ATF_TC(orphan64);
ATF_TC_HEAD(orphan64, tc)
{
	atf_tc_set_md_var(tc, "descr",
		"LFS64 orphan removal");
	atf_tc_set_md_var(tc, "timeout", "20");
}

ATF_TC_BODY(orphan32, tc)
{
	orphan(32);
}

ATF_TC_BODY(orphan64, tc)
{
	orphan(64);
}

void orphan(int width)
{
	char s[MAXLINE];
	struct ufs_args args;
	struct stat statbuf;
	int fd, status;
	pid_t childpid;
	FILE *fp;
	int thisinum, version, found;
	ino_t inum;

	setvbuf(stdout, NULL, _IONBF, 0);

	/*
	 * Initialize.
	 */

	/* Create image file */
	create_lfs(FSSIZE, FSSIZE, width, 0);

	/* Prepare to mount */
	memset(&args, 0, sizeof args);
	args.fspec = __UNCONST(FAKEBLK);

	if ((childpid = fork()) == 0) {
		/* Set up rump */
		rump_init();
		if (rump_sys_mkdir(MP, 0777) == -1)
			atf_tc_fail_errno("cannot create mountpoint");
		rump_pub_etfs_register(FAKEBLK, IMGNAME, RUMP_ETFS_BLK);

		/* Mount filesystem */
		fprintf(stderr, "* Mount fs [1]\n");
		if (rump_sys_mount(MOUNT_LFS, MP, 0, &args, sizeof args) == -1)
			atf_tc_fail_errno("rump_sys_mount failed");

		/* Payload */
		fprintf(stderr, "* Initial payload\n");
		fd = write_file(UNCHANGED_CONTROL, FILE_SIZE, 0, 0);

		/* Make the data go to disk */
		rump_sys_sync();
		rump_sys_sync();

		/* Write the inode number into a temporary file */
		rump_sys_fstat(fd, &statbuf);
		fprintf(stderr, "Inode is => %d <=\n", (int)statbuf.st_ino);

		fp = fopen(TMPFILE, "wb");
		fwrite(&statbuf.st_ino, sizeof(ino_t), 1, fp);
		fclose(fp);

		/* Delete while still open */
		if (rump_sys_unlink(UNCHANGED_CONTROL) != 0)
			atf_tc_fail_errno("rump_sys_unlink failed");

		/* Sanity check values */
		if (statbuf.st_size != FILE_SIZE)
			atf_tc_fail("wrong size in initial stat");
		if (statbuf.st_nlink <= 0)
			atf_tc_fail("file already deleted");
		if (statbuf.st_blocks <= 0)
			atf_tc_fail("no blocks");

		/* Make the data go to disk */
		rump_sys_sync();
		rump_sys_sync();

		/* Simulate a system crash */
		exit(0);
	}

	/* Wait for child to terminate */
	waitpid(childpid, &status, 0);

	/* If it died, die ourselves */
	if (WEXITSTATUS(status))
		exit(WEXITSTATUS(status));

	/* Fsck */
	fprintf(stderr, "* Fsck after crash\n");
	if (fsck())
		atf_tc_fail("fsck found errors after crash");

	/* Read the inode number from temporary file */
	fp = fopen(TMPFILE, "rb");
	fread(&inum, sizeof(ino_t), 1, fp);
	fclose(fp);

	fprintf(stderr, "Seeking inum => %d <=\n", (int)inum);

	/* Set up rump */
	rump_init();
	if (rump_sys_mkdir(MP, 0777) == -1)
		atf_tc_fail_errno("cannot create mountpoint");
	rump_pub_etfs_register(FAKEBLK, IMGNAME, RUMP_ETFS_BLK);

	/* Remount */
	fprintf(stderr, "* Mount fs [2]\n");
	if (rump_sys_mount(MOUNT_LFS, MP, 0, &args, sizeof args) == -1)
		atf_tc_fail_errno("rump_sys_mount failed[2]");

	/* At this point the orphan should be deleted. */

	/* Unmount */
	fprintf(stderr, "* Unmount\n");
	if (rump_sys_unmount(MP, 0) != 0)
		atf_tc_fail_errno("rump_sys_unmount failed[1]");

	/* Check that it was in fact deleted. */
	fprintf(stderr, "* Check for orphaned file\n");
	found = 0;
	fp = popen("dumplfs -i -s9 ./" IMGNAME, "r");
	while (fgets(s, MAXLINE, fp) != NULL) {
		if (sscanf(s, "%d FREE %d", &thisinum, &version) == 2
		    && (ino_t)thisinum == inum) {
			found = 1;
			break;
		}
	}
	fclose(fp);
	if (!found)
		atf_tc_fail("orphaned inode not freed on subsequent mount");

	/* Fsck */
	fprintf(stderr, "* Fsck after final unmount\n");
	if (fsck())
		atf_tc_fail("fsck found errors after final unmount");
}

ATF_TP_ADD_TCS(tp)
{

	ATF_TP_ADD_TC(tp, orphan32);
	ATF_TP_ADD_TC(tp, orphan64);
	return atf_no_error();
}

