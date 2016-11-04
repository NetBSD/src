#	$NetBSD: Makefile,v 1.3.2.1 2016/11/04 14:48:47 pgoyette Exp $

.include <bsd.own.mk>
.include "../Makefile.inc"

LIBISPRIVATE=	yes

LIB=		readline

NOGCCERROR=	yes

.include "${.CURDIR}/arch/${GDB_MACHINE_ARCH}/defs.mk"

CPPFLAGS+=	-DHAVE_CONFIG_H -I${.CURDIR}/arch/${GDB_MACHINE_ARCH} \
		-I${DIST}/ \
		-DHAVE_SYS_IOCTL_H \
		${G_CCFLAGS:M-DRL_*} \

SRCS=		${G_OBJECTS:.o=.c}

.PATH: ${DIST}/readline

.include <bsd.lib.mk>
