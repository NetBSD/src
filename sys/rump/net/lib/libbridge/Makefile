#	$NetBSD: Makefile,v 1.2.6.1 2015/09/22 12:06:16 skrll Exp $
#

.PATH:	${.CURDIR}/../../../../net

LIB=	rumpnet_bridge
IOCONF=	BRIDGE.ioconf
SRCS=	if_bridge.c bridgestp.c

SRCS+=	bridge_component.c

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
