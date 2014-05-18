#	$NetBSD: Makefile,v 1.2.4.1 2014/05/18 17:46:15 rmind Exp $
#

.PATH:	${.CURDIR}/../../../../dev/pad

LIB=	rumpdev_pad

SRCS=	pad.c padvol.c
SRCS+=	pad_component.c

CPPFLAGS+=	-I${.CURDIR}/../../../librump/rumpvfs

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
