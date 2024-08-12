#	$NetBSD: Makefile,v 1.2 2024/08/12 20:32:17 christos Exp $

NOCTF=
#HOSTPROG_CXX=   1

.include <bsd.own.mk>

.include "../Makefile.inc"

LIBISPRIVATE=	yes
LIB=		ctf
GDBARCHDIR=	${.CURDIR}/arch/${GDB_MACHINE_ARCH}

.include "${GDBARCHDIR}/defs.mk"

SRCS=		${G_OBJECTS:.o=.c}
GCPPFLAGS=	${G_INTERNAL_CFLAGS}
CPPFLAGS+=	-I${.CURDIR} \
		-I. \
		-I${DIST} \
		-I${DIST}/libctf \
		-I${DIST}/gnulib/import \
		-I${.CURDIR}/../libgnulib/arch/${GDB_MACHINE_ARCH} \
		-I${.CURDIR}/../libgnulib/arch/${GDB_MACHINE_ARCH}/import \
		-I${.CURDIR}/../libbfd/arch/${GDB_MACHINE_ARCH} \
		-I${GDBARCHDIR} \
		-I${DIST}/bfd \
		-I${DIST}/include \
		-Dsighandler_t=sig_t \
		-DHAVE_DECL_STPCPY \
		-DTARGET_SYSTEM_ROOT=\"\" \
		-DTARGET_SYSTEM_ROOT_RELOCATABLE=0 \
		-DBINDIR=\"${BINDIR}\" \
		${GCPPFLAGS:M-D*}

.include "../../Makefile.inc"
.include <bsd.lib.mk>

.PATH: ${DIST}/libctf ${GDBARCHDIR}
