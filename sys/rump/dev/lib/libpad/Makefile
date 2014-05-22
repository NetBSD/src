#	$NetBSD: Makefile,v 1.1.14.1 2014/05/22 11:41:12 yamt Exp $
#

.PATH:	${.CURDIR}/../../../../dev/pad

LIB=	rumpdev_pad

SRCS=	pad.c padvol.c
SRCS+=	pad_component.c

CPPFLAGS+=	-I${.CURDIR}/../../../librump/rumpvfs

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
