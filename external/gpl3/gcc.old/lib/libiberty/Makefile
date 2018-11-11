#	$NetBSD: Makefile,v 1.6 2018/11/11 22:49:59 mrg Exp $

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

COPTS.argv.c += -Wno-stack-protector
COPTS.cp-demangle.c += -Wno-stack-protector
COPTS.make-relative-prefix.c += -Wno-stack-protector
COPTS.regex.c += -Wno-stack-protector
COPTS += -Wno-missing-prototypes

CWARNFLAGS.clang+=	-Wno-unused-value

.PATH: ${DIST}/libiberty

.include <bsd.lib.mk>
