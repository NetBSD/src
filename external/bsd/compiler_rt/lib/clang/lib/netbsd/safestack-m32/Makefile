#	$NetBSD: Makefile,v 1.2 2019/08/23 19:26:02 kamil Exp $

NOSANITIZER=    # defined

.include "../common.mk"
.include "../sources.mk"

.include <bsd.own.mk>

TOPDIR=	${NETBSDSRCDIR}/sys/external/bsd/compiler_rt/dist

LIB=	libclang_rt.safestack-i386

.PATH: ${TOPDIR}/lib/safestack

SRCS+=	${SAFESTACK_SOURCES}

.PATH: ${TOPDIR}/lib/interception

SRCS+=  ${INTERCEPTION_SOURCES}

.PATH: ${TOPDIR}/lib/sanitizer_common

SRCS+=	${SANITIZER_SOURCES}
SRCS+=	${SANITIZER_NOLIBC_SOURCES}

CXXFLAGS+=	-fno-rtti -fno-exceptions -fno-builtin -fno-stack-protector

COPTS.safestack.cc+=	${${ACTIVE_CXX} == "gcc" :? -Wno-error :}

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
