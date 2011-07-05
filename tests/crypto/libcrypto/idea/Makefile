# $NetBSD: Makefile,v 1.3 2011/07/05 10:03:09 spz Exp $

HELPER_NAME=	ideatest
HELPER_DIR=	idea

.include <bsd.init.mk>

.if ${MKCRYPTO_IDEA} != "no"
LDADD= -lcrypto_idea -lcrypto
.else
CPPFLAGS+= -DOPENSSL_NO_IDEA
.endif

LDADD+=		-Wl,--no-fatal-warnings
