#	$NetBSD: Makefile,v 1.5 1997/05/06 20:45:07 gwr Exp $

PROG=	mount_ados
SRCS=	mount_ados.c getmntopts.c
MAN=	mount_ados.8

MOUNT=	${.CURDIR}/../mount
CFLAGS+= -I${MOUNT}
.PATH.c: ${MOUNT}

.include <bsd.prog.mk>
