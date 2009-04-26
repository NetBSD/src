/*	$NetBSD: t_renamerace.c,v 1.6 2009/04/26 15:15:38 pooka Exp $	*/

/*
 * Modified for rump and atf from a program supplied
 * by Nicolas Joly in kern/40948
 */

#include <sys/types.h>
#include <sys/mount.h>

#include <atf-c.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

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

ATF_TP_ADD_TCS(tp)
{
	ATF_TP_ADD_TC(tp, renamerace);
	return 0; /*XXX?*/
}
