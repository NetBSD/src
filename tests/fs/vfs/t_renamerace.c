/*	$NetBSD: t_renamerace.c,v 1.9 2010/08/25 18:11:20 pooka Exp $	*/

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

#include "../common/h_fsmacros.h"
#include "../../h_macros.h"

static volatile int quittingtime;

static void *
w1(void *arg)
{
	int fd;

	rump_pub_lwp_alloc_and_switch(0, 10);
	rump_sys_chdir(arg);

	while (!quittingtime) {
		fd = rump_sys_open("rename.test1",
		    O_WRONLY|O_CREAT|O_TRUNC, 0666);
		if (fd == -1 && errno != EEXIST)
			atf_tc_fail_errno("create");
		rump_sys_unlink("rename.test1");
		rump_sys_close(fd);
	}

	rump_sys_chdir("/");

	return NULL;
}

static void *
w1_dirs(void *arg)
{

	rump_pub_lwp_alloc_and_switch(0, 10);
	rump_sys_chdir(arg);

	while (!quittingtime) {
		if (rump_sys_mkdir("rename.test1", 0777) == -1)
			atf_tc_fail_errno("mkdir");
		rump_sys_rmdir("rename.test1");
	}

	rump_sys_chdir("/");

	return NULL;
}

static void *
w2(void *arg)
{

	rump_pub_lwp_alloc_and_switch(0, 11);
	rump_sys_chdir(arg);

	while (!quittingtime) {
		rump_sys_rename("rename.test1", "rename.test2");
	}

	rump_sys_chdir("/");

	return NULL;
}

#define NWRK 8
static void
renamerace(const atf_tc_t *tc, const char *mp)
{
	pthread_t pt1[NWRK], pt2[NWRK];
	int i;

	if (FSTYPE_LFS(tc))
		atf_tc_expect_signal(-1, "PR kern/43582");

	if (FSTYPE_MSDOS(tc))
		atf_tc_skip("test fails in some setups, reason unknown");

	for (i = 0; i < NWRK; i++)
		pthread_create(&pt1[i], NULL, w1, __UNCONST(mp));

	for (i = 0; i < NWRK; i++)
		pthread_create(&pt2[i], NULL, w2, __UNCONST(mp));

	sleep(5);
	quittingtime = 1;

	for (i = 0; i < NWRK; i++)
		pthread_join(pt1[i], NULL);
	for (i = 0; i < NWRK; i++)
		pthread_join(pt2[i], NULL);

	/*
	 * XXX: does not always fail on LFS, especially for unicpu
	 * configurations.  see other ramblings about racy tests.
	 */
	if (FSTYPE_LFS(tc))
		abort();

	/*
	 * NFS sillyrename is broken and may linger on in the file system.
	 * This sleep lets them finish so we don't get transient unmount
	 * failures.
	 */
	if (FSTYPE_NFS(tc))
		sleep(1);
}

static void
renamerace_dirs(const atf_tc_t *tc, const char *mp)
{
	pthread_t pt1, pt2;

	if (FSTYPE_SYSVBFS(tc))
		atf_tc_skip("directories not supported");

	/* XXX: msdosfs also sometimes hangs */
	if (FSTYPE_FFS(tc) || FSTYPE_EXT2FS(tc) || FSTYPE_LFS(tc) ||
	    FSTYPE_MSDOS(tc))
		atf_tc_expect_signal(-1, "PR kern/43626");

	pthread_create(&pt1, NULL, w1_dirs, __UNCONST(mp));
	pthread_create(&pt2, NULL, w2, __UNCONST(mp));

	sleep(5);
	quittingtime = 1;

	pthread_join(pt1, NULL);
	pthread_join(pt2, NULL);

	/*
	 * Doesn't always trigger when run on a slow backend
	 * (i.e. not on tmpfs/mfs).  So do the usual kludge.
	 */
	if (FSTYPE_FFS(tc) || FSTYPE_EXT2FS(tc) || FSTYPE_LFS(tc) ||
	    FSTYPE_MSDOS(tc))
		abort();
}

ATF_TC_FSAPPLY(renamerace, "rename(2) race with file unlinked mid-operation");
ATF_TC_FSAPPLY(renamerace_dirs, "rename(2) race with directories");

ATF_TP_ADD_TCS(tp)
{

	ATF_TP_FSAPPLY(renamerace); /* PR kern/41128 */
	ATF_TP_FSAPPLY(renamerace_dirs);

	return atf_no_error();
}
