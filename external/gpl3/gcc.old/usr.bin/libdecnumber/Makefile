#	$NetBSD: Makefile,v 1.13 2023/07/14 19:46:45 mrg Exp $

DIST=		${GCCDIST}
GNUHOSTDIST=	${DIST}

HOSTPROG_CXX=	1
LIBISPRIVATE=   yes

.include <bsd.init.mk>

LIB=		decnumber

SRCS=		decNumber.c decContext.c decimal32.c decimal64.c decimal128.c
CPPFLAGS+=	-I${.CURDIR}/arch/${GCC_MACHINE_ARCH} -I${DIST}/libdecnumber
CPPFLAGS+=	-I${DIST}/libgcc

.if ${MACHINE_ARCH} == "vax"
COPTS.decNumber.c=-O0
.endif

.include <bsd.lib.mk>

# Force using C++ for this
HOST_CC:=	${HOST_CXX}
HOST_CFLAGS:=	${HOST_CXXFLAGS}
CC:=		${CXX}
CFLAGS:=	${CXXFLAGS}

.PATH: ${DIST}/libdecnumber ${DIST}/libdecnumber/dpd
