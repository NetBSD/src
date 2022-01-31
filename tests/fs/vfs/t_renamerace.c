/*	$NetBSD: t_renamerace.c,v 1.44 2022/01/31 17:23:37 ryo Exp $	*/

/*
 * Modified for rump and atf from a program supplied
 * by Nicolas Joly in kern/40948
 */

#include <sys/types.h>
#include <sys/mount.h>
#include <sys/utsname.h>

#include <atf-c.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <rump/rump.h>
#include <rump/rump_syscalls.h>

/* Bump the size of the test file system image to a larger value.
 *
 * These tests cause a lot of churn in the file system by creating and
 * deleting files/directories in quick succession.  A faster CPU will cause
 * more churn because the tests are capped by a run time period in seconds,
 * not number of operations.
 *
 * This is all fine except for LFS, because the lfs_cleanerd cannot keep up
 * with the churn and thus causes the test to fail on fast machines.  Hence
 * the reason for this hack. */
#define FSTEST_IMGSIZE (50000 * 512)

#include "../common/h_fsmacros.h"
#include "h_macros.h"

static volatile int quittingtime;
pid_t wrkpid;

static void *
w1(void *arg)
{
	int fd;

	rump_pub_lwproc_newlwp(wrkpid);

	while (!quittingtime) {
		fd = rump_sys_open("rename.test1",
		    O_WRONLY|O_CREAT|O_TRUNC, 0666);
		if (fd == -1 && errno != EEXIST)
			atf_tc_fail_errno("create");
		rump_sys_unlink("rename.test1");
		rump_sys_close(fd);
	}

	return NULL;
}

static void *
w1_dirs(void *arg)
{

	rump_pub_lwproc_newlwp(wrkpid);

	while (!quittingtime) {
		if (rump_sys_mkdir("rename.test1", 0777) == -1)
			atf_tc_fail_errno("mkdir");
		rump_sys_rmdir("rename.test1");
	}

	return NULL;
}

static void *
w2(void *arg)
{

	rump_pub_lwproc_newlwp(wrkpid);

	while (!quittingtime) {
		rump_sys_rename("rename.test1", "rename.test2");
	}

	return NULL;
}

static void
w3_mkdir(void)
{

	if (rump_sys_mkdir("c", 0777) == -1)
		ATF_CHECK_MSG(errno == ENOENT || errno == EEXIST,
		    "mkdir: %s (errno=%d)\n", strerror(errno), errno);
	if (rump_sys_mkdir("c/d", 0777) == -1)
		ATF_CHECK_MSG(errno == ENOENT || errno == EEXIST,
		    "mkdir: %s (errno=%d)\n", strerror(errno), errno);
	if (rump_sys_mkdir("c/d/e", 0777) == -1)
		ATF_CHECK_MSG(errno == ENOENT || errno == EEXIST,
		    "mkdir: %s (errno=%d)\n", strerror(errno), errno);
}

static void *
w3_rmdir(void *arg)
{

	rump_pub_lwproc_newlwp(wrkpid);

	while (!quittingtime) {
		w3_mkdir();
		rump_sys_rmdir("c/d/e");
		rump_sys_rmdir("c/d");
	}

	return NULL;
}

static void *
w3_rename1(void *arg)
{

	rump_pub_lwproc_newlwp(wrkpid);

	while (!quittingtime) {
		w3_mkdir();
		rump_sys_rename("c", "c/d/e");
	}

	return NULL;
}

static void *
w3_rename2(void *arg)
{

	rump_pub_lwproc_newlwp(wrkpid);

	while (!quittingtime) {
		w3_mkdir();
		rump_sys_rename("c/d/e", "c");
	}

	return NULL;
}

