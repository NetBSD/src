#	$NetBSD: Makefile,v 1.1.2.2 2018/09/23 01:33:26 pgoyette Exp $

.include "../Makefile.inc"

.PATH:	${S}/compat/common

KMOD=	compat_raid_50

.PATH:	${S}/dev/raidframe

SRCS+=	rf_compat50.c 

.include <bsd.kmodule.mk>
