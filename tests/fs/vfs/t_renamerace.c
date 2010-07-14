/*	$NetBSD: t_renamerace.c,v 1.1 2010/07/14 21:39:31 pooka Exp $	*/

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
		if (fd == -1)
			atf_tc_fail_errno("create");
		rump_sys_unlink("rename.test1");
		rump_sys_close(fd);
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

static void
renamerace(const atf_tc_t *tc, const char *mp)
{
	pthread_t pt1, pt2;

	pthread_create(&pt1, NULL, w1, __UNCONST(mp));
	pthread_create(&pt2, NULL, w2, __UNCONST(mp));

	sleep(5);
	quittingtime = 1;

	pthread_join(pt1, NULL);
	pthread_join(pt2, NULL);
}

ATF_TC_FSAPPLY(renamerace, "rename(2) race with file unlinked mid-operation");

ATF_TP_ADD_TCS(tp)
{

	ATF_TP_FSAPPLY(renamerace); /* PR kern/41128 */

	return atf_no_error();
}
