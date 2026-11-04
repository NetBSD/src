# $NetBSD: Makefile.inc,v 1.2.58.2 2026/11/04 23:28:40 perseant Exp $

.include <bsd.own.mk>

DPADD+= ${LIBCRYPTO}
LDADD+= -lcrypto

.if exists(${.CURDIR}/../../Makefile.inc)
.include "${.CURDIR}/../../Makefile.inc"
.endif
