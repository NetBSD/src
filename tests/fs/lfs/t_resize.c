/*	$NetBSD: t_resize.c,v 1.1 2025/10/21 04:25:31 perseant Exp $	*/

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

/* Debugging conditions */
/* #define FORCE_SUCCESS */ /* Don't actually revert, everything worked */
/* #define USE_DUMPLFS */ /* Dump the filesystem at certain steps */

#define UNCHANGED_CONTROL MP "/3-a-random-file"
#define BIGSIZE 15000
#define SMALLSIZE 10000
__CTASSERT(BIGSIZE > SMALLSIZE);

/* Resize filesystem */
void resize(int, size_t);

/* Actually run the test */
void test(int);

ATF_TC(resize32);
ATF_TC_HEAD(resize32, tc)
{
	atf_tc_set_md_var(tc, "descr",
		"LFS32 resize_lfs creates an inconsistent filesystem");
	atf_tc_set_md_var(tc, "timeout", "20");
}

ATF_TC(resize64);
ATF_TC_HEAD(resize64, tc)
{
	atf_tc_set_md_var(tc, "descr",
		"LFS64 resize_lfs creates an inconsistent filesystem");
	atf_tc_set_md_var(tc, "timeout", "20");
}

ATF_TC_BODY(resize32, tc)
{
	test(32);
}

ATF_TC_BODY(resize64, tc)
{
	test(64);
}

void test(int width)
{
	struct ufs_args args;
	int fd;

	setvbuf(stdout, NULL, _IONBF, 0);

	/*
	 * Initialize.
	 */

	/* Create image file larger than filesystem */
	create_lfs(BIGSIZE, SMALLSIZE, width, 1);

	/* Mount filesystem */
	fprintf(stderr, "* Mount fs [1]\n");
	memset(&args, 0, sizeof(args));
	args.fspec = __UNCONST(FAKEBLK);
	if (rump_sys_mount(MOUNT_LFS, MP, 0, &args, sizeof(args)) == -1)
		atf_tc_fail_errno("rump_sys_mount failed");

	/* Payload */
	fprintf(stderr, "* Initial payload\n");
	write_file(UNCHANGED_CONTROL, CHUNKSIZE, 1);

	/* Unmount */
	rump_sys_unmount(MP, 0);
	if (fsck())
		atf_tc_fail_errno("fsck found errors after first unmount");

	/*
	 * Remount and resize.
	 */

	/* Reconfigure and mount filesystem again */
	fprintf(stderr, "* Remount fs [2, to enlarge]\n");
	if (rump_sys_mount(MOUNT_LFS, MP, 0, &args, sizeof(args)) == -1)
		atf_tc_fail_errno("rump_sys_mount failed [2]");

	/* Get a handle to the root of the file system */
	fd = rump_sys_open(MP, O_RDONLY);
	if (fd < 0)
		atf_tc_fail_errno("rump_sys_open mount point root failed");

	/* Enlarge filesystem */
	fprintf(stderr, "* Resize (enlarge)\n");
	resize(fd, BIGSIZE);

	/* Unmount fs and check */
	rump_sys_close(fd);
	rump_sys_unmount(MP, 0);
	if (fsck())
		atf_tc_fail_errno("fsck found errors after enlarge");

	/* Mount filesystem for shrink */
	fprintf(stderr, "* Mount fs [3, to shrink]\n");
	if (rump_sys_mount(MOUNT_LFS, MP, 0, &args, sizeof(args)) == -1)
		atf_tc_fail_errno("rump_sys_mount failed [3]");

	/* Get a handle to the root of the file system */
	fd = rump_sys_open(MP, O_RDONLY);
	if (fd < 0)
		atf_tc_fail_errno("rump_sys_open mount point root failed");

	/* Shrink filesystem */
	fprintf(stderr, "* Resize (shrink)\n");
	resize(fd, SMALLSIZE);

	/* Unmount and check again */
	rump_sys_close(fd);
	if (rump_sys_unmount(MP, 0) != 0)
		atf_tc_fail_errno("rump_sys_umount failed after shrink");
	fprintf(stderr, "* Fsck after shrink\n");
	if (fsck())
		atf_tc_fail("fsck found errors after shrink");

	/*
	 * Check file system contents
	 */

	/* Mount filesystem one last time */
	fprintf(stderr, "* Mount fs [4, to check contents]\n");
	if (rump_sys_mount(MOUNT_LFS, MP, 0, &args, sizeof(args)) == -1)
		atf_tc_fail_errno("rump_sys_mount failed [4]");

	if (check_file(UNCHANGED_CONTROL, CHUNKSIZE) != 0)
		atf_tc_fail("Unchanged control file differs(!)");

	/* Umount filesystem */
	rump_sys_unmount(MP, 0);

	/* Final fsck to double check */
	fprintf(stderr, "* Fsck after final unmount\n");
	if (fsck())
		atf_tc_fail("fsck found errors after final unmount");
}

ATF_TP_ADD_TCS(tp)
{

	ATF_TP_ADD_TC(tp, resize32);
	ATF_TP_ADD_TC(tp, resize64);
	return atf_no_error();
}

void
resize(int fd, size_t size)
{
	int newnseg = (size * DEV_BSIZE) / SEGSIZE;
	
	if (rump_sys_fcntl(fd, LFCNRESIZE, &newnseg) != 0)
		atf_tc_fail_errno("LFCNRESIZE failed");
}
