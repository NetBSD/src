# $NetBSD: Makefile,v 1.3.6.2 2012/11/20 23:26:54 matt Exp $

HELPER_NAME=	mdc2test
HELPER_DIR=	mdc2

.include <bsd.init.mk>

LDADD= -lcrypto

.if ${HAVE_BINUTILS} == "219"
LDADD+=         -Wl,--no-fatal-warnings
.endif
