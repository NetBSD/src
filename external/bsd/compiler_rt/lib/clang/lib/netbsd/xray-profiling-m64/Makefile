#	$NetBSD: Makefile,v 1.1.6.2 2020/04/13 07:45:57 martin Exp $

NOSANITIZER=    # defined

.include "../common.mk"
.include "../sources.mk"

.include <bsd.own.mk>

TOPDIR=	${NETBSDSRCDIR}/sys/external/bsd/compiler_rt/dist

LIB=	libclang_rt.xray-profiling-x86_64

.PATH: ${TOPDIR}/lib/xray

SRCS+=	${XRAY_PROFILING_MODE_SOURCES}

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
CPPFLAGS+=	-I${TOPDIR}/include

CWARNFLAGS.clang+=	-Wno-error=cast-qual-unrelated

COPTS+=		-fPIC

.include "../m64.mk"

.include <bsd.lib.mk>
