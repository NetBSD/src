#	$NetBSD: Makefile,v 1.5 2016/01/26 23:12:19 pooka Exp $
#

.PATH:	${.CURDIR}/../../../../net

LIB=	rumpnet_tap
COMMENT=/dev/tap virtual Ethernet interface

IOCONF=	TAP.ioconf
SRCS=	if_tap.c

SRCS+=	tap_component.c

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
