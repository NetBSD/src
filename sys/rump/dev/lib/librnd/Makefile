#	$NetBSD: Makefile,v 1.6.6.2 2017/12/03 11:39:09 jdolecek Exp $
#

.PATH:	${.CURDIR}/../../../../dev

LIB=	rumpdev_rnd
COMMENT=/dev/{,u}random

IOCONF=	RND.ioconf
SRCS=	rndpseudo.c

SRCS+=	rnd_component.c

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
