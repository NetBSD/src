#	$NetBSD: Makefile,v 1.2 2021/09/17 02:18:02 christos Exp $

.include "../common.mk"
.include "../sources.mk"

LIB=	libclang_rt.asan-i386

.PATH: ${TOPDIR}/lib/interception

SRCS+=	${INTERCEPTION_SOURCES}

.PATH: ${TOPDIR}/lib/asan

SRCS+=	${ASAN_PREINIT_SOURCES}
SRCS+=	${ASAN_SOURCES}

.PATH: ${TOPDIR}/lib/lsan

SRCS+=	${LSAN_COMMON_SOURCES}

.PATH: ${TOPDIR}/lib/ubsan

SRCS+=	${UBSAN_SOURCES}

.PATH: ${TOPDIR}/lib/sanitizer_common

SRCS+=	${SANITIZER_LIBCDEP_SOURCES}
SRCS+=	${SANITIZER_COVERAGE_SOURCES}
SRCS+=	${SANITIZER_SOURCES}
SRCS+=	${SANITIZER_SYMBOLIZER_SOURCES}

CXXFLAGS+=	-fno-rtti -fno-exceptions -fno-builtin -fno-stack-protector

LIBDIR=		/usr/lib/clang/${CLANG_VER}/lib/netbsd
LIBISCXX=	yes

_LIB_PREFIX=
MKDEBUGLIB=	no
MKLINT=		no
MKPICINSTALL=	no
MKPROFILE=	no
MKSTATICLIB=	yes

COPTS+=		-fPIC
CPPFLAGS+=	-I${TOPDIR}/lib

.include "../m32.mk"

.include <bsd.lib.mk>
