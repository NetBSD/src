#	$NetBSD: Makefile,v 1.11 1996/08/08 15:06:39 mycroft Exp $

LIB=	compat
NOPIC=

CPPFLAGS=	${COMPATCPPFLAGS}

SRCS=	compat_exec.c compat_util.c kern_exit_43.c kern_info_09.c \
	kern_info_43.c kern_resource_43.c kern_sig_43.c \
	tty_43.c uipc_syscalls_43.c vfs_syscalls_43.c vm_43.c

# really, all machines were sizeof(int) != sizeof(long)
.if (${MACHINE_ARCH} != "alpha")
SRCS+=	kern_ipc_10.c 
.endif

install:

.include <bsd.lib.mk>
