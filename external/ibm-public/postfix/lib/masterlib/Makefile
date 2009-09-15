#	$NetBSD: Makefile,v 1.1.2.2 2009/09/15 06:04:14 snj Exp $

LIBISPRIVATE=	yes

.include <bsd.own.mk>

LIB=	master

DIST=	${NETBSDSRCDIR}/external/ibm-public/postfix/dist/src/${LIB}
.PATH:	${DIST}

SRCS=	single_server.c multi_server.c trigger_server.c master_proto.c \
	mail_flow.c

.include <bsd.lib.mk>
