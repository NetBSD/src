#include <sys/cdefs.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/sysctl.h>
#include <sys/ioctl.h>

#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>

#include "prog_ops.h"

const struct prog_ops prog_ops = {
	.op_daemon =		daemon,
	.op_socket =		socket,
	.op_open =		open,
	.op_close =		close,
	.op_getpid =		getpid,

	.op_read =		read,
	.op_write =		write,

	.op_sysctl =		sysctl,
	.op_ioctl =		ioctl,

	.op_chdir =		chdir,
	.op_chroot =		chroot,

	.op_setuid =		setuid,
	.op_setgid =		setgid,
	.op_setgroups =		setgroups,

	.op_recvmsg =		recvmsg,
	.op_sendmsg =		sendmsg,

	.op_setsockopt =	setsockopt,
	.op_poll =		poll,
	.op_clock_gettime =	clock_gettime,
};
