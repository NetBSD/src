# $NetBSD: Makefile,v 1.4.2.1 2025/11/20 18:46:42 martin Exp $

PROG=	nbsvtool
LDADD+=	-lcrypto
DPADD+=	${LIBCRYPTO}
CPPFLAGS+=-DOPENSSL_API_COMPAT=0x10100000L

.include <bsd.prog.mk>
