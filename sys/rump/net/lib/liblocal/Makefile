#	$NetBSD: Makefile,v 1.6.6.1 2015/12/27 12:10:17 skrll Exp $
#

.PATH:	${.CURDIR}/../../../../kern

LIB=	rumpnet_local
COMMENT=Local domain sockets (PF_LOCAL/PF_UNIX)

SRCS=	uipc_proto.c uipc_usrreq.c
SRCS+=	local_component.c

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
