#	$NetBSD: Makefile,v 1.2 2006/05/29 19:18:56 nathanw Exp $

.include <bsd.own.mk>

MKPRIVATELIB=	yes

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
