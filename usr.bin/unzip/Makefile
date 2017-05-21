# $NetBSD: Makefile,v 1.6 2017/05/21 15:28:43 riastradh Exp $

.include <bsd.own.mk>
PROG=	unzip

DPADD+=		${LIBARCHIVE} ${LIBZ} ${LIBBZ2}
LDADD+=		-larchive -lz -lbz2

LDADD+=  -lcrypto
DPADD+=  ${LIBCRYPTO}

COPTS.unzip.c += -Wno-format-y2k

.include <bsd.prog.mk>
