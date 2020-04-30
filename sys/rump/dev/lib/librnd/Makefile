#	$NetBSD: Makefile,v 1.12 2020/04/30 03:28:19 riastradh Exp $
#

.PATH:	${.CURDIR}/../../../../dev

LIB=	rumpdev_rnd
COMMENT=/dev/{,u}random

IOCONF=	RND.ioconf
SRCS=	random.c

SRCS+=	rnd_component.c

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
