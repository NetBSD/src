#	$NetBSD: Makefile,v 1.17 1997/06/12 17:35:18 mrg Exp $

LIB=	compat
NOPIC=

CPPFLAGS=	${COMPATCPPFLAGS}

SRCS=	compat_exec.c compat_util.c kern_exit_43.c kern_info_09.c \
	kern_info_43.c kern_resource_43.c kern_sig_43.c kern_xxx_12.c \
	tty_43.c uipc_syscalls_43.c vfs_syscalls_43.c vm_43.c vm_swap_12.c

# really, all machines where sizeof(int) != sizeof(long)
.if (${MACHINE_ARCH} != "alpha")
SRCS+=	kern_ipc_10.c 
.endif

# only needed during build
libinstall::

.include <bsd.lib.mk>
