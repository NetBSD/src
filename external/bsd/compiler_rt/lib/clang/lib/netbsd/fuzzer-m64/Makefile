#	$NetBSD: Makefile,v 1.2 2021/09/17 02:18:02 christos Exp $

.include "../common.mk"
.include "../sources.mk"

LIB=	libclang_rt.fuzzer-x86_64

.PATH: ${TOPDIR}/lib/fuzzer

SRCS+=	${LIBFUZZER_SOURCES}
SRCS+=	${LIBFUZZER_MAIN_SOURCES}

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

.include <bsd.lib.mk>
