# $NetBSD: Makefile,v 1.2 2018/02/06 19:51:03 christos Exp $

PROG=	nbsvtool
LDADD+=	-lcrypto
DPADD+=	${LIBCRYPTO}
CPPFLAGS+=-DOPENSSL_VERSION_NUMBER=0x10100000L

.include <bsd.prog.mk>
