#	$NetBSD: Makefile,v 1.4 2006/02/03 16:30:13 christos Exp $

NOLINKLIB=	# defined
NOLINT=		# defined
NOMAN=		# defined
NOPROFILE=	# defined

.include <bsd.own.mk>

LIB=		opcodes

.if exists(${.CURDIR}/arch/${MACHINE_ARCH}/defs.mk)
.include "${.CURDIR}/arch/${MACHINE_ARCH}/defs.mk"

DIST=		${NETBSDSRCDIR}/gnu/dist/binutils

SHLIB_MAJOR=	2
SHLIB_MINOR=	0

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
