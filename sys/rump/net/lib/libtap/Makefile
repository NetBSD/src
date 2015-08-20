#	$NetBSD: Makefile,v 1.2 2015/08/20 12:20:09 christos Exp $
#

.PATH:	${.CURDIR}/../../../../net

LIB=	rumpnet_tap
IOCONF=	TAP.ioconf
SRCS=	if_tap.c

SRCS+=	tap_component.c

CPPFLAGS+=	-I${.CURDIR}/../libnet/opt -I${.CURDIR}/../../../librump/rumpvfs

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
