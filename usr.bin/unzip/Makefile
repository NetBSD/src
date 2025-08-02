# $NetBSD: Makefile,v 1.7.12.1 2025/08/02 05:58:44 perseant Exp $

.include <bsd.own.mk>

.include "${NETBSDSRCDIR}/external/bsd/zstd/Makefile.zstd"

PROG=	unzip

DPADD+=		${LIBARCHIVE} ${LIBZ} ${LIBBZ2} ${LIBLZMA} ${LIBCRYPTO} ${LIBPTHREAD}
LDADD+=		-larchive -lz -lbz2 -llzma -lcrypto -lpthread
PROGDPLIBS+=	${ZSTDDPLIBS}

COPTS.unzip.c += -Wno-format-y2k

.include <bsd.prog.mk>
