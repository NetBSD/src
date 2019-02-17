#	$NetBSD: Makefile,v 1.3 2019/02/17 04:05:46 rin Exp $

.include "../Makefile.inc"

KMOD=	compat_raid_50

.PATH:	${S}/dev/raidframe

SRCS+=	rf_compat50.c 

WARNS=	3

.include <bsd.kmodule.mk>
