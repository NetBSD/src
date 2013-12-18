#	$NetBSD: Makefile,v 1.5.6.1 2013/12/18 18:41:39 matt Exp $

NOLINKLIB=	# defined
NOLINT=		# defined
NOMAN=		# defined
NOPROFILE=	# defined

.include <bsd.own.mk>

LIB=		opcodes

BFD_MACHINE_ARCH?=	${MACHINE_ARCH:S/armv7/arm/}

.if exists(${.CURDIR}/arch/${BFD_MACHINE_ARCH}/defs.mk)
.include "${.CURDIR}/arch/${BFD_MACHINE_ARCH}/defs.mk"

DIST=		${NETBSDSRCDIR}/gnu/dist/binutils

SHLIB_MAJOR=	2
SHLIB_MINOR=	0

GCPPFLAGS=	${G_archdefs} ${G_DEFS} ${G_INCLUDES} ${G_TDEFAULTS}
CPPFLAGS+=	-I${.CURDIR}/arch/${BFD_MACHINE_ARCH} \
		-I${.CURDIR}/../libbfd/arch/${BFD_MACHINE_ARCH} \
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
