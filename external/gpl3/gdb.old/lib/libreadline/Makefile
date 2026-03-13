#	$NetBSD: Makefile,v 1.11 2026/03/13 22:09:48 christos Exp $

.include <bsd.own.mk>
.include "../Makefile.inc"

LIBISPRIVATE=	yes

LIB=		readline

NOGCCERROR=	yes

.include "${.CURDIR}/arch/${GDB_MACHINE_ARCH}/defs.mk"

CPPFLAGS+=	-DHAVE_CONFIG_H -I${.CURDIR}/arch/${GDB_MACHINE_ARCH} \
		-I${DIST} -I${DIST}/readline \
		-DHAVE_SYS_IOCTL_H \
		${G_CCFLAGS:M-DRL_*} \

SRCS=		${G_OBJECTS:.o=.c}

.PATH: ${DIST}/readline/readline

.include <bsd.lib.mk>
