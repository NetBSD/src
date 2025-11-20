# $NetBSD: Makefile.inc,v 1.3.2.1 2025/11/20 18:46:42 martin Exp $

.include <bsd.own.mk>

DPADD+= ${LIBCRYPTO}
LDADD+= -lcrypto

.if exists(${.CURDIR}/../../Makefile.inc)
.include "${.CURDIR}/../../Makefile.inc"
.endif
