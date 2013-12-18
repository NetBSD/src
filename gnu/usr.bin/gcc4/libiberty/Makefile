#	$NetBSD: Makefile,v 1.4.12.1 2013/12/18 18:42:57 matt Exp $

LIBISPRIVATE=	yes

.include <bsd.own.mk>

LIB=		iberty

.include "${.CURDIR}/defs.mk"

DIST=		${NETBSDSRCDIR}/gnu/dist/gcc4
GNUHOSTDIST=	${DIST}

SRCS=		${G_REQUIRED_OFILES:.o=.c} ${G_EXTRA_OFILES:.o=.c} \
		${G_LIBOBJS:.o=.c} ${G_ALLOCA:.o=.c}

CPPFLAGS+=	-DHAVE_CONFIG_H -I${.CURDIR}/arch/${MACHINE_ARCH:S/armv7/arm/} \
		-I${DIST}/include

COPTS.argv.c = -Wno-stack-protector
COPTS.cp-demangle.c = -Wno-stack-protector
COPTS.make-relative-prefix.c = -Wno-stack-protector
COPTS.regex.c = -Wno-stack-protector

.PATH: ${DIST}/libiberty

.include <bsd.lib.mk>
