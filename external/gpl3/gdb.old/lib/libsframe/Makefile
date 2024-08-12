#	$NetBSD: Makefile,v 1.2 2024/08/12 20:32:37 christos Exp $

NOCTF=
HOSTPROG_CXX=   1

.include <bsd.own.mk>

.include "../Makefile.inc"

LIBISPRIVATE=	yes
LIB=		sframe
GDBARCHDIR=	${.CURDIR}/arch/${GDB_MACHINE_ARCH}

.include "${GDBARCHDIR}/defs.mk"

SRCS=		${G_OBJECTS:.o=.c}
GCPPFLAGS=	${G_INTERNAL_CFLAGS}
CPPFLAGS+=	-I${.CURDIR} \
		-I${GDBARCHDIR} \
		-I${DIST} \
		-I${DIST}/libctf \
		-I${DIST}/libsframe \
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

.PATH: ${DIST}/libsframe ${GDBARCHDIR}
