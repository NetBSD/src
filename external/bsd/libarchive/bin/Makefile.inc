# $NetBSD: Makefile.inc,v 1.5.12.1 2025/08/02 05:21:37 perseant Exp $

.include "../Makefile.inc"

BINDIR=		/bin

.if (${MKDYNAMICROOT} == "no")
LDSTATIC?=	-static
.endif

DPADD+=	${LIBARCHIVE_FE} ${LIBARCHIVE} ${LIBEXPAT} ${LIBZSTD}
DPADD+=	${LIBBZ2} ${LIBLZMA} ${LIBZ} ${LIBCRYPTO} ${LIBPTHREAD}
LDADD+=	-L${LIBARCHIVE_FE_DIR} -larchive_fe -larchive -lexpat -lzstd
LDADD+=	-lbz2 -llzma -lz -lcrypto -lpthread
PROGDPLIBS+= ${ZSTDDPLIBS}

CPPFLAGS+=	-I${LIBARCHIVEDIR}/libarchive_fe
