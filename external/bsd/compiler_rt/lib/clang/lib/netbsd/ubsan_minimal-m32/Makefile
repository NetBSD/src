#	$NetBSD: Makefile,v 1.2 2021/09/17 02:18:04 christos Exp $

.include "../common.mk"
.include "../sources.mk"

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
