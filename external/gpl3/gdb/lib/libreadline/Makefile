#	$NetBSD: Makefile,v 1.1 2011/09/26 03:02:46 christos Exp $

.include <bsd.own.mk>

LIBISPRIVATE=	yes

LIB=		readline

NOGCCERROR=	yes

.include "${.CURDIR}/arch/${MACHINE_ARCH}/defs.mk"

DIST=		${NETBSDSRCDIR}/gnu/dist/gdb6

CPPFLAGS+=	-DHAVE_CONFIG_H -I${.CURDIR}/arch/${MACHINE_ARCH} \
		-I${DIST}/ \
		${G_CCFLAGS:M-DRL_*} \

SRCS=		${G_OBJECTS:.o=.c}

.PATH: ${DIST}/readline

.include <bsd.lib.mk>
