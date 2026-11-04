# $NetBSD: Makefile.inc,v 1.5.24.1 2026/11/04 22:39:54 perseant Exp $

.include <bsd.own.mk>

.include "${.PARSEDIR}/../Makefile.inc"

LIBINSTALL != cd ${.PARSEDIR}/../lib && ${PRINTOBJDIR}

BINDIR?=	/usr/sbin
CPPFLAGS+=	-DBINDIR='"${BINDIR}"'

DPADD+=	${LIBINSTALL}/libinstall.a
LDADD+=	-L${LIBINSTALL} -linstall -ltermcap

DPADD+=		${LIBNETPGPVERIFY} ${LIBFETCH} ${LIBSSL} ${LIBCRYPTO}
LDADD+=		-lnetpgpverify -lfetch -lssl -lcrypto

DPADD+=		${LIBARCHIVE}
LDADD+=		-larchive
.include "${NETBSDSRCDIR}/external/bsd/zstd/Makefile.zstd"
PROGDPLIBS+=	${ZSTDDPLIBS}	# XXX needed by libarchive

DPADD+=		${LIBZ} ${LIBBZ2}
LDADD+=		-lz -lbz2

DPADD+=		${LIBLZMA} ${LIBPTHREAD}
LDADD+=		-llzma -lpthread
