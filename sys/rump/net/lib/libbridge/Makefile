#	$NetBSD: Makefile,v 1.1 2010/12/08 18:06:43 pooka Exp $
#

.PATH:	${.CURDIR}/../../../../net

LIB=	rumpnet_bridge

SRCS=	if_bridge.c bridgestp.c

SRCS+=	component.c

CPPFLAGS+=	-I${.CURDIR}/../libnet/opt

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
