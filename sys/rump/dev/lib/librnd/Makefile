#	$NetBSD: Makefile,v 1.11 2016/01/26 23:12:16 pooka Exp $
#

.PATH:	${.CURDIR}/../../../../dev

LIB=	rumpdev_rnd
COMMENT=/dev/{,u}random

IOCONF=	RND.ioconf
SRCS=	rndpseudo.c

SRCS+=	rnd_component.c

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
