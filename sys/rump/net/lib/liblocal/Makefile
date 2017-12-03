#	$NetBSD: Makefile,v 1.5.20.2 2017/12/03 11:39:18 jdolecek Exp $
#

.PATH:	${.CURDIR}/../../../../kern

LIB=	rumpnet_local
COMMENT=Local domain sockets (PF_LOCAL/PF_UNIX)

SRCS=	uipc_proto.c uipc_usrreq.c
SRCS+=	local_component.c

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
