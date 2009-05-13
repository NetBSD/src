# $NetBSD: Makefile,v 1.1.2.2 2009/05/13 19:19:20 jym Exp $

HELPER_NAME=	lhashtest
HELPER_DIR=	lhash
HELPER_SRCS=	lh_test.c

CPPFLAGS+=	-DMalloc=malloc
DPADD+=		${LIBCRYPTO} ${LIBCRYPT}

.include <bsd.init.mk>
