#	$NetBSD: Makefile,v 1.15.2.1 1997/06/22 02:12:49 cjs Exp $

LIB=	compat
NOPIC=

# this library needed only during build
NOINSTALL=1

CPPFLAGS=	${COMPATCPPFLAGS}

SRCS=	compat_exec.c compat_util.c kern_exit_43.c kern_info_09.c \
	kern_info_43.c kern_resource_43.c kern_sig_43.c kern_xxx_12.c \
	tty_43.c uipc_syscalls_43.c vfs_syscalls_43.c vm_43.c vm_swap_12.c

# really, all machines where sizeof(int) != sizeof(long)
.if (${MACHINE_ARCH} != "alpha")
SRCS+=	kern_ipc_10.c 
.endif

.include <bsd.lib.mk>
