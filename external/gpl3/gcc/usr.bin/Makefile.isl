#	$NetBSD: Makefile.isl,v 1.1 2024/02/25 00:28:02 mrg Exp $

.ifndef _EXTERNAL_GPL3_GCC_USR_BIN_MAKEFILE_LIBISL_
_EXTERNAL_GPL3_GCC_USR_BIN_MAKEFILE_LIBISL_=1

.include <bsd.own.mk>

LIBISL=		${.CURDIR}/../../../../mit/isl
LIBISLOBJ!=	cd ${LIBISL}/lib/libisl && ${PRINTOBJDIR}
DPADD+=		${LIBISLOBJ}/libisl.a
LDADD+=		${LIBISLOBJ}/libisl.a

CFLAGS+=	-I${LIBISL}/dist/include -I${LIBISL}/include

.endif
