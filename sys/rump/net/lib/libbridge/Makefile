#	$NetBSD: Makefile,v 1.3 2015/08/20 12:20:09 christos Exp $
#

.PATH:	${.CURDIR}/../../../../net

LIB=	rumpnet_bridge
IOCONF=	BRIDGE.ioconf
SRCS=	if_bridge.c bridgestp.c

SRCS+=	bridge_component.c

CPPFLAGS+=	-I${.CURDIR}/../libnet/opt

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
