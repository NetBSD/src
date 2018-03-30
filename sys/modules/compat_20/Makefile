#	$NetBSD: Makefile,v 1.1.2.1 2018/03/30 11:15:25 pgoyette Exp $

.include "../Makefile.inc"

.PATH:	${S}/compat/common

KMOD=	compat_20

CPPFLAGS+=	-DCOMPAT_20 -DCOMPAT_30
CPPFLAGS+=	-DCOMPAT_40 -DCOMPAT_50 -DCOMPAT_60 -DCOMPAT_70 -DCOMPAT_80

SRCS+=	compat_20_mod.c
SRCS+=	vfs_syscalls_20.c  ieee80211_20.c if43_20.c

.include <bsd.kmodule.mk>
