# $NetBSD: Makefile,v 1.7 2019/07/28 12:44:36 joerg Exp $

.include <bsd.own.mk>
PROG=	unzip

DPADD+=		${LIBARCHIVE} ${LIBZ} ${LIBBZ2} ${LIBLZMA} ${LIBCRYPTO} ${LIBPTHREAD}
LDADD+=		-larchive -lz -lbz2 -llzma -lcrypto -lpthread

COPTS.unzip.c += -Wno-format-y2k

.include <bsd.prog.mk>
