#	$NetBSD: Makefile,v 1.2 2021/09/17 02:18:02 christos Exp $

.include "../common.mk"
.include "../sources.mk"

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
