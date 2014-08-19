#	$NetBSD: Makefile,v 1.1.20.1 2014/08/20 00:04:42 tls Exp $
#

.PATH:	${.CURDIR}/../../../../net

LIB=	rumpnet_bridge

SRCS=	if_bridge.c bridgestp.c

SRCS+=	bridge_component.c

CPPFLAGS+=	-I${.CURDIR}/../libnet/opt

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
