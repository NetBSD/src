#	$NetBSD: Makefile,v 1.2 2026/01/20 18:06:12 christos Exp $

UNSUPPORTED_COMPILER.clang=	# defined
NOLINT=yes	# no complex128?

.include <bsd.own.mk>

DIST=	${GCCDIST}

LIB=	quadmath

SHLIB_MAJOR=	0
SHLIB_MINOR=	0

.include "${.CURDIR}/defs.mk"
SRCS=	${G_libquadmath_la_SOURCES:S,math/,,:S,printf/,,:S,strtod/,,}

CPPFLAGS+= \
	-DHAVE_CONFIG_H \
	-I${.CURDIR} \
	-I${DIST}/libquadmath \
	-I${DIST}/config \
	-I${DIST}/include

LIBDPLIBS+=	m ${NETBSDSRCDIR}/lib/libm

LDFLAGS+=	-Wl,--version-script,${DIST}/libquadmath/quadmath.map

COPTS.mul.c += -Wno-error=stack-protector
COPTS.printf_fp.c += -Wno-error=stack-protector

.include <bsd.lib.mk>

.PATH: ${DIST}/libquadmath
.PATH: ${DIST}/libquadmath/math
.PATH: ${DIST}/libquadmath/printf
.PATH: ${DIST}/libquadmath/strtod
