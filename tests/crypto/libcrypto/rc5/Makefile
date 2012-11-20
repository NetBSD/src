# $NetBSD: Makefile,v 1.3.6.1 2012/11/20 23:26:54 matt Exp $

HELPER_NAME=	rc5test
HELPER_DIR=	rc5

.include <bsd.init.mk>

.if ${MKCRYPTO_RC5} != "no"
LDADD= -lcrypto_rc5 -lcrypto
.else
CPPFLAGS+= -DOPENSSL_NO_RC5
.endif

.if ${HAVE_BINUTILS} == "219"
LDADD+=		-Wl,--no-fatal-warnings
.endif
