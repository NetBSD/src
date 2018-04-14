#	$NetBSD: Makefile,v 1.1.1.3 2018/04/14 15:37:45 christos Exp $

NOLINT=		# defined
NOPROFILE=	# defined
NOPIC=		# defined
NOLINKLIB=	# defined
NOCLANGERROR=	# defined
NODEBUGLIB=	# defined

.include <bsd.init.mk>

LIB=		iberty

BINUTILS_MACHINE_ARCH=${MACHINE_ARCH:C/armv[4-7]/arm/}

.include "${.CURDIR}/arch/${BINUTILS_MACHINE_ARCH}/defs.mk"

COPTS.argv.c = -Wno-stack-protector
COPTS.cp-demangle.c = -Wno-stack-protector
COPTS.make-relative-prefix.c = -Wno-stack-protector
COPTS.regex.c = -Wno-stack-protector

SRCS=		${G_REQUIRED_OFILES:.o=.c} ${G_EXTRA_OFILES:.o=.c} \
		${G_LIBOBJS:.o=.c} ${G_ALLOCA:.o=.c}

CPPFLAGS+=	-DHAVE_CONFIG_H -I${.CURDIR}/arch/${BINUTILS_MACHINE_ARCH} \
		-I${DIST}/include

.PATH: ${DIST}/libiberty

.include <bsd.lib.mk>
