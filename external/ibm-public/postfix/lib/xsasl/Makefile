#	$NetBSD: Makefile,v 1.2.46.1 2023/12/25 12:55:37 martin Exp $

LIBISPRIVATE=	yes

.include <bsd.own.mk>

LIB=	xsasl

DIST=	${NETBSDSRCDIR}/external/ibm-public/postfix/dist/src/${LIB}
.PATH:	${DIST}

SRCS+= \
xsasl_client.c \
xsasl_cyrus_client.c \
xsasl_cyrus_log.c \
xsasl_cyrus_security.c \
xsasl_cyrus_server.c \
xsasl_dovecot_server.c \
xsasl_saslc_client.c \
xsasl_server.c

.include <bsd.lib.mk>
