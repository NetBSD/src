# $NetBSD: Makefile.inc,v 1.3.34.1 2017/04/21 16:51:25 bouyer Exp $

.include "../Makefile.inc"

BINDIR=		/bin

.if (${MKDYNAMICROOT} == "no")
LDSTATIC?=	-static
.endif

DPADD=	${LIBARCHIVE_FE} ${LIBARCHIVE} ${LIBEXPAT} ${LIBBZ2} ${LIBLZMA} ${LIBZ} ${LIBCRYPTO}
LDADD=	-L${LIBARCHIVE_FE_DIR} -larchive_fe -larchive -lexpat -lbz2 -llzma -lz -lcrypto
CPPFLAGS+=	-I${LIBARCHIVEDIR}/libarchive_fe
