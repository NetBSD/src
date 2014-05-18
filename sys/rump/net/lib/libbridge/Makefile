#	$NetBSD: Makefile,v 1.1.24.1 2014/05/18 17:46:19 rmind Exp $
#

.PATH:	${.CURDIR}/../../../../net

LIB=	rumpnet_bridge

SRCS=	if_bridge.c bridgestp.c

SRCS+=	bridge_component.c

CPPFLAGS+=	-I${.CURDIR}/../libnet/opt

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
