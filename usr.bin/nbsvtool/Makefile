# $NetBSD: Makefile,v 1.3.16.2 2026/11/04 23:28:40 perseant Exp $

PROG=	nbsvtool
LDADD+=	-lcrypto
DPADD+=	${LIBCRYPTO}
CPPFLAGS+=-DOPENSSL_API_COMPAT=0x10100000L

.include <bsd.prog.mk>
