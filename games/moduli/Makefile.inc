# $NetBSD: Makefile.inc,v 1.1 2006/01/19 23:23:58 elad Exp $

.include <bsd.own.mk>

WARNS=4
DPADD+= ${LIBCRYPTO}
LDADD+= -lcrypto
