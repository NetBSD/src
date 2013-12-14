/*	$NetBSD: linux_rump.c,v 1.1 2013/12/14 10:29:45 njoly Exp $	*/

#include <sys/param.h>

#include <compat/linux/common/linux_types.h>
#include <compat/linux/common/linux_signal.h>

#include "rump_linux_syscallargs.h"

/*
    compat/linux/arch/.../syscallargs.h and rump_linux_syscallargs.h
    define the same syscall arguments and prototypes, thus cannot be
    both used. Just copy needed linux stuff for now to avoid conflicts.
*/

struct linux_sys_mknodat_args {
        syscallarg(int) fd;
        syscallarg(const char *) path;
        syscallarg(mode_t) mode;
        syscallarg(unsigned) dev;
};
check_syscall_args(linux_sys_mknodat)

int     linux_sys_mknodat(struct lwp *, const struct linux_sys_mknodat_args *, register_t *);

int
rump_linux_sys_mknodat(struct lwp *l,
    const struct rump_linux_sys_mknodat_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) fd;
		syscallarg(const char *) path;
		syscallarg(mode_t) mode;
		syscallarg(int) PAD;
		syscallarg(dev_t) dev;
	} */
	struct linux_sys_mknodat_args ua;

	SCARG(&ua, fd) = SCARG(uap, fd);
	SCARG(&ua, path) = SCARG(uap, path);
	SCARG(&ua, mode) = SCARG(uap, mode);
	SCARG(&ua, dev) = SCARG(uap, dev);

	return linux_sys_mknodat(l, &ua, retval);
}
