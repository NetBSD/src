#	$NetBSD: Makefile,v 1.4 2023/06/03 21:33:02 lukem Exp $

.include "../common.mk"
.include "../sources.mk"

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
CWARNFLAGS.gcc+=	${CC_WNO_STRINGOP_OVERFLOW}

COPTS+=		-fPIC

.include "../m64.mk"

.include <bsd.lib.mk>
