#	$NetBSD: Makefile,v 1.4 2019/09/26 02:01:02 christos Exp $

.include "../Makefile.inc"

.PATH:	${S}/compat/common

KMOD=	compat_30

CPPFLAGS+=	-DCOMPAT_30
CPPFLAGS+=	-DCOMPAT_40 -DCOMPAT_50 -DCOMPAT_60 -DCOMPAT_70 -DCOMPAT_80

SRCS+=	compat_30_mod.c
SRCS+=	vfs_syscalls_30.c uipc_syscalls_30.c kern_time_30.c bio_30.c vnd_30.c
SRCS+=	usb_subr_30.c

.include <bsd.kmodule.mk>
