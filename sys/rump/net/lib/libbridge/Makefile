#	$NetBSD: Makefile,v 1.2.6.2 2015/12/27 12:10:17 skrll Exp $
#

.PATH:	${.CURDIR}/../../../../net

LIB=	rumpnet_bridge
COMMENT=Bridging for IEEE 802

IOCONF=	BRIDGE.ioconf
SRCS=	if_bridge.c bridgestp.c

SRCS+=	bridge_component.c

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
