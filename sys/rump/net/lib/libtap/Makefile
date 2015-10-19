#	$NetBSD: Makefile,v 1.4 2015/10/19 16:16:39 pooka Exp $
#

.PATH:	${.CURDIR}/../../../../net

LIB=	rumpnet_tap
COMMENT=/dev/tap virtual Ethernet interface

IOCONF=	TAP.ioconf
SRCS=	if_tap.c

SRCS+=	tap_component.c

CPPFLAGS+=	-I${.CURDIR}/../../../librump/rumpvfs

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
