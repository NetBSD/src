#	$NetBSD: Makefile,v 1.6 2020/09/29 18:31:31 roy Exp $
#

.PATH:	${.CURDIR}/../../../../net

LIB=	rumpnet_tap
COMMENT=/dev/tap Ethernet tunnel software network interface

IOCONF=	TAP.ioconf
SRCS=	if_tap.c

SRCS+=	tap_component.c

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
