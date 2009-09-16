#	$NetBSD: Makefile,v 1.1.2.2 2009/09/16 13:38:04 yamt Exp $
#

.PATH:	${.CURDIR}/../../../../dev

LIB=	rumpdev_rnd

SRCS=   rnd.c rndpool.c

SRCS+=	component.c

CPPFLAGS+=	-Wno-pointer-sign
CPPFLAGS+=	-I${RUMPTOP}/librump/rumpvfs

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
