# $NetBSD: Makefile.inc,v 1.4.12.1 2020/04/13 07:46:02 martin Exp $

.include "../Makefile.inc"

BINDIR=		/bin

.if (${MKDYNAMICROOT} == "no")
LDSTATIC?=	-static
.endif

DPADD=	${LIBARCHIVE_FE} ${LIBARCHIVE} ${LIBEXPAT} ${LIBBZ2} ${LIBLZMA} ${LIBZ} \
	${LIBCRYPTO} ${LIBPTHREAD}
LDADD=	-L${LIBARCHIVE_FE_DIR} -larchive_fe -larchive -lexpat -lbz2 -llzma -lz \
	-lcrypto -lpthread

CPPFLAGS+=	-I${LIBARCHIVEDIR}/libarchive_fe
