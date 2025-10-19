# $NetBSD: Makefile.inc,v 1.4 2025/10/19 01:00:00 riastradh Exp $

.include <bsd.own.mk>

DPADD+= ${LIBCRYPTO}
LDADD+= -lcrypto

.if exists(${.CURDIR}/../../Makefile.inc)
.include "${.CURDIR}/../../Makefile.inc"
.endif
