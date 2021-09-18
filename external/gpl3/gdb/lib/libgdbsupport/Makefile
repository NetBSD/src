#	$NetBSD: Makefile,v 1.2 2021/09/18 01:47:09 christos Exp $

NOCTF=
HOSTPROG_CXX=   1

.include <bsd.own.mk>

.include "../Makefile.inc"

LIBISPRIVATE=	yes
LIB=		gdbsupport
GDBARCHDIR=	${.CURDIR}/arch/${GDB_MACHINE_ARCH}

.include "${GDBARCHDIR}/defs.mk"

SRCS=		${G_OBJECTS:.o=.cc}
GCPPFLAGS=	${G_INTERNAL_CFLAGS}
CPPFLAGS+=	-I${.CURDIR} \
		-I${GDBARCHDIR} \
		-I${DIST} \
		-I${DIST}/gdb \
		-I${DIST}/gdbsupport \
		-I${DIST}/gdb/config \
		-I${DIST}/gnulib/import \
		-I${DIST}/include/opcode \
		-I${DIST}/readline \
		-I${DIST}/libdecnumber \
		-I${.CURDIR}/../libgnulib/arch/${GDB_MACHINE_ARCH} \
		-I${.CURDIR}/../libgnulib/arch/${GDB_MACHINE_ARCH}/gnulib/import \
		-I${.CURDIR}/../libbfd/arch/${GDB_MACHINE_ARCH} \
		-I${.CURDIR}/../libdecnumber/arch/${GDB_MACHINE_ARCH} \
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
CWARNFLAGS.clang+=	-Wno-deprecated
HOST_CC:=		${HOST_CXX}
HOST_CFLAGS+=		${HOST_CXXFLAGS}
CC:=			${CXX}
CFLAGS:=		${CXXFLAGS} -std=gnu++11 -Wno-error=stack-protector

.PATH: ${DIST}/gdbsupport ${GDBARCHDIR}