#define NWRK 8
static void
renamerace(const atf_tc_t *tc, const char *mp)
{
	pthread_t pt1[NWRK], pt2[NWRK];
	int i;

	/*
	 * Sysvbfs supports only 8 inodes so this test would exhaust
	 * the inode table and creating files would fail with ENOSPC.
	 */
	if (FSTYPE_SYSVBFS(tc))
		atf_tc_skip("filesystem has not enough inodes");
	if (FSTYPE_RUMPFS(tc))
		atf_tc_skip("rename not supported by file system");
	if (FSTYPE_UDF(tc))
		atf_tc_expect_fail("PR kern/53865");

	RZ(rump_pub_lwproc_rfork(RUMP_RFCFDG));
	RL(wrkpid = rump_sys_getpid());

	RL(rump_sys_chdir(mp));
	for (i = 0; i < NWRK; i++)
		pthread_create(&pt1[i], NULL, w1, NULL);

	for (i = 0; i < NWRK; i++)
		pthread_create(&pt2[i], NULL, w2, NULL);

	sleep(5);
	quittingtime = 1;

	for (i = 0; i < NWRK; i++)
		pthread_join(pt1[i], NULL);
	for (i = 0; i < NWRK; i++)
		pthread_join(pt2[i], NULL);
	RL(rump_sys_chdir("/"));

	if (FSTYPE_UDF(tc))
		atf_tc_fail("race did not trigger this time");
}

static void
renamerace_dirs(const atf_tc_t *tc, const char *mp)
{
	pthread_t pt1, pt2;

	if (FSTYPE_SYSVBFS(tc))
		atf_tc_skip("directories not supported by file system");
	if (FSTYPE_RUMPFS(tc))
		atf_tc_skip("rename not supported by file system");
	if (FSTYPE_UDF(tc))
		atf_tc_expect_fail("PR kern/53865");

	RZ(rump_pub_lwproc_rfork(RUMP_RFCFDG));
	RL(wrkpid = rump_sys_getpid());

	RL(rump_sys_chdir(mp));
	pthread_create(&pt1, NULL, w1_dirs, NULL);
	pthread_create(&pt2, NULL, w2, NULL);

	sleep(5);
	quittingtime = 1;

	pthread_join(pt1, NULL);
	pthread_join(pt2, NULL);
	RL(rump_sys_chdir("/"));

	if (FSTYPE_UDF(tc))
		atf_tc_fail("race did not trigger this time");
}

static void
renamerace_cycle(const atf_tc_t *tc, const char *mp)
{
	pthread_t pt_rmdir, pt_rename1, pt_rename2;

	if (FSTYPE_SYSVBFS(tc))
		atf_tc_skip("directories not supported by file system");
	if (FSTYPE_RUMPFS(tc))
		atf_tc_skip("rename not supported by file system");
	if (FSTYPE_P2K_FFS(tc))
		atf_tc_expect_fail("assertion \"vp->v_size == ip->i_size\" failed");
	if (FSTYPE_PUFFS(tc))
		atf_tc_expect_fail("assertion \"dfd\" failed");
	if (FSTYPE_NFS(tc))
		atf_tc_expect_fail("mkdir fails with ESTALE");
	if (FSTYPE_UDF(tc))
		atf_tc_expect_fail("sometimes fails with ENOSPC, PR kern/56253");

	RZ(rump_pub_lwproc_rfork(RUMP_RFCFDG));
	RL(wrkpid = rump_sys_getpid());

	RL(rump_sys_chdir(mp));
	pthread_create(&pt_rmdir, NULL, w3_rmdir, NULL);
	pthread_create(&pt_rename1, NULL, w3_rename1, NULL);
	pthread_create(&pt_rename2, NULL, w3_rename2, NULL);

	sleep(10);
	quittingtime = 1;

	alarm(5);
	pthread_join(pt_rmdir, NULL);
	pthread_join(pt_rename1, NULL);
	pthread_join(pt_rename2, NULL);
	alarm(0);
	RL(rump_sys_chdir("/"));

	if (FSTYPE_UDF(tc))
		atf_tc_fail("PR kern/56253 did not trigger this time");
	if (FSTYPE_P2K_FFS(tc))
		atf_tc_fail("did not fail this time");
	if (FSTYPE_PUFFS(tc))
		atf_tc_fail("did not fail this time");
	if (FSTYPE_NFS(tc))
		atf_tc_fail("did not fail this time");
}

ATF_TC_FSAPPLY(renamerace, "rename(2) race with file unlinked mid-operation");
ATF_TC_FSAPPLY(renamerace_dirs, "rename(2) race with directories");
ATF_TC_FSAPPLY(renamerace_cycle, "rename(2) race making directory cycles");

ATF_TP_ADD_TCS(tp)
{

	ATF_TP_FSAPPLY(renamerace); /* PR kern/41128 */
	ATF_TP_FSAPPLY(renamerace_dirs);
	ATF_TP_FSAPPLY(renamerace_cycle);

	return atf_no_error();
}
