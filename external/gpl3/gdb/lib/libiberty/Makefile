#	$NetBSD: Makefile,v 1.1 2011/09/26 03:02:45 christos Exp $

.include <bsd.own.mk>
.include "../Makefile.inc"

LIBISPRIVATE=	yes

LIB=		iberty

.include "${.CURDIR}/arch/${MACHINE_ARCH}/defs.mk"

SRCS=		${G_REQUIRED_OFILES:.o=.c} ${G_EXTRA_OFILES:.o=.c} \
		${G_LIBOBJS:.o=.c} ${G_ALLOCA:.o=.c}

CPPFLAGS+=	-DHAVE_CONFIG_H -I${.CURDIR}/arch/${MACHINE_ARCH} \
		-I${DIST}/include

COPTS.argv.c = -Wno-stack-protector
COPTS.cp-demangle.c = -Wno-stack-protector
COPTS.make-relative-prefix.c = -Wno-stack-protector
COPTS.regex.c = -Wno-stack-protector

.PATH: ${DIST}/libiberty

.include <bsd.lib.mk>
