#	$NetBSD: Makefile,v 1.6 1997/05/08 21:11:13 gwr Exp $

PROG=	mount_ados
SRCS=	mount_ados.c getmntopts.c
MAN=	mount_ados.8

MOUNT=	${.CURDIR}/../mount
CFLAGS+= -I${MOUNT}
.PATH:	${MOUNT}

.include <bsd.prog.mk>
