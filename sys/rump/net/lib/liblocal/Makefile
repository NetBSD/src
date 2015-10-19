#	$NetBSD: Makefile,v 1.7 2015/10/19 16:16:38 pooka Exp $
#

.PATH:	${.CURDIR}/../../../../kern

LIB=	rumpnet_local
COMMENT=Local domain sockets (PF_LOCAL/PF_UNIX)

SRCS=	uipc_proto.c uipc_usrreq.c
SRCS+=	local_component.c

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
