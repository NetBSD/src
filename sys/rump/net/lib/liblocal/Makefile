#	$NetBSD: Makefile,v 1.3 2009/09/13 22:51:42 pooka Exp $
#

.PATH:	${.CURDIR}/../../../../kern

LIB=	rumpnet_local

SRCS=	uipc_proto.c uipc_usrreq.c
SRCS+=	component.c

CFLAGS+= -Wno-pointer-sign

RUMP_ISDOMAIN=

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
