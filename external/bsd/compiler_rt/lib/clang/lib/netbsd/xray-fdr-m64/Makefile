#	$NetBSD: Makefile,v 1.1 2019/08/08 14:00:32 kamil Exp $

NOSANITIZER=    # defined

.include "../common.mk"
.include "../sources.mk"

.include <bsd.own.mk>

TOPDIR=	${NETBSDSRCDIR}/sys/external/bsd/compiler_rt/dist

LIB=	libclang_rt.xray-fdr-x86_64

.PATH: ${TOPDIR}/lib/xray

SRCS+=	${XRAY_FDR_MODE_SOURCES}

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
