#	$NetBSD: Makefile,v 1.1.2.1 2009/07/23 23:32:55 jym Exp $
#

.PATH:	${.CURDIR}/../../../../kern

LIB=	rumpnet_local

SRCS=	uipc_proto.c uipc_usrreq.c
SRCS+=	component.c

CFLAGS+= -Wno-pointer-sign

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
