#include <sys/cdefs.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>

#include <unistd.h>

#include <rump/rump.h>
#include <rump/rump_syscalls.h>
#include <rump/rumpclient.h>

#include "prog_ops.h"

const struct prog_ops prog_ops = {
	.op_init =		rumpclient_init,
	.op_daemon =		rumpclient_daemon,

	.op_socket =		rump_sys_socket,
	.op_open =		rump_sys_open,
	.op_close =		rump_sys_close,
	.op_getpid =		rump_sys_getpid,

	.op_read =		rump_sys_read,
	.op_write =		rump_sys_write,

	.op_sysctl =		rump_sys___sysctl,
	.op_ioctl =		rump_sys_ioctl,

	.op_chdir =		rump_sys_chdir,
	.op_chroot =		rump_sys_chroot,

	.op_setuid =		rump_sys_setuid,
	.op_setgid =		rump_sys_setgid,
	.op_setgroups =		rump_sys_setgroups,

	.op_recvmsg =		rump_sys_recvmsg,
	.op_sendmsg =		rump_sys_sendmsg,

	.op_setsockopt =	rump_sys_setsockopt,
	.op_poll =		rump_sys_poll,
	.op_clock_gettime =	rump_sys_clock_gettime,

};

