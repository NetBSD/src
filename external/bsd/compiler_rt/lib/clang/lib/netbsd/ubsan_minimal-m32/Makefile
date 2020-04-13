#	$NetBSD: Makefile,v 1.1.6.2 2020/04/13 07:45:56 martin Exp $

NOSANITIZER=    # defined

.include "../common.mk"
.include "../sources.mk"

.include <bsd.own.mk>

TOPDIR=	${NETBSDSRCDIR}/sys/external/bsd/compiler_rt/dist

LIB=	libclang_rt.ubsan_minimal-i386

.PATH: ${TOPDIR}/lib/ubsan_minimal

SRCS+=	${UBSAN_MINIMAL_SOURCES}

CXXFLAGS+=	-fno-builtin -fno-exceptions -fno-stack-protector -fno-rtti

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
