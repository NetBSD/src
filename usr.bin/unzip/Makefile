# $NetBSD: Makefile,v 1.8 2024/11/11 11:44:51 martin Exp $

.include <bsd.own.mk>

.include "${NETBSDSRCDIR}/external/bsd/zstd/Makefile.zstd"

PROG=	unzip

DPADD+=		${LIBARCHIVE} ${LIBZ} ${LIBBZ2} ${LIBLZMA} ${LIBCRYPTO} ${LIBPTHREAD}
LDADD+=		-larchive -lz -lbz2 -llzma -lcrypto -lpthread
PROGDPLIBS+=	${ZSTDDPLIBS}

COPTS.unzip.c += -Wno-format-y2k

.include <bsd.prog.mk>
