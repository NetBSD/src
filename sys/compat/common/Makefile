#	$NetBSD: Makefile,v 1.22 1999/02/13 02:54:51 lukem Exp $

LIB=	compat
MKPIC=	no

CPPFLAGS=	${COMPATCPPFLAGS}

# Common compatibility code, used by all emulations
SRCS=	compat_exec.c compat_util.c

# Compatibility code for 4.3BSD
SRCS+=	kern_exit_43.c kern_info_43.c kern_resource_43.c kern_sig_43.c \
	tty_43.c uipc_syscalls_43.c vfs_syscalls_43.c vm_43.c

# Compatibility code for NetBSD 0.9
SRCS+=	kern_info_09.c

# Compatibility code for NetBSD 1.2
SRCS+=	kern_xxx_12.c vfs_syscalls_12.c vm_12.c

# Compatibility code for NetBSD 1.3
SRCS+=	kern_sig_13.c

# really, all machines where sizeof(int) != sizeof(long) (LP64)
.if (${MACHINE_ARCH} != "alpha")
SRCS+=	kern_ipc_10.c 
.endif

# only needed during build
libinstall::

.include <bsd.lib.mk>
