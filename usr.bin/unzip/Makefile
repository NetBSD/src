# $NetBSD: Makefile,v 1.7.12.2 2026/11/04 23:28:44 perseant Exp $

.include <bsd.own.mk>

PROG=	unzip

DPADD+=		${LIBARCHIVE} ${LIBZ} ${LIBBZ2} ${LIBLZMA} ${LIBCRYPTO} ${LIBPTHREAD}
LDADD+=		-larchive -lz -lbz2 -llzma -lcrypto -lpthread
.include "${NETBSDSRCDIR}/external/bsd/zstd/Makefile.zstd"
PROGDPLIBS+=	${ZSTDDPLIBS}	# XXX needed by libarchive

COPTS.unzip.c+=	-Wno-format-y2k

.include <bsd.prog.mk>
