#	$NetBSD: Makefile,v 1.1.6.1 2011/02/17 11:59:18 bouyer Exp $

LIBISPRIVATE=	yes

.include <bsd.own.mk>

LIB=	xsasl

DIST=	${NETBSDSRCDIR}/external/ibm-public/postfix/dist/src/${LIB}
.PATH:	${DIST}

SRCS=	xsasl_server.c xsasl_cyrus_server.c xsasl_cyrus_log.c \
	xsasl_cyrus_security.c xsasl_client.c xsasl_cyrus_client.c \
	xsasl_dovecot_server.c xsasl_saslc_client.c

.include <bsd.lib.mk>
