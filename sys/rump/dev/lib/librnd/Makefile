#	$NetBSD: Makefile,v 1.7.6.2 2015/09/22 12:06:13 skrll Exp $
#

.PATH:	${.CURDIR}/../../../../dev

LIB=	rumpdev_rnd
IOCONF=	RND.ioconf
SRCS=	rndpseudo.c

SRCS+=	rnd_component.c

CPPFLAGS+=	-I${RUMPTOP}/librump/rumpvfs

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
