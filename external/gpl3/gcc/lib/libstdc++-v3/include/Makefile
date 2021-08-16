#	$NetBSD: Makefile,v 1.13 2021/08/16 06:56:27 mrg Exp $

SUBDIR=		backward ext bits debug decimal parallel pb profile pstl
SUBDIR+=	tr1 tr2 experimental

.include <bsd.init.mk>
.include <bsd.own.mk>

.include "${.CURDIR}/../arch/${GCC_MACHINE_ARCH}/defs.mk"

.cc: # disable .cc->NULL transform

DIST=		${GCCDIST}
GNUHOSTDIST=	${DIST}

# XXXmknative libstdc++-v3/libsupc++/Makefile.am::std_HEADERS
# libsupc++
INCS=		compare cxxabi.h exception initializer_list new typeinfo

# others
INCS+=		cerrno
INCS+=		${G_std_headers} ${G_c_base_headers}
INCS+=		${G_c_compatibility_headers_extra}
INCSDIR=	/usr/include/g++

.PATH: ${DIST}/libstdc++-v3
.PATH: ${DIST}/libstdc++-v3/libsupc++
.PATH: ${DIST}/libstdc++-v3/include/c_global
.PATH: ${DIST}/libstdc++-v3/include/std

.include "Makefile.includes"

.include <bsd.subdir.mk>
