#	$NetBSD: Makefile,v 1.1 2009/06/23 15:02:56 tron Exp $

LIBISPRIVATE=	yes

.include <bsd.own.mk>

LIB=	xsasl

DIST=	${NETBSDSRCDIR}/external/ibm-public/postfix/dist/src/${LIB}
.PATH:	${DIST}

SRCS=	xsasl_server.c xsasl_cyrus_server.c xsasl_cyrus_log.c \
	xsasl_cyrus_security.c xsasl_client.c xsasl_cyrus_client.c \
	xsasl_dovecot_server.c

.include <bsd.lib.mk>
