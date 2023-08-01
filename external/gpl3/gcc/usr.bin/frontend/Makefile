#	$NetBSD: Makefile,v 1.16 2023/08/01 05:57:55 mrg Exp $

HOSTPROG_CXX=	1
LIBISPRIVATE=	yes

# For ../Makefile.inc and bsd.own.mk
.include <bsd.init.mk>

LIB=		frontend

SRCS=		gcc.cc prefix.cc

.include "../Makefile.driver"
.include "../Makefile.target-defines"

version.cc: ${GCCARCH}/defs.mk ${TOP}/tools/gcc/gcc-version.mk

CPPFLAGS+=	-I${GCCARCH} -I. \
		${G_ALL_CFLAGS:M-D*} ${G_INCLUDES:M-I*:N-I.*} \
		-DPREFIX=\"/usr\"
HOST_CPPFLAGS+=	-I${GCCARCH} ${G_ALL_CFLAGS:M-D*} ${G_INCLUDES:M-I*:N-I.*}

DPSRCS+=	specs.h

${SRCS}: ${GCCARCH}/defs.mk

MKPIC:=		no
MKPICLIB:=	no

.include <bsd.lib.mk>

# Force using C++ for this
HOST_CC:=	${HOST_CXX}
HOST_CFLAGS:=	${HOST_CXXFLAGS}
CC:=		${CXX}
CFLAGS:=	${CXXFLAGS}

CFLAGS+=	-Wno-stack-protector

# XXX print-objdir fix me
CPPFLAGS+=	-I../backend

.PATH: ${DIST}/gcc ${DIST}/gcc/common

# Build the specs.h file
CLEANFILES+=	specs.h
specs.h: Makefile
	for f in cp lto objc objcp; do \
		echo "#include \"$$f/lang-specs.h\""; \
	done > specs.h
