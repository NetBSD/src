#	$NetBSD: Makefile,v 1.3.4.2 2019/06/10 22:09:14 christos Exp $

.include "../Makefile.inc"

.PATH:	${S}/compat/common

KMOD=	compat_20

CPPFLAGS+=	-DCOMPAT_20 -DCOMPAT_30
CPPFLAGS+=	-DCOMPAT_40 -DCOMPAT_50 -DCOMPAT_60 -DCOMPAT_70 -DCOMPAT_80

SRCS+=	compat_20_mod.c
SRCS+=	vfs_syscalls_20.c  ieee80211_20.c if43_20.c

WARNS=	3

.include <bsd.kmodule.mk>
