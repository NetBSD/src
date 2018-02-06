# $NetBSD: Makefile,v 1.3 2018/02/06 20:10:05 christos Exp $

PROG=	nbsvtool
LDADD+=	-lcrypto
DPADD+=	${LIBCRYPTO}
CPPFLAGS+=-DOPENSSL_API_COMPAT=0x10100000L

.include <bsd.prog.mk>
