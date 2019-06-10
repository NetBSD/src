#	$NetBSD: Makefile,v 1.3.4.2 2019/06/10 22:09:19 christos Exp $

.include "../Makefile.inc"

KMOD=	compat_raid_50

.PATH:	${S}/dev/raidframe

SRCS+=	rf_compat50.c 

WARNS=	3

.include <bsd.kmodule.mk>
