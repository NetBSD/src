#	$NetBSD: Makefile,v 1.1.1.4 2018/08/01 22:19:01 mrg Exp $

LIBISPRIVATE=	yes

# For ../Makefile.inc and bsd.own.mk
.include <bsd.init.mk>

LIB=		frontend

SRCS=		gcc.c version.c prefix.c

HOSTPROG_CXX=	1

# XXX
.if ${GCC_MACHINE_ARCH} == "x86_64" || ${GCC_MACHINE_ARCH} == "i386"
SRCS+=		driver-i386.c
.PATH:		${DIST}/gcc/config/i386
.endif
.if ${GCC_MACHINE_ARCH} == "powerpc" || ${GCC_MACHINE_ARCH} == "powerpc64"
SRCS+=		driver-rs6000.c
.PATH:		${DIST}/gcc/config/rs6000
.endif
.if  ${GCC_MACHINE_ARCH} == "aarch64" 
SRCS+=		driver-aarch64.c
.PATH:		${DIST}/gcc/config/aarch64
.endif

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
