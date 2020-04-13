#	$NetBSD: Makefile,v 1.1.6.2 2020/04/13 07:45:56 martin Exp $

NOSANITIZER=    # defined

.include "../common.mk"
.include "../sources.mk"

.include <bsd.own.mk>

TOPDIR=	${NETBSDSRCDIR}/sys/external/bsd/compiler_rt/dist

LIB=	libclang_rt.ubsan_standalone-x86_64

.PATH: ${TOPDIR}/lib/interception

SRCS+=	${INTERCEPTION_SOURCES}

.PATH: ${TOPDIR}/lib/sanitizer_common

SRCS+=	${SANITIZER_SOURCES}
SRCS+=	${SANITIZER_LIBCDEP_SOURCES}
SRCS+=	${SANITIZER_COVERAGE_SOURCES}
SRCS+=	${SANITIZER_SYMBOLIZER_SOURCES}

.PATH: ${TOPDIR}/lib/ubsan

SRCS+=	${UBSAN_SOURCES}
SRCS+=	${UBSAN_STANDALONE_SOURCES}

CXXFLAGS+=	-fno-builtin -fno-exceptions -fno-stack-protector -fno-rtti

LIBDIR=		/usr/lib/clang/${CLANG_VER}/lib/netbsd
LIBISCXX=	yes

LIBISMODULE=	yes

CPPFLAGS+=	-I${TOPDIR}/lib

.include "../m64.mk"
.include "../shared_deps.mk"

.include <bsd.lib.mk>
