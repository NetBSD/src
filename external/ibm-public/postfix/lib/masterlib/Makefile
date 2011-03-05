#	$NetBSD: Makefile,v 1.1.6.1 2011/03/05 15:09:06 bouyer Exp $

LIBISPRIVATE=	yes

.include <bsd.own.mk>

LIB=	master

DIST=	${NETBSDSRCDIR}/external/ibm-public/postfix/dist/src/${LIB}
.PATH:	${DIST}

SRCS=	single_server.c multi_server.c trigger_server.c master_proto.c \
	mail_flow.c event_server.c

.include <bsd.lib.mk>
