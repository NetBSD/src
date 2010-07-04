/*	$NetBSD: t_renamerace.c,v 1.7 2010/07/04 12:43:23 pooka Exp $	*/

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

#include <fs/tmpfs/tmpfs_args.h>

#include "../../h_macros.h"

ATF_TC(renamerace);
ATF_TC_HEAD(renamerace, tc)
{
	atf_tc_set_md_var(tc, "descr", "rename(2) race against files "
	    "unlinked mid-operation, kern/41128");
}

static void *
w1(void *arg)
{
	int fd;

	for (;;) {
		fd = rump_sys_open("/rename.test1",
		    O_WRONLY|O_CREAT|O_TRUNC, 0666);
		rump_sys_unlink("/rename.test1");
		rump_sys_close(fd);
	}
	return NULL;
}

static void *
w2(void *arg)
{

	for (;;) {
		rump_sys_rename("/rename.test1", "/rename.test2");
	}
	return NULL;
}

ATF_TC_BODY(renamerace, tc)
{
	struct tmpfs_args args;
	pthread_t pt1, pt2;

	memset(&args, 0, sizeof(args));
	args.ta_version = TMPFS_ARGS_VERSION;
	args.ta_root_mode = 0777;

	rump_init();
	if (rump_sys_mount(MOUNT_TMPFS, "/", 0, &args, sizeof(args)) == -1)
		atf_tc_fail_errno("could not mount tmpfs");

	pthread_create(&pt1, NULL, w1, NULL);
	pthread_create(&pt2, NULL, w2, NULL);

	sleep(10);
}

ATF_TC(renamerace2);
ATF_TC_HEAD(renamerace2, tc)
{
	atf_tc_set_md_var(tc, "descr", "rename(2) lock order inversion");
	atf_tc_set_md_var(tc, "timeout", "6");
}

static volatile int quittingtime = 0;

static void *
r2w1(void *arg)
{
	int fd;

	rump_pub_lwp_alloc_and_switch(0, 0);

	fd = rump_sys_open("/file", O_CREAT | O_RDWR, 0777);
	if (fd == -1)
		atf_tc_fail_errno("creat");
	rump_sys_close(fd);

	while (!quittingtime) {
		if (rump_sys_rename("/file", "/dir/file") == -1)
			atf_tc_fail_errno("rename 1");
		if (rump_sys_rename("/dir/file", "/file") == -1)
			atf_tc_fail_errno("rename 2");
	}

	return NULL;
}

static void *
r2w2(void *arg)
{
	int fd;

	rump_pub_lwp_alloc_and_switch(0, 0);

	while (!quittingtime) {
		fd = rump_sys_open("/dir/file1", O_RDWR);
		if (fd != -1)
			rump_sys_close(fd);
	}

	return NULL;
}

ATF_TC_BODY(renamerace2, tc)
{
	struct tmpfs_args args;
	struct utsname un;
	pthread_t pt[2];

	/*
	 * Check that we are running on an SMP-capable arch.  It should
	 * be a rump capability, but after the CPU_INFO_FOREACH is
	 * fixed, it will be every arch (for rump), so don't bother.
	 */
	if (uname(&un) == -1)
		atf_tc_fail_errno("uname");
	if (strcmp(un.machine, "i386") != 0 && strcmp(un.machine, "amd64") != 0)
		atf_tc_skip("i386 or amd64 required (have %s)", un.machine);

	/*
	 * Force SMP regardless of how many host CPUs there are.
	 * Deadlock is highly unlikely to trigger otherwise.
	 */
	setenv("RUMP_NCPU", "2", 1);

	rump_init();
	memset(&args, 0, sizeof(args));
	args.ta_version = TMPFS_ARGS_VERSION;
	args.ta_root_mode = 0777;
	if (rump_sys_mount(MOUNT_TMPFS, "/", 0, &args, sizeof(args)) == -1)
		atf_tc_fail_errno("could not mount tmpfs");

	if (rump_sys_mkdir("/dir", 0777) == -1)
		atf_tc_fail_errno("cannot create directory");

	pthread_create(&pt[0], NULL, r2w1, NULL);
	pthread_create(&pt[1], NULL, r2w2, NULL);

	/* usually triggers in <<1s for me */
	sleep(4);
	quittingtime = 1;

	atf_tc_expect_timeout("PR kern/36681");

	pthread_join(pt[0], NULL);
	pthread_join(pt[1], NULL);
}

ATF_TP_ADD_TCS(tp)
{
	ATF_TP_ADD_TC(tp, renamerace);
	ATF_TP_ADD_TC(tp, renamerace2);

	return atf_no_error();
}
