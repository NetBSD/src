#	$NetBSD: Makefile,v 1.15 2025/09/17 05:55:31 mrg Exp $

HOSTPROG_CXX=	1
LIBISPRIVATE=	yes

# For ../Makefile.inc and bsd.own.mk
.include <bsd.init.mk>

LIB=		common

.include "${.CURDIR}/defs.mk"
SRCS=	${G_OBJS-libcommon:S/text-art\///:.o=.cc}

# XXX make sure this doesn't have more than it needs
CPPFLAGS+=	-I${GCCARCH} -I${BACKENDOBJ} -I. \
		${G_ALL_CFLAGS:M-D*} ${G_INCLUDES:M-I*:N-I.*} \
		-DPREFIX=\"/usr\" \
		-DDEFAULT_TARGET_VERSION=\"${G_version}\" \
		-DDEFAULT_TARGET_MACHINE=\"${MACHINE_GNU_PLATFORM}\" \
		-DCONFIGURE_SPECS="\"\""

MKPIC:=		no
MKPICLIB:=	no

#COPTS.diagnostic.cc=	-Wno-stack-protector
#COPTS.intl.cc=		-Wno-stack-protector

version.cc: ${GCCARCH}/defs.mk ${TOP}/tools/gcc/gcc-version.mk

${SRCS}: ${GCCARCH}/defs.mk

COPTS.intl.cc +=	-Wno-error=stack-protector

.include <bsd.lib.mk>

# Force using C++ for this
HOST_CC:=	${HOST_CXX}
HOST_CFLAGS:=	${HOST_CXXFLAGS}
CC:=		${CXX}
CFLAGS:=	${CXXFLAGS}

.PATH: ${DIST}/gcc ${DIST}/gcc/text-art
