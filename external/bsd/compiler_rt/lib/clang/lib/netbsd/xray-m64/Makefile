#	$NetBSD: Makefile,v 1.3 2021/09/17 02:18:05 christos Exp $

.include "../common.mk"
.include "../sources.mk"

LIB=	libclang_rt.xray-x86_64

.PATH: ${TOPDIR}/lib/xray

SRCS+=	${XRAY_SOURCES}
SRCS+=	${XRAY_ARCH_SOURCES}

.PATH: ${TOPDIR}/lib/sanitizer_common

SRCS+=	${SANITIZER_SOURCES}
SRCS+=	${SANITIZER_LIBCDEP_SOURCES}

CXXFLAGS+=	-fno-rtti -fno-exceptions -fno-builtin -fno-stack-protector

COPTS.xray_init.cc+=	${${ACTIVE_CXX} == "gcc" :? -Wno-error :}

# Macro defined in newer builtin subdirectory, it has to be empty on NetBSD
CPPFLAGS+=	-DNO_EXEC_STACK_DIRECTIVE=""

LIBDIR=		/usr/lib/clang/${CLANG_VER}/lib/netbsd
LIBISCXX=	yes

_LIB_PREFIX=
MKDEBUGLIB=	no
MKLINT=		no
MKPICINSTALL=	no
MKPROFILE=	no
MKSTATICLIB=	yes

CPPFLAGS+=	-I${TOPDIR}/lib
CPPFLAGS+=	-I${TOPDIR}/include

COPTS+=		-fPIC

.include "../m64.mk"

.include <bsd.lib.mk>
