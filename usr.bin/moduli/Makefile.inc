# $NetBSD: Makefile.inc,v 1.3 2025/07/19 12:42:30 christos Exp $

.include <bsd.own.mk>

DPADD+= ${LIBCRYPTO} ${LIBPTHREAD}
LDADD+= -lcrypto -lpthread

.if exists(${.CURDIR}/../../Makefile.inc)
.include "${.CURDIR}/../../Makefile.inc"
.endif
