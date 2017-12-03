#	$NetBSD: Makefile,v 1.1.20.2 2017/12/03 11:39:17 jdolecek Exp $
#

.PATH:	${.CURDIR}/../../../../net

LIB=	rumpnet_bridge
COMMENT=Bridging for IEEE 802

IOCONF=	BRIDGE.ioconf
SRCS=	if_bridge.c bridgestp.c

SRCS+=	bridge_component.c

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
