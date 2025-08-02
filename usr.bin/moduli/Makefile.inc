# $NetBSD: Makefile.inc,v 1.2.58.1 2025/08/02 05:58:41 perseant Exp $

.include <bsd.own.mk>

DPADD+= ${LIBCRYPTO} ${LIBPTHREAD}
LDADD+= -lcrypto -lpthread

.if exists(${.CURDIR}/../../Makefile.inc)
.include "${.CURDIR}/../../Makefile.inc"
.endif
