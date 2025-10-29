/*	$NetBSD: t_fcntl.c,v 1.3 2025/10/29 11:46:34 martin Exp $	*/

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
#define A_FEW_BLOCKS 6500  /* In bytes; a few blocks worth */
#define MORE_THAN_A_SEGMENT (4 * SEGSIZE) /* In bytes; > SEGSIZE */

/* Set up filesystem with a file in it */
int setup(int, struct ufs_args *, off_t);

/* Actually run the test */
void coalesce(int);
void cleanseg(int);

/* Unmount and check fsck */
void teardown(int, struct ufs_args *, off_t);

ATF_TC(coalesce32);
ATF_TC_HEAD(coalesce32, tc)
{
	atf_tc_set_md_var(tc, "descr",
		"LFS32 LFCNSCRAMBLE/LFCNREWRITEFILE");
	/* atf_tc_set_md_var(tc, "timeout", "20"); */
}

ATF_TC(coalesce64);
ATF_TC_HEAD(coalesce64, tc)
{
	atf_tc_set_md_var(tc, "descr",
		"LFS64 LFCNSCRAMBLE/LFCNREWRITEFILE");
	/* atf_tc_set_md_var(tc, "timeout", "20"); */
}

ATF_TC(cleanseg32);
ATF_TC_HEAD(cleanseg32, tc)
{
	atf_tc_set_md_var(tc, "descr",
		"LFS32 LFCNSCRAMBLE/LFCNREWRITEFILE");
	/* atf_tc_set_md_var(tc, "timeout", "20"); */
}

ATF_TC(cleanseg64);
ATF_TC_HEAD(cleanseg64, tc)
{
	atf_tc_set_md_var(tc, "descr",
		"LFS64 LFCNSCRAMBLE/LFCNREWRITEFILE");
	/* atf_tc_set_md_var(tc, "timeout", "20"); */
}

ATF_TC_BODY(coalesce32, tc)
{
	coalesce(32);
}

ATF_TC_BODY(coalesce64, tc)
{
	coalesce(64);
}

ATF_TC_BODY(cleanseg32, tc)
{
	cleanseg(32);
}

ATF_TC_BODY(cleanseg64, tc)
{
	cleanseg(64);
}

int setup(int width, struct ufs_args *argsp, off_t filesize)
{
	int fd;

	setvbuf(stdout, NULL, _IONBF, 0);

	/*
	 * Initialize.
	 */

	/* Create image file */
	create_lfs(FSSIZE, FSSIZE, width, 1);

	/* Mount filesystem */
	fprintf(stderr, "* Mount fs [1]\n");
	memset(argsp, 0, sizeof *argsp);
	argsp->fspec = __UNCONST(FAKEBLK);
	if (rump_sys_mount(MOUNT_LFS, MP, 0, argsp, sizeof *argsp) == -1)
		atf_tc_fail_errno("rump_sys_mount failed");

	/* Payload */
	fprintf(stderr, "* Initial payload\n");
	write_file(UNCHANGED_CONTROL, filesize, 1);

	/* Make the data go to disk */
	rump_sys_sync();
	rump_sys_sync();

	/* Get a handle to the root of the file system */
	fd = rump_sys_open(MP, O_RDONLY);
	if (fd < 0)
		atf_tc_fail_errno("rump_sys_open mount point root failed");

	return fd;
}

void teardown(int fd, struct ufs_args *argsp, off_t filesize)
{
	/* Close descriptor */
	rump_sys_close(fd);

	/* Unmount */
	if (rump_sys_unmount(MP, 0) < 0)
		atf_tc_fail_errno("rump_sys_unmount failed[1]");

	/* Fsck */
	fprintf(stderr, "* Fsck after final unmount\n");
	if (fsck())
		atf_tc_fail("fsck found errors after final unmount");

	/*
	 * Check file system contents
	 */

	/* Mount filesystem one last time */
	fprintf(stderr, "* Mount fs again to check contents\n");
	if (rump_sys_mount(MOUNT_LFS, MP, 0, argsp, sizeof *argsp) == -1)
		atf_tc_fail_errno("rump_sys_mount failed [4]");

	if (check_file(UNCHANGED_CONTROL, filesize) != 0)
		atf_tc_fail("Unchanged control file differs(!)");

	/* Umount filesystem */
	if (rump_sys_unmount(MP, 0) < 0)
		atf_tc_fail_errno("rump_sys_unmount failed[2]");

	/* Final fsck to double check */
	fprintf(stderr, "* Fsck after final unmount\n");
	if (fsck())
		atf_tc_fail("fsck found errors after final unmount");
}

