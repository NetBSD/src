#	$NetBSD: Makefile,v 1.1.2.2 2018/03/21 04:48:32 pgoyette Exp $

.include "../Makefile.inc"

.PATH:	${S}/compat/common

KMOD=	compat_50

CPPFLAGS+=	-DCOMPAT_50 -DCOMPAT_60 -DCOMPAT_70 -DCOMPAT_80

SRCS+=	compat_50_mod.c
SRCS+=	kern_50.c kern_time_50.c kern_select_50.c
SRCS+=	vfs_syscalls_50.c uipc_syscalls_50.c uvm_50.c
SRCS+=	rndpseudo_50.c clockctl_50.c
#SRCS+=	rtsock_50.c

.include <bsd.kmodule.mk>
