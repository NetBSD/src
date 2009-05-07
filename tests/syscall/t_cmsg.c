/*	$NetBSD: t_cmsg.c,v 1.4 2009/05/07 14:45:19 pooka Exp $	*/

#include <sys/types.h>
#include <sys/socket.h>

#include <rump/rump.h>
#include <rump/rump_syscalls.h>

#include <atf-c.h>
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
	struct cmsghdr *cmp;
	struct msghdr msg;
	struct iovec iov;
	int s[2], sgot;
	int fd;
	int v1, v2;

	rump_init();

	if (rump_sys_socketpair(AF_LOCAL, SOCK_STREAM, 0, s) == -1)
		atf_tc_fail("rump_sys_socketpair");

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
	*(int *)CMSG_DATA(cmp) = s[1];

	if (rump_sys_sendmsg(s[0], &msg, 0) == -1)
		atf_tc_fail_errno("sendmsg failed");
	if (rump_sys_recvmsg(s[1], &msg, 0) == -1)
		atf_tc_fail_errno("recvmsg failed");
	sgot = *(int *)CMSG_DATA(cmp);

	/* test that we really got the fd by writing something thought it */
	v1 = 37;
	v2 = -1;
	rump_sys_write(s[0], &v1, sizeof(v1));
	rump_sys_read(sgot, &v2, sizeof(v1));
	if (v1 != v2)
		atf_tc_fail("value mismatch: %d vs. %d", v1, v2);
}

ATF_TP_ADD_TCS(tp)
{
	ATF_TP_ADD_TC(tp, cmsg_sendfd);
	ATF_TP_ADD_TC(tp, cmsg_sendfd_bounds);
}
