#	$NetBSD: Makefile,v 1.1.1.3 2015/09/23 03:03:38 mrg Exp $

LIBISPRIVATE=	yes

.include <bsd.init.mk>

GCC_MACHINE_ARCH?=${MACHINE_ARCH:S/earmv5/earm/}

LIB=		iberty

.include "${.CURDIR}/defs.mk"

DIST=		${GCCDIST}
GNUHOSTDIST=	${DIST}

SRCS=		${G_REQUIRED_OFILES:.o=.c} ${G_EXTRA_OFILES:.o=.c} \
		${G_LIBOBJS:.o=.c} ${G_ALLOCA:.o=.c}

CPPFLAGS+=	-DHAVE_CONFIG_H -I${.CURDIR}/arch/${GCC_MACHINE_ARCH} \
		-I${DIST}/include

COPTS.argv.c = -Wno-stack-protector
COPTS.cp-demangle.c = -Wno-stack-protector
COPTS.make-relative-prefix.c = -Wno-stack-protector
COPTS.regex.c = -Wno-stack-protector

CWARNFLAGS.clang+=	-Wno-unused-value

.PATH: ${DIST}/libiberty

.include <bsd.lib.mk>
