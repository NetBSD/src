#	$NetBSD: Makefile.isl,v 1.2 2024/08/10 23:49:12 mrg Exp $

.ifndef _EXTERNAL_GPL3_GCC_USR_BIN_MAKEFILE_LIBISL_
_EXTERNAL_GPL3_GCC_USR_BIN_MAKEFILE_LIBISL_=1

.include <bsd.own.mk>

.if !defined(NOGCCISL)

LIBISL=		${.CURDIR}/../../../../mit/isl
LIBISLOBJ!=	cd ${LIBISL}/lib/libisl && ${PRINTOBJDIR}
DPADD+=		${LIBISLOBJ}/libisl.a
LDADD+=		${LIBISLOBJ}/libisl.a

CFLAGS+=	-I${LIBISL}/dist/include -I${LIBISL}/include

.endif

.endif
