/*	$NetBSD: t_pr.c,v 1.1 2010/06/28 19:04:00 pooka Exp $	*/

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
	atf_tc_set_md_var(tc, "xfail", "PR kern/40688");
}

ATF_TC_BODY(ptyioctl, tc)
{
	struct termios tio;
	int fd;

	rump_init();
	fd = rump_sys_open("/dev/ptyp1", O_RDWR);
	if (fd == -1)
		err(1, "open");

	/* boom, dies with null deref under ptcwakeup() */
	rump_sys_ioctl(fd, TIOCGETA, &tio);
}

ATF_TP_ADD_TCS(tp)
{

	ATF_TP_ADD_TC(tp, ptyioctl);

	return atf_no_error();
}
