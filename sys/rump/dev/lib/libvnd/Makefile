#	$NetBSD: Makefile,v 1.1.8.1 2015/09/22 12:06:14 skrll Exp $
#

.PATH:	${.CURDIR}/../../../../dev

LIB=	rumpdev_vnd
IOCONF=	VND.ioconf
SRCS=   vnd.c

SRCS+=	vnd_component.c

CPPFLAGS+=	-I${RUMPTOP}/librump/rumpvfs

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
