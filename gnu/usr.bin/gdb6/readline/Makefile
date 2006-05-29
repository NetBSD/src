#	$NetBSD: Makefile,v 1.2 2006/05/29 19:27:22 nathanw Exp $

.include <bsd.own.mk>

MKPRIVATELIB=	yes

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
