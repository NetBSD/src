#	$NetBSD: Makefile,v 1.1 2003/03/01 13:18:28 mrg Exp $

NOLINKLIB=	# defined
NOLINT=		# defined
NOMAN=		# defined
NOPROFILE=	# defined

.include <bsd.own.mk>

LIB=		opcodes

.if exists(${.CURDIR}/arch/${MACHINE_ARCH}/defs.mk)
.include "${.CURDIR}/arch/${MACHINE_ARCH}/defs.mk"

SHLIB_MAJOR=	0
SHLIB_MINOR=	0

DIST=		${NETBSDSRCDIR}/gnu/dist/toolchain

GCPPFLAGS=	${G_archdefs} ${G_DEFS} ${G_INCLUDES} ${G_TDEFAULTS}
CPPFLAGS+=	-I${.CURDIR}/arch/${MACHINE_ARCH} \
		-I${.CURDIR}/../libbfd/arch/${MACHINE_ARCH} \
		-I${DIST}/include -I. \
		-I${DIST}/bfd ${GCPPFLAGS:M-D*} \
		${GCPPFLAGS:M-I*:N-I.*}

GSRCS=		${G_BFD_MACHINES}
SRCS=		${GSRCS:.lo=.c} ${G_libopcodes_la_SOURCES}

.PATH: ${DIST}/opcodes

.include <bsd.lib.mk>

.else
.include <bsd.prog.mk> # do nothing
.endif
