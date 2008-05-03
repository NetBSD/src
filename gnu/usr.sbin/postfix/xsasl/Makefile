#	$NetBSD: Makefile,v 1.2 2008/05/03 14:48:31 lukem Exp $

LIBISPRIVATE=	yes

.include <bsd.own.mk>

LIB=	xsasl

DIST=	${NETBSDSRCDIR}/gnu/dist/postfix/src/${LIB}
.PATH:	${DIST}

SRCS=	xsasl_server.c xsasl_cyrus_server.c xsasl_cyrus_log.c \
	xsasl_cyrus_security.c xsasl_client.c xsasl_cyrus_client.c \
	xsasl_dovecot_server.c

.include <bsd.lib.mk>
