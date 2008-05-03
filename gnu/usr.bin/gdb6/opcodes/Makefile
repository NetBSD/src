#	$NetBSD: Makefile,v 1.3 2008/05/03 14:48:31 lukem Exp $

.include <bsd.own.mk>

LIBISPRIVATE=	yes

LIB=		opcodes

.include "${.CURDIR}/arch/${MACHINE_ARCH}/defs.mk"

DIST=		${NETBSDSRCDIR}/gnu/dist/gdb6

GCPPFLAGS=	${G_archdefs} ${G_DEFS} ${G_INCLUDES} ${G_TDEFAULTS}
CPPFLAGS+=	-I${.CURDIR}/arch/${MACHINE_ARCH} \
		-I${.CURDIR}/../bfd/arch/${MACHINE_ARCH} \
		-I${DIST}/include -I. \
		-I${DIST}/bfd ${GCPPFLAGS:M-D*} \
		${GCPPFLAGS:M-I*:N-I.*}

GSRCS=		${G_BFD_MACHINES}
SRCS=		${GSRCS:.lo=.c} ${G_libopcodes_la_SOURCES}

.PATH: ${DIST}/opcodes

.include <bsd.lib.mk>
