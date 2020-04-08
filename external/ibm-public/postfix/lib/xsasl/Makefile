#	$NetBSD: Makefile,v 1.2.44.1 2020/04/08 14:07:01 martin Exp $

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
