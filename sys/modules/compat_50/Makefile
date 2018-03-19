#	$NetBSD: Makefile,v 1.1.2.1 2018/03/19 21:54:43 pgoyette Exp $

.include "../Makefile.inc"

.PATH:	${S}/compat/common

KMOD=	compat_50

CPPFLAGS+=	-DCOMPAT_50 -DCOMPAT_60 -DCOMPAT_70 -DCOMPAT_80

SRCS+=	compat_50_mod.c
SRCS+=	kern_50.c kern_time_50.c kern_select_50.c
SRCS+=	vfs_syscalls_50.c uipc_syscalls_50.c uvm_50.c
#SRCS+=	rndpseudo_50.c rtsock_50.c

.include <bsd.kmodule.mk>
