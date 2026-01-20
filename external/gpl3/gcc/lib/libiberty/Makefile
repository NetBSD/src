#	$NetBSD: Makefile,v 1.13 2026/01/20 20:27:26 christos Exp $

NOLINT=yes	# XXX: too-many-errors
LIBISPRIVATE=	yes

.include <bsd.init.mk>

GCC_MACHINE_ARCH?=${MACHINE_ARCH:S/earmv5/earm/}

LIB=		iberty

.include "${.CURDIR}/defs.mk"

DIST=		${GCCDIST}
GNUHOSTDIST=	${DIST}

SRCS=		${G_REQUIRED_OFILES:Nalloca.o:.o=.c} ${G_EXTRA_OFILES:.o=.c} \
		${G_LIBOBJS:.o=.c}

CPPFLAGS+=	-DHAVE_CONFIG_H -I${.CURDIR}/arch/${GCC_MACHINE_ARCH} \
		-I${DIST}/include

COPTS.argv.c += -Wno-stack-protector
COPTS.cp-demangle.c += -Wno-stack-protector
COPTS.make-relative-prefix.c += -Wno-stack-protector
COPTS.regex.c += -Wno-stack-protector
COPTS += -Wno-missing-prototypes

CWARNFLAGS.clang+=	-Wno-unused-value

.PATH: ${DIST}/libiberty

COPTS.dyn-string.c+=	${CC_WNO_STRINGOP_TRUNCATION}

.include <bsd.lib.mk>
