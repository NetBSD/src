#	$NetBSD: Makefile,v 1.1.6.3 2009/06/20 07:20:36 yamt Exp $
#

.PATH:	${.CURDIR}/../../../../kern

LIB=	rumpnet_local

SRCS=	uipc_proto.c uipc_usrreq.c
SRCS+=	component.c

CFLAGS+= -Wno-pointer-sign

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
