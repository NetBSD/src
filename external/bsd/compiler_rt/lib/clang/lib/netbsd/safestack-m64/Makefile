#	$NetBSD: Makefile,v 1.3 2021/09/17 02:18:03 christos Exp $

.include "../common.mk"
.include "../sources.mk"

LIB=	libclang_rt.safestack-x86_64

.PATH: ${TOPDIR}/lib/safestack

SRCS+=	${SAFESTACK_SOURCES}

.PATH: ${TOPDIR}/lib/interception

SRCS+=  ${INTERCEPTION_SOURCES}

.PATH: ${TOPDIR}/lib/sanitizer_common 
 
SRCS+=	${SANITIZER_SOURCES}
SRCS+=	${SANITIZER_NOLIBC_SOURCES}

CXXFLAGS+=	-fno-rtti -fno-exceptions -fno-builtin -fno-stack-protector

COPTS.safestack.cc+=	${${ACTIVE_CXX} == "gcc" :? -Wno-error :}

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

.include <bsd.lib.mk>
