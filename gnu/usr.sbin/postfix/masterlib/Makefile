#	$NetBSD: Makefile,v 1.7 2008/05/03 14:48:31 lukem Exp $

LIBISPRIVATE=	yes

.include <bsd.own.mk>

LIB=	master

DIST=	${NETBSDSRCDIR}/gnu/dist/postfix/src/${LIB}
.PATH:	${DIST}

SRCS=	single_server.c multi_server.c trigger_server.c master_proto.c \
	mail_flow.c

.include <bsd.lib.mk>
