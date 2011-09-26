#	$NetBSD: Makefile,v 1.1 2011/09/26 03:02:45 christos Exp $

.include <bsd.own.mk>
.include "../Makefile.inc"

LIBISPRIVATE=	yes

LIB=		decnumber

.include "${.CURDIR}/arch/${MACHINE_ARCH}/defs.mk"

GCPPFLAGS=	${G_archdefs} ${G_DEFS} ${G_INCLUDES} ${G_TDEFAULTS}
CPPFLAGS+=	-I${.CURDIR}/arch/${MACHINE_ARCH} \
		-I${.CURDIR}/../libbfd/arch/${MACHINE_ARCH} \
		-I${DIST}/include -I. \
		-I${DIST}/libdecnumber ${GCPPFLAGS:M-D*} \
		${GCPPFLAGS:M-I*:N-I.*}

SRCS=		${G_OBJS:.o=.c} ${G_SOURCES}

.PATH: ${DIST}/libdecnumber ${DIST}/libdecnumber/dpd

.include <bsd.lib.mk>
