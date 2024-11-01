# $NetBSD: Makefile.inc,v 1.6 2024/11/01 00:00:14 christos Exp $

.include "../Makefile.inc"

BINDIR=		/bin

.if (${MKDYNAMICROOT} == "no")
LDSTATIC?=	-static
.endif

DPADD+=	${LIBARCHIVE_FE} ${LIBARCHIVE} ${LIBEXPAT} ${LIBZSTD}
DPADD+=	${LIBBZ2} ${LIBLZMA} ${LIBZ} ${LIBCRYPTO} ${LIBPTHREAD}
LDADD+=	-L${LIBARCHIVE_FE_DIR} -larchive_fe -larchive -lexpat -lzstd
LDADD+=	-lbz2 -llzma -lz -lcrypto -lpthread

CPPFLAGS+=	-I${LIBARCHIVEDIR}/libarchive_fe
