#	$NetBSD: Makefile,v 1.15 2000/10/30 20:57:00 jdolecek Exp $

PROG=	mount_msdos
SRCS=	mount_msdos.c getmntopts.c fattr.c
MAN=	mount_msdos.8

MOUNT=	${.CURDIR}/../mount
CPPFLAGS+= -I${MOUNT} -I${.CURDIR}/../../sys
.PATH:	${MOUNT}

.include <bsd.prog.mk>
