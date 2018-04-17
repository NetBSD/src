#	$NetBSD: Makefile,v 1.1.2.1 2018/04/17 00:02:58 pgoyette Exp $

.include "../Makefile.inc"

.PATH:	${S}/compat/common

KMOD=	compat_43

CPPFLAGS+=	-DCOMPAT_43

SRCS=	compat_43_mod.c
SRCS+=	kern_exit_43.c kern_info_43.c kern_resource_43.c kern_sig_43.c \
	tty_43.c uipc_syscalls_43.c vfs_syscalls_43.c vm_43.c if_43.c

.include <bsd.kmodule.mk>
