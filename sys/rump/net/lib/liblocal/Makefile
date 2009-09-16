#	$NetBSD: Makefile,v 1.1.6.4 2009/09/16 13:38:06 yamt Exp $
#

.PATH:	${.CURDIR}/../../../../kern

LIB=	rumpnet_local

SRCS=	uipc_proto.c uipc_usrreq.c
SRCS+=	component.c

CFLAGS+= -Wno-pointer-sign

RUMP_ISDOMAIN=

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
