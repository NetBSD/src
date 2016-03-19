#	$NetBSD: Makefile,v 1.7.6.4 2016/03/19 11:30:35 skrll Exp $
#

.PATH:	${.CURDIR}/../../../../dev

LIB=	rumpdev_rnd
COMMENT=/dev/{,u}random

IOCONF=	RND.ioconf
SRCS=	rndpseudo.c

SRCS+=	rnd_component.c

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
