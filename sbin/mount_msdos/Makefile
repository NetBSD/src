#	$NetBSD: Makefile,v 1.13.10.1 2000/08/14 21:23:52 deberg Exp $

PROG=	mount_msdos
SRCS=	mount_msdos.c getmntopts.c
MAN=	mount_msdos.8

MOUNT=	${.CURDIR}/../mount
CPPFLAGS+= -I${MOUNT} -I${.CURDIR}/../../sys
.PATH:	${MOUNT}

.include <bsd.prog.mk>
