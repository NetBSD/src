# $NetBSD: Makefile,v 1.6.10.1 2020/04/13 08:05:49 martin Exp $

.include <bsd.own.mk>
PROG=	unzip

DPADD+=		${LIBARCHIVE} ${LIBZ} ${LIBBZ2} ${LIBLZMA} ${LIBCRYPTO} ${LIBPTHREAD}
LDADD+=		-larchive -lz -lbz2 -llzma -lcrypto -lpthread

COPTS.unzip.c += -Wno-format-y2k

.include <bsd.prog.mk>
