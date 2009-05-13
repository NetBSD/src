# $NetBSD: Makefile.inc,v 1.1.30.1 2009/05/13 19:19:58 jym Exp $

.include <bsd.own.mk>

DPADD+= ${LIBCRYPTO}
LDADD+= -lcrypto

.if exists(${.CURDIR}/../../Makefile.inc)
.include "${.CURDIR}/../../Makefile.inc"
.endif
