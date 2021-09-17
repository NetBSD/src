#	$NetBSD: Makefile,v 1.2 2021/09/17 02:18:04 christos Exp $

.include "../common.mk"
.include "../sources.mk"

LIB=	libclang_rt.xray-basic-x86_64

.PATH: ${TOPDIR}/lib/xray

SRCS+=	${XRAY_BASIC_MODE_SOURCES}

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

COPTS+=		-fPIC

.include "../m64.mk"

.include <bsd.lib.mk>
