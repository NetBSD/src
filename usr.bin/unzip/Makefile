# $NetBSD: Makefile,v 1.3 2017/04/21 23:06:18 christos Exp $

.include <bsd.own.mk>
PROG=	unzip

DPADD+=		${LIBARCHIVE} ${LIBZ} ${LIBBZ2}
LDADD+=		-larchive -lz -lbz2

.if ${MKCRYPTO:Uno} 
LDADD=  -lcrypto
DPADD=  ${LIBCRYPTO}
.endif

COPTS.unzip.c += -Wno-format-y2k

.include <bsd.prog.mk>
