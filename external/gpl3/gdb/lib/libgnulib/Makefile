#	$NetBSD: Makefile,v 1.1 2020/09/17 16:36:39 christos Exp $

NOCTF=
#HOSTPROG_CXX=   1

.include <bsd.own.mk>

.include "../Makefile.inc"

LIBISPRIVATE=	yes
LIB=		gnulib
GDBARCHDIR=	${.CURDIR}/arch/${GDB_MACHINE_ARCH}

.include "${GDBARCHDIR}/defs.mk"

SRCS=		${G_OBJS:.o=.c} ${G_OBJECTS:.o=.c}
GCPPFLAGS=	${G_INTERNAL_CFLAGS}
CPPFLAGS+=	-I${.CURDIR} \
		-I${GDBARCHDIR}/gnulib \
		-I${GDBARCHDIR}/gnulib/import \
		-I${DIST} \
		-I${DIST}/gnulib \
		-I${DIST}/gnulib/import \
		-I${DIST}/bfd \
		-I${DIST}/include \
		-Dsighandler_t=sig_t \
		-DTARGET_SYSTEM_ROOT=\"\" \
		-DTARGET_SYSTEM_ROOT_RELOCATABLE=0 \
		-DBINDIR=\"${BINDIR}\" \
		${GCPPFLAGS:M-D*}

CWARNFLAGS.clang+=	-Wno-switch

.include "../../Makefile.inc"
.include <bsd.lib.mk>

# Force c++
#CWARNFLAGS.clang+=	-Wno-deprecated
#HOST_CC:=		${HOST_CXX}
#CC:=			${CXX}
#CFLAGS:=		${CXXFLAGS} -std=gnu++11 -Wno-error=stack-protector
CFLAGS:=		${CFLAGS} -std=gnu11 -Wno-error=stack-protector

.PATH: ${DIST}/gnulib/import ${DIST}/gnulib/import/glthread \
    ${DIST}/gnulib/import/malloc ${GDBARCHDIR}
