#	$NetBSD: Makefile,v 1.9 2000/10/30 20:56:58 jdolecek Exp $

PROG=	mount_ados
SRCS=	mount_ados.c getmntopts.c fattr.c
MAN=	mount_ados.8

MOUNT=	${.CURDIR}/../mount
CPPFLAGS+= -I${.CURDIR}/../../sys -I${MOUNT}
.PATH:	${MOUNT}

.include <bsd.prog.mk>
