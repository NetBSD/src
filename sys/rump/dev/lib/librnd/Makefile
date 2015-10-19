#	$NetBSD: Makefile,v 1.10 2015/10/19 16:16:34 pooka Exp $
#

.PATH:	${.CURDIR}/../../../../dev

LIB=	rumpdev_rnd
COMMENT=/dev/{,u}random

IOCONF=	RND.ioconf
SRCS=	rndpseudo.c

SRCS+=	rnd_component.c

CPPFLAGS+=	-I${RUMPTOP}/librump/rumpvfs

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
