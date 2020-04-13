#	$NetBSD: Makefile,v 1.4.4.2 2020/04/13 07:45:54 martin Exp $

NOSANITIZER=    # defined

.include "../common.mk"
.include "../sources.mk"

.include <bsd.own.mk>

TOPDIR=	${NETBSDSRCDIR}/sys/external/bsd/compiler_rt/dist

LIB=	libclang_rt.asan_cxx-x86_64

.PATH: ${TOPDIR}/lib/asan

SRCS+=	${ASAN_CXX_SOURCES}

.PATH: ${TOPDIR}/lib/ubsan

SRCS+=	${UBSAN_CXX_SOURCES}

CXXFLAGS+=	-fno-exceptions -fno-builtin -fno-stack-protector # RTTI

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

SYMS=		libclang_rt.asan_cxx-x86_64.a.syms
SYMS_EXTRA=	${TOPDIR}/lib/asan/asan.syms.extra # Overlooked in upstream compiler-rt (8.0) ?

beforeinstall: ${SYMS}

CLEANFILES+=	${SYMS}
FILES=		${SYMS}
FILESDIR=	${LIBDIR}

.include <bsd.lib.mk>
