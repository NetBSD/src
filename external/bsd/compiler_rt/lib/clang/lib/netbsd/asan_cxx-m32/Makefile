#	$NetBSD: Makefile,v 1.2 2021/09/17 02:18:02 christos Exp $

.include "../common.mk"
.include "../sources.mk"

LIB=	libclang_rt.asan_cxx-i386

.PATH: ${TOPDIR}/lib/asan

SRCS+=	${ASAN_CXX_SOURCES}

.PATH: ${TOPDIR}/lib/ubsan

SRCS+=	${UBSAN_CXX_SOURCES}

CXXFLAGS+=	-fno-exceptions -fno-builtin -fno-stack-protector # RTTI needed

LIBDIR=		/usr/lib/clang/${CLANG_VER}/lib/netbsd
LIBISCXX=	yes

_LIB_PREFIX=
MKDEBUGLIB=	no
MKLINT=		no
MKPICINSTALL=	no
MKPROFILE=	no
MKSTATICLIB=	yes

CPPFLAGS+=	-I${TOPDIR}/lib

COPTS+=		-fPIC

.include "../m32.mk"

.include <bsd.lib.mk>
