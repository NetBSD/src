#	$NetBSD: Makefile,v 1.6 2004/05/23 02:24:51 lukem Exp $

MKPRIVATELIB=	yes

.include <bsd.own.mk>

LIB=	master

DIST=	${NETBSDSRCDIR}/gnu/dist/postfix/src/${LIB}
.PATH:	${DIST}

SRCS=	single_server.c multi_server.c trigger_server.c master_proto.c \
	mail_flow.c

.include <bsd.lib.mk>
