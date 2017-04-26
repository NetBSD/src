# $NetBSD: Makefile.inc,v 1.3.30.1 2017/04/26 02:52:25 pgoyette Exp $

.include "../Makefile.inc"

BINDIR=		/bin

.if (${MKDYNAMICROOT} == "no")
LDSTATIC?=	-static
.endif

DPADD=	${LIBARCHIVE_FE} ${LIBARCHIVE} ${LIBEXPAT} ${LIBBZ2} ${LIBLZMA} ${LIBZ} ${LIBCRYPTO}
LDADD=	-L${LIBARCHIVE_FE_DIR} -larchive_fe -larchive -lexpat -lbz2 -llzma -lz -lcrypto
CPPFLAGS+=	-I${LIBARCHIVEDIR}/libarchive_fe
