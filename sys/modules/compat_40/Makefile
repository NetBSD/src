#	$NetBSD: Makefile,v 1.1.2.1 2018/03/27 07:29:44 pgoyette Exp $

.include "../Makefile.inc"

.PATH:	${S}/compat/common

KMOD=	compat_40

CPPFLAGS+=	-DCOMPAT_40 -DCOMPAT_50 -DCOMPAT_60 -DCOMPAT_70 -DCOMPAT_80

SRCS+=	compat_40_mod.c
SRCS+=	vfs_syscalls_40.c uipc_syscalls_40.c

.PATH:	${S}/dev/sysmon

SRCS+=	sysmon_power_40.c

.include <bsd.kmodule.mk>
