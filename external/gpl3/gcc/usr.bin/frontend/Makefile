#	$NetBSD: Makefile,v 1.15 2023/05/19 23:28:20 lukem Exp $

HOSTPROG_CXX=	1
LIBISPRIVATE=	yes

# For ../Makefile.inc and bsd.own.mk
.include <bsd.init.mk>

LIB=		frontend

SRCS=		gcc.c version.c prefix.c

.include "../Makefile.driver"
.include "../Makefile.target-defines"

version.c: ${GCCARCH}/defs.mk ${TOP}/tools/gcc/gcc-version.mk

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
