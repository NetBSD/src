#	$NetBSD: Makefile,v 1.2 2010/02/16 20:42:45 pooka Exp $
#

.PATH:	${.CURDIR}/../../../../dev

LIB=	rumpdev_rnd

SRCS=   rnd.c rndpool.c

SRCS+=	component.c

CPPFLAGS+=	-I${RUMPTOP}/librump/rumpvfs

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
