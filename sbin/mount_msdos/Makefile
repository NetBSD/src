#	$NetBSD: Makefile,v 1.14 2000/08/14 16:38:40 deberg Exp $

PROG=	mount_msdos
SRCS=	mount_msdos.c getmntopts.c
MAN=	mount_msdos.8

MOUNT=	${.CURDIR}/../mount
CPPFLAGS+= -I${MOUNT} -I${.CURDIR}/../../sys
.PATH:	${MOUNT}

.include <bsd.prog.mk>