void coalesce(int width)
{
	struct ufs_args args;
	struct stat statbuf;
	struct lfs_filestat_req fsr;
	struct lfs_filestats fss_before, fss_scrambled, fss_after;
	struct lfs_inode_array inotbl;
	int fd;
	ino_t ino;
		
	fd = setup(width, &args, A_FEW_BLOCKS);
	
	/* Get our file's inode number */
	if (rump_sys_stat(UNCHANGED_CONTROL, &statbuf) != 0)
		atf_tc_fail_errno("rump_sys_stat failed");
	ino = statbuf.st_ino;

	fprintf(stderr, "Treating inode %d\n", (int)ino);

	/* Retrieve fragmentation statistics */
	memset(&fss_before, 0, sizeof fss_before);
	memset(&fss_scrambled, 0, sizeof fss_scrambled);
	memset(&fss_after, 0, sizeof fss_after);
	
	fsr.ino = ino;
	fsr.len = 1;
	fsr.fss = &fss_before;
	if (rump_sys_fcntl(fd, LFCNFILESTATS, &fsr) < 0)
		atf_tc_fail_errno("LFCNFILESTATS[1] failed");

	inotbl.len = 1;
	inotbl.inodes = &ino;
	
	fprintf(stderr, "Start ino %d nblk %d count %d total %d\n",
		(int)ino, (int)fss_before.nblk, (int)fss_before.dc_count,
		(int)fss_before.dc_sum);

	/* Scramble */
	if (rump_sys_fcntl(fd, LFCNSCRAMBLE, &inotbl) < 0)
		atf_tc_fail_errno("LFCNSCRAMBLE failed");
	fsr.fss = &fss_scrambled;
	if (rump_sys_fcntl(fd, LFCNFILESTATS, &fsr) < 0)
		atf_tc_fail_errno("LFCNFILESTATS[2] failed");
	if (fss_scrambled.dc_count <= fss_before.dc_count)
		atf_tc_fail("Scramble did not worsen gap count");
	if (fss_scrambled.dc_sum <= fss_before.dc_sum)
		atf_tc_fail("Scramble did not worsen total gap length");
	
	fprintf(stderr, "Scrambled ino %d nblk %d count %d total %d\n",
		(int)ino, (int)fss_scrambled.nblk, (int)fss_scrambled.dc_count,
		(int)fss_scrambled.dc_sum);

	/* Coalesce */
	if (rump_sys_fcntl(fd, LFCNREWRITEFILE, &inotbl) < 0)
		atf_tc_fail_errno("LFCNREWRITEFILE failed");
	fsr.fss = &fss_after;
	if (rump_sys_fcntl(fd, LFCNFILESTATS, &fsr) < 0)
		atf_tc_fail_errno("LFCNFILESTATS[3] failed");
	if (fss_scrambled.dc_count <= fss_after.dc_count)
		atf_tc_fail("Rewrite did not improve gap count");
	if (fss_scrambled.dc_sum <= fss_after.dc_sum)
		atf_tc_fail("Rewrite did not improve total gap length");

	fprintf(stderr, "Coalesced ino %d nblk %d count %d total %d\n",
		(int)ino, (int)fss_after.nblk, (int)fss_after.dc_count,
		(int)fss_after.dc_sum);

	teardown(fd, &args, A_FEW_BLOCKS);
}

void cleanseg(int width)
{
	struct ufs_args args;
	struct lfs_segnum_array sna;
	struct lfs_seguse_array sua;
	int fd, sn;
		
	fd = setup(width, &args, MORE_THAN_A_SEGMENT);
	rump_sys_sync();

	sua.len = LFS_SEGUSE_MAXCNT;
	sua.start = 0;
	sua.seguse = malloc(LFS_SEGUSE_MAXCNT * sizeof(*sua.seguse));
	if (rump_sys_fcntl(fd, LFCNSEGUSE, &sua) < 0)
		atf_tc_fail_errno("LFCNSEGUSE[1] failed");

	for (sn = 0; sn < (int)sua.len; ++sn) {
		if (sua.seguse[sn].su_nbytes == 0)
			continue;
		if ((sua.seguse[sn].su_flags & (SEGUSE_DIRTY | SEGUSE_ACTIVE))
		    != SEGUSE_DIRTY)
			continue;
		break;
	}
	if (sn == (int)sua.len)
		atf_tc_fail("No segments found to clean");
	
	fprintf(stderr, "* Cleaning segment #%d\n", sn);

	memset(&sna, 0, sizeof sna);
	sna.len = 1;
	sna.segments = &sn;
	
	if (rump_sys_fcntl(fd, LFCNREWRITESEGS, &sna) < 0)
		atf_tc_fail_errno("LFCNREWRITESEGS failed");
	rump_sys_sync();
	rump_sys_sync();

	sua.len = 1;
	sua.start = sn;
	if (rump_sys_fcntl(fd, LFCNSEGUSE, &sua) < 0)
		atf_tc_fail_errno("LFCNSEGUSE[2] failed");

	if (sua.seguse[0].su_nbytes > 0)
		atf_tc_fail("Empty bytes after clean");

	teardown(fd, &args, MORE_THAN_A_SEGMENT);
}

ATF_TP_ADD_TCS(tp)
{

	ATF_TP_ADD_TC(tp, coalesce32);
	ATF_TP_ADD_TC(tp, coalesce64);
	ATF_TP_ADD_TC(tp, cleanseg32);
	ATF_TP_ADD_TC(tp, cleanseg64);
	return atf_no_error();
}
