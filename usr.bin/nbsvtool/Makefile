# $NetBSD: Makefile,v 1.4 2025/07/19 15:56:31 christos Exp $

PROG=	nbsvtool
LDADD+=	-lcrypto -lpthread
DPADD+=	${LIBCRYPTO} ${LIBPTHREAD}
CPPFLAGS+=-DOPENSSL_API_COMPAT=0x10100000L

.include <bsd.prog.mk>
