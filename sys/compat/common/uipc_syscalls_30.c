/*	$NetBSD: uipc_syscalls_30.c,v 1.1.16.4 2008/01/21 09:40:44 yamt Exp $	*/

/* written by Pavel Cahyna, 2006. Public domain. */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: uipc_syscalls_30.c,v 1.1.16.4 2008/01/21 09:40:44 yamt Exp $");

/*
 * System call interface to the socket abstraction.
 */

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/msg.h>
#include <sys/sysctl.h>
#include <sys/mount.h>
#include <sys/syscallargs.h>
#include <sys/errno.h>

int
compat_30_sys_socket(struct lwp *l, const struct compat_30_sys_socket_args *uap, register_t *retval)
{
	int	error;

	error = sys___socket30(l, (const void *)uap, retval);
	if (error == EAFNOSUPPORT)
		error = EPROTONOSUPPORT;

	return (error);
}
