#	$NetBSD: Makefile,v 1.1.6.2 2009/05/04 08:14:32 yamt Exp $
#

.PATH:	${.CURDIR}/../../../../kern

LIB=	rumpnet_local

SRCS=	uipc_proto.c uipc_usrreq.c

CFLAGS+= -Wno-pointer-sign

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
