#	$NetBSD: Makefile,v 1.3.12.1 2013/12/18 18:43:30 matt Exp $

.include <bsd.own.mk>

.include "../Makefile.inc"

LIBISPRIVATE=	yes

LIB=		readline

NOGCCERROR=	yes

.include "${.CURDIR}/arch/${GDB_MACHINE_ARCH}/defs.mk"

DIST=		${NETBSDSRCDIR}/gnu/dist/gdb6

CPPFLAGS+=	-DHAVE_CONFIG_H -I${.CURDIR}/arch/${GDB_MACHINE_ARCH} \
		-I${DIST}/ \
		${G_CCFLAGS:M-DRL_*} \

SRCS=		${G_OBJECTS:.o=.c}

.PATH: ${DIST}/readline

.include <bsd.lib.mk>
