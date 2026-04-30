# $NetBSD: Makefile,v 1.9 2026/04/30 13:31:47 simonb Exp $

.include <bsd.own.mk>

PROG=	unzip

DPADD+=		${LIBARCHIVE} ${LIBZ} ${LIBBZ2} ${LIBLZMA} ${LIBCRYPTO} ${LIBPTHREAD}
LDADD+=		-larchive -lz -lbz2 -llzma -lcrypto -lpthread
.include "${NETBSDSRCDIR}/external/bsd/zstd/Makefile.zstd"
PROGDPLIBS+=	${ZSTDDPLIBS}	# XXX needed by libarchive

COPTS.unzip.c+=	-Wno-format-y2k

.include <bsd.prog.mk>
