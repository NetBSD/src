/*	$NetBSD: t_orphan.c,v 1.2 2025/10/30 15:30:17 perseant Exp $	*/

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
	struct ufs_args args;
	struct stat statbuf;
	int status;
	pid_t childpid;
	size_t fh_size;
	void *fhp;
	FILE *fp;
		
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
		write_file(UNCHANGED_CONTROL, FILE_SIZE, 0, 0);

		/* Discover filehandle size and allocate space */
		fh_size = 0;
		if (rump_sys_getfh(UNCHANGED_CONTROL, NULL, &fh_size) != 0
		    && errno != E2BIG)
			atf_tc_fail_errno("getfh failed to report fh_size");
		fprintf(stderr, "fh_size = %zd\n", fh_size);
		fhp = malloc(fh_size);
		/* Get a filehandle for our file */
		if (rump_sys_getfh(UNCHANGED_CONTROL, fhp, &fh_size) != 0)
			atf_tc_fail_errno("rump_sys_getfh failed");

		/* Write the filehandle into a temporary file */
		fp = fopen(TMPFILE, "wb");
		fwrite(fhp, fh_size, 1, fp);
		fclose(fp);
		
		/* Make the data go to disk */
		rump_sys_sync();
		rump_sys_sync();

		/* Stat using a file handle and check */
		if (rump_sys_fhstat(fhp, fh_size, &statbuf) != 0)
			atf_tc_fail_errno("fhstat failed");

		/* Sanity check values */
		if (statbuf.st_size != FILE_SIZE)
			atf_tc_fail("wrong size in initial stat");
		if (statbuf.st_nlink <= 0)
			atf_tc_fail("file already deleted");
		if (statbuf.st_blocks <= 0)
			atf_tc_fail("no blocks");
	
		/* Simulate a system crash */
		exit(0);
	}

	/* Wait for child to terminate */
	waitpid(childpid, &status, 0);

	/* If it died, die ourselves */
	if (WEXITSTATUS(status))
		exit(WEXITSTATUS(status));

	/* Read the file handle from temporary file */
	stat(TMPFILE, &statbuf);
	fh_size = statbuf.st_size;
	fhp = malloc(fh_size);
	fp = fopen(TMPFILE, "wb");
	fread(fhp, fh_size, 1, fp);
	fclose(fp);
	
	/* Set up rump */
	rump_init();
	if (rump_sys_mkdir(MP, 0777) == -1)
		atf_tc_fail_errno("cannot create mountpoint");
	rump_pub_etfs_register(FAKEBLK, IMGNAME, RUMP_ETFS_BLK);

	/* Remount */
	fprintf(stderr, "* Mount fs [2]\n");
	if (rump_sys_mount(MOUNT_LFS, MP, 0, &args, sizeof args) == -1)
		atf_tc_fail_errno("rump_sys_mount failed[2]");

	/*
	 * At this point the orphans should be deleted.
	 * Check that our orphaned file in fact was.
	 */
	
	/* Stat using a file handle and check */
	if (rump_sys_fhstat(fhp, fh_size, &statbuf) == 0)
		atf_tc_fail_errno("orphaned file not deleted");
	
	/* Unmount */
	if (rump_sys_unmount(MP, 0) != 0)
		atf_tc_fail_errno("rump_sys_unmount failed[1]");

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
