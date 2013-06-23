#	$NetBSD: Makefile,v 1.1.24.1 2013/06/23 06:20:27 tls Exp $
#

.PATH:	${.CURDIR}/../../../../dev/pad

LIB=	rumpdev_pad

SRCS=	pad.c padvol.c
SRCS+=	component.c

CPPFLAGS+=	-I${.CURDIR}/../../../librump/rumpvfs

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
