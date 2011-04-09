/*	$NetBSD: t_pr.c,v 1.4 2011/04/09 06:36:03 martin Exp $	*/

#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/tty.h>

#include <atf-c.h>
#include <fcntl.h>

#include <rump/rump.h>
#include <rump/rump_syscalls.h>

ATF_TC(ptyioctl);
ATF_TC_HEAD(ptyioctl, tc)
{

	atf_tc_set_md_var(tc, "descr", "ioctl on pty");
}

ATF_TC_BODY(ptyioctl, tc)
{
	struct termios tio;
	int fd;

	rump_init();
	fd = rump_sys_open("/dev/ptyp1", O_RDWR);
	ATF_CHECK(fd != -1);

	/*
	 * This used to die with null deref under ptcwakeup()
	 * atf_tc_expect_signal(-1, "PR kern/40688");
	 */
	rump_sys_ioctl(fd, TIOCGETA, &tio);
}

ATF_TP_ADD_TCS(tp)
{

	ATF_TP_ADD_TC(tp, ptyioctl);

	return atf_no_error();
}
