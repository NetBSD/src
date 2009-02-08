#	$NetBSD: Makefile,v 1.1 2009/02/08 16:28:35 pooka Exp $
#

.PATH:	${.CURDIR}/../../../../kern

LIB=	rumpnet_local

SRCS=	uipc_proto.c uipc_usrreq.c

CFLAGS+= -Wno-pointer-sign

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
