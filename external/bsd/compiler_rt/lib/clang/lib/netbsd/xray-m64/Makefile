#	$NetBSD: Makefile,v 1.2 2019/08/23 19:26:02 kamil Exp $

NOSANITIZER=    # defined

.include "../common.mk"
.include "../sources.mk"

.include <bsd.own.mk>

TOPDIR=	${NETBSDSRCDIR}/sys/external/bsd/compiler_rt/dist

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
