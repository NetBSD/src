#	$NetBSD: Makefile,v 1.2 2022/12/24 20:17:08 christos Exp $

NOLINKLIB=	# defined
NOLINT=		# defined
NOMAN=		# defined
NOPROFILE=	# defined
NOCLANGERROR=	# defined

.include <bsd.init.mk>

LIB=		gnuctf

BFD_MACHINE_ARCH?=	${MACHINE_ARCH:C/armv[4-7]/arm/}

.if exists(${.CURDIR}/arch/${BFD_MACHINE_ARCH}/defs.mk)
.include "${.CURDIR}/arch/${BFD_MACHINE_ARCH}/defs.mk"

SHLIB_MAJOR=	1
SHLIB_MINOR=	0

GCPPFLAGS=	${G_archdefs} ${G_DEFS} ${G_INCLUDES} ${G_TDEFAULTS}
CPPFLAGS+=	-I${.CURDIR}/arch/${BFD_MACHINE_ARCH} \
		-I${.CURDIR}/../libbfd/arch/${BFD_MACHINE_ARCH} \
		-I${DIST}/include -I. -I${DIST}/libctf \
		-I${DIST}/bfd ${GCPPFLAGS:M-D*} \
		${GCPPFLAGS:M-I*:N-I.*}

SRCS=		${G_libctf_la_SOURCES}

.PATH: ${DIST}/libctf

.include <bsd.lib.mk>

.else
.include <bsd.prog.mk> # do nothing
.endif
