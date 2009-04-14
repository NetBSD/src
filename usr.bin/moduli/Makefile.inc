# $NetBSD: Makefile.inc,v 1.2 2009/04/14 22:15:24 lukem Exp $

.include <bsd.own.mk>

DPADD+= ${LIBCRYPTO}
LDADD+= -lcrypto

.if exists(${.CURDIR}/../../Makefile.inc)
.include "${.CURDIR}/../../Makefile.inc"
.endif
