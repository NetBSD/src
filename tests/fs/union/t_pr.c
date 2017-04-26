/*	$NetBSD: t_pr.c,v 1.8.24.2 2017/04/26 02:53:32 pgoyette Exp $	*/

#include <sys/types.h>
#include <sys/mount.h>

#include <atf-c.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#include <rump/rump.h>
#include <rump/rump_syscalls.h>

#include <miscfs/union/union.h>

#include "h_macros.h"

ATF_TC(multilayer);
ATF_TC_HEAD(multilayer, tc)
{
	atf_tc_set_md_var(tc, "descr", "mount_union -b twice");
}

ATF_TC_BODY(multilayer, tc)
{
	struct union_args unionargs;

	rump_init();

	if (rump_sys_mkdir("/Tunion", 0777) == -1)
		atf_tc_fail_errno("mkdir mp1");
	if (rump_sys_mkdir("/Tunion2", 0777) == -1)
		atf_tc_fail_errno("mkdir mp2");
	if (rump_sys_mkdir("/Tunion2/A", 0777) == -1)
		atf_tc_fail_errno("mkdir A");
	if (rump_sys_mkdir("/Tunion2/B", 0777) == -1)
		atf_tc_fail_errno("mkdir B");

	unionargs.target = __UNCONST("/Tunion2/A");
	unionargs.mntflags = UNMNT_BELOW;

	if (rump_sys_mount(MOUNT_UNION, "/Tunion", 0,
	    &unionargs, sizeof(unionargs)) == -1)
		atf_tc_fail_errno("union mount");

	unionargs.target = __UNCONST("/Tunion2/B");
	unionargs.mntflags = UNMNT_BELOW;

	rump_sys_mount(MOUNT_UNION, "/Tunion", 0,&unionargs,sizeof(unionargs));
}

ATF_TC(multilayer2);
ATF_TC_HEAD(multilayer2, tc)
{
	atf_tc_set_md_var(tc, "descr", "mount_union twice then unmount");
}

ATF_TC_BODY(multilayer2, tc)
{
	struct union_args unionargs;

	atf_tc_expect_signal(-1, "PR kern/2423");

	rump_init();

	if (rump_sys_mkdir("/Tunion", 0777) == -1)
		atf_tc_fail_errno("mkdir mp1");
	if (rump_sys_mkdir("/Tunion2", 0777) == -1)
		atf_tc_fail_errno("mkdir mp2");
	if (rump_sys_mkdir("/Tunion2/A", 0777) == -1)
		atf_tc_fail_errno("mkdir A");
	if (rump_sys_mkdir("/Tunion2/B", 0777) == -1)
		atf_tc_fail_errno("mkdir B");

	unionargs.target = __UNCONST("/Tunion2/A");
	unionargs.mntflags = UNMNT_ABOVE;

	if (rump_sys_mount(MOUNT_UNION, "/Tunion", 0,
	    &unionargs, sizeof(unionargs)) == -1)
		atf_tc_fail_errno("union mount");
	if (rump_sys_mkdir("/Tunion2/A/A", 0777) == -1)
		atf_tc_fail_errno("mkdir A/A");

	unionargs.target = __UNCONST("/Tunion2/A/A");
	unionargs.mntflags = UNMNT_ABOVE;

	rump_sys_mount(MOUNT_UNION, "/Tunion", 0,&unionargs,sizeof(unionargs));

	rump_sys_unmount("/Tunion/A", 0);
}

ATF_TC(cyclic);
ATF_TC_HEAD(cyclic, tc)
{
	atf_tc_set_md_var(tc, "descr", "cyclic mount_union");
}

ATF_TC_BODY(cyclic, tc)
{
	struct union_args unionargs;

	atf_tc_expect_signal(-1, "PR kern/3645");

	rump_init();

	if (rump_sys_mkdir("/Tunion", 0777) == -1)
		atf_tc_fail_errno("mkdir mp1");
	if (rump_sys_mkdir("/Tunion/A", 0777) == -1)
		atf_tc_fail_errno("mkdir mp2");

	unionargs.target = __UNCONST("/Tunion/A");
	unionargs.mntflags = UNMNT_ABOVE;

	if (rump_sys_mount(MOUNT_UNION, "/Tunion/A", 0,
	    &unionargs, sizeof(unionargs)) == -1)
		atf_tc_fail_errno("union mount");

	if (rump_sys_mkdir("/Tunion/A/A", 0777) == -1)
		atf_tc_fail_errno("mkdir failed");
}

