/*	$NetBSD: svc_fdset.h,v 1.3 2015/11/07 00:42:04 christos Exp $	*/

# ifdef RUMP_RPC
#  include <rump/rump.h>
#  include <rump/rump_syscalls.h>
#  undef	close
#  define	close(a)		rump_sys_close(a)
#  undef	fcntl
#  define	fcntl(a, b, c)		rump_sys_fcntl(a, b, c)
#  undef	read
#  define	read(a, b, c)		rump_sys_read(a, b, c)
#  undef	write
#  define	write(a, b, c)		rump_sys_write(a, b, c)
#  undef	pollts
#  define	pollts(a, b, c, d)	rump_sys_pollts(a, b, c, d)
#  undef	select
#  define	select(a, b, c, d, e)	rump_sys_select(a, b, c, d, e)
# endif

typedef struct __fd_set_256 {
	__fd_mask fds_bits[__NFD_LEN(256)];
} __fd_set_256;
