#	$NetBSD: Makefile,v 1.2 2021/09/17 02:18:01 christos Exp $

.include "../common.mk"
.include "../sources.mk"

LIB=	libclang_rt.asan-x86_64

.PATH: ${TOPDIR}/lib/interception

SRCS+=	${INTERCEPTION_SOURCES}

.PATH: ${TOPDIR}/lib/asan

SRCS+=	${ASAN_SOURCES}
SRCS+=	${ASAN_CXX_SOURCES}

.PATH: ${TOPDIR}/lib/lsan

SRCS+=	${LSAN_COMMON_SOURCES}

.PATH: ${TOPDIR}/lib/ubsan

SRCS+=	${UBSAN_SOURCES}

.PATH: ${TOPDIR}/lib/sanitizer_common

SRCS+=	${SANITIZER_LIBCDEP_SOURCES}
SRCS+=	${SANITIZER_COVERAGE_SOURCES}
SRCS+=	${SANITIZER_SOURCES}
SRCS+=	${SANITIZER_SYMBOLIZER_SOURCES}

CPPFLAGS+=	-DASAN_DYNAMIC=1

CXXFLAGS+=	-fno-rtti -fno-exceptions -fno-builtin -fno-stack-protector -ftls-model=initial-exec

LIBDIR=		/usr/lib/clang/${CLANG_VER}/lib/netbsd
LIBISCXX=	yes

LIBISMODULE=	yes

CPPFLAGS+=	-I${TOPDIR}/lib

.include "../m64.mk"
.include "../shared_deps.mk"

.include <bsd.lib.mk>