ATF_TC(cyclic2);
ATF_TC_HEAD(cyclic2, tc)
{
	atf_tc_set_md_var(tc, "descr", "cyclic mount_union");
}

ATF_TC_BODY(cyclic2, tc)
{
	struct union_args unionargs;

	atf_tc_expect_signal(-1, "PR kern/4597");

	rump_init();

	if (rump_sys_mkdir("/Tunion", 0777) == -1)
		atf_tc_fail_errno("mkdir mp1");
	if (rump_sys_mkdir("/Tunion/A", 0777) == -1)
		atf_tc_fail_errno("mkdir mp2");
	if (rump_sys_mkdir("/Tunion/B", 0777) == -1)
		atf_tc_fail_errno("mkdir mp3");

	unionargs.target = __UNCONST("/Tunion/A");
	unionargs.mntflags = UNMNT_ABOVE;

	if (rump_sys_mount(MOUNT_UNION, "/Tunion/B", 0,
	    &unionargs, sizeof(unionargs)) == -1)
		atf_tc_fail_errno("union mount");

	unionargs.target = __UNCONST("/Tunion/B");
	unionargs.mntflags = UNMNT_ABOVE;

	if (rump_sys_mount(MOUNT_UNION, "/Tunion/A", 0,
	    &unionargs, sizeof(unionargs)) == -1)
		atf_tc_fail_errno("union mount2");

	if (rump_sys_mkdir("/Tunion/A/A", 0777) == -1)
		atf_tc_fail_errno("mkdir failed");
}

ATF_TC(devnull1);
ATF_TC_HEAD(devnull1, tc)
{
	atf_tc_set_md_var(tc, "descr", "mount_union -b and "
	    "'echo x > /un/null'");
}

ATF_TC_BODY(devnull1, tc)
{
	struct union_args unionargs;
	int fd, res;

	rump_init();

	if (rump_sys_mkdir("/mp", 0777) == -1)
		atf_tc_fail_errno("mkdir mp");

	unionargs.target = __UNCONST("/dev");
	unionargs.mntflags = UNMNT_BELOW;

	if (rump_sys_mount(MOUNT_UNION, "/mp", 0,
	    &unionargs, sizeof(unionargs)) == -1)
		atf_tc_fail_errno("union mount");

	fd = rump_sys_open("/mp/null", O_WRONLY | O_CREAT | O_TRUNC);

	if (fd == -1)
		atf_tc_fail_errno("open");

	res = rump_sys_write(fd, &fd, sizeof(fd));
	if (res != sizeof(fd))
		atf_tc_fail("write");
}

ATF_TC(devnull2);
ATF_TC_HEAD(devnull2, tc)
{
	atf_tc_set_md_var(tc, "descr", "mount_union -b and "
	    "'echo x >> /un/null'");
}

ATF_TC_BODY(devnull2, tc)
{
	struct union_args unionargs;
	int fd, res;

	rump_init();

	if (rump_sys_mkdir("/mp", 0777) == -1)
		atf_tc_fail_errno("mkdir mp");

	unionargs.target = __UNCONST("/dev");
	unionargs.mntflags = UNMNT_BELOW;

	if (rump_sys_mount(MOUNT_UNION, "/mp", 0,
	    &unionargs, sizeof(unionargs)) == -1)
		atf_tc_fail_errno("union mount");

	fd = rump_sys_open("/mp/null", O_WRONLY | O_CREAT | O_APPEND);
	if (fd == -1)
		atf_tc_fail_errno("open");

	res = rump_sys_write(fd, &fd, sizeof(fd));
	if (res != sizeof(fd))
		atf_tc_fail("write");
}

ATF_TP_ADD_TCS(tp)
{
	ATF_TP_ADD_TC(tp, multilayer);
	ATF_TP_ADD_TC(tp, multilayer2);
	ATF_TP_ADD_TC(tp, cyclic);
	ATF_TP_ADD_TC(tp, cyclic2);
	ATF_TP_ADD_TC(tp, devnull1);
	ATF_TP_ADD_TC(tp, devnull2);

	return atf_no_error();
}
