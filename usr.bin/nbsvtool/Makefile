# $NetBSD: Makefile,v 1.3.16.1 2025/08/02 05:58:41 perseant Exp $

PROG=	nbsvtool
LDADD+=	-lcrypto -lpthread
DPADD+=	${LIBCRYPTO} ${LIBPTHREAD}
CPPFLAGS+=-DOPENSSL_API_COMPAT=0x10100000L

.include <bsd.prog.mk>
