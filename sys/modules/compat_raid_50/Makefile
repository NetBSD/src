#	$NetBSD: Makefile,v 1.1.2.1 2018/09/22 22:21:46 pgoyette Exp $

.include "../Makefile.inc"

.PATH:	${S}/compat/common

KMOD=	compat_raid_50

CPPFLAGS+=	-DCOMPAT_50

SRCS+=	compat_50_mod.c

.PATH:	${S}/dev/raidframe

SRCS+=	rf_compat50.c 

.include <bsd.kmodule.mk>
