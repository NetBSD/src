#	$NetBSD: Makefile,v 1.6 2017/11/28 18:10:20 christos Exp $

.include <bsd.own.mk>
.include "../Makefile.inc"

LIBISPRIVATE=	yes

LIB=		decnumber

.include "${.CURDIR}/arch/${GDB_MACHINE_ARCH}/defs.mk"

GCPPFLAGS=	${G_archdefs} ${G_DEFS} ${G_INCLUDES} ${G_TDEFAULTS}
CPPFLAGS+=	-I${.CURDIR}/arch/${GDB_MACHINE_ARCH} \
		-I${.CURDIR}/../libbfd/arch/${GDB_MACHINE_ARCH} \
		-I${DIST}/include -I. \
		-I${DIST}/libdecnumber ${GCPPFLAGS:M-D*} \
		${GCPPFLAGS:M-I*:N-I.*}

SRCS=		${G_OBJS:.o=.c} ${G_SOURCES}

.if ${MACHINE_ARCH} == "vax"
COPTS.decNumber.c=-O0
.endif

.PATH: ${DIST}/libdecnumber ${DIST}/libdecnumber/dpd

.include <bsd.lib.mk>
