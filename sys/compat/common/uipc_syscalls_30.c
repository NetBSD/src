/*	$NetBSD: uipc_syscalls_30.c,v 1.3.96.1 2018/03/28 04:18:24 pgoyette Exp $	*/

/* written by Pavel Cahyna, 2006. Public domain. */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: uipc_syscalls_30.c,v 1.3.96.1 2018/03/28 04:18:24 pgoyette Exp $");

/*
 * System call interface to the socket abstraction.
 */

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/msg.h>
#include <sys/sysctl.h>
#include <sys/mount.h>
#include <sys/syscall.h>
#include <sys/syscallvar.h>
#include <sys/syscallargs.h>
#include <sys/errno.h>

#include <compat/common/compat_mod.h>

static const struct syscall_package uipc_syscalls_30_syscalls[] = {
	{ SYS_compat_30_socket, 0, (sy_call_t *)compat_30_sys_socket },
	{ 0, 0, NULL}
};

int
compat_30_sys_socket(struct lwp *l,
    const struct compat_30_sys_socket_args *uap, register_t *retval)
{
	int	error;

	error = sys___socket30(l, (const void *)uap, retval);
	if (error == EAFNOSUPPORT)
		error = EPROTONOSUPPORT;

	return (error);
}

int
uipc_syscalls_30_init(void)
{

	return syscall_establish(NULL, uipc_syscalls_30_syscalls);
}

int
uipc_syscalls_30_fini(void)
{

	return syscall_disestablish(NULL, uipc_syscalls_30_syscalls);
}
