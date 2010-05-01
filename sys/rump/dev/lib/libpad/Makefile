#	$NetBSD: Makefile,v 1.1 2010/05/01 23:21:24 pooka Exp $
#

.PATH:	${.CURDIR}/../../../../dev/pad

LIB=	rumpdev_pad

SRCS=	pad.c padvol.c

CPPFLAGS+=	-I${.CURDIR}/../../../librump/rumpvfs

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
