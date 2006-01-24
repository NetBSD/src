# $NetBSD: Makefile.inc,v 1.1 2006/01/24 18:59:23 elad Exp $

.include <bsd.own.mk>

WARNS=4
DPADD+= ${LIBCRYPTO}
LDADD+= -lcrypto

.if exists(${.CURDIR}/../../Makefile.inc)
.include "${.CURDIR}/../../Makefile.inc"
.endif
