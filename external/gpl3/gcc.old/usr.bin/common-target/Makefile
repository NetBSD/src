#	$NetBSD: Makefile,v 1.7 2020/08/19 07:48:03 mrg Exp $

LIBISPRIVATE=	yes

# For ../Makefile.inc and bsd.own.mk
.include <bsd.init.mk>

LIB=		common-target

.include "${.CURDIR}/arch/${GCC_MACHINE_ARCH}.mk"
SRCS=		${G_OBJS-libcommon-target:S/common\///:.o=.c}

CPPFLAGS+=	-I${GCCARCH} -I${BACKENDOBJ} -I. \
		${G_ALL_CFLAGS:M-D*} ${G_INCLUDES:M-I*:N-I.*} \
		-DPREFIX=\"/usr\" \
		-DDEFAULT_TARGET_VERSION=\"${G_version}\" \
		-DDEFAULT_TARGET_MACHINE=\"${MACHINE_GNU_PLATFORM}\" \
		-DCONFIGURE_SPECS="\"\"" \
		-I${BACKENDOBJ}

COPTS.prefix.c+=	-Wno-stack-protector
COPTS.opts-common.c+=	-Wno-stack-protector
COPTS.rs6000-common.c+=	-Wno-stack-protector
COPTS.arm-common.c+=	-Wno-stack-protector

GENPROG_ERROR_DEPENDS=errors.lo

.include "../Makefile.options"
.include "../../Makefile.hooks"

${SRCS}: ${GCCARCH}/defs.mk

${G_common_out_file:T:R}.o:  common/common-target-hooks-def.h

DPSRCS+=	options.h

HOST_CPPFLAGS+=	-I${BACKENDOBJ} -I${GCCARCH} -I. -I${DIST}/include -I${DIST}/gcc
HOST_CPPFLAGS+=	-DGENERATOR_FILE

MKPIC:=		no
MKPICLIB:=	no

HOSTPROG_CXX=	1

.include <bsd.lib.mk>

# Force using C++ for this
HOST_CC:=	${HOST_CXX}
CC:=		${CXX}
CFLAGS:=	${CXXFLAGS}

.PATH: ${DIST}/gcc ${DIST}/gcc/common
.PATH: ${G_common_out_file:H}
