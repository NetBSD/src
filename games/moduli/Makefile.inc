# $NetBSD: Makefile.inc,v 1.2 2006/01/20 14:19:31 elad Exp $

.include <bsd.own.mk>

WARNS=4
DPADD+= ${LIBCRYPTO}
LDADD+= -lcrypto

.if exists(${.CURDIR}/../../Makefile.inc)
.include "${.CURDIR}/../../Makefile.inc"
.endif
