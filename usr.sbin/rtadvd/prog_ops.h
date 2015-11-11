#ifndef _PROG_OPS_H_
#define _PROG_OPS_H_

#include <sys/types.h>
#include <sys/socket.h>
#include <poll.h>

#ifndef CRUNCHOPS
struct prog_ops {
	int (*op_init)(void);
	int (*op_daemon)(int, int);

	int (*op_sysctl)(const int *, u_int, void *, size_t *,
			 const void *, size_t);
	int (*op_ioctl)(int, unsigned long, ...);

	int (*op_socket)(int, int, int);
	int (*op_open)(const char *, int, ...);
	int (*op_close)(int);
	pid_t (*op_getpid)(void);

	ssize_t (*op_read)(int, void *, size_t);
	ssize_t (*op_write)(int, const void *, size_t);

	int (*op_chdir)(const char *);
	int (*op_chroot)(const char *);

	int (*op_setuid)(uid_t);
	int (*op_setgid)(gid_t);
	int (*op_setgroups)(int, const gid_t *);

	ssize_t (*op_recvmsg)(int, struct msghdr *, int);
	ssize_t (*op_sendmsg)(int, const struct msghdr *, int);

	int (*op_setsockopt)(int, int, int, const void *, socklen_t);
	int (*op_poll)(struct pollfd *, u_int, int);
	int (*op_clock_gettime)(clockid_t, struct timespec *);
};
extern const struct prog_ops prog_ops;

#define prog_init prog_ops.op_init
#define prog_daemon prog_ops.op_daemon
#define prog_socket prog_ops.op_socket
#define prog_open prog_ops.op_open
#define prog_close prog_ops.op_close
#define prog_getpid prog_ops.op_getpid
#define prog_read prog_ops.op_read
#define prog_write prog_ops.op_write
#define prog_sysctl prog_ops.op_sysctl
#define prog_ioctl prog_ops.op_ioctl
#define prog_chdir prog_ops.op_chdir
#define prog_chroot prog_ops.op_chroot
#define prog_setuid prog_ops.op_setuid
#define prog_setgid prog_ops.op_setgid
#define prog_setgroups prog_ops.op_setgroups
#define prog_recvmsg prog_ops.op_recvmsg
#define prog_sendmsg prog_ops.op_sendmsg
#define prog_setsockopt prog_ops.op_setsockopt
#define prog_poll prog_ops.op_poll
#define prog_clock_gettime prog_ops.op_clock_gettime
#else
#define prog_init ((int (*)(void))NULL)
#define prog_daemon daemon
#define prog_socket socket
#define prog_open open
#define prog_close close
#define prog_getpid getpid
#define prog_read read
#define prog_write write
#define prog_sysctl sysctl
#define prog_ioctl ioctl
#define prog_chdir chdir
#define prog_chroot chroot
#define prog_setuid setuid
#define prog_setgid setgid
#define prog_setgroups setgroups
#define prog_recvmsg recvmsg
#define prog_sendmsg sendmsg
#define prog_setsockopt setsockopt
#define prog_poll poll
#define prog_clock_gettime clock_gettime
#endif

#endif /* _PROG_OPS_H_ */
