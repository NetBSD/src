#	$NetBSD: Makefile,v 1.2 2014/03/13 02:08:24 pooka Exp $
#

.PATH:	${.CURDIR}/../../../../net

LIB=	rumpnet_bridge

SRCS=	if_bridge.c bridgestp.c

SRCS+=	bridge_component.c

CPPFLAGS+=	-I${.CURDIR}/../libnet/opt

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
