#	$NetBSD: Makefile,v 1.2 2025/10/20 14:31:36 nat Exp $

NOLINT=		# defined
NOPROFILE=	# defined
NOPIC=		# defined
NOLINKLIB=	# defined
NOCLANGERROR=	# defined
NODEBUGLIB=	# defined

.include <bsd.init.mk>

LIB=		sframe

BINUTILS_MACHINE_ARCH?=${MACHINE_ARCH:C/armv[4-7]/arm/}

.include "${.CURDIR}/arch/${BINUTILS_MACHINE_ARCH}/defs.mk"

SRCS=		${G_libsframe_la_SOURCES}

CPPFLAGS+=	-DHAVE_CONFIG_H -I${.CURDIR}/arch/${BINUTILS_MACHINE_ARCH} \
		-I${DIST}/include -I${DIST}/libctf

.PATH: ${DIST}/libsframe

.include <bsd.lib.mk>
