#	$NetBSD: Makefile,v 1.3 2019/08/27 22:48:55 kamil Exp $

NOSANITIZER=    # defined

.include "../common.mk"
.include "../sources.mk"

.include <bsd.own.mk>

TOPDIR=	${NETBSDSRCDIR}/sys/external/bsd/compiler_rt/dist

LIB=	libclang_rt.tsan_cxx-x86_64

.PATH: ${TOPDIR}/lib/tsan/rtl

SRCS+=	${TSAN_CXX_SOURCES}

.PATH: ${TOPDIR}/lib/ubsan

SRCS+=	${UBSAN_CXX_SOURCES}

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

.include "../m64.mk"
.include "../syms.mk"

SYMS=		libclang_rt.tsan_cxx-x86_64.a.syms

beforeinstall: ${SYMS}

CLEANFILES+=	${SYMS}
FILES=		${SYMS}
FILESDIR=	${LIBDIR}

.include <bsd.lib.mk>
