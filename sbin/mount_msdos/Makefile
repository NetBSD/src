#	$NetBSD: Makefile,v 1.13 1998/03/01 02:20:45 fvdl Exp $

PROG=	mount_msdos
SRCS=	mount_msdos.c getmntopts.c
MAN=	mount_msdos.8

MOUNT=	${.CURDIR}/../mount
CPPFLAGS+= -I${MOUNT} -I../../sys
.PATH:	${MOUNT}

.include <bsd.prog.mk>
