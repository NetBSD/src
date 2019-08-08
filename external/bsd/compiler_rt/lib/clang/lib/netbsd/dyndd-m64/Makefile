#	$NetBSD: Makefile,v 1.1 2019/08/08 14:00:30 kamil Exp $

NOSANITIZER=    # defined

.include "../common.mk"
.include "../sources.mk"

.include <bsd.own.mk>

TOPDIR=	${NETBSDSRCDIR}/sys/external/bsd/compiler_rt/dist

LIB=	libclang_rt.dyndd-x86_64

.PATH: ${TOPDIR}/lib/tsan/dd

SRCS+=	${DD_SOURCES}

.PATH: ${TOPDIR}/lib/interception

SRCS+=	${INTERCEPTION_SOURCES}

.PATH: ${TOPDIR}/lib/sanitizer_common

SRCS+=	${SANITIZER_LIBCDEP_SOURCES}
SRCS+=	${SANITIZER_SOURCES}
SRCS+=	${SANITIZER_SYMBOLIZER_SOURCES}

CXXFLAGS+=	-fno-rtti -fno-exceptions -fno-builtin -fno-stack-protector

LIBDIR=		/usr/lib/clang/${CLANG_VER}/lib/netbsd
LIBISCXX=	yes

LIBISMODULE=	yes

CPPFLAGS+=	-I${TOPDIR}/lib

COPTS+=		-fPIC

.include "../m64.mk"
.include "../shared_deps.mk"

.include <bsd.lib.mk>
