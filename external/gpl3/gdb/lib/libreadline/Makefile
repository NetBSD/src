#	$NetBSD: Makefile,v 1.2 2011/10/16 19:41:07 christos Exp $

.include <bsd.own.mk>
.include "../Makefile.inc"

LIBISPRIVATE=	yes

LIB=		readline

NOGCCERROR=	yes

.include "${.CURDIR}/arch/${MACHINE_ARCH}/defs.mk"

CPPFLAGS+=	-DHAVE_CONFIG_H -I${.CURDIR}/arch/${MACHINE_ARCH} \
		-I${DIST}/ \
		-DHAVE_SYS_IOCTL_H \
		${G_CCFLAGS:M-DRL_*} \

SRCS=		${G_OBJECTS:.o=.c}

.PATH: ${DIST}/readline

.include <bsd.lib.mk>
