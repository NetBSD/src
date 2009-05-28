#	$NetBSD: Makefile,v 1.2 2009/05/28 00:02:16 pooka Exp $
#

.PATH:	${.CURDIR}/../../../../kern

LIB=	rumpnet_local

SRCS=	uipc_proto.c uipc_usrreq.c
SRCS+=	component.c

CFLAGS+= -Wno-pointer-sign

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
