#	$NetBSD: Makefile,v 1.5.16.2 2017/12/03 11:39:19 jdolecek Exp $
#

.PATH:	${.CURDIR}/../../../../net

LIB=	rumpnet_tap
COMMENT=/dev/tap virtual Ethernet interface

IOCONF=	TAP.ioconf
SRCS=	if_tap.c

SRCS+=	tap_component.c

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
