/*	$NetBSD: t_cmsg.c,v 1.7 2009/05/15 15:54:03 pooka Exp $	*/

#include <sys/types.h>
#include <sys/mount.h>
#include <sys/socket.h>
#include <sys/un.h>

#include <rump/rump.h>
#include <rump/rump_syscalls.h>

#include <fs/tmpfs/tmpfs_args.h>

#include <atf-c.h>
#include <fcntl.h>
#include <err.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <util.h>

#include "../h_macros.h"

ATF_TC(cmsg_sendfd_bounds);
ATF_TC_HEAD(cmsg_sendfd_bounds, tc)
{
	atf_tc_set_md_var(tc, "descr", "Checks that attempting to pass an "
	    "invalid fd returns an error");
}

ATF_TC_BODY(cmsg_sendfd_bounds, tc)
{
	struct cmsghdr *cmp;
	struct msghdr msg;
	struct iovec iov;
	int s[2];
	int fd;

	rump_init();

	if (rump_sys_socketpair(AF_LOCAL, SOCK_STREAM, 0, s) == -1)
		atf_tc_fail("rump_sys_socket");

	cmp = malloc(CMSG_LEN(sizeof(int)));

	iov.iov_base = &fd;
	iov.iov_len = sizeof(int);

	cmp->cmsg_level = SOL_SOCKET;
	cmp->cmsg_type = SCM_RIGHTS;
	cmp->cmsg_len = CMSG_LEN(sizeof(int));

	msg.msg_iov = &iov;
	msg.msg_iovlen = 1;
	msg.msg_name = NULL;
	msg.msg_namelen = 0;
	msg.msg_control = cmp;
	msg.msg_controllen = CMSG_LEN(sizeof(int));

	/*
	 * ERROR HERE: trying to pass invalid fd
	 * (This value was previously directly used to index the fd
	 *  array and therefore we are passing a hyperspace index)
	 */
	*(int *)CMSG_DATA(cmp) = 0x12345678;

	rump_sys_sendmsg(s[0], &msg, 0);
	if (errno != EBADF)
		atf_tc_fail("descriptor passing failed: expected EBADF (9), "
		    "got %d\n(%s)", errno, strerror(errno));
}


ATF_TC(cmsg_sendfd);
ATF_TC_HEAD(cmsg_sendfd, tc)
{
	atf_tc_set_md_var(tc, "descr", "Checks that fd passing works");
	atf_tc_set_md_var(tc, "timeout", "2");
}

ATF_TC_BODY(cmsg_sendfd, tc)
{
	char buf[128];
	struct cmsghdr *cmp;
	struct msghdr msg;
	struct tmpfs_args args;
	struct sockaddr_un sun;
	struct lwp *l1, *l2;
	struct iovec iov;
	socklen_t sl;
	int s1, s2, sgot;
	int rfd, fd, storage;

	memset(&args, 0, sizeof(args));
	args.ta_version = TMPFS_ARGS_VERSION;
	args.ta_root_mode = 0777;

	rump_init();
	/*
	 * mount tmpfs as root -- rump root file system does not support
	 * unix domain sockets.
	 */
	if (rump_sys_mount(MOUNT_TMPFS, "/", 0, &args, sizeof(args)) == -1)
		atf_tc_fail_errno("mount tmpfs");

	/* create unix socket and bind it to a path */
	memset(&sun, 0, sizeof(sun));
	sun.sun_family = AF_LOCAL;
#define SOCKPATH "/com"
	strncpy(sun.sun_path, SOCKPATH, sizeof(SOCKPATH));
	s1 = rump_sys_socket(AF_LOCAL, SOCK_STREAM, 0);
	if (s1 == -1)
		atf_tc_fail_errno("socket 1");
	if (rump_sys_bind(s1, (struct sockaddr *)&sun, SUN_LEN(&sun)) == -1)
		atf_tc_fail_errno("socket 1 bind");
	if (rump_sys_listen(s1, 1) == -1)
		atf_tc_fail_errno("socket 1 listen");

	/* store our current lwp/proc */
	l1 = rump_get_curlwp();

	/* create new process */
	l2 = rump_newproc_switch();

	/* connect to unix domain socket */
	memset(&sun, 0, sizeof(sun));
	sun.sun_family = AF_LOCAL;
	strncpy(sun.sun_path, SOCKPATH, sizeof(SOCKPATH));
	s2 = rump_sys_socket(AF_LOCAL, SOCK_STREAM, 0);
	if (s2 == -1)
		atf_tc_fail_errno("socket 2");
	if (rump_sys_connect(s2, (struct sockaddr *)&sun, SUN_LEN(&sun)) == -1)
		atf_tc_fail_errno("socket 2 connect");

	/* open a file and write stuff to it */
	fd = rump_sys_open("/foobie", O_RDWR | O_CREAT, 0777);
	if (fd == -1)
		atf_tc_fail_errno("can't open file");
#define MAGICSTRING "duam xnaht"
	if (rump_sys_write(fd, MAGICSTRING, sizeof(MAGICSTRING)) !=
	    sizeof(MAGICSTRING))
		atf_tc_fail_errno("file write"); /* XXX: errno */
	/* reset offset */
	rump_sys_lseek(fd, 0, SEEK_SET);

	cmp = malloc(CMSG_LEN(sizeof(int)));

	iov.iov_base = &storage;
	iov.iov_len = sizeof(int);

	cmp->cmsg_level = SOL_SOCKET;
	cmp->cmsg_type = SCM_RIGHTS;
	cmp->cmsg_len = CMSG_LEN(sizeof(int));

	msg.msg_iov = &iov;
	msg.msg_iovlen = 1;
	msg.msg_name = NULL;
	msg.msg_namelen = 0;
	msg.msg_control = cmp;
	msg.msg_controllen = CMSG_LEN(sizeof(int));
	*(int *)CMSG_DATA(cmp) = fd;

	/* pass the fd */
	if (rump_sys_sendmsg(s2, &msg, 0) == -1)
		atf_tc_fail_errno("sendmsg failed");

	/* switch back to original proc */
	rump_set_curlwp(l1);

	/* accept connection and read fd */
	sl = sizeof(sun);
	sgot = rump_sys_accept(s1, (struct sockaddr *)&sun, &sl);
	if (sgot == -1)
		atf_tc_fail_errno("accept");
	if (rump_sys_recvmsg(sgot, &msg, 0) == -1)
		atf_tc_fail_errno("recvmsg failed");
	rfd = *(int *)CMSG_DATA(cmp);

	/* read from the fd */
	memset(buf, 0, sizeof(buf));
	if (rump_sys_read(rfd, buf, sizeof(buf)) == -1)
		atf_tc_fail_errno("read rfd");

	/* check that we got the right stuff */
	if (strcmp(buf, MAGICSTRING) != 0)
		atf_tc_fail("expected \"%s\", got \"%s\"", MAGICSTRING, buf);
}

ATF_TP_ADD_TCS(tp)
{
	ATF_TP_ADD_TC(tp, cmsg_sendfd);
	ATF_TP_ADD_TC(tp, cmsg_sendfd_bounds);
}
