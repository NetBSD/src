#	$NetBSD: Makefile,v 1.1 2019/08/08 14:00:30 kamil Exp $

NOSANITIZER=    # defined

.include "../common.mk"
.include "../sources.mk"

.include <bsd.own.mk>

TOPDIR=	${NETBSDSRCDIR}/sys/external/bsd/compiler_rt/dist

LIB=	libclang_rt.msan_cxx-x86_64

.PATH: ${TOPDIR}/lib/ubsan

SRCS+=	${UBSAN_CXX_SOURCES}

.PATH: ${TOPDIR}/lib/msan

SRCS+=	${MSAN_RTL_CXX_SOURCES}

CXXFLAGS+=	-fno-rtti -fno-exceptions -fno-builtin -fno-stack-protector

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

.include "../m64.mk"

.include "../syms.mk"

FILES=		libclang_rt.msan_cxx-x86_64.a.syms
FILESDIR=	${LIBDIR}

.include <bsd.lib.mk>
