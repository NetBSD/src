#	$NetBSD: Makefile,v 1.7 1997/10/10 19:49:02 christos Exp $

PROG=	mount_ados
SRCS=	mount_ados.c getmntopts.c
MAN=	mount_ados.8

MOUNT=	${.CURDIR}/../mount
CPPFLAGS+= -I${MOUNT}
.PATH:	${MOUNT}

.include <bsd.prog.mk>
