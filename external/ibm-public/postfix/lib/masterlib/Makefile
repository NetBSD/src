#	$NetBSD: Makefile,v 1.1 2009/06/25 18:21:52 tron Exp $

LIBISPRIVATE=	yes

.include <bsd.own.mk>

LIB=	master

DIST=	${NETBSDSRCDIR}/external/ibm-public/postfix/dist/src/${LIB}
.PATH:	${DIST}

SRCS=	single_server.c multi_server.c trigger_server.c master_proto.c \
	mail_flow.c

.include <bsd.lib.mk>
