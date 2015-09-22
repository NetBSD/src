#	$NetBSD: Makefile,v 1.1.2.3 2015/09/22 12:06:16 skrll Exp $
#

.PATH:	${.CURDIR}/../../../../net

LIB=	rumpnet_tap
IOCONF=	TAP.ioconf
SRCS=	if_tap.c

SRCS+=	tap_component.c

CPPFLAGS+=	-I${.CURDIR}/../../../librump/rumpvfs

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
