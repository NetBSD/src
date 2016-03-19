#	$NetBSD: Makefile,v 1.1.2.5 2016/03/19 11:30:39 skrll Exp $
#

.PATH:	${.CURDIR}/../../../../net

LIB=	rumpnet_tap
COMMENT=/dev/tap virtual Ethernet interface

IOCONF=	TAP.ioconf
SRCS=	if_tap.c

SRCS+=	tap_component.c

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
