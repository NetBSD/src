#	$NetBSD: Makefile,v 1.3 2020/03/18 19:05:23 christos Exp $

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
