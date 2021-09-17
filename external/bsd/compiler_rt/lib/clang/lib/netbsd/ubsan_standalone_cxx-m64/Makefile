#	$NetBSD: Makefile,v 1.7 2021/09/17 02:18:04 christos Exp $

.include "../common.mk"
.include "../sources.mk"

LIB=	libclang_rt.ubsan_standalone_cxx-x86_64

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

SYMS=		libclang_rt.ubsan_standalone_cxx-x86_64.a.syms
SYMS_EXTRA=	${TOPDIR}/lib/ubsan/ubsan.syms.extra

realall:	${SYMS}

CLEANFILES+=	${SYMS}
FILES=		${SYMS}
FILESDIR=	${LIBDIR}

.include <bsd.lib.mk>
