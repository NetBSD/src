# $NetBSD: Makefile,v 1.3 2011/07/05 10:03:10 spz Exp $

HELPER_NAME=	mdc2test
HELPER_DIR=	mdc2

.include <bsd.init.mk>

.if ${MKCRYPTO_MDC2} != "no"
LDADD= -lcrypto_mdc2 -lcrypto
.else
CPPFLAGS+= -DOPENSSL_NO_MDC2
.endif

LDADD+=         -Wl,--no-fatal-warnings
