#	$NetBSD: Makefile,v 1.1 2010/11/22 21:20:32 pooka Exp $
#

.PATH:	${.CURDIR}/../../../../dev

LIB=	rumpdev_md
IOCONF=	MD.ioconf

SRCS=	md.c
SRCS+=	component.c

CPPFLAGS+=	-I${.CURDIR}/../../../librump/rumpvfs

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
