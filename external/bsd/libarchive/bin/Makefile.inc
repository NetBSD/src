# $NetBSD: Makefile.inc,v 1.2 2010/02/20 02:55:53 joerg Exp $

.include "../Makefile.inc"

BINDIR=		/bin

.if (${MKDYNAMICROOT} == "no")
LDSTATIC?=	-static
.endif

DPADD=	${LIBARCHIVE_FE} ${LIBARCHIVE} ${LIBBZ2} ${LIBZ}
LDADD=	-L${LIBARCHIVE_FE_DIR} -larchive_fe -larchive -lbz2 -lz
CPPFLAGS+=	-I${LIBARCHIVEDIR}/libarchive_fe
